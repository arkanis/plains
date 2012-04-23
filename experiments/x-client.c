#include <stdio.h>
#include <X11/Xlib.h>

void inspect_event(XEvent *event);

void main(){
	// Basic X display stuff
	Display *display = XOpenDisplay(NULL);
	
	if (display == NULL) {
		printf("failed to open current X display\n");
		return;
	}
	
	printf("X connection number (file descriptor of connection): %d\n", ConnectionNumber(display));
	printf("X server: protocol version: %d, protocol revision: %d, server vendor: %s, vendor release: %d\n",
		ProtocolVersion(display), ProtocolRevision(display), ServerVendor(display), VendorRelease(display));
	
	printf("Message queue length: %d\n", QLength(display));
	
	printf("Got %d screens:\n", ScreenCount(display));
	int i;
	for(i = 0; i < ScreenCount(display); i++){
		printf("- %dpx × %dpx, %dmm × %dmm\n", DisplayWidth(display, i), DisplayHeight(display, i),
			DisplayWidthMM(display, i), DisplayHeightMM(display, i));
	}
	
	
	// Create a basic window
	XSetWindowAttributes attr;
	attr.event_mask = ExposureMask | VisibilityChangeMask | KeyPressMask | PointerMotionMask | StructureNotifyMask;
	Window win = XCreateWindow(display, DefaultRootWindow(display), 50, 50, 200, 200, 0, CopyFromParent, InputOutput, CopyFromParent,
		CWEventMask, &attr);
	XMapWindow(display, win);
	
	
	// Event loop
	XEvent event;
	while(1){
		XNextEvent(display, &event);
		inspect_event(&event);
	}
	
	//XNoOp(display);
	
	// Clean up
	XDestroyWindow(display, win);
	XCloseDisplay(display);
}

void inspect_event(XEvent *event){
	
	void print_common_event_stuff(const char *name){
		printf("%s (type %d) serial: %lu, caused by send: %d, win: %p\n  ",
			name, event->xany.type, event->xany.serial, event->xany.send_event, event->xany.window);
	}
	
	switch(event->type){
		case KeyPress: {
			print_common_event_stuff("KeyPress");
			XKeyEvent e = event->xkey;
			printf("root win: %p, sub win: %p, time: %lu ms, x: %d, y: %d, x_root: %d, y_root: %d, state: %x, keycode: %d, same_screen: %d\n",
				e.root, e.subwindow, e.time, e.x, e.y, e.x_root, e.y_root, e.state, e.keycode, e.same_screen);
			} break;
		case KeyRelease: {
			print_common_event_stuff("KeyRelease");
			XKeyEvent e = event->xkey;
			printf("root win: %p, sub win: %p, time: %lu ms, x: %d, y: %d, x_root: %d, y_root: %d, state: %x, keycode: %d, same_screen: %d\n",
				e.root, e.subwindow, e.time, e.x, e.y, e.x_root, e.y_root, e.state, e.keycode, e.same_screen);
			} break;
		case ButtonPress: {
			print_common_event_stuff("ButtonPress");
			XButtonEvent e = event->xbutton;
			printf("root win: %p, sub win: %p, time: %lu ms, x: %d, y: %d, x_root: %d, y_root: %d, state: %x, button: %d, same_screen: %d\n",
				e.root, e.subwindow, e.time, e.x, e.y, e.x_root, e.y_root, e.state, e.button, e.same_screen);
			} break;
		case ButtonRelease: {
			print_common_event_stuff("ButtonRelease");
			XButtonEvent e = event->xbutton;
			printf("root win: %p, sub win: %p, time: %lu ms, x: %d, y: %d, x_root: %d, y_root: %d, state: %x, button: %d, same_screen: %d\n",
				e.root, e.subwindow, e.time, e.x, e.y, e.x_root, e.y_root, e.state, e.button, e.same_screen);
			} break;
		case MotionNotify: {
			print_common_event_stuff("MotionNotify");
			XMotionEvent e = event->xmotion;
			printf("root win: %p, sub win: %p, time: %lu ms, x: %d, y: %d, x_root: %d, y_root: %d, state: %x, is_hint: %d, same_screen: %d\n",
				e.root, e.subwindow, e.time, e.x, e.y, e.x_root, e.y_root, e.state, e.is_hint, e.same_screen);
			} break;
		default:
			printf("unknown event, type: %d, serial: %lu, caused by send: %d\n", event->xany.type, event->xany.serial, event->xany.send_event);
			break;
	}
}