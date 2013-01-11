# X11: For X functions
# GL: For gl and glX calls
# Development packages: xutils-dev libxcomposite-dev libxdamage-dev libxrender-dev
basic-xgl: basic-xgl.c
	gcc basic-xgl.c -o basic-xgl -lX11 -lGL

x-client: x-client.c
	gcc x-client.c -o x-client -lX11

xcb-client: xcb-client.c
	gcc xcb-client.c -o xcb-client `pkg-config --cflags --libs xcb`

x-sample: x-sample.c
	gcc x-sample.c -o x-sample -lX11

image_loader: image_loader.c
	gcc -std=gnu99 image_loader.c -o image_loader -lm


clean:
	rm -f basic-xgl x-client xcb-client x-sample image_loader