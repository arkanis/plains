<!DOCTYPE html>
<html lang="en">
<head>
	<meta charset="utf-8">
	<title>Plain</title>
	<link rel="stylesheet" href="styles/pastel.css" type="text/css" />
	<script src="scripts/jquery-1.6.2.min.js"></script>
	<script src="scripts/plain.js"></script>
	<script>
		$(document).ready(function(){
			loadRootPlain('./data/<?= basename($_GET['user']) ?>', $('#map'));
		});
	</script>
</head>
<body class="movable">

<nav>
	<ul>
		<li class="new note" data-type="note" title="New note">new note</li>
		<li class="new idea" data-type="idea" title="New idea">new idea</li>
		<li class="new plain" data-type="plain" title="New plain">new plain</li>
		<li class="reset-zoom" title="Reset zoom">reset zoom</li>
	</ul>
	
	Plains
</nav>

<section id="editor">
  <h1>Editor</h1>
  <nav>
    <ul class="actions">
      <li class="close" title="Close Editor">Close      
      <li class="save" title="Save Changes">Save
    </ul>
  </nav>
  <textarea></textarea>
</section>

<section id="map" class="movable">
</section>

<aside id="templates">
	<section id="plain" class="entry pannable">
		<header class="movable">
			<h1>title</h1>
		</header>
		<aside>
			<ul class="actions">
				<li class="create" title="Create">create</li>
				<li class="abort" title="Abort">abort</li>
				
				<li class="edit" title="Edit">edit</li>
				<li class="save" title="Save changes">save</li>
				<li class="close" title="Close editor">close</li>
				
				<li class="delete" title="Delete">delete</li>
			</ul>
			<span class="movable resizer"></span>
		</aside>
	</section>
	
	<article id="entry" class="entry">
		<header class="movable">
			<h1 id="/data/arkanis/test.idea">title</h1>
			<ul class="tags">
			</ul>
		</header>
		<aside>
			<ul class="actions">
				<li class="create" title="Create">create</li>
				<li class="abort" title="Abort">abort</li>
				
				<li class="edit" title="Edit">edit</li>
				<li class="save" title="Save changes">save</li>
				<li class="close" title="Close editor">close</li>
				
				<li class="delete" title="Delete">delete</li>
			</ul>
			<span class="movable resizer"></span>
		</aside>
	</article>
</aside>

</body>
</html>
