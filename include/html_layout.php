<!DOCTYPE html>
<html>
<head>
	<meta charset="utf-8">
	<meta http-equiv="content-type" content="text/html; charset=utf-8">
	<title><?= htmlspecialchars($content['headers']['title'], ENT_NOQUOTES, 'UTF-8') ?></title>
<?	if ( isset($content['headers']['style']) ): ?>
	<link rel="stylesheet" type="text/css" href="/styles/<?= htmlspecialchars($content['headers']['style'], ENT_QUOTES, 'UTF-8') ?>.css">
<?	else: ?>
	<link rel="stylesheet" type="text/css" href="/styles/document.css">
<?	endif ?>
</head>
<body>

<?= $content['content'] ?>

</body>
</html>
