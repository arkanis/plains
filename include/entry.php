<?php

/** 
 * Quick summary of usage:
 * 
 * Entry::find('*.post')
 * Entry::load('test.post')
 * Entry::save('test.post', array('Header' => 'value'), 'Some multiline content')
 * 
 * $entry->title
 * $entry->tags
 * $entry->tags_as_list
 * $entry->content
 * $entry->raw_content
 * $entry->path
 * $entry->public_path
 * $entry->public_path_for('figure.png')
 */
class Entry
{
	private static $processors = array();
	private static $routers = array();
	
	/**
	 * You can use this function to register a new processor for the content text
	 * of entries. If the "processor" header of the entry is the specified name (converted
	 * to lower case) the handler (a closure) is called with the raw content as argument.
	 * 
	 * The return value of the handler is then used as the processed entry content.
	 * 
	 * Example adding a markdown processor:
	 * 
	 * 	Entry::add_processor('markdown', function($raw_content, $entry){
	 * 		return Markdown($raw_content);
	 *	});
	 */
	static function add_processor($name, $handler)
	{
		self::$processors[$name] = $handler;
	}
	
	static function add_router($type, $handler)
	{
		self::$routers[$type] = $handler;
	}
	
	/**
	 * Saves an entry at the specified path. $headers is expected to be an array of
	 * header field name and header field value pairs. $content is the actual content
	 * of the entry.
	 * 
	 * Example:
	 * 
	 * 	Entry::save('example.post', array('Name' => 'Example post'), 'Example content');
	 */
	static function save($path, $headers, $content)
	{
		$head = join("\n", array_map(function($name, $value){
			if (is_array($value))
				return join("\n", array_map(function($line){
					strtr($name, "\n:", '  '). ': ' . str_replace("\n", ' ', $line);
				}, $value));
			else
				return strtr($name, "\n:", '  '). ': ' . str_replace("\n", ' ', $value);
		}, array_keys($headers), $headers) );
		
		return @file_put_contents($path, $head . "\n\n" . $content);
	}
	
	/**
	 * Loads the entity files matching the specified glob pattern.
	 * 
	 * Options:
	 * 	limit: The max. number of entities to load.
	 * 	sort_with: The sort function used for the files. You can use 'rsort' (newest first) or 'sort' (oldest first).
	 * 	prepare_for_caching: If set to true every found entry will be prepared for caching by calculating everything.
	 */
	static function find($glob_pattern, $options = array())
	{
		$options = array_merge(array('sort_with' => 'rsort', 'prepare_for_caching' => false), $options);
		
		$entries = array();
		$entry_files = glob($glob_pattern);
		$options['sort_with']($entry_files);
		
		if ( array_key_exists('limit', $options) )
			$entry_files = array_slice($entry_files, 0, $options['limit']);
		
		foreach($entry_files as $file)
		{
			$entry = self::load($file);
			if ($options['prepare_for_caching'])
				$entry->prepare_for_caching();
			
			$entries[] = $entry;
		}
		
		return $entries;
	}
	
	/**
	 * Disassembles the specified entry file and returns its headers and content.
	 * 
	 * Everything is returned without manipulation and therefore perfect to
	 * reconstruct the entry again after a slight manipulation (e.g. adding a  new
	 * header). This does not destroy any formatting (e.g. with whitespaces) the
	 * user applied.
	 * 
	 * Note that this function does not return an Entry object. It always returns an
	 * array with the first element being a list of header fields and the second
	 * element the entry content. If the specified file does not exists both elements
	 * are set to false.
	 */
	static function analyze($path)
	{
		$data = @file_get_contents($path);
		if (!$data)
			return array(false, false);
		
		list($head, $content) = explode("\n\n", $data, 2);
		$headers = self::parse_head($head, false);
		
		return array($headers, $content);
	}
	
	/**
	 * Loads the specified file and returns an entry object.
	 */
	static function load($path)
	{
		if ( ! file_exists($path) )
			return false;
		
		$data = file_get_contents($path);
		@list($head, $content) = explode("\n\n", $data, 2);
		$header = self::parse_head($head);
		
		return new self($header, $content, $path);
	}
	
	function __construct($header, $content, $path)
	{
		$this->header = $header;
		$this->raw_content = $content;
		$this->path = $path;
		
		$id = pathinfo($path, PATHINFO_FILENAME);
		$this->type = pathinfo($path, PATHINFO_EXTENSION);
		if ( array_key_exists($this->type, self::$routers) ) {
			// Use the router for this type
			$router = self::$routers[$this->type];
			$this->public_path = $router($id, $path, $this);
		} elseif ( array_key_exists(null, self::$routers) ) {
			// If there is no type specific router use the general router if possible
			$router = self::$routers[null];
			$this->public_path = $router($id, $path, $this);
		} else {
			// If there is no general router just fall back to '/'
			$this->public_path = '/';
		}
	}
	
	function __get($property_name)
	{
		if ($property_name == 'content')
			return ( $this->content = self::process_content($this->raw_content, $this->processor_as_list, $this) );
		if ($property_name == 'id')
			return pathinfo($this->path, PATHINFO_FILENAME);
		if (preg_match('/^(.+)_as_list$/i', $property_name, &$matches))
			return self::parse_list_header(@$this->header[$matches[1]]);
		if (preg_match('/^(.+)_as_time$/i', $property_name, &$matches))
			return self::parse_time_header(@$this->header[$matches[1]]);
		if (preg_match('/^(.+)_as_array$/i', $property_name, &$matches))
			if( is_array(@$this->header[$matches[1]]) )
				return @$this->header[$matches[1]];
			else
				return array(@$this->header[$matches[1]]);
		
		return @$this->header[$property_name];
	}
	
	function public_path_for($relative_path)
	{
		return $this->public_path . '/' . $relative_path;
	}
	
	/**
	 * Prepares this entry for caching by doing all expensive calculations
	 * and store the results in the respective properties.
	 */
	function prepare_for_caching()
	{
		$this->content = self::process_content($this->raw_content, $this->processor_as_list, $this);
	}
	
	/**
	 * Parses the specified head text of an entity and returns an array
	 * with the headers.
	 * 
	 * If the clean_up parameter is set to false the header names and
	 * values are not cleaned up (lower case and trimmed). Use this if
	 * you want to reconstruct the headers in their original state.
	 */
	static function parse_head($head, $clean_up = true)
	{
		$headers = array();
		foreach( explode("\n", $head) as $header_line )
		{
			list($name, $value) = explode(': ', $header_line, 2);
			if ($clean_up)
			{
				$name = strtolower(trim($name));
				$value = trim($value);
			}
			
			if ( !array_key_exists($name, $headers) )
				$headers[$name] = $value;
			else
				if (is_array($headers[$name]))
					array_push($headers[$name], $value);
				else
					$headers[$name] = array($headers[$name], $value);
		}
		
		return $headers;
	}
	
	/**
	 * Parses header as a list and returns the elements as an array.
	 * If the input parameter is invalid an empty array is returned.
	 */
	static function parse_list_header($header_content)
	{
		if ($header_content)
		{
			$elements = explode(',', $header_content);
			return array_map('trim', $elements);
		}
		
		return array();
	}
	
	/**
	 * Parses the specified header content as a date and returns its
	 * the timestamp. If the header isn't a valid date false is returned.
	 */
	static function parse_time_header($header_content)
	{
		$matched = preg_match('/(\d{4})(-(\d{2})(-(\d{2})(\s+(\d{2}):(\d{2})(:(\d{2}))?)?)?)?/i', $header_content, &$matches);
		if (!$matched)
			return false;
		
		@list($year, $month, $day, $hour, $minute, $second) = array($matches[1], $matches[3], $matches[5], $matches[7], $matches[8], $matches[10]);
		
		// Set default values to 1 for month and day (we usually
		// mean that when we omit that part of the date).
		if ( empty($month) )
			$month = 1;
		if ( empty($day) )
			$day = 1;
		
		return mktime($hour, $minute, $second, $month, $day, $year);
	}
	
	/**
	 * Sends the specified content though the processors specified in the $processor_list
	 * argument. Returns the processed content.
	 */
	static function process_content($content, $processor_list, $context_entry = null)
	{
		foreach($processor_list as $processor_name)
		{
			$handler = @self::$processors[$processor_name];
			if ($handler)
				$content = $handler($content, $context_entry);
		}
		
		return $content;
	}
	
	/**
	 * Converts the specified name into a better readable form that
	 * can be used in "pretty" URLs.
	 */
	static function parameterize($name)
	{
		return trim( preg_replace('/[^\w\däüöß]+/', '-', strtolower($name)), '-' );
	}
}

?>