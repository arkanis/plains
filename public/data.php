<?php

require_once('../include/entry.php');


// Markdown processor
require_once('../include/processors/markdown.php');
Entry::add_processor('markdown', function($raw_content, $entry){
	return Markdown($raw_content);
});

// Add a comment processor used to savely process the comments.
require_once('../include/processors/comment.php');
Entry::add_processor('comment', function($raw_content, $entry){
	return comment_to_html($raw_content);
});


$_CONFIG = array(
	'data_dir' => '../data',
	'plain_content_files' => '/.*\.(idea|note)/i',
	'plain_file' => '.plain'
);

function path_to_id($path){
	global $_CONFIG;
	$full_entry_path = realpath($path);
	$full_data_path = realpath($_CONFIG['data_dir']);
	return '/data' . substr($full_entry_path, strlen($full_data_path));
}

function load_plain($plain_path){
	global $_CONFIG;
	
	// First look for a file with data of our plain
	$plain = Entry::load($plain_path . '/' . $_CONFIG['plain_file']);
	if ($plain) {
		$data = array('type' => 'plain', 'id' => path_to_id($plain_path), 'title' => $plain->title, 'color' => $plain->color, 'plain' => $plain->plain_as_list);
	} else {
		// Return default data (just the directory name)
		$data = array('type' => 'plain', 'id' => path_to_id($plain_path), 'title' => basename($plain_path));
	}
	
	$content = array();
	foreach(glob($plain_path . '/*') as $child_path){
		if (is_dir($child_path)) {
			// We got a directory, so go in recursively (dot directories are not returned by glob)
			$content[] = load_plain($child_path);
		} elseif (preg_match($_CONFIG['plain_content_files'], basename($child_path))) {
			// We got a content file, load it and add it's data
			$entry = Entry::load($child_path);
			$content[] = array(
				'type' => $entry->type,
				'id' => path_to_id($child_path),
				'title' => $entry->title,
				'date' => $entry->date_as_time,
				'tags' => $entry->tags_as_list,
				'plain' => $entry->plain_as_list,
				'content' => $entry->content
			);
		}
	}
	
	$data['content'] = $content;
	return $data;
}

function is_path_in_data_dir($path){
	global $_CONFIG;
	$full_entry_path = realpath($path);
	$full_data_path = realpath($_CONFIG['data_dir']);
	return substr($full_entry_path, 0, strlen($full_data_path)) == $full_data_path;
}



if ( !isset($_GET['id']) ) {
	// GET / => 200
	// POST / => 201 or 500
	if ( $_SERVER['REQUEST_METHOD'] == 'GET' ) {
		$data = load_plain($_CONFIG['data_dir']);
		// Throw away the outer plain since it's just the raw data directory
		$data = $data['content'];
		header('Content-Type: application/json');
		exit(json_encode($data));
	} elseif ( $_SERVER['REQUEST_METHOD'] == 'POST' ) {
		// TODO: Figure out the file name for the new entry based on its title
		if ( file_exists($path) ) {
			header('', false, 500);
		} else {
			$content = file_get_contents('php://input');
			if ( file_put_contents($filename, $content) )
				header('Location: http://' . $_SERVER['HTTP_HOST'] . path_to_id($path), false, 201);
			else
				header('', false, 500);
		}
		exit();
	}
} else {
	// GET /id => 200 or 404
	// PUT /id => 204 or 404
	// DELETE /id => 204 or 404
	$path = $_CONFIG['data_dir'] . '/' . $_GET['id'];
	
	// Make sure the path in the id does not escape from the data directory
	if ( !is_path_in_data_dir($path) ){
		header('Content-Type: text/plain', false, 404);
		exit();
	}
	
	if ( $_SERVER['REQUEST_METHOD'] == 'GET' ) {
		$entry = Entry::load($path);
		if ($entry) {
			$content = array(
				'type' => $entry->type,
				'id' => path_to_id($path),
				'title' => $entry->title,
				'date' => $entry->date_as_time,
				'tags' => $entry->tags_as_list,
				'plain' => $entry->plain_as_list,
				'content' => $entry->content,
				'raw_content' => $entry->raw_content
			);
			header('Content-Type: text/plain');
			echo(json_encode($content));
		} else {
			header('Content-Type: text/plain', false, 404);
		}
		exit();
	} elseif ( $_SERVER['REQUEST_METHOD'] == 'PUT' ) {
		$incomming_data = json_decode(file_get_contents('php://input'), true);
		list($entry_headers, $entry_content) = Entry::analyze($path);
		
		if (!$entry_headers){
			header('Content-Type: text/plain', false, 404);
			exit();
		}
		
		foreach($incomming_data as $field => $value)
			$entry_headers[$field] = $value;
		
		if ( isset($incomming_data['content']) and !empty($incomming_data['content']) )
			$entry_content = $incomming_data['content'];
		
		if ( Entry::save($path, $entry_headers, $entry_content) ) {
			// TODO: change the filename according to it's title and add the Location header for the new path
			header('Content-Type: text/plain', false, 204);
		} else {
			header('Content-Type: text/plain', false, 422);
		}
		exit();
	} elseif ( $_SERVER['REQUEST_METHOD'] == 'DELETE' ) {
		if ( rename($path, $path . '.deleted') )
			header('Content-Type: text/plain', false, 204);
		else
			header('Content-Type: text/plain', false, 404);
		exit();
	}
}

header('', false, 405);
exit();

?>