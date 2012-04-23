# X11: For X functions
# GL: For gl and glX calls
basic-xgl: basic-xgl.c
	gcc basic-xgl.c -o basic-xgl -lX11 -lGL

x-client: x-client.c
	gcc x-client.c -o x-client -lX11

clean:
	rm -f basic-xgl x-client