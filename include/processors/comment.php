<?php

/**
 * Converts the specified text to HTML using the folowing rules:
 * 
 * - Blocks between two "----" lines are displayed as code blocks
 *   and the content is HTML escaped.
 * - Two or more line breaks start a new paragraph. Everything
 *   within is HTML escaped.
 * - Within a paragraph links are made clickable.
 * - List markers (* and -) at the beginning of a line are replaced
 *   with a bullet character.
 * 
 * Example:
 * 
 * 	A paragraph over
 * 	multiple lines.
 * 	
 * 	A new paragraph.
 * 	
 * 	Code:
 * 	----
 * 	writefln("hello world");
 * 	----
 * 	
 * 	A small fake list:
 * 	* first
 * 	* second
 * 	- with link: http://arkanis.de/
 */
function comment_to_html($text)
{
	return comment_parse_code_blocks($text);
}

/**
 * Parses and escapes the code blocks. None code blocks are handed
 * of to the comment_parse_content_blocks() function.
 */
function comment_parse_code_blocks($text)
{
	$processed_text = '';
	
	$parts = preg_split('/(^|\n)----\n/', $text);
	foreach($parts as $index => $content)
	{
		// All even parts are normal text, odd parts are code (since we start at a normal context)
		if ($index % 2 == 0)
		{
			$processed_text .= comment_parse_content_blocks($content);
		}
		else
		{
			$processed_text .= '<pre><code>' . htmlspecialchars($content, ENT_NOQUOTES, 'UTF-8') . '</code></pre>' . "\n";
		}
	}
	
	return $processed_text;
}

/**
 * Parses normal content blocks. Diveds them into paragraphs and
 * applies the link and bulet rules described above.
 */
function comment_parse_content_blocks($text)
{
	if (trim($text) == '')
		return '';
	
	$paras = preg_split('/\n{2,}/', $text);
	
	$processed_text = '';
	foreach($paras as $para)
	{
		if (trim($para) == '')
			continue;
		
		$para = htmlspecialchars($para, ENT_NOQUOTES, 'UTF-8');
		$para = str_replace('...', '…', $para);
		$para = preg_replace('#https?://[^\s")]+#i', '<a href="$0">$0</a>', $para);
		$para = preg_replace("/((^|\n)\s*)[*-](\s+)/", '$1•$3', $para);
		$processed_text .= '<p>' . trim($para) . '</p>' . "\n";
	}
	
	return $processed_text;
}

?>