<!DOCTYPE html>
<html lang="en">
<head>
	<meta charset="utf-8">
	<title>Plain</title>
	<link rel="stylesheet" href="/styles/pastel.css" type="text/css" />
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
			/**
			 * This event sends the updated entry shape to the server once an entry finished
			 * moving or resizing.
			 */
			$('.entry', $('#map > div').get(0)).live({
				'reshaped': function(){
					var entry = $(this);
					var data = entry.data('entry');
					
					if (!data.shape)
						data.shape = [];
					
					data.shape[0] = parseFloat(entry.css('left')) || 0;
					data.shape[1] = parseFloat(entry.css('top')) || 0;
					data.shape[2] = entry.width();
					if (data.type == 'plain')
						data.shape[3] = entry.height();
					
					$.ajax(data.id, {
						type: 'PUT',
						data: JSON.stringify({headers: {'Shape': data.shape.join(', ')}})
					});
					
					return false;
				},
				'sync-to-data': function(){
					var entry = $(this);
					var data = entry.data('entry');
					
					if (data.headers.tags){
						var tags = data.headers.tags.split(',');
						var tag_list = entry.find('> header > .tags');
						tag_list.find('> li').remove();
						for(var t = 0; t < tags.length; t++)
							$('<li>').text(tags[t]).appendTo(tag_list);
					}
					
					if (data.content && data.content != ''){
						var content_wrapper = entry.find('> div');
						if ( content_wrapper.size() == 0 )
							content_wrapper = $('<div>').appendTo(entry);
						content_wrapper.html(data.content);
					}
					
					// If a plain ID changes the IDs of all child entries change as well. To avoid an
					// update cascade for all children we monkey patch all IDs. Just changing the
					// old ID part with the new ID.
					var old_id = entry.attr('id');
					var new_id = data.id;
					entry.find('.entry').attr('id', function(index, attr){
						var updated_id = new_id + attr.substr(old_id.length);
						$(this).data('entry').id = updated_id;
						return updated_id;
					});
					
					entry.attr('id', new_id).addClass(data.type).
						find('> header > h1').text(data.headers.title);
					
					if (data.headers.color)
						entry.css('background-color', data.headers.color);
				},
				'resize': function(){
					var entry = $(this);
					var header_hight = entry.find('> header').outerHeight();
					
					entry.find('> textarea').css({
						position: 'absolute',
						top: header_hight + 5, left: 5,
						width: entry.width(),
						height: entry.height() - header_hight - 5
					});
				}
			});
			
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
					entry.removeClass('moving').trigger('reshaped');
					return false;
				}
			});
			
			/**
			 * Resizing logic for plains and entries. We move the expander around but
			 * instead of changing a position we just change the size of the entry.
			 */
			$('.entry > aside > .expander', $('#map > div').get(0)).live({
				'movement-started': function(){
					$(this).closest('.entry').addClass('expanding');
					return false;
				},
				'movement': function(event, dx, dy){
					var entry = $(this).closest('.entry');
					var width = parseFloat(entry.css('width')) || entry.outerWidth();
					var height = parseFloat(entry.css('height')) || entry.outerHeight();
					var scale = $('#map').data('view').scale;
					
					entry.css('width', (width + dx / scale) + 'px');
					if (entry.is('section') || entry.hasClass('editing'))
						entry.css('min-height', (height + dy / scale) + 'px');
					entry.trigger('resize');
					
					return false;
				},
				'movement-stopped': function(){
					$(this).closest('article, section').removeClass('expanding').trigger('reshaped');
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
					
					var view = $(this).data('view');
					jQuery.ajax('/data', { type: 'PUT', data: JSON.stringify({pos: [view.x, view.y], scale: view.scale}) });
					
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
					jQuery.ajax('/data', { type: 'PUT', data: JSON.stringify({pos: [view.x, view.y], scale: view.scale}) });
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
				},
				'reset-zoom': function(event){
					var view = $(this).data('view');
					view.scale = 1.0;
					$(this).data('view', view).children().trigger('update');
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
			
			
			//
			// Load the initial data
			//
			function createEntryIn(entryData, plain){
				var entry = null;
				if (entryData.type == 'plain') {
					entry = $('#templates > #plain').clone();
				} else {
					entry = $('#templates > #entry').clone();
				}
				
				if (entryData.headers.shape){
					var shape = entryData.headers.shape.split(',');
					var styles = {left: parseFloat(shape[0]) + 'px', top: parseFloat(shape[1]) + 'px'};
					if (shape[2])
						styles.width = parseFloat(shape[2]) + 'px';
					if (shape[3])
						styles.minHeight = parseFloat(shape[3]) + 'px';
					entry.css(styles);
				}
				
				entry.data('entry', entryData).appendTo(plain).trigger('sync-to-data');
				
				if (entryData.type == 'plain') {
					for(var i = 0; i < entryData.entries.length; i++){
						createEntryIn(entryData.entries[i], entry);
					}
				}
			}
			
			$.ajax('/data', {success: function(data){
				// Apply the stored position and scale data
				$('#map').data('view', {
					x: parseFloat(data.pos[0]),
					y: parseFloat(data.pos[1]),
					scale: parseFloat(data.scale)
				}).children().trigger('update');
				
				// Add the entries to the DOM
				var rootPlain = $('#map > div');
				for(var i = 0; i < data.entries.length; i++){
					createEntryIn(data.entries[i], rootPlain);
				}
			}});
		});
	</script>
</head>
<body>

<nav>
	<ul>
		<li class="new note" data-type="note" title="New note">new note</li>
		<li class="new idea" data-type="idea" title="New idea">new idea</li>
		<li class="new plain" data-type="plain" title="New plain">new plain</li>
		<li class="reset-zoom" title="Reset zoom">reset zoom</li>
	</ul>
	
	Plains	
</nav>

<section id="map" class="movable">
	<div>
	</div>
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
			<span class="movable expander"></span>
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
			<span class="movable expander"></span>
		</aside>
	</article>
</aside>

</body>
</html>