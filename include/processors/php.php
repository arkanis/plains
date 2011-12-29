<?php

function load($path, $heading_level = 1)
{
	global $_CONFIG;
	
	$entry_dir = dirname($_CONFIG['data_dir'] . $GLOBALS['entry_for_php_processor_functions']->id);
	$target_path = $entry_dir . '/' . $path;
	
	ob_start();
	include($target_path);
	$entry_data = ob_get_clean();
	
	$loaded_entry = Entry::load_from_string($entry_data);
	$content = preg_replace_callback('/^\s*(#+)\s+(.*)$/m', function($match) use($heading_level) {
		$level = strlen($match[1]);
		$text = $match[2];
		return str_repeat('#', $heading_level + $level) . ' ' . $text;
	}, $loaded_entry->raw_content);
	return str_repeat('#', $heading_level) . ' ' . $loaded_entry->title . "\n\n" . $content . "\n";
}

/**
 * Based upon the markdown mail escape routine encodeEmailAddress().
 */
function obfuscate($text)
{
	$seed = (int)abs(crc32($text) / strlen($text)); # Deterministic seed.
	$chars = str_split($text);
	foreach($chars as $index => $char)
	{
		$ord = ord($char);
		if ($ord < 128)	// Ignore non-ascii chars.
		{
			$r = ($seed * (1 + $index)) % 100; // Pseudo-random function.
			// roughly 10% raw, 45% hex, 45% dec
			// '@' *must* be encoded. I insist.
			if ($r > 90 && $char != '@')
				/* do nothing */;
			elseif ($r < 45)
				$chars[$index] = '&#x' . dechex($ord) . ';';
			else
				$chars[$index] = '&#' . $ord . ';';
		}
	}
	
	return implode('', $chars);
}

?>