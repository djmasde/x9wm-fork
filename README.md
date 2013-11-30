x9wm - x window manager without windows

Please see w9wm online and in several repositories.
x9wm is just like w9wm but is merged in a single file
is compiled as a single static binary and linked against
libx11-dev 

to install:

1 install libx11-dev x11 developer files, install gmrun

1.1 in Slackware these headers is present in the default install :)

2 see and run the build file

3 Install feh for wallpaper changing example: feh --bg-fill '/home/foo/foowall.png'

3.1  add in .xinitrc: sh $HOME/.fehbg &

4 makes sure you have a three button mouse

5 add it to your xsessions as a window manger

6 login and select x9wm as the window manager

7 enjoy the smallest / fastest window manager for Linux 

8 install rxvt-unicode

8.1 urxvt is now the default shell in menu New

9 Deleted xoot warpper, aterm is very nice, cute but is not friend of utf-8 :/

10 please read 

11 Hold both "shift" and "left ctrl" keys and press button 1.  

x9wm will then  display  a  menu

that allows you to exec programs specified in the $HOME/.x9wmrc

Definition: w9wm: Enhanced window manager based on 9wm
w9wm is a quick and dirty hack based on 9wm. 
It provides support for virtual screens as well as for keyboard bindings. 

Definition: x9wm is enhanced w9wm - providing small bug fixes / small binary an
easier modification and fastest remote desktop for VNC / RDP desktop sessions
for multi-admin remote administration for Windows / Linux and Mac systems from 
a single remote location / interface.

Added Two new workspaces =14, the maximus hidden windows is 36, 

Added a menu launcher: Run..

Added a .x9wmrc menu

See screenshot: http://a.pomf.se/g790a.png