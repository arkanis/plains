#include <xcb/xcb.h>

int main(){
	xcb_connection_t* con = xcb_connect(NULL, NULL);
	if ( xcb_connection_has_error(con) )
		return -1;
	
	xcb_screen_t* screen = xcb_setup_root_iterator(xcb_get_setup(con)).data;
	xcb_window_t* window = xcb_generate_id(con);
	xcb_create_window
	
	xcb_disconnect(con);
}