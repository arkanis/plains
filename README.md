# Plains project

The plains project allows you to arrange personal notes, ideas, bookmarks, etc. on a large pannable
and zoomable 2D space. You can group related stuff together into a new "plain". After a while the information
create a unique landscape and it's easy to find old or related stuff again.

The structure and userbility is heavily inspired by the map characteristics of graphical user interfaces. The
basic idea is that every piece of information has its place on this 2D space and can be found there. As soon
as enought information was added to form small structures (e.g. some notes grouped into a few plains) the
spacial sense of direction kicks in. While the layout you chose might be arbitary at first and might seem a
bit chaotic it's easy to find unique "landmarks" that lead to the information you search.

It's a web application and once you set it up on a server you can use it from everywhere. The data is stored
in simple text files and directories. Plains correspond to directories and every piece of information to a file. This
makes it easy to backup your data, search it with other tools or just use a file browser and text editor to get
around.

BE AWARE: The project is in an very early stage right now. I only use it for my personal stuff. Therefore its
nither cleaned up nor stable.

# Controls

- Pan around by click and draging the right mouse button on the background or a plain.
- Zoom in and out with the mousewheel as you like
- Use the buttons on the top left corner to create new entries: notes, ideas and plains. Click on them
  and drag a rectangle on the area you want the new entry to cover. You can also reset the zoom
  to 1.0 there.
- The content of entries is written in a mail like format. At the top you have some headers and after
  a blank like the content. Usually you just have to insert the name of the new entry in the `Title`
  header. Write your contents after a blank line. Right now only markdown is enabled so you should
  write the contents in markdown.
- If you hover the small wrench at the uper right corner of an entry a menu is shown. There you can
  save, edite and delete an entry. If you delete an entry only `.deleted` will be appended to the entry
  filename and it will no longer be shown. This allows you to bring back accidentally deleted entries.
- You can move entries around by click and draging their symbol in the upper left corner directly
  left to the title.
- To resize an entry click and drag the dots at the lower right corner.

# Installations

- Checkout the repository
- Modify the `plain.localhost.conf` Apache2 configuration.
	- Set the `ServerName` to the domain you want to use. Please also make sure that this domain is
	  routed to the IP of the server you're using. Either by adding an entry to your local `hosts` file if
	  you use it on your own computer or by adding an corresponding name server entry.
	- Update the absolute pahts in `DocumentRoot`, `ErrorLog`, `CustomLog`.
	- You might want to turn `display_errors` off if you're using it on a public accessible server.
- Make sure the webserver user can read the `public` and `include` directory and __write__ to the `data` directory.

# Problems and bugs

- Performance is abysmal (because of the CSS transformations)
- Only runs more or less well on Chrome. Opera has redraw issues and Firefox jumps a bit around if you
  start to edit an entry.
- In Chrome you can not properly scroll the textareas as soon as they are a bit off the center. To work around
  this you can resize the entry large enought so the text area does not have scroll bars.
- You can not move entries from one plain to another right now. Move the corresponding directory in a file
  browser instead.