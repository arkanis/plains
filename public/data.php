<?php

/**
 * REST like interface to the plain data
 * 
 * 	GET		/data	→	full recursive list of plains and entries
 * 	POST	/data	→	new entry
 * 	GET		/data/id	→	entry data
 * 	PUT		/data/id	→	update entry
 * 	DELETE	/data/id	→	delete entry
 */


// Allow other domains to use AJAX requests on this domain
header('Access-Control-Allow-Origin: * ');

$_CONFIG = array(
	'data_dir' => '../data/' . basename($_GET['user']),
	'plain_file' => '.plain'
);

//
// Configure the Entry class
//

require_once('../include/entry.php');

// Markdown processor
require_once('../include/processors/markdown.php');
Entry::processor('markdown', function($raw_content, $entry){
	return Markdown($raw_content);
});

// A small smiley processor. It's supported by CSS rules that map
// the classes to style specific image. An image element would simply be
// to much for a smiley and this way it degrade gracefully within the
// newsfeed.
Entry::processor('smilies', function($raw_content, $entry){
	$smiley_map = array(
		':)' => 'smile',
		';)' => 'wink',
		':o' => 'surprised',
		':D' => 'grin',
		'¦D' => 'happy',
		':P' => 'tongue',
		'B)' => 'evilgrin',
		':3' => 'waii',
		':(' => 'unhappy'
	);
	
	foreach($smiley_map as $smiley => $smiley_class)
		$raw_content = preg_replace('/(\s)' . preg_quote($smiley) . '/', '$1<span class="smiley ' . $smiley_class . '">' . $smiley . '</span>', $raw_content);
	
	return $raw_content;
});

Entry::route_in(function($id) use($_CONFIG){
	// Prepend the data directory
	$path = $_CONFIG['data_dir'] . $id;
	
	// Make sure the path does not escape out of the data directory (if so it's invalid
	// and return `false`)
	$expanded_path = realpath($path);
	$expanded_data_dir = realpath($_CONFIG['data_dir']);
	
	// If realpath() returns `false` the file does not exist, so there should be no danger in
	// the path. Otherwise we only allow the path if the absolute path is still in the data
	// directory.
	if ( $expanded_path === false or substr($expanded_path, 0, strlen($expanded_data_dir)) == $expanded_data_dir ) {
		// If we got a directory (a plain) modify the path to point to the entry for the directory
		if ( is_dir($path) )
			$path .= '/' . $_CONFIG['plain_file'];
		return $path;
	} else {
		return false;
	}
});

Entry::route_out(null, function($path, $entry) use($_CONFIG){
	// Strip away the leading data directory path (if the path does not start with it we don't
	// create an ID for that entry.
	if ( substr($path, 0, strlen($_CONFIG['data_dir'])) == $_CONFIG['data_dir'] )
		$id = substr($path, strlen($_CONFIG['data_dir']));
	else
		return null;
	
	// For plain files the ID is just the directory, not the .plain file
	if ( pathinfo($path, PATHINFO_EXTENSION) == 'plain')
		$id = dirname($id);
	
	return $id;
});


//
// Action helper functions
//

function exit_with_error($http_status_code, $message){
	header('Content-Type: application/json', false, $http_status_code);
	exit( json_encode(array('message' => $message)) );
}

function load_plain($plain_path){
	global $_CONFIG;
	
	// First look for a file with data of our plain
	$plain = Entry::load($plain_path . '/' . $_CONFIG['plain_file']);
	$data = array('id' => Entry::translate_path_to_id($plain_path), 'type' => 'plain');
	$data['headers'] = empty($plain) ? array() : $plain->headers;
	
	// If no title is set return the directory name as title
	if ( !isset($data['headers']['title']) )
		$data['headers']['title'] = basename($plain_path);
	
	$entries = array();
	foreach(glob($plain_path . '/*') as $child_path){
		if ( pathinfo($child_path, PATHINFO_EXTENSION) != 'deleted' ){
			if (is_dir($child_path)) {
				// We got a directory, so go in recursively (dot directories are not returned by glob)
				$entries[] = load_plain($child_path);
			} elseif ( basename($child_path) != $_CONFIG['plain_file'] ) {
				// We got a content file, load it and add it's data
				$entry = Entry::load($child_path);
				$entries[] = array(
					'id' => $entry->id,
					'type' => $entry->type,
					'headers' => $entry->headers,
					'content' => $entry->content
				);
			}
		}
	}
	
	$data['entries'] = $entries;
	return $data;
}


//
// Here the real action takes place
//

if ( $_SERVER['REQUEST_METHOD'] == 'POST' )
{
	// POST / => 201 or 500
	$incomming_data = json_decode(file_get_contents('php://input'), true);
	
	if ( !isset($incomming_data['raw']) or !$incomming_data['type'] )
		exit_with_error(422, 'The raw or type field is missing in the uploaded data');
	
	// Load the new data
	$entry = Entry::load_from_string($incomming_data['raw'], null, false);
	$entry_headers = $entry->headers;
	$entry_content = $entry->raw_content;
	
	// And apply the send headers to it
	if ( isset($incomming_data['headers']) and !empty($incomming_data['headers']) ){
		foreach($incomming_data['headers'] as $field => $value)
			$entry_headers[$field] = $value;
	}
	
	$entry_headers['Created'] = strftime('%F %T');
	
	// Determine the path for the new entry
	$parent_id = isset($_GET['id']) ? $_GET['id'] : '';
	$title = Entry::parameterize($entry_headers['Title']);
	$path = Entry::translate_id_to_path($parent_id . '/' . $title);
	
	// Create the files is possible
	if ($incomming_data['type'] == 'plain') {
		if ( file_exists($path) )
			exit_with_error(422, 'Plain with that name already exists');
		
		if ( ! mkdir($path) )
			exit_with_error(500, 'Could not create directory for new plain');
		
		$entry_path = $path . '/' . $_CONFIG['plain_file'];
		if ( ! Entry::save($entry_path, $entry_headers, $entry_content) )
			exit_with_error(500, 'Could not create plain file for new plain');
	} else {
		$entry_path = $path . '.' . $incomming_data['type'];
		if ( file_exists($entry_path) )
			exit_with_error(422, 'Entry with that name and type already exists');
		
		if ( ! Entry::save($entry_path, $entry_headers, $entry_content) )
			exit_with_error(500, 'Could not create new entry');
	}
	
	$entry = Entry::load($entry_path);
	$content = array(
		'id' => $entry->id,
		'type' => $entry->type,
		'headers' => $entry->headers,
		'content' => $entry->content,
		'raw_content' => $entry->raw_content
	);
	header('Content-Type: application/json');
	echo(json_encode($content));
	
	exit();
}
elseif ( !isset($_GET['id']) )
{
	if ( $_SERVER['REQUEST_METHOD'] == 'GET' ) {
		// GET / => 200
		// Build a recursive plain and entry list
		
		$data = load_plain($_CONFIG['data_dir']);
		
		// Throw away the outer plain since it's just the raw data directory
		$data = array(
			'entries' => $data['entries']
		);
		
		// Add the position and scale data
		$user_data = Entry::load($_CONFIG['data_dir'] . '/.user');
		if ($user_data) {
			$data['pos'] = $user_data->position_as_list;
			$data['scale'] = $user_data->scale;
		} else {
			$data['pos'] = array(0, 0);
			$data['scale'] = 1.0;
		}
		
		header('Content-Type: application/json');
		exit(json_encode($data));
	} elseif ( $_SERVER['REQUEST_METHOD'] == 'PUT' ) {
		// PUT / => 204, 500
		// Stores the new position and scale data in the user file
		
		$data = json_decode( file_get_contents('php://input') );
		$headers = array(
			'Position' => join(', ', $data->pos),
			'Scale' => $data->scale
		);
		
		if ( Entry::save($_CONFIG['data_dir'] . '/.user', $headers) )
			header('Content-Type: application/json', false, 204);
		else
			exit_with_error(500, 'Could not save user data');
		exit();
	}
} else {
	// GET /id => 200 or 404
	// PUT /id => 204 or 404
	// DELETE /id => 204 or 404
	
	if ( $_SERVER['REQUEST_METHOD'] == 'GET' ) {
		$path = Entry::translate_id_to_path($_GET['id']);
		if ($path) {
			$raw = @file_get_contents($path);
			if ($raw) {
				$entry = Entry::load_from_string($raw, $path);
				$content = array(
					'id' => $entry->id,
					'type' => $entry->type,
					'headers' => $entry->headers,
					'content' => $entry->content,
					'raw_content' => $entry->raw_content,
					'raw' => $raw
				);
				header('Content-Type: application/json');
				echo(json_encode($content));
			} else {
				exit_with_error(404, 'Could not find entry');
			}
		} else {
			exit_with_error(404, 'Could not find entry');
		}
		exit();
	} elseif ( $_SERVER['REQUEST_METHOD'] == 'PUT' ) {
		$incomming_data = json_decode(file_get_contents('php://input'), true);
		$path = Entry::translate_id_to_path($_GET['id']);
		if ( ! $path )
			exit_with_error(404, 'Could not find entry');
		
		// If we got raw entry data load that, otherwise load the original entry
		if ( isset($incomming_data['raw']) and !empty($incomming_data['raw']) ) {
			$entry = Entry::load_from_string($incomming_data['raw'], $path, false);
		} else {
			$entry = Entry::load($path, false);
		}
		
		$entry_headers = $entry->headers;
		$entry_content = $entry->raw_content;
		
		// Apply the new headers
		if ( isset($incomming_data['headers']) and !empty($incomming_data['headers']) ){
			foreach($incomming_data['headers'] as $field => $value)
				$entry_headers[$field] = $value;
		}
		
		if ( Entry::save($path, $entry_headers, $entry_content) ) {
			if ($entry->type == 'plain') {
				$dir = isset($incomming_data['plain']) ? dirname(Entry::translate_id_to_path($incomming_data['plain'])) : dirname(dirname($path));
				$filename = isset($entry_headers['Title']) ? Entry::parameterize($entry_headers['Title']) : basename(dirname($path));
				
				$old_path = dirname($path);
				$new_path = $dir . '/' . $filename;
				$entry_path = $new_path . '/' . $_CONFIG['plain_file'];
			} else {
				$dir = isset($incomming_data['plain']) ? dirname(Entry::translate_id_to_path($incomming_data['plain'])) : dirname($path);
				$filename = isset($entry_headers['Title']) ? Entry::parameterize($entry_headers['Title']) . '.' . $entry->type : basename($path);
				
				$old_path = $path;
				$new_path = $dir . '/' . $filename;
				$entry_path = $new_path;
			}
			
			if ( $old_path == $new_path or rename($old_path, $new_path) ) {
				$entry = Entry::load($entry_path);
				if ($entry) {
					$content = array(
						'id' => $entry->id,
						'type' => $entry->type,
						'headers' => $entry->headers,
						'content' => $entry->content,
						'raw' => $entry->raw_content
					);
					header('Content-Type: application/json');
					echo(json_encode($content));
				} else {
					exit_with_error(500, 'Failed to read updated entry');
				}
			} else {
				exit_with_error(500, 'Failed to move entry to a new location');
			}
		} else {
			exit_with_error(500, 'Failed to save updated entry');
		}
		exit();
	} elseif ( $_SERVER['REQUEST_METHOD'] == 'DELETE' ) {
		$path = Entry::translate_id_to_path($_GET['id']);
		if ( file_exists($path) ) {
			// If we got a plain file we actually need to rename the directory
			if ( pathinfo($path, PATHINFO_EXTENSION) == 'plain' )
				$path = dirname($path);
			
			if ( rename($path, $path . '.deleted') )
				header('Content-Type: application/json', false, 204);
			else
				exit_with_error(500, 'Could not delete entry');
		} else {
			exit_with_error(404, 'Could not find entry');
		}
		exit();
	}
}

exit_with_error(405, 'Unknown action');

?>