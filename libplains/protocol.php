<?php

$header = [
	'type' => 'uint16_t',
	'seq' => 'uint16_t'
];
$messages = [
	// Handshake
	'hello' => [
		'version' => 'uint8_t',
		'name' => ['char', 'max_len' => 64],
		'caps' => ['uint16_t', 'max_len' => 64]
	],
	'welcome' => [
		'version' => 'uint8_t',
		'name' => ['char', 'max_len' => 64],
		'caps' => ['uint16_t', 'max_len' => 64]
	],
	
	// Common messages
	'status' => [
		'seq' => 'uint16_t',
		'status' => 'uint32_t',
		'id' => 'uint32_t'
	],
	
	// Requests for clients
	'layer_create' => [
		'x' => 'int64_t',
		'y' => 'int64_t',
		'z' => 'int64_t',
		'width' => 'uint64_t',
		'height' => 'uint64_t',
		'private' => 'void*'
	],
	
	// Events triggered by the server
	'keydown' => [
		'key' => 'int',
		'mod' => 'int'
	],
	'keyup' => [
		'key' => 'int',
		'mod' => 'int'
	],
	'mouse_motion' => [
		'state' => 'uint8_t',
		'x' => 'uint16_t',
		'y' => 'uint16_t'
	],
	'mouse_button' => [
		'type' => 'uint8_t',
		'which' => 'uint8_t',
		'button' => 'uint8_t',
		'state' => 'uint8_t',
		'x' => 'uint16_t',
		'y' => 'uint16_t'
	],
	'draw' => [
		'layer_id' => 'uint32_t',
		'private' => 'void*',
		'shm_fd' => 'plains_fd_t',
		
		'x' => 'int64_t',
		'y' => 'int64_t',
		'z' => 'int64_t',
		'width' => 'uint64_t',
		'height' => 'uint64_t',
		'scale_index' => 'int8_t',
		'scale' => 'float'
	]
];
$len_type = 'size_t';

$types = [
	'int8_t'   => ['size' => 1, 'format' => '%hhd'],
	'int16_t'  => ['size' => 2, 'format' => '%hd'],
	'int32_t'  => ['size' => 4, 'format' => '%d'],
	'int64_t'  => ['size' => 8, 'format' => '%ld'],
	'uint8_t'  => ['size' => 1, 'format' => '%hhu'],
	'uint16_t' => ['size' => 2, 'format' => '%hu'],
	'uint32_t' => ['size' => 4, 'format' => '%u'],
	'uint64_t' => ['size' => 8, 'format' => '%lu'],
	'size_t'   => ['size' => 8, 'format' => '%zu'],
	'int'      => ['size' => 4, 'format' => '%d'],
	'float'    => ['size' => 4, 'format' => '%f'],
	'double'   => ['size' => 8, 'format' => '%lf'],
	'char'     => ['size' => 1, 'format' => '%s'],
	'void*'    => ['size' => 8, 'format' => '%p'],
	'plains_fd_t'     => ['size' => 4, 'format' => '%d']
];

?>