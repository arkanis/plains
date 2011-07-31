<?php

$gramar = array(
	'global' => array(
		'code' => array('bounds' => '/-{4,}/'),
		'paragraphs' => array('delimiter' => '/\n{2,}/', 'content-context' => 'para')
	),
	'para' => array(
		'link' => array('match' => '#https?://[^\s]+#i')
	)
);

function parse($text, $gramar, $start_context)
{
	foreach($gramar[$start_context] as $rule)
	{
		if (array_key_exists('bounds', $rule))
		{
			
		}
		elseif (array_key_exists('delimiter', $rule))
		{
		}
		elseif (array_key_exists('match', $rule))
		{
			$matched = preg_match($rule['match'], $text, &$matches, PREG_OFFSET_CAPTURE);
			if ($matched)
			{
				$pre = substr($text, 0, $matches[0][1]);
				$post = substr($text, $matches[0][1] + strlen($matches[0][0]));
				$text = $pre . parse($text, $gramar, $rule['content-context']) . $post;
			}
		}
		else
		{
			// unknown rule condition
		}
	}
}

?>