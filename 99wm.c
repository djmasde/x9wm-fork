// licensed as w9wm 9wm 
// w9wm Version 0.4.2
// Copyright 2000 Benjamin Drieu.

// with large portions of code
// Copyright 1994 David Hogan.

// this version is single source file with a
// red mouse pointer and smaller object file
// for xoot, embedded, mini rescue builds...
// x9wm single source file version with
// additional code for forming, imaging, movies...
//
// X9wm is Copyright 2005 Joseph Altea.
// Built for slitaz linux trunk build ..
// Xoot 0.1 LINUX system
//
// X9wm fork Copyright 2013 Helmuth Schmelzer 
// Added two new workspaces
// Added launcher/run, in this case gmrun, etc
// Removed xoot warpper for aterm, aterm is not friend of utf-8 :/
// Now for default urxvt is the default shell
//
// 99wm fork of X9wm Copyright 2015 Jacob Adams
// Modernize build system
// Remove launcher/run
// SEE http://www.drieu.org/code/w9wm/README
// for original w9wm code 
// and google plan 9 code for Dave Hogans 9wm 
// SEE this for more info http://unauthorised.org/dhog/9wm.html
// SEE www.skyfalcon.co.cc for x9wm development NEWS
//
//   www.skyfalcon.co.cc

#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <X11/X.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#include <stdlib.h>
#include <X11/keysym.h>
#include <pwd.h>
#include <sys/types.h>
#include <malloc.h>
#include <string.h>
#include <X11/X.h>
#include <X11/keysym.h>
#include <X11/Xproto.h>
#include "x9wm.h"
#include <unistd.h>
#include <unistd.h>
#include <sys/wait.h>

#ifdef  DEBUG_EV
#include "showevent/ShowEvent.c"
#endif

char    *version[] = 
{
    "x9wm - a child of w9wm,9wm, fork of w9wm, etc", 
    "copyright (c) Helmuth Schmelzer, Joseph Altea, Benjamin Drieu, David Hogan", 0,
};

Display         *dpy;
int             screen;
Window          root;
Window          menuwin;
Colormap        def_cmap;
int             initting;
GC              gc;
unsigned long   black;
unsigned long   white;
unsigned long   red;
XFontStruct     *font;
int             nostalgia;
char            **myargv;
char            *termprog;
char            *display = NULL;
char            *shell;
Bool            shape;
int             shape_event;
int             _border = 4;
int             min_cmaps;
int             curtime;
int             debug;
int             signalled;
Bool 		click_passes = 0;
Bool		use_keys = 1;
int		numvirtuals = 14;
char *		progsnames[16];

Atom        exit_9wm;
Atom        restart_9wm;
Atom        wm_state;
Atom        wm_change_state;
Atom        wm_protocols;
Atom        wm_delete;
Atom        wm_take_focus;
Atom        wm_colormaps;
Atom        _9wm_running;
Atom        _9wm_hold_mode;

void    usage(), sighandler(), getevent();

char    *fontlist[] = {
    "6x13",
    "lucm.latin1.9",
    "smoothansi",
    "9x15bold",
    "lucidasanstypewriter-12",
    "fixed",
    0,
};

int
main(argc, argv)
int argc;
char    *argv[];
{
    int i, background, do_exit, do_restart, dummy;
    unsigned long mask;
    XEvent ev;
    XGCValues gv;
    XSetWindowAttributes attr;
    char *fname;

    myargv = argv;          /* for restart */

    background = do_exit = do_restart = 0;
    font = 0;
    fname = 0;
    for (i = 1; i < argc; i++)
        if (strcmp(argv[i], "-nostalgia") == 0)
            nostalgia++;
        else if (strcmp(argv[i], "-debug") == 0)
            debug++;
        else if ((strcmp(argv[i], "-display") == 0 || strcmp(argv[i], "-dpy") == 0 ) 
		 && i+1<argc)
	  {
            display = argv[++i];
	    setenv("DISPLAY", display, 1);
	  }
        else if (strcmp(argv[i], "-pass") == 0)
	  {
	    click_passes = 1;
	  }
        else if (strcmp(argv[i], "-nokeys") == 0)
	  {
	    use_keys = 0;
	  }
        else if (strcmp(argv[i], "-font") == 0 && i+1<argc) {
            i++;
            fname = argv[i];
        }
        else if (strcmp(argv[i], "-grey") == 0)
            background = 1;
        else if (strcmp(argv[i], "-term") == 0 && i+1<argc)
            termprog = argv[++i];
        else if (strcmp(argv[i], "-version") == 0) {
            fprintf(stderr, "%s", version[1]);
            if (PATCHLEVEL > 0)
                fprintf(stderr, "; patch level %d", PATCHLEVEL);
            putc('\n', stderr);
            exit(0);
        }
        else if (strcmp(argv[i], "-virtuals") == 0 && i+1<argc)
	  {
	    int n = atoi(argv[++i]);
	    if (n > 0 && n <= 14)
	      numvirtuals = n;
	    else
	      fprintf(stderr, "9wm: wrong number of virtual screens, must be between 1 and 14\n");
	  }
        else if (argv[i][0] == '-')
            usage();
        else
            break;
    for (; i < argc; i++)
        if (strcmp(argv[i], "exit") == 0)
            do_exit++;
        else if (strcmp(argv[i], "restart") == 0)
            do_restart++;
        else
            usage();

    if (do_exit && do_restart)
        usage();

    shell = (char *)getenv("SHELL");
    if (shell == NULL)
        shell = DEFSHELL;

    parseprogsfile();

    dpy = XOpenDisplay(display);
    if (dpy == 0)
        fatal("can't open display");
    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);
    def_cmap = DefaultColormap(dpy, screen);
    min_cmaps = MinCmapsOfScreen(ScreenOfDisplay(dpy, screen));

    initting = 1;
    XSetErrorHandler(handler);
    if (signal(SIGTERM, sighandler) == SIG_IGN)
        signal(SIGTERM, SIG_IGN);
    if (signal(SIGINT, sighandler) == SIG_IGN)
        signal(SIGINT, SIG_IGN);
    if (signal(SIGHUP, sighandler) == SIG_IGN)
        signal(SIGHUP, SIG_IGN);

    exit_9wm = XInternAtom(dpy, "9WM_EXIT", False);
    restart_9wm = XInternAtom(dpy, "9WM_RESTART", False);

    curtime = -1;       /* don't care */
    if (do_exit) {
        sendcmessage(root, exit_9wm, 0L);
        XSync(dpy, False);
        exit(0);
    }
    if (do_restart) {
        sendcmessage(root, restart_9wm, 0L);
        XSync(dpy, False);
        exit(0);
    }

    wm_state = XInternAtom(dpy, "WM_STATE", False);
    wm_change_state = XInternAtom(dpy, "WM_CHANGE_STATE", False);
    wm_protocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
    wm_delete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    wm_take_focus = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
    wm_colormaps = XInternAtom(dpy, "WM_COLORMAP_WINDOWS", False);
    _9wm_running = XInternAtom(dpy, "_9WM_RUNNING", False);
    _9wm_hold_mode = XInternAtom(dpy, "_9WM_HOLD_MODE", False);

    black = BlackPixel(dpy, screen);
    white = WhitePixel(dpy, screen);

    if (fname != 0)
        if ((font = XLoadQueryFont(dpy, fname)) == 0)
            fprintf(stderr, "9wm: warning: can't load font %s\n", fname);

    if (font == 0) {
        i = 0;
        for (;;) {
            fname = fontlist[i++];
            if (fname == 0) {
                fprintf(stderr, "9wm: can't find a font\n");
                exit(1);
            }
            font = XLoadQueryFont(dpy, fname);
            if (font != 0)
                break;
        }
    }
    if (nostalgia)
        _border--;

    gv.foreground = black^white;
    gv.background = white;
    gv.font = font->fid;
    gv.function = GXxor;
    gv.line_width = 0;
    gv.subwindow_mode = IncludeInferiors;
    mask = GCForeground | GCBackground | GCFunction | GCFont | GCLineWidth
        | GCSubwindowMode;
    gc = XCreateGC(dpy, root, mask, &gv);

    initcurs();

#ifdef  SHAPE
    shape = XShapeQueryExtension(dpy, &shape_event, &dummy);
#endif

    attr.cursor = arrow;
    attr.event_mask = SubstructureRedirectMask
      | SubstructureNotifyMask | ColormapChangeMask
      | ButtonPressMask | ButtonReleaseMask
      | PropertyChangeMask | StructureNotifyMask
      | (use_keys ? KeyPressMask : 0) ;
    mask = CWCursor|CWEventMask;
    XChangeWindowAttributes(dpy, root, mask, &attr);
    XSync(dpy, False);

    if (background) {
        XSetWindowBackgroundPixmap(dpy, root, root_pixmap);
        XClearWindow(dpy, root);
    }

    menuwin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 1, black, white);
    initb2menu(numvirtuals);

    /* set selection so that 9term knows we're running */
    curtime = CurrentTime;
    XSetSelectionOwner(dpy, _9wm_running, menuwin, timestamp());

    XSync(dpy, False);
    initting = 0;

    nofocus();
    scanwins();

    for (;;) {
        getevent(&ev);

#ifdef  DEBUG_EV
        if (debug) {
            ShowEvent(&ev);
            printf("\n");
        }
#endif
        switch (ev.type) {
        default:
#ifdef  SHAPE
            if (shape && ev.type == shape_event)
                shapenotify((XShapeEvent *)&ev);
            else
#endif
                fprintf(stderr, "9wm: unknown ev.type %d\n", ev.type);
            break;
	case KeyRelease:
	  if (use_keys && current)
	    {
	      ev.xkey.window = current->window;
	      XSendEvent(dpy, current->window, False, NoEventMask, &ev);
	    }
	  break;
	case KeyPress:
	  if (use_keys)
	    {
	      if (XLookupKeysym(&(ev.xkey),0) == XK_Tab && 
		  (ev.xkey.state & ControlMask))
		{
		  if (ev.xkey.state & ShiftMask)
		    activateprevious();
		  else
		    activatenext();		      
		}
	      else if (current)
		{
		  ev.xkey.window = current->window;
		  XSendEvent(dpy, current->window, False, NoEventMask, &ev);
		}
	    }
	  break;
        case ButtonPress:
            button(&ev.xbutton);
	    /*  option */
	    {
	      XAllowEvents (dpy, SyncPointer, ev.xbutton.time);
	    }
            break;
        case ButtonRelease:
            break;
        case MapRequest:
            mapreq(&ev.xmaprequest);
            break;
        case ConfigureRequest:
	  configurereq(&ev.xconfigurerequest);
	  break;
        case CirculateRequest:
            circulatereq(&ev.xcirculaterequest);
            break;
        case UnmapNotify:
            unmap(&ev.xunmap);
            break;
        case CreateNotify:
            newwindow(&ev.xcreatewindow);
            break;
        case DestroyNotify:
            destroy(ev.xdestroywindow.window);
            break;
        case ClientMessage:
            clientmesg(&ev.xclient);
            break;
        case ColormapNotify:
            cmap(&ev.xcolormap);
            break;
        case PropertyNotify:
            property(&ev.xproperty);
            break;
        case SelectionClear:
            fprintf(stderr, "9wm: SelectionClear (this should not happen)\n");
            break;
        case SelectionNotify:
            fprintf(stderr, "9wm: SelectionNotify (this should not happen)\n");
            break;
        case SelectionRequest:
            fprintf(stderr, "9wm: SelectionRequest (this should not happen)\n");
            break;
        case EnterNotify:
            enter(&ev.xcrossing);
            break;
        case ReparentNotify:
            reparent(&ev.xreparent);
            break;
        case MotionNotify:
        case Expose:
        case FocusIn:
        case FocusOut:
        case ConfigureNotify:
        case MappingNotify:
        case MapNotify:
            /* not interested */
            break;
        }
    }
}

void
activateprevious()
{
  Client * c, * tmp = NULL;

  if (!current)
    current = clients;

  if (!clients || !current)
    return;

  for (c = clients; c->next; c=c->next)
    {
      if (c->virtual == virtual &&
	  c->state == NormalState)
	tmp = c;
      if (tmp && 
	  c->next == current)
	break;
    }

  if (tmp && 
      tmp->state == NormalState && 
      tmp->parent != root)	/* paranoid */
    {
      active(tmp);
      XMapRaised(dpy, tmp->parent);
      return;
    }
}

void
activatenext()
{
  Client * c;

  if (!current)
    current = clients;

  if (!clients || !current)
    return;

  for (c=current->next; c != current; c = ((c && c->next) ? c->next : clients))
    if (c &&
	c->state == NormalState && 
	c->virtual == virtual && 
	c->parent != root)	/* paranoid */
      {
	active(c);
	XMapRaised(dpy, c->parent);
	return;
      }
}

void
usage()
{
    fprintf(stderr, "usage: x9wm [[-display|-dpy] dpy] [-grey] [-version] [-font fname] [-pass]\n"
"       [-nokeys] [-debug] [-nostalgia] [-term prog] [-pass] [-virtuals n]\n"
"       [exit|restart]\n");
    exit(1);
}

void
sendcmessage(w, a, x)
Window w;
Atom a;
long x;
{
    XEvent ev;
    int status;
    long mask;

    memset(&ev, 0, sizeof(ev));
    ev.xclient.type = ClientMessage;
    ev.xclient.window = w;
    ev.xclient.message_type = a;
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = x;
    ev.xclient.data.l[1] = timestamp();
    mask = 0L;
    if (w == root)
        mask = SubstructureRedirectMask;        /* magic! */
    status = XSendEvent(dpy, w, False, mask, &ev);
    if (status == 0)
        fprintf(stderr, "9wm: sendcmessage failed\n");
}

Time
timestamp()
{
    XEvent ev;

    if (curtime == CurrentTime) {
        XChangeProperty(dpy, root, _9wm_running, _9wm_running, 8,
                PropModeAppend, (unsigned char *)"", 0);
        XMaskEvent(dpy, PropertyChangeMask, &ev);
        curtime = ev.xproperty.time;
    }
    return curtime;
}

void
sendconfig(c)
Client *c;
{
    XConfigureEvent ce;

    ce.type = ConfigureNotify;
    ce.event = c->window;
    ce.window = c->window;
    ce.x = c->x;
    ce.y = c->y;
    ce.width = c->dx;
    ce.height = c->dy;
    ce.border_width = c->border;
    ce.above = None;
    ce.override_redirect = 0;
    XSendEvent(dpy, c->window, False, StructureNotifyMask, (XEvent*)&ce);
}

void
scanwins()
{
    unsigned int i, nwins;
    Client *c;
    Window dw1, dw2, *wins;
    XWindowAttributes attr;

    XQueryTree(dpy, root, &dw1, &dw2, &wins, &nwins);
    for (i = 0; i < nwins; i++) {
        XGetWindowAttributes(dpy, wins[i], &attr);
        if (attr.override_redirect || wins[i] == menuwin)
            continue;
        c = getclient(wins[i], 1);
        if (c != 0 && c->window == wins[i]) {
            c->x = attr.x;
            c->y = attr.y;
            c->dx = attr.width;
            c->dy = attr.height;
            c->border = attr.border_width;
            if (attr.map_state == IsViewable)
                manage(c, 1);
        }
    }
    XFree((void *) wins);   /* cast is to shut stoopid compiler up */
}

void
configurereq(e)
XConfigureRequestEvent *e;
{
    XWindowChanges wc;
/*      XConfigureEvent ce; */
    Client *c;

    /* we don't set curtime as nothing here uses it */
    c = getclient(e->window, 0);
    if (c) {
        gravitate(c, 1);

        if (e->value_mask & CWX)
            c->x = e->x;
        if (e->value_mask & CWY)
            c->y = e->y;
        if (e->value_mask & CWWidth)
	  {
            c->dx = e->width;
	    XResizeWindow(dpy, c->parent, c->dx+2*(BORDER-1), c->dy+2*(BORDER-1)); 
	  }
        if (e->value_mask & CWHeight)
	  {
            c->dy = e->height;
	    XResizeWindow(dpy, c->parent, c->dx+2*(BORDER-1), c->dy+2*(BORDER-1)); 
	  }
        if (e->value_mask & CWBorderWidth)
            c->border = e->border_width;
        gravitate(c, 0);
        if (c->parent != root && c->window == e->window) {
            wc.x = c->x-BORDER;
            wc.y = c->y-BORDER;
            wc.width = c->dx+2*(BORDER-1);
            wc.height = c->dy+2*(BORDER-1);
            wc.border_width = 1;
            wc.sibling = e->above;
            wc.stack_mode = e->detail;
            XConfigureWindow(dpy, e->parent, e->value_mask, &wc);
            sendconfig(c);
        }
    }

    if (c && c->init) {
        wc.x = BORDER-1;
        wc.y = BORDER-1;
    }
    else {
        wc.x = e->x;
        wc.y = e->y;
    }
    wc.width = e->width;
    wc.height = e->height;
    wc.border_width = 0;
    wc.sibling = e->above;
    wc.stack_mode = e->detail;
    e->value_mask |= CWBorderWidth;

    XConfigureWindow(dpy, e->window, e->value_mask, &wc);
}

void
mapreq(e)
XMapRequestEvent *e;
{
    Client *c;

    curtime = CurrentTime;
    c = getclient(e->window, 0);
    if (c == 0 || c->window != e->window) {
        fprintf(stderr, "9wm: bogus mapreq %p %p\n", c, (void*)e->window);
        return;
    }

    switch (c->state) {
    case WithdrawnState:
        if (c->parent == root) {
            if (!manage(c, 0))
                return;
            break;
        }
        XReparentWindow(dpy, c->window, c->parent, BORDER-1, BORDER-1);
        XAddToSaveSet(dpy, c->window);
        /* fall through... */
    case NormalState:
        XMapRaised(dpy, c->parent);
        XMapWindow(dpy, c->window);
        setstate9(c, NormalState);
	//        if (c->trans != None && current && c->trans == current->window)
                active(c);
        break;
    case IconicState:
        unhidec(c, 1);
        break;
    }
}

void
unmap(e)
XUnmapEvent *e;
{
    Client *c;

    curtime = CurrentTime;
    c = getclient(e->window, 0);
    if (c) {
        switch (c->state) {
        case IconicState:
            if (e->send_event) {
                unhidec(c, 0);
                withdraw(c);
            }
            break;
        case NormalState:
            if (c == current)
                nofocus();
            if (!c->reparenting)
	      {
                withdraw(c);
	      }
            break;
        }
        c->reparenting = 0;
    }
}

void
circulatereq(e)
XCirculateRequestEvent *e;
{
    fprintf(stderr, "It must be the warlock Krill!\n");  /* :-) */
}

void
newwindow(e)
XCreateWindowEvent *e;
{
    Client *c;

    /* we don't set curtime as nothing here uses it */
    if (e->override_redirect)
        return;
    c = getclient(e->window, 1);
    if (c && c->parent == root) {
        c->x = e->x;
        c->y = e->y;
        c->dx = e->width;
        c->dy = e->height;
        c->border = e->border_width;
    }
}

void
destroy(w)
Window w;
{
    Client *c;

    curtime = CurrentTime;
    c = getclient(w, 0);
    if (c == 0)
        return;

    rmclient(c);

    /* flush any errors generated by the window's sudden demise */
    ignore_badwindow = 1;
    XSync(dpy, False);
    ignore_badwindow = 0;
}

void
clientmesg(e)
XClientMessageEvent *e;
{
    Client *c;

    curtime = CurrentTime;
    if (e->window == root && e->message_type == exit_9wm) {
        cleanup();
        exit(0);
    }
    if (e->window == root && e->message_type == restart_9wm) {
        fprintf(stderr, "*** 9wm restarting ***\n");
        cleanup();
        execvp(myargv[0], myargv);
        perror("9wm: exec failed");
        exit(1);
    }
    if (e->message_type == wm_change_state) {
        c = getclient(e->window, 0);
        if (e->format == 32 && e->data.l[0] == IconicState && c != 0) {
            if (normal(c))
                hide(c);
        }
        else
            fprintf(stderr, "9wm: WM_CHANGE_STATE: format %d data %d w 0p%p\n",
                e->format, (int)e->data.l[0], (void*)e->window);
        return;
    }
    fprintf(stderr, "9wm: strange ClientMessage, type 0x%x window 0x%x\n",
        (int)e->message_type, (int)e->window);
}

void
cmap(e)
XColormapEvent *e;
{
    Client *c;
    int i;

    /* we don't set curtime as nothing here uses it */
    if (e->new) {
        c = getclient(e->window, 0);
        if (c) {
            c->cmap = e->colormap;
            if (c == current)
                cmapfocus(c);
        }
        else
            for (c = clients; c; c = c->next)
                for (i = 0; i < c->ncmapwins; i++)
                    if (c->cmapwins[i] == e->window) {
                        c->wmcmaps[i] = e->colormap;
                        if (c == current)
                            cmapfocus(c);
                        return;
                    }
    }
}

void
property(e)
XPropertyEvent *e;
{
    Atom a;
    int delete;
    Client *c;

    /* we don't set curtime as nothing here uses it */
    a = e->atom;
    delete = (e->state == PropertyDelete);
    c = getclient(e->window, 0);
    if (c == 0)
        return;

    switch (a) {
    case XA_WM_ICON_NAME:
        if (c->iconname != 0)
            XFree((char*) c->iconname);
        c->iconname = getprop(c->window, XA_WM_ICON_NAME);
        setlabel(c);
        renamec(c, c->label);
        return;
    case XA_WM_NAME:
        if (c->name != 0)
            XFree((char*) c->name);
        c->name = getprop(c->window, XA_WM_NAME);
        setlabel(c);
        renamec(c, c->label);
        return;
    case XA_WM_TRANSIENT_FOR:
        gettrans(c);
        return;
    }
    if (a == _9wm_hold_mode) {
        c->hold = getiprop(c->window, _9wm_hold_mode);
        if (c == current)
            draw_border(c, 1);
    }
    else if (a == wm_colormaps) {
        getcmaps(c);
        if (c == current)
            cmapfocus(c);
    }
}

void
reparent(e)
XReparentEvent *e;
{
    Client *c;
    XWindowAttributes attr;

    if (e->event != root || e->override_redirect)
        return;
    if (e->parent == root) {
        c = getclient(e->window, 1);
        if (c != 0 && (c->dx == 0 || c->dy == 0)) {
            XGetWindowAttributes(dpy, c->window, &attr);
            c->x = attr.x;
            c->y = attr.y;
            c->dx = attr.width;
            c->dy = attr.height;
            c->border = attr.border_width;
        }
    }
    else {
        c = getclient(e->window, 0);
        if (c != 0 && (c->parent == root || withdrawn(c))) 
            rmclient(c);
    }
}

#ifdef  SHAPE
void
shapenotify(e)
XShapeEvent *e;
{
    Client *c;

    c = getclient(e->window, 0);
    if (c == 0)
        return;

    setshape(c);
}
#endif

void
enter(e)
XCrossingEvent *e;
{
    Client *c;

    curtime = e->time;
    if (e->mode != NotifyGrab || e->detail != NotifyNonlinearVirtual)
        return;
    c = getclient(e->window, 0);
    if (c != 0 && c != current) {
        XMapRaised(dpy, c->parent);
        active(c);
    }
}

void
sighandler()
{
    signalled = 1;
}

void
getevent(e)
XEvent *e;
{
    int fd;
    fd_set rfds;
    struct timeval t;

    if (!signalled) {
        if (QLength(dpy) > 0) {
            XNextEvent(dpy, e);
            return;
        }
        fd = ConnectionNumber(dpy);
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        t.tv_sec = t.tv_usec = 0;
        if (select(fd+1, &rfds, NULL, NULL, &t) == 1) {
            XNextEvent(dpy, e);
            return;
        }
        XFlush(dpy);
        FD_SET(fd, &rfds);
        if (select(fd+1, &rfds, NULL, NULL, NULL) == 1) {
            XNextEvent(dpy, e);
            return;
        }
        if (errno != EINTR || !signalled) {
            perror("9wm: select failed");
            exit(1);
        }
    }
    cleanup();
    fprintf(stderr, "9wm: exiting on signal\n");
    exit(1);
}


void
parseprogsfile ()
{
  FILE *file;
  int i;
  struct passwd * p = getpwuid(getuid());
  char * buffer;

  buffer = (char *) malloc (1024);
  snprintf (buffer, 1024, "%s/.x9wmrc", p->pw_dir);

  file = fopen (buffer, "r");
  if (! file)
    {
      fprintf (stderr, "cannot open %s\n", buffer);
      progsnames[0] = NULL;
      return;
    }

  for (i = 0; i<16 && ! feof(file); i++)
    {
      buffer = (char *) malloc (1024);
      if (! fgets(buffer, 1024, file))
	break;
      if (buffer[strlen(buffer)-1] == '\n')
	buffer[strlen(buffer)-1] = 0;
      progsnames[i] = buffer;
    }

  progsnames[i] = NULL;

  fclose (file);
  return;
}

Client      *clients;
Client      *current = NULL;	/* paranoid */

void
setactive(c, on)
Client *c;
int on;
{
    if (on) {
        XUngrabButton(dpy, AnyButton, AnyModifier, c->parent);
	XSetInputFocus(dpy, c->window, RevertToPointerRoot, timestamp()); 

	if (use_keys)
	  {
	    XGrabKey(dpy, 
		     XKeysymToKeycode(dpy, XK_Tab),
		     ControlMask, 
		     root, False, GrabModeAsync, GrabModeAsync);
	    XGrabKey(dpy, 
		     XKeysymToKeycode(dpy, XK_Tab),
		     ControlMask|ShiftMask, 
		     root, False, GrabModeAsync, GrabModeAsync);
	  }

        if (c->proto & Ptakefocus)
            sendcmessage(c->window, wm_protocols, wm_take_focus);
        cmapfocus(c);
    }
    else
        XGrabButton(dpy, AnyButton, AnyModifier, c->parent, False,
            ButtonMask, GrabModeAsync, GrabModeSync, None, None);
    draw_border(c, on);
}

void
draw_border(c, active)
Client *c;
int active;
{
    XSetWindowBackground(dpy, c->parent, active ? black : white);
    XClearWindow(dpy, c->parent);
    if (c->hold && active)
        XDrawRectangle(dpy, c->parent, gc, 1, 1, c->dx+BORDER-1, c->dy+BORDER-1);
}

#ifdef  DEBUG
void
dump_revert()
{
    Client *c;
    int i;

    i = 0;
    for (c = current; c; c = c->revert) {
        fprintf(stderr, "%s(%x:%d)", c->label ? c->label : "?", c->window, c->state);
        if (i++ > 100)
            break;
        if (c->revert)
            fprintf(stderr, " -> ");
    }
    if (current == 0)
        fprintf(stderr, "empty");
    fprintf(stderr, "\n");
}

void
dump_clients()
{
    Client *c;

    for (c = clients; c; c = c->next)
        fprintf(stderr, "w 0x%x parent 0x%x @ (%d, %d)\n", c->window, c->parent, c->x, c->y);
}
#endif

void
active(c)
Client *c;
{
    Client *cc;

    if (c == 0) {
        fprintf(stderr, "9wm: active(c==0)\n");
        return;
    }
    if (c == current)
        return;
    if (current)
        setactive(current, 0);
    setactive(c, 1);
    for (cc = clients; cc; cc = cc->next)
        if (cc->revert == c)
            cc->revert = c->revert;
    c->revert = current;
    while (c->revert && !normal(c->revert))
        c->revert = c->revert->revert;
    current = c;
#ifdef  DEBUG
    if (debug)
        dump_revert();
#endif
}

void
nofocus()
{
    static Window w = 0;
    int mask;
    XSetWindowAttributes attr;
    Client *c;

    if (current) {
        setactive(current, 0);
        for (c = current->revert; c; c = c->revert)
            if (normal(c)) {
                active(c);
                return;
            }
        /* if no candidates to revert to, fall through */
    }
    current = 0;
    if (w == 0) {
        mask = CWOverrideRedirect;
        attr.override_redirect = 1;
        w = XCreateWindow(dpy, root, 0, 0, 1, 1, 0, CopyFromParent,
            InputOnly, CopyFromParent, mask, &attr);
        XMapWindow(dpy, w);
    }
    XSetInputFocus(dpy, w, RevertToPointerRoot, timestamp());
    cmapfocus(0);
}

Client *
getclient(w, create)
Window w;
int create;
{
    Client *c;

    if (w == 0 || w == root)
        return 0;

    for (c = clients; c; c = c->next)
        if (c->window == w || c->parent == w)
            return c;

    if (!create)
        return 0;

    c = (Client *)malloc(sizeof(Client));
    memset(c, 0, sizeof(Client));
    c->window = w;
    c->parent = root;
    c->reparenting = 0;
    c->state = WithdrawnState;
    c->init = 0;
    c->cmap = None;
    c->label = c->class = 0;
    c->revert = 0;
    c->is9term = 0;
    c->hold = 0;
    c->ncmapwins = 0;
    c->cmapwins = 0;
    c->wmcmaps = 0;
    c->next = clients;
    c->virtual = virtual;
    clients = c;
    return c;
}

void
rmclient(c)
Client *c;
{
    Client *cc;

    for (cc = current; cc && cc->revert; cc = cc->revert)
        if (cc->revert == c)
            cc->revert = cc->revert->revert;

    if (c == clients)
        clients = c->next;
    for (cc = clients; cc && cc->next; cc = cc->next)
        if (cc->next == c)
            cc->next = cc->next->next;

    if (hidden(c))
        unhidec(c, 0);

    if (c->parent != root)
        XDestroyWindow(dpy, c->parent);

    c->parent = c->window = None;       /* paranoia */
    if (current == c) {
        current = c->revert;
        if (current == 0)
            nofocus();
        else
            setactive(current, 1);
    }
    if (c->ncmapwins != 0) {
        XFree((char *)c->cmapwins);
        free((char *)c->wmcmaps);
    }
    if (c->iconname != 0)
        XFree((char*) c->iconname);
    if (c->name != 0)
        XFree((char*) c->name);
    if (c->instance != 0)
        XFree((char*) c->instance);
    if (c->class != 0)
        XFree((char*) c->class);
    memset(c, 0, sizeof(Client));       /* paranoia */
    free(c);
}

Cursor  target;
Cursor  sweep0;
Cursor  boxcurs;
Cursor  arrow;
Pixmap  root_pixmap;

typedef struct {
    int             width;
    int             hot[2];
    unsigned char   mask[64];
    unsigned char   fore[64];
} Cursordata;

Cursordata sweep0data = {
    16,
    {7, 7},
    {0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03,
     0xC0, 0x03, 0xC0, 0x03, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x03, 0xC0, 0x03,
     0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03},
    {0x00, 0x00, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01,
     0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0xFE, 0x7F,
     0xFE, 0x7F, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01,
     0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x00, 0x00}
};

Cursordata boxcursdata = {
    16,
    {7, 7},
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0x1F, 0xF8, 0x1F, 0xF8, 0x1F, 0xF8,
     0x1F, 0xF8, 0x1F, 0xF8, 0x1F, 0xF8, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {0x00, 0x00, 0xFE, 0x7F, 0xFE, 0x7F, 0xFE, 0x7F,
     0x0E, 0x70, 0x0E, 0x70, 0x0E, 0x70, 0x0E, 0x70,
     0x0E, 0x70, 0x0E, 0x70, 0x0E, 0x70, 0x0E, 0x70,
     0xFE, 0x7F, 0xFE, 0x7F, 0xFE, 0x7F, 0x00, 0x00}
};

Cursordata sightdata = {
    16,
    {7, 7},
    {0xF8, 0x1F, 0xFC, 0x3F, 0xFE, 0x7F, 0xDF, 0xFB,
     0xCF, 0xF3, 0xC7, 0xE3, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xC7, 0xE3, 0xCF, 0xF3,
     0xDF, 0x7B, 0xFE, 0x7F, 0xFC, 0x3F, 0xF8, 0x1F,},
    {0x00, 0x00, 0xF0, 0x0F, 0x8C, 0x31, 0x84, 0x21,
     0x82, 0x41, 0x82, 0x41, 0x82, 0x41, 0xFE, 0x7F,
     0xFE, 0x7F, 0x82, 0x41, 0x82, 0x41, 0x82, 0x41,
     0x84, 0x21, 0x8C, 0x31, 0xF0, 0x0F, 0x00, 0x00,}
};

Cursordata arrowdata = {
    16,
    {1, 1},
    {0xFF, 0x07, 0xFF, 0x07, 0xFF, 0x03, 0xFF, 0x00,
     0xFF, 0x00, 0xFF, 0x01, 0xFF, 0x03, 0xFF, 0x07,
     0xE7, 0x0F, 0xC7, 0x1F, 0x83, 0x3F, 0x00, 0x7F,
     0x00, 0xFE, 0x00, 0x7C, 0x00, 0x38, 0x00, 0x10,},
    {0x00, 0x00, 0xFE, 0x03, 0xFE, 0x00, 0x3E, 0x00,
     0x7E, 0x00, 0xFE, 0x00, 0xF6, 0x01, 0xE6, 0x03,
     0xC2, 0x07, 0x82, 0x0F, 0x00, 0x1F, 0x00, 0x3E,
     0x00, 0x7C, 0x00, 0x38, 0x00, 0x10, 0x00, 0x00,}
};

Cursordata whitearrow = {
    16,
    {0, 0},
    {0xFF, 0x07, 0xFF, 0x07, 0xFF, 0x03, 0xFF, 0x00,
     0xFF, 0x00, 0xFF, 0x01, 0xFF, 0x03, 0xFF, 0x07,
     0xE7, 0x0F, 0xC7, 0x1F, 0x83, 0x3F, 0x00, 0x7F,
     0x00, 0xFE, 0x00, 0x7C, 0x00, 0x38, 0x00, 0x10,},
    {0xFF, 0x07, 0xFF, 0x07, 0x83, 0x03, 0xC3, 0x00,
     0xC3, 0x00, 0x83, 0x01, 0x1B, 0x03, 0x3F, 0x06,
     0x67, 0x0C, 0xC7, 0x18, 0x83, 0x31, 0x00, 0x63,
     0x00, 0xC6, 0x00, 0x6C, 0x00, 0x38, 0x00, 0x10,}
};

Cursordata blittarget = {
    18,
    {8, 8},
    {0xe0, 0x1f, 0x00, 0xf0, 0x3f, 0x00, 0xf8, 0x7f, 0x00,
     0xfc, 0xff, 0x00, 0xfe, 0xff, 0x01, 0xff, 0xff, 0x03,
     0xff, 0xff, 0x03, 0xff, 0xff, 0x03, 0xff, 0xff, 0x03,
     0xff, 0xff, 0x03, 0xff, 0xff, 0x03, 0xff, 0xff, 0x03,
     0xff, 0xff, 0x03, 0xfe, 0xff, 0x01, 0xfc, 0xff, 0x00,
     0xf8, 0x7f, 0x00, 0xf0, 0x3f, 0x00, 0xe0, 0x1f, 0x00},
    {0x00, 0x00, 0x00, 0xc0, 0x0f, 0x00, 0xf0, 0x3f, 0x00,
     0x38, 0x73, 0x00, 0x8c, 0xc7, 0x00, 0xec, 0xdf, 0x00,
     0x66, 0x9b, 0x01, 0x36, 0xb3, 0x01, 0xfe, 0xff, 0x01,
     0xfe, 0xff, 0x01, 0x36, 0xb3, 0x01, 0x66, 0x9b, 0x01,
     0xec, 0xdf, 0x00, 0x8c, 0xc7, 0x00, 0x38, 0x73, 0x00,
     0xf0, 0x3f, 0x00, 0xc0, 0x0f, 0x00, 0x00, 0x00, 0x00}
};

Cursordata blitarrow = {
    18,
    {1, 1},
    {0xff, 0x0f, 0x00, 0xff, 0x07, 0x00, 0xff, 0x03, 0x00,
     0xff, 0x00, 0x00, 0xff, 0x00, 0x00, 0xff, 0x01, 0x00,
     0xff, 0x03, 0x00, 0xff, 0x07, 0x00, 0xe7, 0x0f, 0x00,
     0xc7, 0x1f, 0x00, 0x87, 0x3f, 0x00, 0x03, 0x7f, 0x00,
     0x01, 0xfe, 0x00, 0x00, 0xfc, 0x01, 0x00, 0xf8, 0x03,
     0x00, 0xf0, 0x01, 0x00, 0xe0, 0x00, 0x00, 0x40, 0x00},
    {0x00, 0x00, 0x00, 0xfe, 0x03, 0x00, 0xfe, 0x00, 0x00,
     0x3e, 0x00, 0x00, 0x7e, 0x00, 0x00, 0xfe, 0x00, 0x00,
     0xf6, 0x01, 0x00, 0xe6, 0x03, 0x00, 0xc2, 0x07, 0x00,
     0x82, 0x0f, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x3e, 0x00,
     0x00, 0x7c, 0x00, 0x00, 0xf8, 0x00, 0x00, 0xf0, 0x01,
     0x00, 0xe0, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00}
};

Cursordata blitsweep = {
    18,
    {8, 8},
    {0xc4, 0xff, 0x03, 0xce, 0xff, 0x03, 0xdf, 0xff, 0x03,
     0x3e, 0x80, 0x03, 0x7c, 0x83, 0x03, 0xf8, 0x83, 0x03,
     0xf7, 0x83, 0x03, 0xe7, 0x83, 0x03, 0xf7, 0x83, 0x03,
     0xf7, 0x83, 0x03, 0x07, 0x80, 0x03, 0x07, 0x80, 0x03,
     0x07, 0x80, 0x03, 0x07, 0x80, 0x03, 0x07, 0x80, 0x03,
     0xff, 0xff, 0x03, 0xff, 0xff, 0x03, 0xff, 0xff, 0x03},
    {0x00, 0x00, 0x00, 0x84, 0xff, 0x01, 0x0e, 0x00, 0x01,
     0x1c, 0x00, 0x01, 0x38, 0x00, 0x01, 0x70, 0x01, 0x01,
     0xe0, 0x01, 0x01, 0xc2, 0x01, 0x01, 0xe2, 0x01, 0x01,
     0x02, 0x00, 0x01, 0x02, 0x00, 0x01, 0x02, 0x00, 0x01,
     0x02, 0x00, 0x01, 0x02, 0x00, 0x01, 0x02, 0x00, 0x01,
     0x02, 0x00, 0x01, 0xfe, 0xff, 0x01, 0x00, 0x00, 0x00}
};

/*
 *  Grey tile pattern for root background
 */

#define grey_width 4
#define grey_height 2
static char grey_bits[] = {
   0x01, 0x04};

Cursor
getcursor(c)
Cursordata *c;
{
    Pixmap f, m;
    XColor bl, wh, rd, gr, bu, d;

    f = XCreatePixmapFromBitmapData(dpy, root, (char *)c->fore, c->width, c->width, 1, 0, 1);
    m = XCreatePixmapFromBitmapData(dpy, root, (char *)c->mask, c->width, c->width, 1, 0, 1);
    XAllocNamedColor(dpy, DefaultColormap(dpy, screen), "black", &bl, &d);
    XAllocNamedColor(dpy, DefaultColormap(dpy, screen), "white", &wh, &d);
    XAllocNamedColor(dpy, DefaultColormap(dpy, screen), "red", &rd, &d);
    XAllocNamedColor(dpy, DefaultColormap(dpy, screen), "green", &gr, &d);
    XAllocNamedColor(dpy, DefaultColormap(dpy, screen), "blue", &bu, &d);
    return XCreatePixmapCursor(dpy, f, m, &rd, &wh, c->hot[0], c->hot[1]);
}

void
initcurs()
{
    if (nostalgia) {
        arrow = getcursor(&blitarrow);
        target = getcursor(&blittarget);
        sweep0 = getcursor(&blitsweep);
        boxcurs = getcursor(&blitsweep);
    }
    else {
        arrow = getcursor(&arrowdata);
        target = getcursor(&sightdata);
        sweep0 = getcursor(&sweep0data);
        boxcurs = getcursor(&boxcursdata);
    }

    root_pixmap = XCreatePixmapFromBitmapData(dpy, root,
        grey_bits, grey_width, grey_height,
        black, white, DefaultDepth(dpy, screen));
}

int     ignore_badwindow;

void
fatal(s)
char *s;
{
    fprintf(stderr, "9wm: ");
    perror(s);
    fprintf(stderr, "\n");
    exit(1);
}

int
handler(d, e)
Display *d;
XErrorEvent *e;
{
    char msg[80], req[80], number[80];

    if (initting && (e->request_code == X_ChangeWindowAttributes) && (e->error_code == BadAccess)) {
        fprintf(stderr, "9wm: it looks like there's already a window manager running;  9wm not started\n");
        exit(1);
    }

    if (ignore_badwindow && (e->error_code == BadWindow || e->error_code == BadColor))
        return 0;

    XGetErrorText(d, e->error_code, msg, sizeof(msg));
    sprintf(number, "%d", e->request_code);
    XGetErrorDatabaseText(d, "XRequest", number, "<unknown>", req, sizeof(req));

    fprintf(stderr, "9wm: %s(0x%x): %s\n", req, (unsigned int)e->resourceid, msg);

    if (initting) {
        fprintf(stderr, "9wm: failure during initialisation; aborting\n");
        exit(1);
    }
    return 0;
}

void
graberror(f, err)
char *f;
int err;
{
    char *s;

    switch (err) {
    case GrabNotViewable:
        s = "not viewable";
        break;
    case AlreadyGrabbed:
        s = "already grabbed";
        break;
    case GrabFrozen:
        s = "grab frozen";
        break;
    case GrabInvalidTime:
        s = "invalid time";
        break;
    case GrabSuccess:
        return;
    default:
        fprintf(stderr, "9wm: %s: grab error: %d\n", f, err);
        return;
    }
    fprintf(stderr, "9wm: %s: grab error: %s\n", f, s);
}

void
showhints(c, size)
Client *c;
XSizeHints *size;
{
#ifdef  DEBUG
    fprintf(stderr, "\nNew window: %s(%s) ", c->label ? c->label : "???", c->class ? c->class : "???");
    fprintf(stderr, "posn (%d,%d) size (%d,%d)\n", c->x, c->y, c->dx, c->dy);

    if (size == 0) {
        fprintf(stderr, "no size hints\n");
        return;
    }

    fprintf(stderr, "size hints: ");
    if (size->flags&USPosition)
        fprintf(stderr, "USPosition(%d,%d) ", size->x, size->y);
    if (size->flags&USSize)
        fprintf(stderr, "USSize(%d,%d) ", size->width, size->height);
    if (size->flags&PPosition)
        fprintf(stderr, "PPosition(%d,%d) ", size->x, size->y);
    if (size->flags&PSize)
        fprintf(stderr, "PSize(%d,%d) ", size->width, size->height);
    if (size->flags&PMinSize)
        fprintf(stderr, "PMinSize(%d,%d) ", size->min_width, size->min_height);
    if (size->flags&PMaxSize)
        fprintf(stderr, "PMaxSize(%d,%d) ", size->max_width, size->max_height);
    if (size->flags&PResizeInc)
        fprintf(stderr, "PResizeInc ");
    if (size->flags&PAspect)
        fprintf(stderr, "PAspect ");
    if (size->flags&PBaseSize)
        fprintf(stderr, "PBaseSize ");
    if (size->flags&PWinGravity)
        fprintf(stderr, "PWinGravity ");
    fprintf(stderr, "\n");
#endif
}


int
nobuttons(e)    /* Einstuerzende */
XButtonEvent *e;
{
    int state;

    state = (e->state & AllButtonMask);
    return (e->type == ButtonRelease) && (state & (state - 1)) == 0;
}

int
grab(w, mask, curs, t)
Window w;
int mask;
Cursor curs;
int t;
{
    int status;

    if (t == 0)
        t = timestamp();
    status = XGrabPointer(dpy, w, False, mask,
        GrabModeAsync, GrabModeAsync, None, curs, t);
    return status;
}

void
ungrab(e)
XButtonEvent *e;
{
    XEvent ev;

    if (!nobuttons(e))
        for (;;) {
            XMaskEvent(dpy, ButtonMask | ButtonMotionMask, &ev);
            if (ev.type == MotionNotify)
                continue;
            e = &ev.xbutton;
            if (nobuttons(e))
                break;
        }
    XUngrabPointer(dpy, e->time);
    curtime = e->time;
}


void 
draw_text(been_here, wide, high, n, cur, m)
int * been_here;
int wide;
int high;
int n;
int cur;
Menu * m;
{
	int i;
	int tx, ty;

	if (*been_here){
		return;
	}

	*been_here = 1;

	for (i=0; i<n; i++){
		tx = (wide - XTextWidth(font, m->item[i], strlen(m->item[i])))/2;
		ty = i*high + font->ascent + 1;
		XDrawString(dpy, menuwin, gc, tx, ty, m->item[i], strlen(m->item[i]));
                
	}

	if (cur >= 0 && cur < n) {
                XFillRectangle(dpy, menuwin, gc, 0, cur*high, wide, high);
	}
}

int menuhit(e, m)
XButtonEvent * e;
Menu * m;
{
    int drawn;
    XEvent ev;
    int i, n, cur, old, wide, high, status, warp;
    int x, y, dx, dy, xmax, ymax;

    if (e->window == menuwin)       /* ugly event mangling */
        return -1;

    dx = 0;
    for (n = 0; m->item[n]; n++) {
        wide = XTextWidth(font, m->item[n], strlen(m->item[n])) + 4;
        if (wide > dx)
            dx = wide;
    }

    wide = dx;
    cur = m->lasthit;
    if (cur >= n)
        cur = n - 1;

    high = font->ascent + font->descent + 1;
    dy = n*high;
    x = e->x - wide/2;
    y = e->y - cur*high - high/2;
    warp = 0;
    xmax = DisplayWidth(dpy, screen);
    ymax = DisplayHeight(dpy, screen);
    if (x < 0) {
        e->x -= x;
        x = 0;
        warp++;
    }
    if (x+wide >= xmax) {
        e->x -= x+wide-xmax;
        x = xmax-wide;
        warp++;
    }
    if (y < 0) {
        e->y -= y;
        y = 0;
        warp++;
    }
    if (y+dy >= ymax) {
        e->y -= y+dy-ymax;
        y = ymax-dy;
        warp++;
    }
    if (warp)
        setmouse(e->x, e->y);
    XMoveResizeWindow(dpy, menuwin, x, y, dx, dy);
    XSelectInput(dpy, menuwin, MenuMask);
    XMapRaised(dpy, menuwin);
    status = grab(menuwin, MenuGrabMask, None, e->time);
    if (status != GrabSuccess) {
        /* graberror("menuhit", status); */
        XUnmapWindow(dpy, menuwin);
        return -1;
    }
   
    drawn = 0;
    for (;;) {
        XMaskEvent(dpy, MenuMask, &ev);
        switch (ev.type) {
        default:
            fprintf(stderr, "9wm: menuhit: unknown ev.type %d\n", ev.type);
            break;
        case ButtonPress:
            break;
        case ButtonRelease:
            if (ev.xbutton.button != e->button)
                break;
            x = ev.xbutton.x;
            y = ev.xbutton.y;
            i = y/high;
            if (cur >= 0 && y >= cur*high-3 && y < (cur+1)*high+3)
                i = cur;
            if (x < 0 || x > wide || y < -3)
                i = -1;
            else if (i < 0 || i >= n)
                i = -1;
            else 
                m->lasthit = i;
            if (!nobuttons(&ev.xbutton))
                i = -1;
            ungrab(&ev.xbutton);
            XUnmapWindow(dpy, menuwin);
            return i;
        case MotionNotify:
	    draw_text(&drawn, wide, high, n, cur, m);
            x = ev.xbutton.x;
            y = ev.xbutton.y;
            old = cur;
            cur = y/high;
            if (old >= 0 && y >= old*high-3 && y < (old+1)*high+3)
                cur = old;
            if (x < 0 || x > wide || y < -3)
                cur = -1;
            else if (cur < 0 || cur >= n)
                cur = -1;
            if (cur == old)
                break;
            if (old >= 0 && old < n)
                XFillRectangle(dpy, menuwin, gc, 0, old*high, wide, high);
            if (cur >= 0 && cur < n)
                XFillRectangle(dpy, menuwin, gc, 0, cur*high, wide, high);
            break;
        case Expose:
	    draw_text(&drawn, wide, high, n, cur, m);
        }
    }
}



Client *
selectwin(release, shift)
int release;
int *shift;
{
    XEvent ev;
    XButtonEvent *e;
    int status;
    Window w;
    Client *c;

    status = grab(root, ButtonMask, target, 0);
    if (status != GrabSuccess) {
        graberror("selectwin", status); /* */
        return 0;
    }
    w = None;
    for (;;) {
        XMaskEvent(dpy, ButtonMask, &ev);
        e = &ev.xbutton;
        switch (ev.type) {
        case ButtonPress:
            if (e->button != Button3) {
                ungrab(e);
                return 0;
            }
            w = e->subwindow;
            if (!release) {
                c = getclient(w, 0);
                if (c == 0)
                    ungrab(e);
                if (shift != 0)
                    *shift = (e->state&ShiftMask) != 0;
                return c;
            }
            break;
        case ButtonRelease:
            ungrab(e);
            if (e->button != Button3 || e->subwindow != w)
                return 0;
            if (shift != 0)
                *shift = (e->state&ShiftMask) != 0;
            return getclient(w, 0);
        }
    }
}

void
sweepcalc(c, x, y)
Client *c;
int x;
int y;
{
    int dx, dy, sx, sy;

    dx = x - c->x;
    dy = y - c->y;
    sx = sy = 1;
    if (dx < 0) {
        x += dx;
        dx = -dx;
        sx = -1;
    }
    if (dy < 0) {
        y += dy;
        dy = -dy;
        sy = -1;
    }

    dx -= 2*BORDER;
    dy -= 2*BORDER;

    if (!c->is9term) {
        if (dx < c->min_dx)
            dx = c->min_dx;
        if (dy < c->min_dy)
            dy = c->min_dy;
    }

    if (c->size.flags & PResizeInc) {
        dx = c->min_dx + (dx-c->min_dx)/c->size.width_inc*c->size.width_inc;
        dy = c->min_dy + (dy-c->min_dy)/c->size.height_inc*c->size.height_inc;
    }

    if (c->size.flags & PMaxSize) {
        if (dx > c->size.max_width)
            dx = c->size.max_width;
        if (dy > c->size.max_height)
            dy = c->size.max_height;
    }
    c->dx = sx*(dx + 2*BORDER);
    c->dy = sy*(dy + 2*BORDER);
}

void
dragcalc(c, x, y)
Client *c;
int x;
int y;
{
    c->x = x;
    c->y = y;
}

void
drawbound(c)
Client *c;
{
    int x, y, dx, dy;

    x = c->x;
    y = c->y;
    dx = c->dx;
    dy = c->dy;
    if (dx < 0) {
        x += dx;
        dx = -dx;
    }
    if (dy < 0) {
        y += dy;
        dy = -dy;
    }
    if (dx <= 2 || dy <= 2)
        return;
    XDrawRectangle(dpy, root, gc, x, y, dx-1, dy-1);
    XDrawRectangle(dpy, root, gc, x+1, y+1, dx-3, dy-3);
}

int
sweepdrag(c, e0, recalc)
Client *c;
XButtonEvent *e0;
void (*recalc)();
{
    XEvent ev;
    int cx, cy, rx, ry;
    int ox, oy, odx, ody;
    struct timeval t;
    XButtonEvent *e;

    ox = c->x;
    oy = c->y;
    odx = c->dx;
    ody = c->dy;
    c->x -= BORDER;
    c->y -= BORDER;
    c->dx += 2*BORDER;
    c->dy += 2*BORDER;
    if (e0) {
        c->x = cx = e0->x;
        c->y = cy = e0->y;
        recalc(c, e0->x, e0->y);
    }
    else
        getmouse(&cx, &cy);
    XGrabServer(dpy);
    drawbound(c);
    t.tv_sec = 0;
    t.tv_usec = 50*1000;
    for (;;) {
        if (XCheckMaskEvent(dpy, ButtonMask, &ev) == 0) {
            getmouse(&rx, &ry);
            if (rx == cx && ry == cy)
                continue;
            drawbound(c);
            recalc(c, rx, ry);
            cx = rx;
            cy = ry;
            drawbound(c);
            XFlush(dpy);
            select(0, 0, 0, 0, &t);
            continue;
        }
        e = &ev.xbutton;
        switch (ev.type) {
        case ButtonPress:
        case ButtonRelease:
            drawbound(c);
            ungrab(e);
            XUngrabServer(dpy);
            if (e->button != Button3 && c->init)
                goto bad;
            recalc(c, ev.xbutton.x, ev.xbutton.y);
            if (c->dx < 0) {
                c->x += c->dx;
                c->dx = -c->dx;
            }
            if (c->dy < 0) {
                c->y += c->dy;
                c->dy = -c->dy;
            }
            c->x += BORDER;
            c->y += BORDER;
            c->dx -= 2*BORDER;
            c->dy -= 2*BORDER;
            if (c->dx < 4 || c->dy < 4 || c->dx < c->min_dx || c->dy < c->min_dy)
                goto bad;
            return 1;
        }
    }
bad:
    c->x = ox;
    c->y = oy;
    c->dx = odx;
    c->dy = ody;
    return 0;
}

int
sweep(c)
Client *c;
{
    XEvent ev;
    int status;
    XButtonEvent *e;

    status = grab(root, ButtonMask, sweep0, 0);
    if (status != GrabSuccess) {
        graberror("sweep", status); /* */
        return 0;
    }

    XMaskEvent(dpy, ButtonMask, &ev);
    e = &ev.xbutton;
    if (e->button != Button3) {
        ungrab(e);
        return 0;
    }
    if (c->size.flags & (PMinSize|PBaseSize))
        setmouse(e->x+c->min_dx, e->y+c->min_dy);
    XChangeActivePointerGrab(dpy, ButtonMask, boxcurs, e->time);
    return sweepdrag(c, e, sweepcalc);
}

int
drag(c)
Client *c;
{
    int status;

    if (c->init)
        setmouse(c->x-BORDER, c->y-BORDER);
    else
        getmouse(&c->x, &c->y);     /* start at current mouse pos */
    status = grab(root, ButtonMask, boxcurs, 0);
    if (status != GrabSuccess) {
        graberror("drag", status); /* */
        return 0;
    }
    return sweepdrag(c, 0, dragcalc);
}

void
getmouse(x, y)
int *x;
int *y;
{
    Window dw1, dw2;
    int t1, t2;
    unsigned int t3;

    XQueryPointer(dpy, root, &dw1, &dw2, x, y, &t1, &t2, &t3);
}

void
setmouse(x, y)
int x;
int y;
{
    XWarpPointer(dpy, None, root, None, None, None, None, x, y);
}


int
manage(c, mapped)
Client *c;
int mapped;
{
    int fixsize, dohide, doreshape, state;
    long msize;
    XClassHint class;
    XWMHints *hints;

    XSelectInput(dpy, c->window, ColormapChangeMask | EnterWindowMask | PropertyChangeMask);

    /* Get loads of hints */

    if (XGetClassHint(dpy, c->window, &class) != 0) {   /* ``Success'' */
        c->instance = class.res_name;
        c->class = class.res_class;
        c->is9term = (strcmp(c->class, "9term") == 0);
    }
    else {
        c->instance = 0;
        c->class = 0;
        c->is9term = 0;
    }
    c->iconname = getprop(c->window, XA_WM_ICON_NAME);
    c->name = getprop(c->window, XA_WM_NAME);
    setlabel(c);

    hints = XGetWMHints(dpy, c->window);
    if (XGetWMNormalHints(dpy, c->window, &c->size, &msize) == 0 || c->size.flags == 0)
        c->size.flags = PSize;      /* not specified - punt */

    getcmaps(c);
    getproto(c);
    gettrans(c);
    if (c->is9term)
        c->hold = getiprop(c->window, _9wm_hold_mode);

    /* Figure out what to do with the window from hints */

    if (!getstate(c->window, &state))
        state = hints ? hints->initial_state : NormalState;
    dohide = (state == IconicState);

    fixsize = 0;
    if ((c->size.flags & (USSize|PSize)))
        fixsize = 1;
    if ((c->size.flags & (PMinSize|PMaxSize)) == (PMinSize|PMaxSize) && c->size.min_width == c->size.max_width && c->size.min_height == c->size.max_height)
        fixsize = 1;
    doreshape = !mapped;
    if (fixsize) {
        if (c->size.flags & USPosition)
            doreshape = 0;
        if (dohide && (c->size.flags & PPosition))
            doreshape = 0;
        if (c->trans != None)
            doreshape = 0;
    }
    if (c->is9term)
        fixsize = 0;
    if (c->size.flags & PBaseSize) {
        c->min_dx = c->size.base_width;
        c->min_dy = c->size.base_height;
    }
    else if (c->size.flags & PMinSize) {
        c->min_dx = c->size.min_width;
        c->min_dy = c->size.min_height;
    }
    else if (c->is9term) {
        c->min_dx = 100;
        c->min_dy = 50;
    }
    else
        c->min_dx = c->min_dy = 0;

    if (hints)
        XFree(hints);

    /* Now do it!!! */

    if (doreshape) {
        cmapfocus(0);
        if (!(fixsize ? drag(c) : sweep(c)) && c->is9term) {
            XDestroyWindow(dpy, c->window);
            rmclient(c);
            cmapfocus(current);
            return 0;
        }
    }
    else
        gravitate(c, 0);

    c->parent = XCreateSimpleWindow(dpy, root,
            c->x - BORDER, c->y - BORDER,
            c->dx + 2*(BORDER-1), c->dy + 2*(BORDER-1),
            1, black, white);
    XSelectInput(dpy, c->parent, SubstructureRedirectMask | SubstructureNotifyMask);
    if (mapped)
        c->reparenting = 1;
    if (doreshape && !fixsize)
        XResizeWindow(dpy, c->window, c->dx, c->dy);
    XSetWindowBorderWidth(dpy, c->window, 0);
    XReparentWindow(dpy, c->window, c->parent, BORDER-1, BORDER-1);
#ifdef  SHAPE
    if (shape) {
        XShapeSelectInput(dpy, c->window, ShapeNotifyMask);
        ignore_badwindow = 1;       /* magic */
        setshape(c);
        ignore_badwindow = 0;
    }
#endif
    XAddToSaveSet(dpy, c->window);
    if (dohide)
        hide(c);
    else {
        XMapWindow(dpy, c->window);
        XMapWindow(dpy, c->parent);
        if (nostalgia || doreshape)
            active(c);
        else if (c->trans != None && current && current->window == c->trans)
            active(c);
        else
            setactive(c, 0);
        setstate9(c, NormalState);
    }
    if (current != c)
        cmapfocus(current);
    c->init = 1;
    return 1;
}

void
gettrans(c)
Client *c;
{
    Window trans;

    trans = None;
    if (XGetTransientForHint(dpy, c->window, &trans) != 0)
        c->trans = trans;
    else
        c->trans = None;
}

void
withdraw(c)
Client *c;
{
    XUnmapWindow(dpy, c->parent);
    gravitate(c, 1);
    XReparentWindow(dpy, c->window, root, c->x, c->y);
    gravitate(c, 0);
    XRemoveFromSaveSet(dpy, c->window);
    setstate9(c, WithdrawnState);

    /* flush any errors */
    ignore_badwindow = 1;
    XSync(dpy, False);
    ignore_badwindow = 0;
}

void
gravitate(c, invert)
Client *c;
int invert;
{
    int gravity, dx, dy, delta;

    gravity = NorthWestGravity;
    if (c->size.flags & PWinGravity)
        gravity = c->size.win_gravity;

    delta = c->border-BORDER;
    switch (gravity) {
    case NorthWestGravity:
        dx = 0;
        dy = 0;
        break;
    case NorthGravity:
        dx = delta;
        dy = 0;
        break;
    case NorthEastGravity:
        dx = 2*delta;
        dy = 0;
        break;
    case WestGravity:
        dx = 0;
        dy = delta;
        break;
    case CenterGravity:
    case StaticGravity:
        dx = delta;
        dy = delta;
        break;
    case EastGravity:
        dx = 2*delta;
        dy = delta;
        break;
    case SouthWestGravity:
        dx = 0;
        dy = 2*delta;
        break;
    case SouthGravity:
        dx = delta;
        dy = 2*delta;
        break;
    case SouthEastGravity:
        dx = 2*delta;
        dy = 2*delta;
        break;
    default:
	dx = 0;
	dy = 0;
        fprintf(stderr, "9wm: bad window gravity %d for 0x%x\n", (int)gravity, (int)c->window);
    }
    dx += BORDER;
    dy += BORDER;
    if (invert) {
        dx = -dx;
        dy = -dy;
    }
    c->x += dx;
    c->y += dy;
}

void
cleanup()
{
    Client *c;
    XWindowChanges wc;

    for (c = clients; c; c = c->next) {
        gravitate(c, 1);
        XReparentWindow(dpy, c->window, root, c->x, c->y);
        wc.border_width = c->border;
        XConfigureWindow(dpy, c->window, CWBorderWidth, &wc);
    }
    XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, timestamp());
    cmapfocus(0);
    XCloseDisplay(dpy);
}

static void
installcmap(cmap)
Colormap cmap;
{
    XInstallColormap(dpy, (cmap == None) ? def_cmap : cmap);
}

void
cmapfocus(c)
Client *c;
{
    int i, found;
    Client *cc;

    if (c == 0)
        installcmap(None);
    else if (c->ncmapwins != 0) {
        found = 0;
        for (i = c->ncmapwins-1; i >= 0; i--) {
            installcmap(c->wmcmaps[i]);
            if (c->cmapwins[i] == c->window)
                found++;
        }
        if (!found)
            installcmap(c->cmap);
    }
    else if (c->trans != None && (cc = getclient(c->trans, 0)) != 0 && cc->ncmapwins != 0)
        cmapfocus(cc);
    else
        installcmap(c->cmap);
}

void
getcmaps(c)
Client *c;
{
    int n, i;
    Window *cw;
    XWindowAttributes attr;

    if (!c->init) {
        XGetWindowAttributes(dpy, c->window, &attr);
        c->cmap = attr.colormap;
    }

    n = _getprop(c->window, wm_colormaps, XA_WINDOW, 100L, (unsigned char **)&cw);
    if (c->ncmapwins != 0) {
        XFree((char *)c->cmapwins);
        free((char *)c->wmcmaps);
    }
    if (n <= 0) {
        c->ncmapwins = 0;
        return;
    }

    c->ncmapwins = n;
    c->cmapwins = cw;

    c->wmcmaps = (Colormap*)malloc(n*sizeof(Colormap));
    for (i = 0; i < n; i++) {
        if (cw[i] == c->window)
            c->wmcmaps[i] = c->cmap;
        else {
            XSelectInput(dpy, cw[i], ColormapChangeMask);
            XGetWindowAttributes(dpy, cw[i], &attr);
            c->wmcmaps[i] = attr.colormap;
        }
    }
}

void
setlabel(c)
Client *c;
{
    if (c->iconname != 0)
        c->label = c->iconname;
    else if (c->name != 0)
        c->label = c->name;
    else if (c->instance != 0)
        c->label = c->instance;
    else
        c->label = "<unlabelled>";
}

#ifdef  SHAPE
void
setshape(c)
Client *c;
{
    int n, order;
    XRectangle *rect;

    /* cheat: don't try to add a border if the window is non-rectangular */
    rect = XShapeGetRectangles(dpy, c->window, ShapeBounding, &n, &order);
    if (n > 1)
        XShapeCombineShape(dpy, c->parent, ShapeBounding, BORDER-1, BORDER-1,
            c->window, ShapeBounding, ShapeSet);
    XFree((void*)rect);
}
#endif

int
_getprop(w, a, type, len, p)
Window w;
Atom a;
Atom type;
long len;       /* in 32-bit multiples... */
unsigned char **p;
{
    Atom real_type;
    int format;
    unsigned long n, extra;
    int status;

    status = XGetWindowProperty(dpy, w, a, 0L, len, False, type, &real_type, &format, &n, &extra, p);
    if (status != Success || *p == 0)
        return -1;
    if (n == 0)
        XFree((void*) *p);
    /* could check real_type, format, extra here... */
    return n;
}

char *
getprop(w, a)
Window w;
Atom a;
{
    unsigned char *p;

    if (_getprop(w, a, XA_STRING, 100L, &p) <= 0)
        return 0;
    return (char *)p;
}

int
get1prop(w, a, type)
Window w;
Atom a;
Atom type;
{
    char **p, *x;

    if (_getprop(w, a, type, 1L, &p) <= 0)
        return 0;
    x = *p;
    XFree((void*) p);
    return (int)x;
}

Window
getwprop(w, a)
Window w;
Atom a;
{
    return get1prop(w, a, XA_WINDOW);
}

int
getiprop(w, a)
Window w;
Atom a;
{
    return get1prop(w, a, XA_INTEGER);
}

void
setstate9(c, state)
Client *c;
int state;
{
    long data[2];

    data[0] = (long) state;
    data[1] = (long) None;

    c->state = state;
    XChangeProperty(dpy, c->window, wm_state, wm_state, 32,
        PropModeReplace, (unsigned char *)data, 2);
}

int
getstate(w, state)
Window w;
int *state;
{
    long *p = 0;

    if (_getprop(w, wm_state, wm_state, 2L, (char**)&p) <= 0)
        return 0;

    *state = (int) *p;
    XFree((char *) p);
    return 1;
}

void
getproto(c)
Client *c;
{
    Atom *p;
    int i;
    long n;
    Window w;

    w = c->window;
    c->proto = 0;
    if ((n = _getprop(w, wm_protocols, XA_ATOM, 20L, (char**)&p)) <= 0)
        return;

    for (i = 0; i < n; i++)
        if (p[i] == wm_delete)
            c->proto |= Pdelete;
        else if (p[i] == wm_take_focus)
            c->proto |= Ptakefocus;

    XFree((char *) p);
}

Client  *hiddenc[MAXHIDDEN];

int numhidden;
int virtual = 0;

Client * currents[NUMVIRTUALS] =
{
  NULL, NULL, NULL, NULL, 
};

char    *b2items[NUMVIRTUALS+1] = 
{
  "  One  ",			/* to produce a nicer menu */
  "Two",                        /* Two new workspaces */
  "Three",                      /* Is necessary? */
  "Four",                       /* Need moar.. */
  "Five",
  "Six",
  "Seven",
  "Eight",
  "Nine",
  "Ten",
  "Eleven",
  "Twelve",
  "Thirteen",
  "Fourteen",
  0,
};


Menu    b2menu =
{
    b2items,
};


char    *b3items[B3FIXED+MAXHIDDEN+1] = 
{
    "New",
    "Reshape",
    "Move",
    "Delete",
    "Hide",
    0,
    
};

Menu    b3menu =
{
    b3items,
};

Menu    egg =
{
    version,
};

Menu    progs =
{
  progsnames,
};

void
button(e)
XButtonEvent *e;
{
    Client *c;
    Window dw;

    curtime = e->time;
    c = getclient(e->window, 0);
    if (c) {
        e->x += c->x - BORDER + 1;
        e->y += c->y - BORDER + 1;
    }
    else if (e->window != root)
        XTranslateCoordinates(dpy, e->window, root, e->x, e->y,
                &e->x, &e->y, &dw);
    switch (e->button) {
    case Button1:
        if (c) {
            XMapRaised(dpy, c->parent);
            active(c);
	    if (click_passes)
	      XAllowEvents (dpy, ReplayPointer, curtime);
        }
	else if ((e->state&(ShiftMask|ControlMask))==(ShiftMask|ControlMask)
		 && progsnames[0] != NULL)
	  {
	    int n;
	    if ((n = menuhit(e, &progs)) != -1)
	      {
		if (fork() == 0) {
		  if (fork() == 0) {
		    close(ConnectionNumber(dpy));
		    execlp(progsnames[n], progsnames[n], 0);
		    exit(1);
		  }
		  exit(0);
		}
		wait((int *) 0);
	      }
	  }
        return;
    case Button2:
        if (c && click_passes) {
            XMapRaised(dpy, c->parent);
            active(c);
	    XAllowEvents (dpy, ReplayPointer, curtime);
        }
	else {
            if ((e->state&(ShiftMask|ControlMask))==(ShiftMask|ControlMask))
                menuhit(e, &egg);
	    else
	        button2(e);
	}
        return;
    default:
        return;
    case Button3:
        if (c && click_passes) {
            XMapRaised(dpy, c->parent);
            active(c);
	    XAllowEvents (dpy, ReplayPointer, curtime);
        }
	else
            button3(e);
        break;
    }
}


void 
button2(e)
XButtonEvent *e;
{
    int n;
    cmapfocus(0);
    if ((n = menuhit(e, &b2menu)) == -1)
      return;
    switch_to(n);
    if (current)
        cmapfocus(current);
}

void
initb2menu(n)
int n;
{
  b2items[n] = 0;
}

void 
button3(e)
XButtonEvent *e;
{
    int n, shift;
    Client *c;
    cmapfocus(0);
    switch (n = menuhit(e, &b3menu)) {
    case 0:     /* New */
        spawn();
        break;
    case 1:     /* Reshape */
        reshape(selectwin(1, 0));
        break;
    case 2:     /* Move */
        move(selectwin(0, 0));
        break;
    case 3:     /* Delete */
        shift = 0;
        c = selectwin(1, &shift);
        delete(c, shift);
        break;
    case 4:     /* Hide */
        hide(selectwin(1, 0));
        break;
    default:    /* unhide window */
        unhide(n - B3FIXED, 1);
        break;
    case -1:    /* nothing */
        break;
    }
    if (current)
        cmapfocus(current);
}




void
switch_to(n)
int n;
{
  if (n == virtual)
    return;
  currents[virtual] = current;
  virtual = n;
  switch_to_c(n,clients);
  current = currents[virtual];
}


void
switch_to_c(n,c)
int n;
Client * c;
{
  if (c && c->next)
    switch_to_c(n,c->next);

  if (c->parent == DefaultRootWindow(dpy))
    return;

  if (c->virtual != virtual && c->state == NormalState)
    {
      XUnmapWindow(dpy, c->parent);
      XUnmapWindow(dpy, c->window);
      setstate9(c, IconicState);
      if (c == current)
	nofocus();
    }
  else if (c->virtual == virtual &&  c->state == IconicState)
    {
      int i;

      for (i = 0; i < numhidden; i++)
	if (c == hiddenc[i]) 
	  break;

      if (i == numhidden)
	{
	  XMapWindow(dpy, c->window);
	  XMapWindow(dpy, c->parent);
	  setstate9(c, NormalState);
	  if (currents[virtual] == c)
	    active(c); 
	}
    }
}



void
spawn()
{
    /*
     * ugly dance to avoid leaving zombies.  Could use SIGCHLD,
     * but it's not very portable, and I'm in a hurry...
     */
    if (fork() == 0) {
        if (fork() == 0) {
            close(ConnectionNumber(dpy));
            if (termprog != NULL) {
                execl(shell, shell, "-c", termprog, 0);
                fprintf(stderr, "9wm: exec %s", shell);
                perror(" failed");
            }
            //execlp("xoot", "xoot", "-ut", 0);
            execlp("urxvt", "urxvt", 0);
            //execlp("aterm", "aterm", "-9wm", 0);
            //perror("9wm: exec aterm/xterm failed");
            exit(1);
        }
        exit(0);
    }
    wait((int *) 0);
}
void
launcher()
{
    /*
     * ugly dance to avoid leaving zombies.  Could use SIGCHLD,
     * but it's not very portable, and I'm in a hurry...
     * 9wm/w9wm/x9wm not have launcher/runner, gmrun is the launcher app :)
     */
    if (fork() == 0) {
        if (fork() == 0) {
            close(ConnectionNumber(dpy));
            execlp("gmrun", "gmrun", 0);
            perror("9wm: exec gmrun failed");
            exit(1);
        }
        exit(0);
    }
    wait((int *) 0);
}
// where the placement takes place
void
reshape(c)
Client *c;
{
    int odx, ody;

    if (c == 0)
        return;
    odx = c->dx;
    ody = c->dy;
    if (sweep(c) == 0)
        return;
    active(c);
    XRaiseWindow(dpy, c->parent);
    XMoveResizeWindow(dpy, c->parent, c->x-BORDER, c->y-BORDER,
                    c->dx+2*(BORDER-1), c->dy+2*(BORDER-1));
    if (c->dx == odx && c->dy == ody)
        sendconfig(c);
    else
        XMoveResizeWindow(dpy, c->window, BORDER-1, BORDER-1, c->dx, c->dy);
}

void
move(c)
Client *c;
{
    if (c == 0)
        return;
    if (drag(c) == 0)
        return;
    active(c);
    XRaiseWindow(dpy, c->parent);
    XMoveWindow(dpy, c->parent, c->x-BORDER, c->y-BORDER);
    sendconfig(c);
}

void
delete(c, shift)
Client *c;
int shift;
{
    if (c == 0)
        return;
    if ((c->proto & Pdelete) && !shift)
        sendcmessage(c->window, wm_protocols, wm_delete);
    else
        XKillClient(dpy, c->window);        /* let event clean up */
}

void
hide(c)
Client *c;
{
    if (c == 0 || numhidden == MAXHIDDEN)
        return;
    if (hidden(c)) {
        fprintf(stderr, "9wm: already hidden: %s\n", c->label);
        return;
    }
    XUnmapWindow(dpy, c->parent);
    XUnmapWindow(dpy, c->window);
    setstate9(c, IconicState);
    if (c == current)
        nofocus();
    hiddenc[numhidden] = c;
    b3items[B3FIXED+numhidden] = c->label;
    numhidden++;
    b3items[B3FIXED+numhidden] = 0;
}

void
unhide(n, map)
int n;
int map;
{
    Client *c;
    int i;

    if (n >= numhidden) {
        fprintf(stderr, "9wm: unhide: n %d numhidden %d\n", n, numhidden);
        return;
    }
    c = hiddenc[n];
    if (!hidden(c)) {
        fprintf(stderr, "9wm: unhide: not hidden: %s(0x%lx)\n",
            c->label, c->window);
        return;
    }

    if (map) {
        XMapWindow(dpy, c->window);
        XMapRaised(dpy, c->parent);
        setstate9(c, NormalState);
        active(c);
    }

    c->virtual = virtual;
    numhidden--;
    for (i = n; i < numhidden; i ++) {
        hiddenc[i] = hiddenc[i+1];
        b3items[B3FIXED+i] = b3items[B3FIXED+i+1];
    }
    b3items[B3FIXED+numhidden] = 0;
}

void
unhidec(c, map)
Client *c;
int map;
{
    int i;
    
    for (i = 0; i < numhidden; i++)
        if (c == hiddenc[i]) {
            unhide(i, map);
            return;
        }
    fprintf(stderr, "9wm: unhidec: not hidden: %s(0x%lx)\n",
        c->label, c->window);
}

void
renamec(c, name)
Client *c;
char *name;
{
    int i;

    if (name == 0)
        name = "???";
    c->label = name;
    if (!hidden(c))
        return;
    for (i = 0; i < numhidden; i++)
        if (c == hiddenc[i]) {
            b3items[B3FIXED+i] = name;
            return;
        }
}
