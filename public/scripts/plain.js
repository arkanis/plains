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
			textarea.show();
			entry.trigger('resize');
		} else {
			jQuery.ajax(entry.attr('id'), {dataType: 'json', success: function(data){
				$('<textarea>').val(data.raw).appendTo(entry).show();
				entry.trigger('resize');
			}});
		}
	});
	
	$('.entry > aside > .actions > .save', map).live('click', function(){
		$(this).closest('ul.actions').removeClass('editing');
		
		var entry = $(this).closest('.entry')
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
					data('entry', data).trigger('sync-to-data').trigger('reshaped');
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
	$('body > nav > ul > li.new').click(function(){
		var type = $(this).data('type');
		
		$('#map').one('click', function(event){
			// Figure out in which element the new stuff should be put
			var target_elem = $(event.target).closest('section');
			// If we create something directly on the level of the map (root plain) use
			// the div element of the map as parent for new stuff
			if (target_elem.attr('id') == 'map')
				target_elem = target_elem.find('> div');
			
			// Convert the mouse position of the click from screen space to world space
			var view = $('#map').data('view');
			var x = (event.pageX - view.x) / view.scale;
			var y = (event.pageY - view.y) / view.scale;
			
			// If the entry is created in a plain take the plain position into account
			var parent_offsets = target_elem.offset();
			x = x - parent_offsets.left;
			y = y - parent_offsets.top;
			
			if (type == 'plain')
				var entry = $('#templates > #plain').clone();
			else
				var entry = $('#templates > #entry').clone();
			
			entry.addClass('editing').addClass(type).css({left: x + 'px', top: y + 'px'}).
				removeAttr('id').data('type', type).data('entry', {}).
				find('> header > h1').text('New entry').end().
				find('> aside > ul.actions').addClass('creating').end().
				append( $('<textarea>') ).
				appendTo(target_elem).trigger('resize');
			
			return false;
		});
		return false;
	});
});