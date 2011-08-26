$(document).ready(function(){
	// Setup a general error handler for AJAX requests
	jQuery.ajaxSetup({
		complete: function(xhr, status){
			if (xhr.status >= 400)
				alert('AJAX request failed with status ' + xhr.status + "\n" + xhr.responseText);
		}
	});
	
	var map = $('#map > div').get(0);
	
	$('.entry > aside > .actions > .edit', map).live('click', function(){
		$(this).closest('ul.actions').addClass('editing');
		
		var entry = $(this).closest('.entry');
		entry.addClass('editing').find('> div').hide();
		
		// If an textarea with old data is still present show it, otherwise request
		// the original data and show that.
		var textarea = entry.find('> textarea');
		if (textarea.size() > 0) {
			textarea.show().focus();
			entry.trigger('resize');
		} else {
			jQuery.ajax(entry.attr('id'), {dataType: 'json', success: function(data){
				$('<textarea>').val(data.raw).appendTo(entry).show().focus();
				entry.trigger('resize');
			}});
		}
	});
	
	$('.entry > aside > .actions > .save', map).live('click', function(){
		$(this).closest('ul.actions').removeClass('editing');
		
		var entry = $(this).closest('.entry');
		entry.removeClass('editing').
			find('> textarea').hide().end().
			find('> div').show();
		
		jQuery.ajax(entry.attr('id'), {
			type: 'PUT', data: JSON.stringify({raw: entry.find('> textarea').val()}),
			dataType: 'json', success: function(data){
				entry.data('entry', data).trigger('sync-to-data');
			}
		});
	});
	
	$('.entry > aside > .actions > .close', map).live('click', function(){
		$(this).closest('ul.actions').removeClass('editing');
		
		$(this).closest('.entry').
			find('> textarea').hide().end().
			find('> div').show();
	});
	
	
	$('.entry > aside > .actions > .create', map).live('click', function(){
		var entry = $(this).closest('.entry');
		var plain = entry.parents('section').eq(0);
		
		var parent_id = plain.attr('id');
		// If the section element is the map root use the root plain as parent
		if ( parent_id == 'map' )
			parent_id = '/data';
		
		jQuery.ajax(parent_id, {
			type: 'POST', data: JSON.stringify({
				raw: entry.find('> textarea').val(),
				type: entry.data('type')
			}), success: function(data){
				entry.removeClass('editing').
					find('> aside > ul.actions').removeClass('creating').end().
					find('> textarea').hide().end().
					data('entry', data).trigger('sync-to-data').
					trigger('reshaped');
			}
		});
	});
	
	$('.entry > aside > .actions > .abort', map).live('click', function(){
		$(this).closest('.entry').remove();
	});
	
	
	$('.entry > aside > .actions > .delete', map).live('click', function(){
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
	
	function translate_page_to_world_space(page_x, page_y, target_elem){
		var view = $('#map').data('view');
		var world_x = (page_x - view.x) / view.scale;
		var world_y = (page_y - view.y) / view.scale;
		var pos = target_elem.position();
		
		return {x: world_x - pos.left, y: world_y -  pos.top};
	}
	
	$('body > nav > ul > li.new').click(function(){
		var type = $(this).data('type');
		
		var movement_handler = function(event){
			var indicator = $('#region-draft');
			var mouse_pos = translate_page_to_world_space(event.pageX, event.pageY, indicator.parent());
			var start_pos = indicator.position();
			console.log(mouse_pos.x - start_pos.left, mouse_pos.y - start_pos.top);
			indicator.css({
				width: mouse_pos.x - start_pos.left,
				height: mouse_pos.y - start_pos.top
			});
			return false;
		};
		
		$('#map').one('mousedown', function(event){
			// Figure out in which element the new stuff should be put
			var target_elem = $(event.target).closest('section');
			// If we create something directly on the level of the map (root plain) use
			// the div element of the map as parent for new stuff
			if (target_elem.attr('id') == 'map')
				target_elem = target_elem.find('> div');
			
			var world = translate_page_to_world_space(event.pageX, event.pageY, target_elem);
			
			$('<div id="region-draft" />').css({
				left: world.x, top: world.y, width: 0, height: 0
			}).appendTo(target_elem);
			
			$('#map').bind('mousemove', movement_handler);
			
			return false;
		});
		
		$('#map').one('mouseup', function(event){
			// Unbind the movement handler and use the mouseup event as a final mouse move.
			$('#map').unbind('mousemove', movement_handler);
			movement_handler(event);
			
			var indicator = $('#region-draft');
			target_elem = indicator.parent();
			var pos = indicator.position();
			
			if (type == 'plain')
				var entry = $('#templates > #plain').clone();
			else
				var entry = $('#templates > #entry').clone();
			
			entry.addClass('editing').addClass(type).
				removeAttr('id').data('type', type).data('entry', {}).css({
					left: pos.left, top: pos.top, width: indicator.width(), height: indicator.height()
				}).
				find('> header > h1').text('New entry').end().
				find('> aside > ul.actions').addClass('creating').end().
				append( $('<textarea>') ).
				appendTo(target_elem).trigger('resize').
				find('> textarea').focus().end();
			
			entry.trigger('resize').
				find('> textarea').val("Title: \nProcessors: markdown").focus();
			indicator.remove();
			
			return false;
		});
		
		return false;
	});
});