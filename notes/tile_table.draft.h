#include <stdint.h>
#include <stddef.h>

// Forward declarations for structures
typedef struct layer_s layer_t, *layer_p;
typedef struct layer_scale_s layer_scale_t, *layer_scale_p;
typedef struct tile_table_s tile_table_t, *tile_table_p;
typedef union tile_u tile_t, *tile_p;

// Tile index type, used to index into the tile_table->tiles array
typedef uint32_t tile_id_t;
// The scale index represents how far the user zoomed in or out
typedef int8_t scale_index_t;


//
// A layer is a drawable object, either of pixel data or of vertex data
//

// Layer types
#define LAYER_PIXEL  1
#define LAYER_VERTEX 2

struct layer_s {
	uint8_t type;
	union {
		struct {
			// Position of the upper left corner and dimensions of the layer for scale
			// index 0 (original size)
			int64_t x, y;
			uint64_t width, height;

			// Pointer into a double linked list of layer scales. We just remember the
			// pointer to the current scale. If we zoom in we follow the larger link,
			// when zooming out we follow the smaller link.
			layer_scale_p current_scale;
		} pixel;
		struct {
			// Center of the vertex layer
			int64_t x, y;
			// Vertex positions
			size_t vertex_count;
			float vertecies[];
		} vertex;
	};
};


//
// A pixel layer consits of zero or more layer scales. These contain the pixel data of the
// layer for a specific scale index. The pixel data itself is stored as tiles in the tile
// table texture. The layer scale contains the ids of the tiles that contain the pixel data.
//

struct layer_scale_s {
	// Scale index of this layer scale
	scale_index_t scale_index;
	// Dimensions of this layer scale. We could calculate them out of the scale index but that
	// would required to ask the viewport to translate the scale index to a scale. To avoid the
	// extra dependency we store them here.
	uint64_t width, height;

	// Pointer to the layer this scale belongs to. Needed to fix up the layers current_scale
	// pointer in case we delete the layer (it's reclaimed to free up the tiles).
	layer_p layer;

	// Double linked list of scales for this layer ordered by scale index.
	layer_scale_p larger, smaller;

	// Pointer to the tile table this scale uses. We need to fix up the tables most_used and
	// least_used pointers in case we delete the layer.
	tile_table_p tile_table;

	// Double linked list of all scales of all layers ordered by usage. That way it's
	// easier to reclaim less used layers for memory.
	layer_scale_p more_used, less_used;
	
	// Tile ids for this scale
	size_t tile_id_count;
	tile_id_t tile_ids[];
};

#define LAYER_SCALE_HEADER_SIZE (offsetof(struct layer_scale_s, tile_ids))


//
// Type to represent the entire tile table (texture, tile meta data, etc.)
//

struct tile_table_s {
	// Side length (width and height) of one tile in pixels
	uint16_t tile_size;
	// Texture dimensions in pixels
	uint32_t width, height;
	// OpenGL texture object
	GLuint texture;
	
	// ID of the next free tile or 0 if no free tile is left
	tile_id next_free_tile;

	// Head and tail of double linked list of layer scales. The least_used layer scale
	// is reclaimed if new tiles are needed.
	layer_scale_p most_used, least_used;

	// Number of tiles the texture consists of
	size_t tile_count;
	tile_t tiles[];
};


//
// Type for tile meta data kept on the CPU side. The actual pixel data it stored in the
// OpenGL texture of the tile table.
//
// The union is designed to be one pointer in size. Free tiles have an least significant bit of
// 1 and allocated tiles 0. Allocated tiles contain a pointer to a layer_scale_t structure which
// is at least 16 bit (2 byte) aligned. Therefore the least significant bit is always zero. Free
// tiles have the TILE_FREE flag set which sets the least significant bit to 1.
//

#define TILE_FREE 1<<0

typedef union {
	// When a tile is allocated it points to the layer scale that uses the tile
	layer_scale_p used_by;
	// Data for free tiles (a basic free list)
	struct {
		// ID of the next free tile of 0 if no free tiles are left
		tile_id_t next_free_tile;
		// Flags for this tile
		uint32_t flags;
	};
} tile_t, *tile_p;