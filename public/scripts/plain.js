// don't pollute global-namespace
(function() {

	/**
	 * Some kind of small config. The data path is the path to the directory that functions as AJAX
	 * interface for the entry data.
	 */
	var data_path = './data/steven';

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
		
	}).keydown(function(evt) {
		
		// Only use the keys to navigate the map if no other element (e.g. textarea) has the focus
		if ( $(evt.target).is('body') ){
			var keys = {      
				LEFT: 37,
				UP: 38,
				RIGHT: 39,
				DOWN: 40,
				PLUS: 107,
				MINUS: 109
			};
			
			var map = $('#map');
			
			switch(evt.keyCode) {
			
				case keys.LEFT:
					map.trigger('movement', [25, 0]);                 
					return false;
			
				case keys.RIGHT:
					map.trigger('movement', [-25, 0]);
					return false;
				
				case keys.UP:
					map.trigger('movement', [0, 25]);
					return false;
					
				case keys.DOWN:
					map.trigger('movement', [0, -25]);
					return false;
					
				case keys.PLUS:
					map.data('view').scale = map.data('view').scale * 1.1;
					map.trigger('interacted');
					return false;
					
				case keys.MINUS:
					map.data('view').scale = map.data('view').scale / 1.1;
					map.trigger('interacted');
					return false;
			}
		}
	});

	$(document).ready(function(){
		/**
		 * The map is the "canvas" for all our entries. The zoom and panning is achieved via CSS transformations.
		 * The current position and scale is stored within the map DOM node and on the `interacting` event
		 * the CSS `transform` property is updated with the new data. So once you changed the data remember to
		 * trigger the `interacting` event. This position and scale data is send to the server on every
		 * `interacted` event. For example `interacting` is tiggered on every mouse move during panning
		 * and `interacted` is triggered just once at the end of the panning.
		 */
		$('#map').data('view', {x: 0, y: 0, scale: 1}).bind({
			'interacting': function(){
				var map = $(this);
				var view = map.data('view');
				var transform = 'scale(' + view.scale + ') translate(' + view.x + 'px, ' + view.y + 'px)';
				map.css({
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
			},
			'interacted': function(){
				var map = $(this);
				var view = map.data('view');
			
				// Delay the AJAX request to store the view information a bit. In case this event is triggered
				// many times consecutively (e.g. while zooming with the mouse wheel) we avoid unnecessary
				// load on the server.
				var storage_countdown = map.data('storage-countdown');
				if (storage_countdown)
					clearTimeout(storage_countdown);
			
				storage_countdown = setTimeout(function(){
					jQuery.ajax(data_path, { type: 'PUT', data: JSON.stringify({pos: [view.x, view.y], scale: view.scale}) });
					map.data('storage-countdown', null);
				}, 2000);
				map.data('storage-countdown', storage_countdown);
			
				$(this).trigger('interacting');
				return false;
			}
		});
	
		/**
		 * Movement and zoom events of the map are bound to both, the map and the body element. Because the body
		 * element covers everything we don't have to adjust the size of the map element to cover the whole page. This
		 * seems to be buggy with modern browsers as soon as CSS transformations are added to the mix.
		 * 
		 * Movement is done through the movement events triggered above. Zooming is done by capturing some browser
		 * dependent mouse wheel events and mapping them to the custom `zoom` event. This event then changes the scale
		 * and rescales the map origin in a way that the pixel below the mouse does not move. If a mouse wheel event is
		 * triggered on a textarea it is ignored. Otherwise we would break textarea scolling.
		 * 
		 * The `reset-zoom` event resets the scale to 1.0 and adjusts the origin so the center of the page stays in the
		 * center. This is not a real event, you can trigger it to reset the scale. So it's more or less some kind of function.
		 * 
		 * Note: We have to use `live()` for the movement events. If we use `bind()` these events seem to be processed before
		 * any live events. This makes it impossible for the live events of entries to supress map panning e.g. when an entry is
		 * moved or resized.
		 */
		$('#map, body').live({
			'movement-started': function(event, x, y){
				$('#map').addClass('moving');
				return false;
			},
			'movement': function(event, dx, dy){
				var elem = $('#map');
				var view = elem.data('view');
			
				elem.data('view').x += dx / view.scale;
				elem.data('view').y += dy / view.scale;
				elem.trigger('interacting');
			
				return false;
			},
			'movement-stopped': function(event, x, y){
				$('#map').removeClass('moving').trigger('interacted');
				return false;
			}
		});
	
		/**
		 * For the browser events like mousewheel, etc. we have to use `bind()` again. `live()` does not support all events
		 * and we only need the `live()` stuff for the movement events anyway.
		 */
		$('#map, body').bind({
			'zoom': function(event, dir, page_x, page_y){
				var elem = $('#map');
				var view = elem.data('view');
				var scale_multiplier = (dir > 0) ? 1.1 : 1 / 1.1;
			
				// page to world space:
				// w = -view + p / view_scale
				// 
				// world to page space:
				// p = (view + w) * view_scale
			
				// After one sheet full of math I no longer fully understand why this works, but it does
				// quite well.
				view.x = view.x + page_x / (view.scale * scale_multiplier) - page_x / view.scale;
				view.y = view.y + page_y / (view.scale * scale_multiplier) - page_y / view.scale;
				view.scale *= scale_multiplier;
			
				elem.data('view', view).trigger('interacted');
				return false;
			},
			'mousewheel': function(event){
				// Map the Opera, Chrome and IE mouse wheel events to our custom zoom event
				var dir = (event.wheelDelta >= 0) ? 1 : -1;
				if ( !$(event.target).is('textarea') )
					return $('#map').triggerHandler('zoom', [dir, event.pageX, event.pageY]);
			},
			'DOMMouseScroll': function(event){
				// Map the Gecko mouse wheel events to our custom zoom event
				var dir = (event.detail < 0) ? 1 : -1;
				if ( !$(event.target).is('textarea') )
					return $('#map').triggerHandler('zoom', [dir, event.pageX, event.pageY]);
			},
			'reset-zoom': function(event){
				var map = $('#map');
				var view = map.data('view');
				var w = $(window).width();
				var h = $(window).height();
			
				map.data('view').x -= w / view.scale / 2 - w / 2;
				map.data('view').y -= h / view.scale / 2 - h / 2;
				map.data('view').scale = 1.0;
				map.trigger('interacted');
			}
		});
		 
		var map_node = $('#map').get(0);
	
	
		/** 
		 * Editor
		 */
		 // initialize texteditor
		(function() {
			var editor = $('#editor');
			editor.data('textarea', ExtendedTextarea(editor.find('> textarea')[0]));	  	  
		})();
	
		$('#editor').bind({
			
			create: function(evt, entry) {	
			
				var editor = $(this),
						entry = $(entry); 
				
				editor.data('editing', entry);
				editor.addClass(entry.data('entry').type);
				editor.find('> h1').html(entry.data('entry').headers.title);

				editor.data('textarea').val("Title: \nProcessors: markdown");			  
				entry.trigger('interacting');	
				editor.show().data('textarea').focus();
			},
			
			edit: function(evt, entry) {
			
				var editor = $(this),
						entry = $(entry);
				
				editor.data('editing', entry);
				editor.addClass(entry.data('entry').type);	      
				editor.find('> h1').html(entry.data('entry').headers.title);
				
				jQuery.ajax(entry.attr('id'), {dataType: 'json', success: function(data){
					editor.data('textarea').val(data.raw);
					entry.trigger('interacting');				
					editor.show().data('textarea').focus();
				}});			
			
			},
			
			save: function(evt) {
			
				var editor = $(this),
						entry = editor.data('editing');
		
				// update
				if(!entry.hasClass('creating'))		
					jQuery.ajax(entry.attr('id'), {
						type: 'PUT', 
						data: JSON.stringify({raw: editor.data('textarea').val()}),
						dataType: 'json', 
						success: function(data){
							entry.data('entry', data).trigger('content-updated');
							editor.trigger('close');
						}
					});
					
				// create
				else {
					
					var parent_entry = entry.parents('section').eq(0);
					
					if (parent_entry.data('entry') && parent_entry.data('entry').headers.origin)
						var parent_id = parent_entry.data('entry').headers.origin;
					else					
						var parent_id = entry.parents('section').eq(0).attr('id');
					
					// If the section element is the map root use the root plain as parent
					if(parent_id == 'map')
						parent_id = data_path;
					
					jQuery.ajax(parent_id, {		  
						type: 'POST', 
						data: JSON.stringify({
							raw: editor.data('textarea').val(),
							type: entry.data('entry').type,
							headers: {'Shape': entry.data('entry').headers.shape}
						}), 
						success: function(data){
							data.id = parent_id + data.id;
							entry.removeClass('creating').data('entry', data).trigger('content-updated');
							editor.trigger('close');
						}
					});	      
				}
			},
			
			abort: function(evt, entry) {
				entry.remove();
				$(this).trigger('close');
			},
			
			close: function(evt) {
				$(this).hide();
				$(this).removeClass('note').removeClass('idea').removeClass('plain');
			},	    
			
			keydown: function(evt) {
			
				var keys = {
					TAB: 9,
					ESC: 27,
					S: 83,
					ENTER: 13
				};
				
				var editor = $('#editor'),
						textarea = editor.data('textarea');	           
					
				switch(evt.keyCode) {
				
					case keys.TAB:	          
						// backtab
						if(evt.shiftKey) {        
							textarea.applyMultilineTabs(true);
							
						// normal tab
						} else {	          
							if(textarea.hasMultilineSelection()) {
								textarea.applyMultilineTabs(false);
							} else {
								// we only have a cursor
								textarea.insertTab();
							}	        
						}
						return false;   
					
					case keys.ESC:
						$('#editor').trigger('close');
						return false;
					
					// overrides default behaviour (save document)
					case keys.S:
						if(!!evt.ctrlKey) {
							$('#editor').trigger('save');
							return false;
						}
						break;	         
				}
			}	
		}); 
	
		$('#editor .save').click(function() { $('#editor').trigger('save'); return false; });	  
		$('#editor .close').click(function() { $('#editor').trigger('close'); return false; });
	
		/**
		 * Entry interface:
		 * 
		 * To change the shape (position and size) of an entry use the `shape` data object of the DOM node. After
		 * changing the data there trigger the `interacting` event. It sets the necessary CSS properties to realize
		 * the new position and size.
		 * 
		 * The `interacted` event stores the shape data on the server.
		 * 
		 * To update the content of the entry put the new stuff in the `entry` data object. Then trigger the
		 * `content-updated` event.
		 */
		$('.entry', map_node).live({
		
			// pos/size/textarea update
			'interacting': function(){
				var entry = $(this);
			
				// Update the position and size based on the shape data (parsed version of the shape header)
				var shape = entry.data('shape');
				entry.css({
					left: shape.left + 'px',
					top: shape.top + 'px',
					width: shape.width + 'px',
					minHeight: shape.height + 'px'
				});			  
			
				return false;
			},
			// send pos and size to the server
			'interacted': function(){
				var entry = $(this);
				var data = entry.data('entry');
				var shape = entry.data('shape');
			
				// Abort if this is a new entry that has not been saved yet
				if (!entry.attr('id'))
					return false;
			
				// Construct a new shape header and send it to the server
				shape_header = [
					Math.round(shape.left), Math.round(shape.top),
					Math.round(shape.width), Math.round(shape.height)
				].join(', ');
				$.ajax(data.id, {
					type: 'PUT',
					data: JSON.stringify({headers: {'Shape': shape_header}})
				});
			
				return false;
			},
			// modify entity to mirror updated content
			'content-updated': function(){
				var entry = $(this);
				var data = entry.data('entry');
			
				// Build a shape data object for the interacting and interacted events
				var shape = {left: 0, top: 0, width: 200, height: 100};
				if (data.headers.shape) {
					var parts = data.headers.shape.split(',');
					shape.left = parseFloat(parts[0]);
					shape.top = parseFloat(parts[1]);
					if (parts[2])
						shape.width = parseFloat(parts[2]);
					if (parts[3])
						shape.height = parseFloat(parts[3]);
				}
				// Store the shape object and trigger the interacting event to actually set the
				// CSS properties for position and size
				entry.data('shape', shape).trigger('interacting');
			
				// Update entry type class and title
				entry.addClass(data.type);
				entry.find('> header > h1').text(data.headers.title);
			
				// Update the tag list (or hide it if there are no tags)
				if (data.headers.tags) {
					var tags = data.headers.tags.split(',');
					var tag_list = entry.find('> header > .tags').show();
					tag_list.find('> li').remove();
					for(var t = 0; t < tags.length; t++)
						$('<li>').text(tags[t]).appendTo(tag_list);
				} else {
					entry.find('> header > .tags').hide();
				}
			
				// Update the entry HTML content
				if (data.content && data.content != ''){
					var content_wrapper = entry.find('> div');
					if ( content_wrapper.size() == 0 )
						content_wrapper = $('<div>').appendTo(entry);
					content_wrapper.html(data.content);
				}
			
				// Set the background color if one is specified in the headers
				if (data.headers.color)
					entry.css('background-color', data.headers.color);
			
				// If a plain ID changes the IDs of all child entries change as well. To avoid an
				// update cascade for all children we monkey patch all IDs. Just changing the
				// old ID part with the new ID.
				var old_id = entry.attr('id');
				var new_id = data.id;
				if (old_id){
					entry.find('.entry').attr('id', function(index, attr){
						var updated_id = new_id + attr.substr(old_id.length);
						$(this).data('entry').id = updated_id;
						return updated_id;
					});
				}
				entry.attr('id', new_id);
				
				// If this entry got an origin header we will start a subrequest to fetch its data
				// from the specified origin.
				if (data.headers.origin){
					var origin = data.headers.origin;
					$.ajax(origin, {
						success: function(data){
							for(var i = 0; i < data.entries.length; i++)
								createEntryIn(data.entries[i], origin, entry);
						}
					});
				}
			
				return false;
			}
		});
	
	
		/**
		 * High level movement logic for plains and entries.
		 */
		$('.entry > .movable', map_node).live({
			'movement-started': function(){
				$(this).closest('.entry').addClass('moving');
				return false;
			},
			'movement': function(event, dx, dy){
				var entry = $(this).closest('.entry');
				var scale = $('#map').data('view').scale;
			
				entry.data('shape').left += dx / scale;
				entry.data('shape').top += dy / scale;
				entry.trigger('interacting');
			
				return false;
			},
			'movement-stopped': function(event, x, y){
				var entry = $(this).closest('.entry');
				entry.removeClass('moving').trigger('interacted');
				return false;
			}
		});
	
	
		/**
		 * Resizing logic for plains and entries. We move the expander around but
		 * instead of changing a position we just change the size of the entry.
		 */
		$('.entry > aside > .resizer', map_node).live({
			'movement-started': function(){
				$(this).closest('.entry').addClass('resizing');
				return false;
			},
			'movement': function(event, dx, dy){
				var entry = $(this).closest('.entry');
				var scale = $('#map').data('view').scale;
			
				entry.data('shape').width += dx / scale;
				entry.data('shape').height += dy / scale;
				entry.trigger('interacting');
			
				return false;
			},
			'movement-stopped': function(){
				$(this).closest('.entry').removeClass('resizing').trigger('interacted');
				return false;
			}
		});
	
	
		//
		// Load the initial data
		//
		function createEntryIn(data, plain_url, plain){
			var template = (data.type == 'plain') ? '#templates > #plain' : '#templates > #entry';
			var entry = $(template).clone();
			data.id = plain_url + data.id;
			entry.data('entry', data).appendTo(plain);
			entry.trigger('content-updated');
			
			if (data.type == 'plain') {
				for(var i = 0; i < data.entries.length; i++)
					createEntryIn(data.entries[i], plain_url, entry);
			}
		}
		
		function loadRootPlain(root_plain_url, container_element){
			data_path = root_plain_url;
			$.ajax(root_plain_url, {success: function(data){
				// Apply the stored view data (position and scale)
				$('#map').data('view', {
					x: parseFloat(data.pos[0]),
					y: parseFloat(data.pos[1]),
					scale: parseFloat(data.scale)
				}).trigger('interacting');
				
				// Recursively add the entries to the DOM
				for(var i = 0; i < data.entries.length; i++)
					createEntryIn(data.entries[i], root_plain_url, container_element);
			}});
		}
		
		loadRootPlain(data_path, $('#map'));
	
		//
		// Entry menu actions
		//
	
		// Setup a general error handler for AJAX requests
		jQuery.ajaxSetup({
			complete: function(xhr, status){
				if (xhr.status >= 400)
					alert('AJAX request failed with status ' + xhr.status + "\n" + xhr.responseText);
			}
		});	
	
		$('.entry', map_node).live('dblclick', function(){
			$('#editor').trigger('edit', this);
			return false;
		});
	
		$('.entry > aside > .actions > .edit', map_node).live('click', function(){
			var entry = $(this).closest('.entry');		
			$('#editor').trigger('edit', entry);
			return false;
		});
		
		$('.entry > aside > .actions > .abort', map_node).live('click', function(){
			$(this).closest('.entry').remove();
		});
	
	
		$('.entry > aside > .actions > .delete', map_node).live('click', function(){
			var entry = $(this).closest('.entry');
			var id = entry.attr('id');
		
			jQuery.ajax(id, {type: 'DELETE', success: function(data, status, xhr){
				entry.remove();
			}});
		});
	
	
		//
		// Navigation menu links
		//
		$('body > nav > ul > li.reset-zoom').click(function(){
			$('#map').trigger('reset-zoom');
			return false;
		});
	
		/**
		 * Taken from a jQuery bug report (http://bugs.jquery.com/ticket/8362) and slightly
		 * modified. This function returns the offset up to the map before the CSS transformation
		 * is applied. The normal `offset()` uses another way that isn't consistent on browsers.
		 */
		function getWorldOffset(el){
			var _x = 0;
			var _y = 0;
			while( el && !isNaN( el.offsetLeft ) && !isNaN( el.offsetTop ) ) {
				if (el.id == 'map')
					break;
			
				_x += el.offsetLeft;
				_y += el.offsetTop;
				el = el.offsetParent;
			}
			return { top: _y, left: _x };
		}
	
		function translate_page_to_world_space(page_x, page_y, target_elem){
			var view = $('#map').data('view');
			var world_x = -view.x + page_x / view.scale;
			var world_y = -view.y + page_y / view.scale;
		
			var offset = getWorldOffset(target_elem.get(0));
			return {x: world_x - offset.left, y: world_y -  offset.top};
		}
	
		$('body > nav > ul > li.new').click(function(){
			var type = $(this).data('type');
		
			var movement_handler = function(event){
				var indicator = $('#region-draft');
				var mouse_pos = translate_page_to_world_space(event.pageX, event.pageY, indicator.parent());
				var start_pos = indicator.data('start-pos');
			
				indicator.css({
					width: mouse_pos.x - start_pos.x,
					height: mouse_pos.y - start_pos.y
				});
				return false;
			};
		
			
			$('body').addClass('drawing');
		
			$('body').one('mousedown', function(event){		  
			
				// Figure out in which element the new stuff should be put
				var target_elem = $(event.target).closest('section');
				// If we hit the body element use the map (root plain) as container
				if (target_elem.size() == 0)
					target_elem = $('#map');
			
				var world = translate_page_to_world_space(event.pageX, event.pageY, target_elem);
				$('<div id="region-draft" />').css({
					left: world.x + 'px', top: world.y + 'px', width: 0, height: 0
				}).data('start-pos', world).appendTo(target_elem);
			
				$('body').bind('mousemove', movement_handler);
			
				return false;
			});
		
			$('body').one('mouseup', function(event){
				// Unbind the movement handler and use the mouseup event as a final mouse move.
				$('body').unbind('mousemove', movement_handler);
				$('body').removeClass('drawing');
				movement_handler(event);
			
				var indicator = $('#region-draft');
				target_elem = indicator.parent();
				var pos = indicator.data('start-pos');
			
				var template = (type == 'plain') ? '#templates > #plain' : '#templates > #entry';
				var entry = $(template).clone();
			
				entry.data('entry', {
					type: type,
					headers: {
						title: 'New entry',
						shape: [
							Math.round(pos.x), Math.round(pos.y),
							Math.round(indicator.width()), Math.round(indicator.height())
						].join(', ')
					}
				}).addClass('creating').appendTo(target_elem).trigger('content-updated');		
				indicator.remove();
				
				$('#editor').trigger('create', entry);
							
				return false;
			});
		
			return false;
		});
	});

	/**
	 * Wrapperclass to provide textarea with additional functionality
	 */
	function ExtendedTextarea(original) {
			
			var options = {
				tab: "  "
			};

			var F = function() {}
			F.prototype = original;	
			var self = $(new F());

			self.hasSelection = function() {
					return self.getSelection() !== "";
			};
			
			self.hasMultilineSelection = function() {
				return self.getSelection().match(/\n/);
			};
			
			self.getSelection = function() {
				return original.value.slice(original.selectionStart, original.selectionEnd);
			};
			
			self.insertTab = function() {
							
				var value = original.value,
						position = original.selectionStart;
				
				var before = value.slice(0, position),
						after = value.slice(position, value.length);
						
				original.value = before + options.tab + after;
				
				// reset selection	      
				var selection = before.length + options.tab.length;
				self.setSelection(selection, selection);
			};
		
			self.setSelection = function(start, end) {
				original.selectionStart = start;
				original.selectionEnd = end;
			};
		
			self.applyMultilineTabs = function(remove) {
			
				var value = original.value,
						previousSelection = original.selectionStart;
				
				var start = value.slice(0, original.selectionStart).lastIndexOf("\n"),
						end = original.selectionEnd;
					 
				// if start is -1 there is no linebreak before, so we use 0
				if(start === -1) {
					start = 0;
					// we have to indent the first line too...
				}
				
				var before = value.slice(0, start),	          
						part = value.slice(start, end),
						after = value.slice(end, value.length);	      
				
				if(!remove) 
					part = part.replace(/\n/g, "\n" + options.tab);
				else
					part = part.replace(RegExp("\n" + options.tab, "g"), "\n");	        
								
				original.value = before + part + after;   
				
				// restore selection - FIXME
				self.setSelection(previousSelection, before.length + part.length); 	        
			};
			
			return self;	
	}
})();
