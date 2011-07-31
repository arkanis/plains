<!DOCTYPE html>
<html lang="en">
<head>
	<meta charset="utf-8">
	<title>Space</title>
	<style>
		html, body { margin: 0; padding: 0; }
		body { overflow: hidden; font-family: ubuntu, sans-serif; color: #333; }
		
		body > nav { position: absolute; z-index: 2; top: 0; right: 0; margin: 0; padding: 0.5em; color: #ddd; background: hsl(0, 0%, 20%); border-radius: 0 0 0 10px; }
		body > section#map { position: relative; z-index: 1; width: 100%; height: 600px; border: 1px solid blue; }
		
		section#map.dragging { cursor: move; }
		section#map > div { border: 1px solid red; }
		section#map > div > ul { position: absolute; margin: 0; padding: 1em; list-style: none; border-radius: 5px; }
		section#map > div > ul:nth-of-type(1) { top: 0; left: 0; background: hsl(150, 50%, 75%); }
		section#map > div > ul:nth-of-type(2) { top: 400px; left: 450px; background: hsl(270, 50%, 75%); }
	</style>
	<script src="jquery-1.6.2.min.js"></script>
	<script>
		$(document).ready(function(){
			// Generate some random test sections
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
			
			$('#map').data('dragging', false).data('view', {x: 0, y: 0, scale: 1}).mousedown(function(event){
				if (event.target == this)
					$(this).data('dragging', true).data('mouse-pos', {x: event.pageX, y: event.pageY}).addClass('dragging');
			}).mouseup(function(){
				$(this).data('dragging', false).removeClass('dragging');
			}).mousemove(function(event){
				var elem = $(this);
				if (elem.data('dragging')){
					var dx = event.pageX - elem.data('mouse-pos').x;
					var dy = event.pageY - elem.data('mouse-pos').y;
					var view = elem.data('view');
					
					elem.data('view').x += dx;
					elem.data('view').y += dy;
					elem.children().trigger('update');
					
					elem.data('mouse-pos', {x: event.pageX, y: event.pageY});
				}
				return false;
			}).bind('zoom', function(event, dir, pivotX, pivotY){
				var elem = $(this);
				var view = elem.data('view');
				var pivot = elem.data('mouse-pos');
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
				return false;
			}).bind('mousewheel', function(event){
				// Map the Opera, Chrome and IE mouse wheel events to our custom zoom event
				var dir = (event.wheelDelta >= 0) ? 1 : -1;
				return $(this).triggerHandler('zoom', [dir, event.pageX, event.pageY]);
			}).bind('DOMMouseScroll', function(event){
				// Map the Gecko mouse wheel events to our custom zoom event
				var dir = (event.detail > 0) ? 1 : -1;
				return $(this).triggerHandler('zoom', [dir, event.pageX, event.pageY]);
			}).find('> div').bind('update', function(event){
				var map = $(this).parent();
				var view = map.data('view');
				var transform = 'translate(' + view.x + 'px, ' + view.y + 'px) scale(' + view.scale + ')';
				//console.log("transform", transform);
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
	Space Experiment
</nav>

<section id="map">
	<div>
		<ul>
			<li>Bla</li>
			<li>blub</li>
			<li>hallo</li>
			<li>Welt</li>
		</ul>
		<ul>
			<li>asf</li>
			<li>asf</li>
			<li>4et poiuas</li>
			<li>jghjg</li>
		</ul>
	</div>
</section>

</body>
</html>