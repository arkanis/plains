<?php

require_once('../include/entry.php');

$_CONFIG = array(
	'data_dir' => '../data',
	'plain_content_files' => '/.*\.(idea|note)/i',
	'plain_file' => '.plain'
);


/**
 * The public path of normal entities is the path relative to the data directory. We append
 * a "/data" to force the entries into this URL "directory" in requests.
 */
function public_entry_path($path){
	global $_CONFIG;
	$full_entry_path = realpath($path);
	$full_data_path = realpath($_CONFIG['data_dir']);
	return '/data' . substr($full_entry_path, strlen($full_data_path));
}

/**
 * The public path for plain files is just the directory, without the "/.plain" file. This makes
 * sure we can handle this transparently with directories without a plain file.
 */
Entry::add_router('plain', function($id, $path, $entry) use($_CONFIG) {
	$public_path = public_entry_path($path);
	return substr($public_path, 0, strlen($public_path) - strlen('/' . $_CONFIG['plain_file']));
});

/**
 * Add a default route for all other types (notes, ideas, etc).
 */
Entry::add_router(null, function($id, $path, $entry) use($_CONFIG) {
	return public_entry_path($path);
});

if ( !isset($_GET['id']) ) {
	// GET / => 200
	// POST / => 201 or 500
	if ( $_SERVER['REQUEST_METHOD'] == 'GET' ) {
		$data = array();
		
		$dir_iterator = new RecursiveDirectoryIterator($_CONFIG['data_dir'], FilesystemIterator::SKIP_DOTS | FilesystemIterator::FOLLOW_SYMLINKS);
		$files = new RecursiveIteratorIterator($dir_iterator, RecursiveIteratorIterator::SELF_FIRST);
		foreach($files as $path){
			if ( is_dir($path) ) {
				// Got a plain
				$plain_file = $path . '/' . $_CONFIG['plain_file'];
				if ( file_exists($plain_file) ) {
					// Read the available info from the plain file
					$entry = Entry::load($plain_file);
					$data[] = array('type' => 'plain', 'id' => $entry->public_path, 'title' => $entry->title, 'color' => $entry->color, 'plain' => $entry->plain_as_list);
				} else {
					// Return default data (just the directory name)
					$data[] = array('type' => 'plain', 'id' => public_entry_path($path), 'title' => basename($path));
				}
			} elseif ( preg_match($_CONFIG['plain_content_files'], basename($path)) ) {
				// Got a content file
				$entry = Entry::load($path);
				$data[] = array(
					'type' => $entry->type,
					'id' => $entry->public_path,
					'title' => $entry->title,
					'date' => $entry->date_as_time,
					'tags' => $entry->tags_as_list,
					'plain' => $entry->plain_as_list,
					'content' => $entry->content
				);
			}
		}
		
		header('Content-Type: application/json');
		exit(json_encode($data));
	} elseif ( $_SERVER['REQUEST_METHOD'] == 'POST' ) {
		$filename = date('Y-m-d-H-i-s') . '.note';
		$content = file_get_contents('php://input');
		if ( file_put_contents($filename, $content) )
			header('Location: http://' . $_SERVER['HTTP_HOST'] . dirname($_SERVER['PHP_SELF']) . '/' . $filename, false, 201);
		else
			header('', false, 500);
		exit();
	}
} else {
	// GET /id => 200 or 404
	// PUT /id => 204 or 404
	// DELETE /id => 204 or 404
	$id = basename($_GET['id'] . '.note');
	
	if ( $_SERVER['REQUEST_METHOD'] == 'GET' ) {
		$content = file_get_contents($id);
		if ($content)
			echo($content);
		else
			header('Content-Type: text/plain', false, 404);
		exit();
	} elseif ( $_SERVER['REQUEST_METHOD'] == 'PUT' ) {
		$content = file_get_contents('php://input');
		if ( file_put_contents($id, $content) )
			header('Content-Type: text/plain', false, 204);
		else
			header('Content-Type: text/plain', false, 404);
		exit();
	} elseif ( $_SERVER['REQUEST_METHOD'] == 'DELETE' ) {
		if ( unlink($id) )
			header('Content-Type: text/plain', false, 204);
		else
			header('Content-Type: text/plain', false, 404);
		exit();
	}
}

header('', false, 405);
exit();

?>