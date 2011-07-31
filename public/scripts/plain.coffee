format = (template, values) ->
	lines = template.split("\n")
	code = []
	for line in lines
		code.push("var code = #{ line }")
	
	console.log(code)
	code = code.replace( /<% %>/ )
	code.replace( /:([\w\d]+)/g, (match, name, offset, string) -> 
		console.log(name, values[name], values)
		values[name]
	)

console.log(format('
	<article class="entry note">
		<header>
			<h1 class="movable">:name</h1>
			<ul class="tags">
				<% for(var i = 0; i < tags.length; i++){ %>
				<li><%= tags[i] %></li>
				<% } %>
				<li>engine</li>
			</ul>
		</header>
		<aside>
			<ul class="actions">
				<li class="edit" title="Edit">edit</li>
				<li class="delete" title="Delete">delete</li>
			</ul>
			<span class="movable expander"></span>
		</aside>
	</article>
', {
	name: 'Arkanis', tags: ['pixeltack', 'engine']
}))