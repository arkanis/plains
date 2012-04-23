#include <stdio.h>
#include <stdlib.h>
#include <GL/gl.h>
// For glXCreateContextAttribsARB
#define GLX_GLXEXT_PROTOTYPES
#include <GL/glx.h>

typedef GLXContext (*ptr_glXCreateContextAttribsARB)(Display *dpy, GLXFBConfig config, GLXContext share_context, Bool direct, const int *attrib_list);

void main(){
	Display *display = XOpenDisplay(getenv("DISPLAY"));
	
	int major, minor;
	glXQueryVersion(display, &major, &minor);
	printf("glx version: %d.%d\n", major, minor);
	
	const char *exts = glXQueryExtensionsString(display, 0);
	printf("glx exts: %s\n", exts);
	// TODO: check for GLX_ARB_create_context_profile
	ptr_glXCreateContextAttribsARB glXCreateContextAttribsARB = (ptr_glXCreateContextAttribsARB) glXGetProcAddress("glXCreateContextAttribsARB");
	
	int num_configs = 0;
	static int fb_attribs[] = {
		GLX_RENDER_TYPE,		GLX_RGBA_BIT,
		GLX_X_RENDERABLE,	True,
		GLX_DRAWABLE_TYPE,	GLX_WINDOW_BIT,
		GLX_DOUBLEBUFFER,	True,
		GLX_CONFIG_CAVEAT,	GLX_NONE,
		GLX_RED_SIZE,	8,
		GLX_BLUE_SIZE,	8,
		GLX_GREEN_SIZE,	8,
		0
	};
	GLXFBConfig *configs = glXChooseFBConfig(display, 0, fb_attribs, &num_configs);
	
	printf("got %d configs\n", num_configs);
	if (num_configs > 0) {
		XVisualInfo *visual = glXGetVisualFromFBConfig(display, configs[0]);
		
		XSetWindowAttributes win_attribs;
		win_attribs.background_pixel = 0;
		win_attribs.border_pixel = 0xffffffff;
		win_attribs.event_mask = ExposureMask | VisibilityChangeMask | KeyPressMask | PointerMotionMask | StructureNotifyMask;
		
		win_attribs.bit_gravity = StaticGravity;
		win_attribs.colormap = XCreateColormap(display, RootWindow(display, visual->screen), visual->visual, AllocNone);
		
		Window win = XCreateWindow(display, DefaultRootWindow(display), 20, 20, 400, 400, 0, visual->depth, InputOutput, visual->visual,
			CWBorderPixel | CWBitGravity | CWEventMask | CWColormap, &win_attribs);
		XMapWindow(display, win);
		
		
		GLint attribs[] = {
			GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
			GLX_CONTEXT_MINOR_VERSION_ARB, 3,
			0
		};
		GLXContext gl_context = glXCreateContextAttribsARB(display, configs[0], NULL, True, attribs);
		glXMakeCurrent(display, win, gl_context);
		
		glViewport(0, 0, 400, 400);
		glClearColor(0, 0, 1, 1);
		int i = 0;
		for(i = 0; i < 10; i++){
			glClear(GL_COLOR_BUFFER_BIT);
			glXSwapBuffers(display, win);
			//XSync(display, False);
			sleep(1);
			printf("clear\n");
		}
		
		glXMakeCurrent(display, None, NULL);
		glXDestroyContext(display, gl_context);
		
		
		// -----------------
		XSync(display, False);
		sleep(5);
		
		XDestroyWindow(display, win);
		
		XFree(visual);
	}
	
	XFree(configs);
	
	XCloseDisplay(display);
}