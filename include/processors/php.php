<?php

function path($relative_path)
{
	return $GLOBALS['entry_for_php_processor_functions']->public_path_for($relative_path);
}

function ref($name, $uri = null)
{
	static $references = array();
	
	if ($uri != null)
		return $references[$name] = $uri;
	else
		return @$references[$name];
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