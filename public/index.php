<!DOCTYPE html>
<html lang="en">
<head>
	<meta charset="utf-8">
	<title>Plain</title>
	<style>
		section, article, header { display: block; }
		#templates { display: hidden; }
		
		html, body { margin: 0; padding: 0; }
		body { overflow: hidden; font-family: ubuntu, sans-serif; color: #333; }
		
		body > nav { position: absolute; z-index: 2; top: 0; right: 0; margin: 0; padding: 0.5em; color: #ddd; background: hsl(0, 0%, 20%); border-radius: 0 0 0 10px; }
		body > section#map { position: relative; z-index: 1; width: 100%; height: 100%; }
		body > section#map .entry { position: absolute; }
		
		
		.entry { position: relative; z-index: 1; margin: 0; padding: 10px;
			border-radius: 5px; border-width: 3px; border-style: solid; }
		.entry::before { content: ''; position: absolute; z-index: -1; top: 0; left: 0; bottom: 0; right: 0;
			border-radius: 3px; border-width: 1px; border-style: solid; }
		
		.entry > header { display: table; margin: -10px 24px 10px -10px; padding: 0.2em 0.4em 0.2em 25px;
			border-width: 1px; border-style: solid; border-radius: 5px 0 5px 0;
			background: url(/styles/icons/map.png) no-repeat 4px 50%; }
		/* the special hover styles will override the color scheme styles because the selector is more specific */
		.entry > header:not(:hover) { border-top-color: transparent !important; border-left-color: transparent !important; border-top-left-radius: 0; }
		
		/* entity title and tag list */
		.entry > header > h1 { font-size: 1.25em; margin: 0; padding: 0; }
		.entry > header > ul.tags { margin: 0; padding: 0; list-style: none; }
		.entry > header > ul.tags::before { content: 'Tags: '; }
		.entry > header > ul.tags > li { display: inline; }
		.entry > header > ul.tags > li::after { content: ', '; }
		.entry > header > ul.tags > li:last-child::after { content: ''; }
		
		/* radial action menu */
		.entry > aside > ul.actions { position: absolute; right: 0; top: 0; width: 24px; height: 24px; margin: 0; padding: 0; list-style: none;
			border-width: 1px; border-style: solid; border-radius: 0 0 0 5px;
			background: url(/styles/icons/wrench.png) no-repeat center center; }
		.entry > aside > ul.actions > li { display: none; }
		
		.entry > aside > ul.actions:not(:hover) { border-top-color: transparent !important; border-right-color: transparent !important; }
		.entry > aside > ul.actions:hover { z-index: 1; width: 72px; height: 72px; right: -24px; top: -24px; border-radius: 50%; }
		.entry > aside > ul.actions:hover::before { content: ''; position: absolute; z-index: 3; left: 22px; top: 22px; right: 22px; bottom: 22px;
			border-radius: 50%; background: inherit; border: inherit; }
		
		.entry > aside > ul.actions:hover > li { display: block; position: absolute; width: 72px; height: 36px; text-indent: -10000px; border: inherit; cursor: pointer; }
		.entry > aside > ul.actions:hover > li:hover { z-index: 2; }
		.entry > aside > ul.actions:hover > li.edit { top: -1px; left: -1px; border-radius: 50% 50% 0 0 /100% 100% 0 0; background: url(/styles/icons/pencil.png) no-repeat 50% 3px; }
		.entry > aside > ul.actions:hover > li.edit:not(:hover) { border-top-color: transparent; border-right-color: transparent; border-left-color: transparent; }
		.entry > aside > ul.actions:hover > li.save { top: -1px; left: -1px; width: 36px; border-radius: 100% 0 0 0; background: url(/styles/icons/disk.png) no-repeat 50% 50%; }
		.entry > aside > ul.actions:hover > li.save:not(:hover) { border-top-color: transparent; border-left-color: transparent; }
		.entry > aside > ul.actions:hover > li.close { top: -1px; right: -1px; width: 35px; border-radius: 0 100% 0 0; background: url(/styles/icons/cross.png) no-repeat 50% 50%; }
		.entry > aside > ul.actions:hover > li.close:not(:hover) { border-top-color: transparent; border-right-color: transparent; }
		.entry > aside > ul.actions:hover > li.delete { bottom: -1px; left: -1px; height: 35px; border-radius: 0 0 50% 50% / 0 0 100% 100%; background: url(/styles/icons/delete.png) no-repeat 50% 16px; }
		.entry > aside > ul.actions:hover > li.delete:not(:hover) { border-right-color: transparent; border-bottom-color: transparent; border-left-color: transparent; }
		
		article.entry { font-size: 0.77em; }
		article.entry > header > h1 { font-size: 1em; }
		article.entry > header > ul.tags { font-size: 0.9em; }
		
		/* color schemes for entries */
		.entry { background-color: hsl(0, 0%, 95%); border-color: hsl(0, 0%, 75%); }
		.entry > header, .entry > aside > ul.actions { background-color: hsl(0, 0%, 75%); }
		.entry::before, .entry > header, .entry > aside > ul.actions { border-color: hsl(0, 0%, 50%); }
		.entry > header:hover, .entry > aside > ul.actions:hover > li:hover { border-color: hsl(0, 0%, 60%); background-color: hsl(0, 0%, 80%); }
		.entry > header:active, .entry > aside > ul.actions:hover > li:active { border-color: hsl(120, 50%, 60%); background-color: hsl(120, 25%, 80%); }
		
		/* ideas (green) */
		.entry.idea { background-color: hsl(80, 60%, 95%); border-color: hsl(80, 50%, 75%); }
		.entry.idea > header, .entry.idea > aside > ul.actions { background-color: hsl(80, 50%, 75%); }
		.entry.idea > header { background-image: url(/styles/icons/lightbulb.png); }
		.entry.idea::before, .entry.idea > header, .entry.idea > aside > ul.actions { border-color: hsl(80, 30%, 50%); }
		.entry.idea > header:hover, .entry.idea > aside > ul.actions:hover > li:hover { border-color: hsl(80, 50%, 60%); background-color: hsl(80, 50%, 85%);  }
		.entry.idea > header:active, .entry.idea > aside > ul.actions:hover > li:active { border-color: hsl(120, 50%, 60%); background-color: hsl(120, 25%, 80%); }
		
		/* notes (yellow) */
		.entry.note { background-color: hsl(50, 85%, 95%); border-color: hsl(50, 85%, 70%); }
		.entry.note > header, .entry.note > aside > ul.actions { background-color: hsl(50, 85%, 70%); }
		.entry.note > header { background-image: url(/styles/icons/note.png); }
		.entry.note::before, .entry.note > header, .entry.note > aside > ul.actions { border-color: hsl(50, 40%, 60%); }
		.entry.note > header:hover, .entry.note > aside > ul.actions:hover > li:hover { border-color: hsl(50, 90%, 60%); background-color: hsl(50, 90%, 80%); }
		.entry.note > header:active, .entry.note > aside > ul.actions:hover > li:active { border-color: hsl(50, 70%, 60%); background-color: hsl(50, 90%, 85%); }
		
		
		/*
		article.entry.idea { background-color: #f6fbea; border-color: #c2de88; }
		article.entry.note { background-color: #fbfbeb; border-color: #e9e69b; }
		article.entry > header { margin: 0; padding: 0 0 1px 0; border-bottom: 1px solid rgba(0, 0, 0, 0.1); }
		article.entry > header > h3 { font-size: 1em; margin: -0.5em -0.5em 0 -0.5em; padding: 0.5em 0.5em 0 0.5em; font-weight: bold; }
		article.entry > header > ul { margin: 0; padding: 0 0 0 16px; list-style: none; color: gray; background: left center url(/styles/icons/tags.png) no-repeat; }
		article.entry > header > ul > li { display: inline; }
		article.entry > header > ul > li::after { content: ', '; }
		article.entry > header > ul > li:last-child::after { content: ''; }
		
		section#map.moving { cursor: move; }
		section#map .moving { opacity: 0.5; cursor: move; }
		*/
		/*section#map .movable:hover { background-color: rgba(0, 0, 0, 0.1); }*/
		
		section#map .expander { position: absolute; right: 2px; bottom: 2px; width: 10px; height: 10px; background: url(/styles/resizer.png) no-repeat right bottom; cursor: se-resize; }
	</style>
	<script src="scripts/jquery-1.6.2.min.js"></script>
	<script src="scripts/plain.js"></script>
	<script>
		/**
		 * Infrastructure event handlers that take care of the low level element movement logic.
		 * An element just has to have the 'movable' class.
		 * 
		 * The event handlers on the document handle the low level movement (calculating the
		 * mouse position difference, etc.). Movement of an element is started with an mousedown
		 * event and continues with mousemove events until a mouseup event is received. However
		 * these events are handled by the infrastructure.
		 * 
		 * The "movable" element only receives 'movement-started', 'movement' and 'movement-stopped'
		 * events. The 'movement' event gets the dx and dy as additional parameters, the other two the
		 * current mouse coordinates.
		 */
		$(document).mousedown(function(event){
			var target = $(event.target);
			if ( target.hasClass('pannable') )
				target = $('#map');
			
			if ( target.hasClass('movable') ){
				var elem = $(this);
				elem.data('moving-elem', target).data('mouse-pos', {x: event.pageX, y: event.pageY});
				target.trigger('movement-started', [event.pageX, event.pageY]);
				return false;
			}
		}).mousemove(function(event){
			var elem = $(this);
			var moving_elem = elem.data('moving-elem');
			var mouse_pos = elem.data('mouse-pos');
			if (moving_elem && mouse_pos){
				var dx = event.pageX - mouse_pos.x;
				var dy = event.pageY - mouse_pos.y;
				moving_elem.trigger('movement', [dx, dy]);
				elem.data('mouse-pos', {x: event.pageX, y: event.pageY});
				return false;
			}
		}).mouseup(function(event){
			var elem = $(this);
			var moving_elem = elem.data('moving-elem');
			var mouse_pos = elem.data('mouse-pos');
			
			if (moving_elem || mouse_pos){
				// Be aware: elem.removeData(name) does not seem to work. Therefore we reset
				// the values to null.
				if (moving_elem){
					moving_elem.trigger('movement-stopped', [event.pageX, event.pageY]);
					elem.data('moving-elem', null);
				}
				if (mouse_pos)
					elem.data('mouse-pos', null);
				return false;
			}
		});
		
		$(document).ready(function(){
			// Generate some random test sections
			/*
			for(var i = 0; i < 15; i++){
				var x = Math.random() * 1500;
				var y = Math.random() * 1000;
				var color = Math.floor(Math.random() * 360);
				
				var list = $('<ul>');
				for(var j = 0; j < 5; j++){
					$('<li>xyz</li>').appendTo(list);
				}
				list.css({'top': x + 'px', 'left': y + 'px', 'background-color': 'hsl(' + color + ', 50%, 75%)'});
				
				$('#map > div').append(list);
			}
			*/
			
			function createEntryIn(entryData, plain){
				var entry = null;
				if (entryData.type == 'plain') {
					entry = $('#templates > #plain').clone();
				} else {
					entry = $('#templates > #entry').clone();
					entry.attr('id', entryData.id);
					
					var tags = entry.find('> header > .tags');
					for(var t = 0; t < entryData.tags.length; t++)
						$('<li>').text(entryData.tags[t]).appendTo(tags);
					
					$(entryData.content).appendTo(entry);
					//entry.html();
					//$('<header>').append( $('<h1 class="movable">').attr('id', entryData.id).text(entryData.title), tags ).prependTo(entry);
				}
				
				entry.find('> header > h1').text(entryData.title);
				
				/*
				var actions = $('<ul class="actions">').
					append('<li class="save" title="Save changes">save</li>').
					append('<li class="close" title="Close editor">close</li>').
					append('<li class="delete" title="Delete">delete</li>');
				$('<aside>').append(actions).append( $('<span class="movable expander">') ).appendTo(entry);
				*/
				if (entryData.plain){
					var styles = {left: entryData.plain[0] + 'px', top: entryData.plain[1] + 'px'};
					if (entryData.plain[2])
						styles.width = entryData.plain[2] + 'px';
					if (entryData.plain[3])
						styles.height = entryData.plain[3] + 'px';
					entry.css(styles);
				}
				if (entryData.color)
					entry.css('background-color', entryData.color);
				
				if (entryData.type == 'plain') {
					for(var i = 0; i < entryData.content.length; i++){
						createEntryIn(entryData.content[i], entry);
					}
				}
				
				entry.appendTo(plain).data('entry', entryData);
			}
			
			// Load the initial data
			$.ajax('/data', {success: function(data){
				var rootPlain = $('#map > div');
				for(var i = 0; i < data.length; i++)
					createEntryIn(data[i], rootPlain);
			}});
			
			/**
			 * High level movement logic for plains and entries.
			 */
			$('.entry > .movable', $('#map > div').get(0)).live({
				'movement-started': function(){
					$(this).closest('.entry').addClass('moving');
					return false;
				},
				'movement': function(event, dx, dy){
					var entry = $(this).closest('.entry');
					var left = parseFloat(entry.css('left')) || 0;
					var top = parseFloat(entry.css('top')) || 0;
					var scale = $('#map').data('view').scale;
					entry.css({left: (left + dx / scale) + 'px', top: (top + dy / scale) + 'px'});
					return false;
				},
				'movement-stopped': function(event, x, y){
					var entry = $(this).closest('.entry');
					entry.removeClass('moving');
					var data = entry.data('entry');
					
					if (!data.plain)
						data.plain = [];
					
					data.plain[0] = parseFloat(entry.css('left')) || 0;
					data.plain[1] = parseFloat(entry.css('top')) || 0;
					
					$.ajax({
						type: 'PUT', url: data.id,
						data: JSON.stringify({"Plain": data.plain.join(", ")}),
						error: function(jqXHR, textStatus, errorThrown){
							//alert("could not save possition!", jqXHR, textStatus, errorThrown);
							console.log(jqXHR, textStatus, errorThrown);
						}
					});
					return false;
				}
			});
			
			/**
			 * Resizing logic for plains and entries. We move the expander around but
			 * instead of changing a position we just change the size of the entry.
			 */
			$('.entry .expander', $('#map > div').get(0)).live({
				'movement-started': function(){
					$(this).closest('.entry').addClass('expanding');
					return false;
				},
				'movement': function(event, dx, dy){
					var entry = $(this).closest('.entry');
					var width = parseFloat(entry.css('width')) || entry.outerWidth();
					var height = parseFloat(entry.css('height')) || entry.outerHeight();
					var scale = $('#map').data('view').scale;
					entry.css({width: (width + dx / scale) + 'px', height: (height + dy / scale) + 'px'});
					return false;
				},
				'movement-stopped': function(){
					$(this).closest('article, section').removeClass('expanding');
					return false;
				}
			});
			
			// Make sure the map covers the complete window. Therefore we adjust the size
			// on every resize and once after loading.
			var map_size_maintainer = function(){
				$('#map').width( $(window).width() ).height( $(window).height() );
			};
			$(window).resize(map_size_maintainer);
			map_size_maintainer();
			
			// Wire the map events for movement and zoom
			$('#map').data('view', {x: 0, y: 0, scale: 1}).bind({
				'movement-started': function(event, x, y){
					$(this).addClass('moving');
					return false;
				},
				'movement': function(event, dx, dy){
					var elem = $(this);
					var view = elem.data('view');
					
					elem.data('view').x += dx;
					elem.data('view').y += dy;
					elem.children().trigger('update');
					
					return false;
				},
				'movement-stopped': function(event, x, y){
					$(this).removeClass('moving');
					return false;
				},
				'zoom': function(event, dir, pivotX, pivotY){
					var elem = $(this);
					var view = elem.data('view');
					var scale = (dir > 0) ? 1.1 : 1 / 1.1;
					
					// Scale the origin relative to the pivot point
					// new origin = pivot + (origin - pivot) * scale
					var px = view.x - pivotX;
					var py = view.y - pivotY;
					view.x = pivotX + px * scale;
					view.y = pivotY + py * scale;
					view.scale *= scale;
					
					elem.data('view', view);
					elem.children().trigger('update');
				},
				'mousewheel': function(event){
					// Map the Opera, Chrome and IE mouse wheel events to our custom zoom event
					var dir = (event.wheelDelta >= 0) ? 1 : -1;
					return $(this).triggerHandler('zoom', [dir, event.pageX, event.pageY]);
				},
				'DOMMouseScroll': function(event){
					// Map the Gecko mouse wheel events to our custom zoom event
					var dir = (event.detail < 0) ? 1 : -1;
					return $(this).triggerHandler('zoom', [dir, event.pageX, event.pageY]);
				}
			}).find('> div').bind('update', function(event){
				var map = $(this).parent();
				var view = map.data('view');
				var transform = 'translate(' + view.x + 'px, ' + view.y + 'px) scale(' + view.scale + ')';
				$(event.target).css({
					'-o-transform-origin': 'left top',
					'-o-transform': transform,
					'-moz-transform-origin': 'left top',
					'-moz-transform': transform,
					'-webkit-transform-origin': 'left top',
					'-webkit-transform': transform,
					'-ms-transform-origin': 'left top',
					'-ms-transform': transform,
					'transform-origin': 'left top',
					'transform': transform
				});
				return false;
			});
		});
	</script>
</head>
<body>

<nav>
	Plain Experiment
</nav>

<section id="map" class="movable">
	<div>
		<!--
		<article class="idea">
			<header>
				<h3>Futex Cheat Sheet</h3>
			</header>
			
			<p>Reference of the futex kernel API. Useful for building you own synchronization primitives.</p>
		</article>
		-->
	</div>
</section>

<aside id="templates">
	<section id="plain" class="entry pannable">
		<header>
			<h1 class="movable">title</h1>
		</header>
		<aside>
			<ul class="actions">
				<li class="edit" title="Edit">edit</li>
				<li class="save" title="Save changes">save</li>
				<li class="close" title="Close editor">close</li>
				<li class="delete" title="Delete">delete</li>
			</ul>
			<span class="movable expander"></span>
		</aside>
	</section>
	
	<article id="entry" class="entry note">
		<header>
			<h1 class="movable" id="/data/arkanis/test.idea">title</h1>
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
</aside>

</body>
</html>