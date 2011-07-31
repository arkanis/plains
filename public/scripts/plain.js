function format(template, values){
	//var code = template.replace(/<? ?>/)
	return template.replace(/:([\w\d]+)/g, function(match, name, offset, string){
		return values[name];
	});
}

function doc(comment_function){
	return comment_function.toString();
}

var text = doc(function(){/*!
	<article class="entry note">
		<header>
			<h1 class="movable">Namen für Pixeltack-Engines</h1>
			<ul class="tags">
				<li>pixeltack</li>
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
*/});

console.log(text);

/*
console.log(format(' \
	<article class="entry note"> \
		<header> \
			<h1 class="movable">:name</h1> \
			<ul class="tags"> \
				<% for(var i = 0; i < tags.length; i++){ %> \
				<li><%= tags[i] %></li> \
				<% } %> \
				<li>engine</li> \
			</ul> \
		</header> \
		<aside> \
			<ul class="actions"> \
				<li class="edit" title="Edit">edit</li> \
				<li class="delete" title="Delete">delete</li> \
			</ul> \
			<span class="movable expander"></span> \
		</aside> \
	</article> \
', {
	name: 'Arkanis', tags: ['pixeltack', 'engine']
}))

console.log(format('<section class=":x"> \
	<header class="movable"> \
		<h1>:name</h1> \
	</header> \
</section>', {
	x: 'entry', name: 'Arkanis'
}));

elem = {
	article: {
		header: {
			h1: x,
			ul: y
		},
		aside: {
			ul: x,
			span: y
		}
	}
}

var elems = {
	article: {
		attr: {class: 'entry note'},
		header: {
			h1: {}
		}
	}
}

var elems = {
	ul: {
		attr: {class: 'tags'},
		looped: tags,
		li: undefined,
	}
}
*/