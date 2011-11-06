<!DOCTYPE html>
<html>
<head>
	<meta charset="utf-8">
	<title><?= htmlspecialchars($content['headers']['title'], ENT_NOQUOTES, 'UTF-8') ?></title>
	<style>
		html { margin: 0; padding: 0; }
		body { margin: 1em 2em; padding: 0; font-size: medium; font-family: sans-serif; color: hsl(0, 0%, 15%); }
		
		table { margin: 1em; border-spacing: 0; }
		table th { text-shadow: 0 1px 1px white; }
		table tr:nth-child(odd) { background: #eaeaea; } 
		table th, table td { padding: 0.125em 0.5em; }
		table th { font-size: 0.77em; }
		
		table.numeric-data td { text-align: right; }
		
		figure { display: table; margin: 1em auto; padding: 0; }
		figure > figcaption { display: table-caption; font-size: 0.77em; margin: 0 0 0.5em 0; padding: 0; text-align: center; }
		figure.inline { float: right; margin: 0 0 0.5em 0.5em; }
	</style>
</head>
<body>

<?= $content['content'] ?>

</body>
</html>