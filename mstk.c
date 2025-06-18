/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/mstk.c                             */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Sat Oct 11 05:33:42 2003                          */
/*    Last change :  Wed Jun 18 08:42:28 2025 (serrano)                */
/*    Copyright   :  2003-25 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    Small X toolkit                                                  */
/*=====================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#ifdef HAVE_XPM
#  include <X11/xpm.h>
#endif

#include "configure.h"
#include "config.h"
#include "mstk.h"
#include "tooltips.h"
#include "taskbar.h"
#include "shaker.h"

/*---------------------------------------------------------------------*/
/*    X atoms                                                          */
/*---------------------------------------------------------------------*/
static char *atom_names[] = {
  /* clients */
   "KWM_WIN_ICON",
   "WM_STATE",
   "_MOTIF_WM_HINTS",
   "_NET_WM_STATE",
   "_NET_WM_STATE_SKIP_TASKBAR",
   "_NET_WM_STATE_SHADED",
   "_NET_WM_DESKTOP",
   "_NET_WM_WINDOW_TYPE",
   "_NET_WM_WINDOW_TYPE_DOCK",	/* 8 */
   "_NET_WM_WINDOW_TYPE_TOOLBAR", 
   "_NET_WM_STRUT",
   "_NET_WM_NAME",
   "_NET_WM_ICON",
   "_WIN_HINTS",
  /* root */
   "_NET_CLIENT_LIST",
   "_NET_NUMBER_OF_DESKTOPS",
   "_NET_CURRENT_DESKTOP",
   "_NET_WM_STATE_STICKY",
   /* misc */
   "CARDINAL",
   "UTF8_STRING",
};

#define ATOM_COUNT (sizeof(atom_names) / sizeof(atom_names[0]))
Atom atoms[ATOM_COUNT];

/*---------------------------------------------------------------------*/
/*    palette                                                          */
/*---------------------------------------------------------------------*/
color_t palette[] = {
   {0xd75c, 0xd75c, 0xd75c},	/* 0. light gray */
   {0xbefb, 0xbaea, 0xbefb},	/* 1. mid gray */
   {0xaefb, 0xaaea, 0xaefb},	/* 2. dark gray */
   {0xefbe, 0xefbe, 0xefbe},	/* 3. white */
   {0x8617, 0x8207, 0x8617},	/* 4. darkest gray */
   {0x0000, 0x0000, 0x0000},	/* 5. black */
   {0xffff, 0xffff, 0xffff},	/* 6  */
   {0xeeee, 0xeeee, 0xeeee},    /* 7  */
   {0xdddd, 0xdddd, 0xdddd},    /* 8  */
   {0xcccc, 0xcccc, 0xcccc},    /* 9  */
   {0xbbbb, 0xbbbb, 0xbbbb},    /* 10 */
   {0xaaaa, 0xaaaa, 0xaaaa},    /* 11 */
   {0x9999, 0x9999, 0x9999},    /* 12 */
   {0x8888, 0x8888, 0x8888},    /* 13 */
   {0x7777, 0x7777, 0x7777},    /* 14 */
   {0x6666, 0x6666, 0x6666},    /* 15 */
   {0x5555, 0x5555, 0x5555},    /* 16 */
   {0x4444, 0x4444, 0x4444},    /* 17 */
   {0x3333, 0x3333, 0x3333},    /* 18 */
   {0x2222, 0x2222, 0x2222},    /* 19 */
   {0x1111, 0x1111, 0x1111},    /* 20 */
   {0x0000, 0x0000, 0x0000},    /* 21 */
   {0xffff, 0x0000, 0x0000},    /* 22 */
   {0xffff, 0xffff, 0xcccc},    /* 23 */
   {0x0000, 0x0000, 0x0000},    /* 24 */
   {0x6666, 0x6666, 0x6666},    /* 25 */
   {0x0000, 0x0000, 0xffff},    /* 26 */
   {0x0000, 0xffff, 0x0000},    /* 27 */
   {0xffff, 0x9000, 0x0000},    /* 28 */
};

/*---------------------------------------------------------------------*/
/*    unsigned long                                                    */
/*    palette ...                                                      */
/*---------------------------------------------------------------------*/
unsigned long mstk_palette[MSTK_PALETTE_COUNT];
unsigned long mstk_grey_palette[256];

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    handle_error ...                                                 */
/*---------------------------------------------------------------------*/
static void
handle_error(Display *d, XErrorEvent *ev) {
   char buf[1024];
   
   XGetErrorText(d, ev->error_code, buf, sizeof(buf));

   fprintf(stderr, "Xlib error: %s\nicccmpanel exit...\n", buf);
   fprintf(stderr, "%s:%d forcing sigsev %d\n", __FILE__, __LINE__, 1/0);
   exit (1);
}

/*---------------------------------------------------------------------*/
/*    static int                                                       */
/*    init_X ...                                                       */
/*---------------------------------------------------------------------*/
static int
init_X(Xinfo_t *xinfo) {
   Display *disp;
   int screen;
   XGCValues gcbv;
   XGCValues gcpv;
    
   if (!(disp = XOpenDisplay(NULL)))
      return 1;

   xinfo->disp = disp;
   xinfo->screen = screen = DefaultScreen(disp);
   xinfo->screen_depth = DefaultDepth(disp, screen);
   xinfo->screen_height = DisplayHeight(disp, screen);
   xinfo->screen_width = DisplayWidth(disp, screen);
   xinfo->root_win = RootWindow(disp, screen);

   // collect property keys
   XInternAtoms(xinfo->disp, atom_names, ATOM_COUNT, False, atoms);

   /* helps us catch windows closing/opening */
   XSelectInput(disp, xinfo->root_win, PropertyChangeMask);

   XSetErrorHandler((XErrorHandler)handle_error);

   /* bold context */
   do {
      xinfo->xfsb = XLoadQueryFont(disp, xinfo->fontname_bold);
      xinfo->fontname_bold = "fixed";
   } while (!xinfo->xfsb);

   gcbv.graphics_exposures = False;
   xinfo->text_y = xinfo->xfsb->ascent + ((20 - xinfo->xfsb->ascent) / 2);
   gcbv.font = xinfo->xfsb->fid;
   xinfo->gcb = XCreateGC(disp,
			   xinfo->root_win,
			   GCFont | GCGraphicsExposures,
			   &gcbv);
   
   /* plain context */
   do {
      xinfo->xfsp = XLoadQueryFont(disp, xinfo->fontname_plain);
      xinfo->fontname_plain = "fixed";
   } while (!xinfo->xfsp);

   gcpv.graphics_exposures = False;
   gcpv.font = xinfo->xfsp->fid;
   xinfo->gcp = XCreateGC(disp,
			   xinfo->root_win,
			   GCFont | GCGraphicsExposures,
			   &gcpv);
   

   return 0;
}

/*---------------------------------------------------------------------*/
/*    Xinfo_t *                                                        */
/*    init_mstk ...                                                    */
/*---------------------------------------------------------------------*/
Xinfo_t *
init_mstk(config_t *config) {
   XColor xcl;
   unsigned int i;
   Xinfo_t *xinfo = (Xinfo_t *)calloc(1, sizeof(Xinfo_t));
   int greylen;
   int greystep;
   int g;

   if (config->gradient_grey_min > config->gradient_grey_max) {
      greylen = config->gradient_grey_min - config->gradient_grey_max;
      greystep = -1 * (greylen / config->taskbar_height);
   } else {
      greylen = config->gradient_grey_max - config->gradient_grey_min;
      greystep = greylen / config->taskbar_height;
   }
   
   config->gradient_grey_len = greylen;
   
   xinfo->fontname_plain = config->fontname_plain;
   xinfo->fontname_bold = config->fontname_bold;
   xinfo->tooltips_fontname = config->tooltips_fontname;

   /* initialize X connection */
   if (init_X(xinfo))
      return 0L;

   /* initialize colors */
   for (i = 0; i < MSTK_PALETTE_COUNT; i++) {
      xcl.red = palette[i].red;
      xcl.blue = palette[i].blue;
      xcl.green = palette[i].green;
      XAllocColor(xinfo->disp,
		   DefaultColormap(xinfo->disp, xinfo->screen),
		   &xcl);
      mstk_palette[i] = xcl.pixel;
   }

   /* initialize the grey colors */
   for (i = 0, g = config->gradient_grey_min;
	i < greylen;
	i++, g += greystep) {
      xcl.red = xcl.blue = xcl.green = g | (g<<8);
      XAllocColor(xinfo->disp,
		   DefaultColormap(xinfo->disp, xinfo->screen),
		   &xcl);
      mstk_grey_palette[i] = xcl.pixel;
   }

   /* initialize the tooltips */
   init_tooltips(xinfo);
   
   /* initialize the big shaker */
   if (config->mouse_shaker_speed > 0) {
      init_shaker(xinfo, config->shaker_path);
   }
   
   return xinfo;
}

/*---------------------------------------------------------------------*/
/*    Xinfo_t *                                                        */
/*    copy_xinfo ...                                                   */
/*---------------------------------------------------------------------*/
Xinfo_t *
copy_xinfo(Xinfo_t *old) {
   Xinfo_t *new = (Xinfo_t *)calloc(1, sizeof(Xinfo_t));

   *new = *old;

   return new;
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    closeup_mstk ...                                                 */
/*---------------------------------------------------------------------*/
void
closeup_mstk(Xinfo_t *xinfo) {
   XCloseDisplay(xinfo->disp);
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    scale_icon ...                                                   */
/*---------------------------------------------------------------------*/
void
scale_icon(Xinfo_t *xinfo, Window win, 
	   Pixmap icon, Pixmap mask,
	   Pixmap *ricon, Pixmap *rmask,
	   int nw, int nh) {
   Display *disp = (xinfo)->disp;
   GC gc = (xinfo)->gcb;
   int depth = xinfo->screen_depth;
   int xx, yy, x, y;
   unsigned int w, h, d, bw;
   Pixmap pix, mk = None;
   XGCValues gcv;
   GC mgc;

   if (!XGetGeometry(disp, icon, &pix, &x, &y, &w, &h, &bw, &d)) {
      char *name = window_name(disp, win);
      fprintf(stderr, "*** ERROR(%s:%d): cannot get geometry win=%p name=%s\n", 
	      __FILE__, __LINE__, win, name);
      free(name);
      return;
   }

#if DEBUG      
   fprintf(stderr, "%s:%d scale_icon x=%d y=%d w=%d h=%d\n", x, y, w, h);
#endif
      
   pix = XCreatePixmap(disp, win, nw, nh, depth);

   if (mask != None) {
      mk = XCreatePixmap(disp, win, nw, nw, 1);
      gcv.subwindow_mode = IncludeInferiors;
      gcv.graphics_exposures = False;
      mgc = XCreateGC(disp, mk, GCGraphicsExposures | GCSubwindowMode, &gcv);
   }

   /* this is my simple & dirty scaling routine */
   for (y = nh - 1; y >= 0; y--) {
      yy = (y * h) / nh;
      for (x = nw - 1; x >= 0; x--) {
	 xx = (x * w) / nw;
	 if (d != depth)
	    XCopyPlane(disp, icon, pix, gc, xx, yy, 1, 1, x, y, 1);
	 else
	    XCopyArea(disp, icon, pix, gc, xx, yy, 1, 1, x, y);
	 if (mk != None)
	    XCopyArea(disp, mask, mk, mgc, xx, yy, 1, 1, x, y);
      }
   }

   if (mk != None) {
      XFreeGC(disp, mgc);
      *rmask = mk;
   }

   *ricon = pix;
}

/*---------------------------------------------------------------------*/
/*    static unsigned long *                                           */
/*    pixmap_to_rgba ...                                               */
/*---------------------------------------------------------------------*/
static unsigned long *pixmap_to_rgba(Display *disp, Pixmap icon, Pixmap mask, int *rlen) {
   XImage *icon_img;
   XImage *mask_img = NULL;
   Window root;
   int x, y, i;
   unsigned int width, height, border_width, depth;
   
   if (!XGetGeometry(disp, icon, &root, &x, &y,
		     &width, &height, &border_width, &depth)) {
      fprintf(stderr, "*** ERROR(%s:%d): cannot get geometry\n", 
	      __FILE__, __LINE__);
      return 0;
   }

   icon_img =
      XGetImage(disp, icon, 0, 0, width, height, 0xFFFFFFFF, ZPixmap);

   if (mask) {
      mask_img = 
	 XGetImage(disp, mask, 0, 0, width, height, 0xFFFFFFFF, ZPixmap);
   }

   *rlen = (2 + width * height);
   unsigned long *data = malloc(*rlen * sizeof(long));

   if (!data) {
      fprintf(stderr, "*** ERROR(%s:%d): cannot allocate %ld width=%d height=%d\n",
	      __FILE__, __LINE__, *rlen * sizeof(long), width, height);
      return 0;
   }
   
   data[0] = width;
   data[1] = height;

   for (i = 2, y = 0; y < height; y++) {
      for (x = 0; x < width; x++, i++) {
	 data[i] = XGetPixel(icon_img, x, y);
	 if (mask_img) {
	    if (XGetPixel(mask_img, x, y)) {
	       data[i] += ((unsigned long)255 << 24);
	    }
	 }
       }
   }

   if (icon_img) XDestroyImage(icon_img);
   if (mask_img) XDestroyImage(mask_img);
   
   return data;
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    rgba_to_pixmap ...                                               */
/*---------------------------------------------------------------------*/
static void
rgba_to_pixmap(Xinfo_t *xinfo, Window win, unsigned long *buf, int width, int height, Pixmap *ricon, Pixmap *rmask) {
   Display *disp = (xinfo)->disp;
   GC gc = (xinfo)->gcb;
   Pixmap pix, mk;
   int depth = xinfo->screen_depth;
   int bitmap_pad = 32; // 32 for 24 and 32 bpp, 16, for 15&16
   int bytes_per_line = 0; // number of bytes in the client image
                           // between the start of one scanline and the
                           // start of the next
   

   XImage *img = XCreateImage(disp, DefaultVisual(disp, 0), depth,
			       ZPixmap, 0, 0L, width, height,
			       bitmap_pad, bytes_per_line);

   XImage *mask = XCreateImage(disp, DefaultVisual(disp, 0), 1,
				XYBitmap, 0, 0L, width, height,
				bitmap_pad, bytes_per_line);

   unsigned char r, g, b, a;
   long x, y;
   unsigned int rgba;
   unsigned long pixel;

   img->data = malloc(img->bytes_per_line * height);
   mask->data = malloc(mask->bytes_per_line * height);

   for (y = 0; y < height; y++) {
       for (x = 0; x < width; x++, buf++) {

	  rgba = *buf; // use only 32bit

	  a = (rgba & 0xff000000) >> 24;
	  r = (rgba & 0x00ff0000) >> 16;
	  g = (rgba & 0x0000ff00) >> 8;
	  b = (rgba & 0x000000ff);

	  if (img->red_mask == 0x7c00
	      && img->green_mask == 0x03e0
	      && img->blue_mask == 0x1f) {
	     // 15 bit display, 5R 5G 5B
	     pixel = ((r << 7) & 0x7c00)
		| ((g << 2) & 0x03e0)
		| ((b >> 3) & 0x001f);
            } else if (img->red_mask == 0xf800
                       && img->green_mask == 0x07e0
                       && img->blue_mask == 0x1f) {
	     // 16 bit display, 5R 6G 5B
	     pixel = ((r << 8) & 0xf800)
		| ((g << 3) & 0x07e0)
		| ((b >> 3) & 0x001f);
            } else if (img->red_mask == 0xff0000
                       && img->green_mask == 0xff00
                       && img->blue_mask == 0xff) {
	     // 24/32 bit display, 8R 8G 8B
	     pixel = rgba & 0x00ffffff;
	  } else {
	     pixel = 0;
	  }

	  // transfer rgb data
	  XPutPixel(img, x, y, pixel);

	  // transfer mask data
	  XPutPixel(mask, x, y, a > 127 ? 0 : 1);
       }
   }
   
   pix = XCreatePixmap(disp, win, width, height, depth);
   mk = XCreatePixmap(disp, win, width, height, 1);

   GC gcimg = XCreateGC(disp, pix, 0, 0);
   GC gcmask = XCreateGC(disp, mk, 0, 0);
   
   XPutImage(disp, pix, gcimg, img, 0, 0, 0, 0, width, height);
   XPutImage(disp, mk, gcmask, mask, 0, 0, 0, 0, width, height);
   
   XDestroyImage(img);
   XDestroyImage(mask);

   XFreeGC(disp, gcimg);
   XFreeGC(disp, gcmask);
   
   *ricon = pix;
   *rmask = mk;
}

/*---------------------------------------------------------------------*/
/*    static int                                                       */
/*    client_msg ...                                                   */
/*---------------------------------------------------------------------*/
static int
client_msg(Display * disp, Window win, char *msg,
	    unsigned long data0, unsigned long data1,
	    unsigned long data2, unsigned long data3, unsigned long data4) {
   XEvent event;
   long mask = SubstructureRedirectMask | SubstructureNotifyMask;

   event.xclient.type = ClientMessage;
   event.xclient.serial = 0;
   event.xclient.send_event = True;
   event.xclient.message_type = XInternAtom(disp, msg, False);
   event.xclient.window = win;
   event.xclient.format = 32;
   event.xclient.data.l[0] = data0;
   event.xclient.data.l[1] = data1;
   event.xclient.data.l[2] = data2;
   event.xclient.data.l[3] = data3;
   event.xclient.data.l[4] = data4;

   if (XSendEvent(disp, DefaultRootWindow(disp), False, mask, &event)) {
      return 0;
   } else {
      fprintf(stderr, "Cannot send %s event.\n", msg);
      return 1;
   }
}

/*---------------------------------------------------------------------*/
/*    void *                                                           */
/*    get_window_prop_data ...                                         */
/*---------------------------------------------------------------------*/
void *
get_window_prop_data(Display *disp, Window win,
		      Atom prop, Atom type, long *items) {
   XWindowAttributes attr;
   
   // check first that the window exists
   if (XGetWindowAttributes(disp, win, &attr)) {
      Atom type_ret;
      int format_ret;
      unsigned long items_ret;
      unsigned long after_ret;
      unsigned char *prop_data = 0;
      int status;

      status = XGetWindowProperty(disp, win, prop, 0, 0x7fffffff, False,
				  type, &type_ret, &format_ret, &items_ret,
				  &after_ret, &prop_data);
   
      if (items) *items = items_ret;

      return prop_data;
   } else {
      return 0L;
   }
}

/*---------------------------------------------------------------------*/
/*    static int                                                       */
/*    get_window_prop_int ...                                          */
/*---------------------------------------------------------------------*/
static int
get_window_prop_int(Display *disp, Window win, Atom at) {
   int num = 0;
   unsigned long *data;

   data = get_window_prop_data(disp, win, at, XA_CARDINAL, 0);
   if (data) {
      num = *data;
      XFree(data);
   }
   return num;
}

/*---------------------------------------------------------------------*/
/*    char *                                                           */
/*    window_name ...                                                  */
/*---------------------------------------------------------------------*/
char *
window_name(Display *disp, Window win) {
   char *data = get_window_prop_data(disp, win, atom__NET_WM_NAME, atom__UTF8_STRING, 0);

#if DEBUG > 2
   fprintf(stderr, "%s:%d window_name w=%p\n", __FILE__, __LINE__, win);
#endif
   
   if (data) {
      char *res = strdup(data);
      XFree(data);
      return res;
   }

   data = get_window_prop_data(disp, win, XA_WM_NAME, XA_STRING, 0);

   if (data) {
      char *res = strdup(data);
      XFree(data);
      return res;
   }

   return 0;
}

/*---------------------------------------------------------------------*/
/*    char *                                                           */
/*    window_class ...                                                 */
/*---------------------------------------------------------------------*/
char *
window_class(Display *disp, Window win) {
   long num;
   XClassHint ch;

#if DEBUG > 2
   fprintf(stderr, "%s:%d window_class w=%p\n", __FILE__, __LINE__, win);
#endif
   
   if (XGetClassHint(disp, win, &ch) == 0) {
      char *data = get_window_prop_data(disp, win, XA_WM_CLASS, XA_STRING, &num);

      if (data) {
	 char *res = strdup(data);
	 XFree(data);
	 return res;
      } else {
	 return 0L;
      }
   } else {
      char *res = 0;
      if (ch.res_class != 0) {
	 res = strdup(ch.res_class);
	 XFree(ch.res_class);
      }

      if (ch.res_name != 0) {
	 XFree(ch.res_name);
      }

      return res;
   }
}

/*---------------------------------------------------------------------*/
/*    char                                                             */
/*    window_hint_icon ...                                             */
/*---------------------------------------------------------------------*/
char
window_hint_icon(Xinfo_t *xinfo, Window win, Pixmap *icon, Pixmap *mask, int icon_size) {
   XWMHints *hin;
   Display *disp = xinfo->disp;

   hin = (XWMHints *)get_window_prop_data(disp, win, XA_WM_HINTS, XA_WM_HINTS, 0);

   if (hin) {
      int res = 0;
      
      if (hin->flags & IconPixmapHint) {
	 if (hin->flags & IconMaskHint) {
	    *mask = hin->icon_mask;
	 }

	 *icon = hin->icon_pixmap;
	 res = 1;
      }
      XFree(hin);

#if DEBUG
      fprintf(stderr, "%s:%d window_hint_icon icon=%p mask=%p\n",
	      __FILE__, __LINE__, icon, mask);
      char *name = window_name(disp, win);
      fprintf(stderr, "%s:%d window_hint_icon name=%s\n",
	      __FILE__, __LINE__, name);
      free(name);
#endif
      
      scale_icon(xinfo, win, *icon, *mask, icon, mask, icon_size, icon_size);
      
      return res;
   } else {
      return 0;
   }
}

/*---------------------------------------------------------------------*/
/*    char                                                             */
/*    window_netwm_icon ...                                            */
/*---------------------------------------------------------------------*/
char
window_netwm_icon(Xinfo_t *xinfo, Window win, Pixmap *icon, Pixmap *mask, int icon_size) {
   unsigned long *data;
   long len;
   Display *disp = xinfo->disp;
   
   if (data = (long *)get_window_prop_data(disp, win, atom__NET_WM_ICON, AnyPropertyType, &len)) {
      long i = 0;
      long m = -1;
      long d = LONG_MAX;

      /* traversal to find the best match */
      while (i < len) {
	 long w = data[i];
	 long h = data[i + 1];
	 long di = w - icon_size;

	 if (di == 0) {
	    m = i;
	    break;
	 } else if (di < d) {
	    d = di;
	    m = i;
	 }
	 
	 i += 2 + (w * h);
      }

      /* get the icon */
      if (m >= 0) {
	 long w = data[m];
	 long h = data[m + 1];
	 unsigned long *buf = &(data[m + 2]);
	    
	 rgba_to_pixmap(xinfo, win, buf, w, h, icon, mask);

	 scale_icon(xinfo, win, *icon, *mask, icon, mask,
		     icon_size, icon_size);
	 XFree(data);
	 
	 return 1;
      } else {
	 XFree(data);
	 return 0;
      }
   }
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    set_window_prop ...                                              */
/*---------------------------------------------------------------------*/
void
set_window_prop(Display *disp,  Window win, Atom at, Atom type, long val) {
   XChangeProperty(disp, win, at, type, 32,
		    PropModeReplace, (unsigned char *)&val, 1);
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    window_update_netwm_icon ...                                     */
/*---------------------------------------------------------------------*/
void
window_update_netwm_icon(Xinfo_t *xinfo, Window win, char *name, Pixmap *icon, Pixmap *mask, int icon_size) {
   int length;
   unsigned long *buffer = pixmap_to_rgba(xinfo->disp, *icon, *mask, &length);

   if (buffer) {
      if (!XChangeProperty(xinfo->disp, win, atom__NET_WM_ICON, atom__CARDINAL,
		       32, PropModeReplace,
			   (unsigned char *)buffer, length)) {
	 fprintf(stderr, "*** ERROR(%s:%d): XChangeProperty failed %s\n",
		 __FILE__, __LINE__, name);
      }
      free(buffer);
   } else {
      fprintf(stderr, "*** ERROR(%s:%d): cannot update icon %s\n",
	      __FILE__, __LINE__, name);
   }
}

/*---------------------------------------------------------------------*/
/*    int                                                              */
/*    window_desktop ...                                               */
/*---------------------------------------------------------------------*/
int
window_desktop(Display *disp, Window win) {
   return get_window_prop_int(disp, win, atom__NET_WM_DESKTOP);
}

/*---------------------------------------------------------------------*/
/*    int                                                              */
/*    current_desktop ...                                              */
/*---------------------------------------------------------------------*/
int
current_desktop(Display *disp, Window win) {
   return get_window_prop_int(disp, win, atom__NET_CURRENT_DESKTOP);
}

/*---------------------------------------------------------------------*/
/*    int                                                              */
/*    number_of_desktops ...                                           */
/*---------------------------------------------------------------------*/
int
number_of_desktops(Display *disp, Window win) {
   return get_window_prop_int(disp, win, atom__NET_NUMBER_OF_DESKTOPS);
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    switch_desktop ...                                               */
/*---------------------------------------------------------------------*/
void
switch_desktop(Display *disp, Window win, int desk) {
   XClientMessageEvent xev;

   xev.type = ClientMessage;
   xev.window = win;
   xev.message_type = atom__NET_CURRENT_DESKTOP;
   xev.format = 32;
   xev.data.l[0] = desk;
   XSendEvent(disp, win, False, SubstructureNotifyMask | SubstructureRedirectMask, (XEvent *)&xev);
}

/*---------------------------------------------------------------------*/
/*    int                                                              */
/*    window_iconifiedp ...                                            */
/*---------------------------------------------------------------------*/
int
window_iconifiedp(Display *disp, Window win) {
   unsigned long *data;
   int ret = 0;
   
   data = get_window_prop_data(disp, win, atom_WM_STATE, atom_WM_STATE, 0);

   if (data) {
      if (data[0] == IconicState) 
	 ret = 1;
      XFree(data);
   }
   
   return ret;
}

/*---------------------------------------------------------------------*/
/*    int                                                              */
/*    window_hiddenp ...                                               */
/*---------------------------------------------------------------------*/
int
window_hiddenp(Display *disp, Window win) {
   unsigned long *data;
   int ret = 0;
   long num;

   data = get_window_prop_data(disp, win, atom__NET_WM_STATE, XA_ATOM, &num);
   
   if (data) {
      while (num) {
	 num--;
	 if ((data[num]) == atom__NET_WM_STATE_SKIP_TASKBAR)
	    ret = 1;
      }
      XFree(data);
   }

   return ret;
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    window_deiconify ...                                             */
/*---------------------------------------------------------------------*/
void
window_deiconify(Display *disp, Window win) {
   client_msg(disp, win, "_NET_ACTIVE_WINDOW", 0, 0, 0, 0, 0);
   XMapRaised(disp, win);
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    draw_text ...                                                    */
/*---------------------------------------------------------------------*/
void
draw_text(Xinfo_t *xinfo, Window win, int x, int y,
	   char *text, int len,
	   unsigned long palette[], int c, int shadow, int shadow_size) {
   Display *disp = (xinfo)->disp;
   GC gc = (xinfo)->gcb;
   unsigned long *color = palette ? palette : mstk_palette;

   if (shadow >= 0) {
      XSetForeground(disp, gc, color[shadow]);
      XDrawString(disp, win, gc, x + 1, y + 1, text, len);
      if (shadow_size >= 2) {
	 XDrawString(disp, win, gc, x + 2, y + 2, text, len);
	 if (shadow_size >= 3) {
	    int i;
	    for (i = 3; i < shadow_size; i++) {
	       XDrawString(disp, win, gc, x + i, y + i, text, len);
	    }
	 }
      }
   }
   
   XSetForeground(disp, gc, color[c]);
   XDrawString(disp, win, gc, x, y, text, len);
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    draw_text_plain ...                                              */
/*---------------------------------------------------------------------*/
void
draw_text_plain(Xinfo_t *xinfo, Window win, int x, int y,
		 char *text, int len,
		 unsigned long palette[], int c, int shadow, int shadow_size) {
   Display *disp = (xinfo)->disp;
   GC gc = (xinfo)->gcp;
   unsigned long *color = palette ? palette : mstk_palette;

   if (shadow >= 0) {
      XSetForeground(disp, gc, color[shadow]);
      XDrawString(disp, win, gc, x + 1, y + 1, text, len);
      if (shadow_size >= 2) {
	 XDrawString(disp, win, gc, x + 2, y + 2, text, len);
	 if (shadow_size >= 3) {
	    int i;
	    for (i = 3; i < shadow_size; i++) {
	       XDrawString(disp, win, gc, x + i, y + i, text, len);
	    }
	 }
      }
   }
   
   XSetForeground(disp, gc, color[c]);
   XDrawString(disp, win, gc, x, y, text, len);
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    draw_line ...                                                    */
/*---------------------------------------------------------------------*/
void
draw_line(Xinfo_t *xinfo, Window win,
	   int x, int y, int w , int h,
	   unsigned long palette[], int c) {
   Display *disp = (xinfo)->disp;
   GC gc = (xinfo)->gcb;
   unsigned long *color = palette ? palette : mstk_palette;
   XSetForeground(disp, gc, color[c]);
   XDrawLine(disp, win, gc, x, y, x + w, y + h);
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    draw_gradient ...                                                */
/*    -------------------------------------------------------------    */
/*    Fill a window with a gradient coloring starting from COLOR       */
/*    CF to CL.                                                        */
/*---------------------------------------------------------------------*/
void
draw_gradient(Xinfo_t *xinfo, Window win,
	       int x, int y, int w, int h,
	       unsigned long palette[], int cf, int step, int sw) {
   Display *disp = (xinfo)->disp;
   GC gc = (xinfo)->gcb;
   int i, c, j;
   unsigned long *color = palette ? palette : mstk_palette;

   if (step || sw) {
      for (i = 0, c = cf, j = sw; i < h; i++) {
	 XSetForeground(disp, gc, color[c]);
	 XDrawLine(disp, win, gc, x, y + i, x + w, y + i);

	 if (--j == 0) {
	    c += step;
	    j = sw;
	 }
      }
   } else {
      for (i = 0; i < h; i++) {
	 XSetForeground(disp, gc, color[cf]);
	 XDrawLine(disp, win, gc, x, y + i, x + w, y + i);
      }
   }
}


/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    draw_relief ...                                                  */
/*---------------------------------------------------------------------*/
void
draw_relief(Xinfo_t *xinfo, Window win, 
	     int x, int y, int w, int h,
	     unsigned long palette[], int c0, int c1,
	     int border) {
   Display *disp = (xinfo)->disp;
   GC gc = (xinfo)->gcb;
   unsigned long *color = palette ? palette : mstk_palette;
   int i;
   
   for (i = 0; i < border; i++) {
      XSetForeground(disp, gc, color[c0]);
      XDrawLine(disp, win, gc, x + i, y + i, x + i, y + h - i);
      XDrawLine(disp, win, gc, x + i, y + i, x + w - i, y + i);
      XSetForeground(disp, gc, color[c1]);
      XDrawLine(disp, win, gc, x + w - i , y + h - i, x + w - i, y + i);
      XDrawLine(disp, win, gc, x + i + 1, y + h - i, x + w - i, y + h - i);
   }
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    draw_partial_relief ...                                          */
/*---------------------------------------------------------------------*/
void
draw_partial_relief(Xinfo_t *xinfo, Window win, int mask,
		     int x, int y, int w, int h,
		     unsigned long palette[], int c0, int c1,
		     int border) {
   Display *disp = (xinfo)->disp;
   GC gc = (xinfo)->gcb;
   unsigned long *color = palette ? palette : mstk_palette;
   int i;
   
   for (i = 0; i < border; i++) {
      XSetForeground(disp, gc, color[c0]);
      if (mask & RELIEF_LEFT)
	 XDrawLine(disp, win, gc, x + i, y + i, x + i, y + h - i);
      if (mask & RELIEF_TOP)
	 XDrawLine(disp, win, gc, x + i, y + i, x + w - i, y + i);
      XSetForeground(disp, gc, color[c1]);
      if (mask & RELIEF_RIGHT)
	 XDrawLine(disp, win, gc, x + w - i , y + h - i, x + w - i, y + i);
      if (mask & RELIEF_BOTTOM)
	 XDrawLine(disp, win, gc,
		    x + i + (mask & RELIEF_LEFT) ? 1 : 0, y + h - i,
		    x + w - i, y + h - i);

   }
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    draw_pixmap ...                                                  */
/*---------------------------------------------------------------------*/
void
draw_pixmap(Xinfo_t *xinfo, Window win,
	     Pixmap icon, Pixmap mask,
	     int x, int y, int w, int h) {
   Display *disp = (xinfo)->disp;
   GC gc = (xinfo)->gcb;
   
#if DEBUG > 2
   fprintf(stderr, "%s:%d draw_pixmap win=%p px=%p\n",
	   __FILE__, __LINE__,
	   win, icon);
#endif
   
   XSetClipMask(disp, gc, mask);
   XSetClipOrigin(disp, gc, x, y);
   XCopyArea(disp, icon, win, gc, 0, 0, w, h, x, y);
   XSetClipMask(disp, gc, None);
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    draw_dot ...                                                     */
/*---------------------------------------------------------------------*/
static void
draw_dot(Display *disp, Window win, GC gc, int x, int y) {
   XSetForeground(disp, gc, mstk_grey_palette[0xf7]);
   XDrawPoint(disp, win, gc, x, y);
   XSetForeground(disp, gc, mstk_grey_palette[0x40]);
   XDrawPoint(disp, win, gc, x + 1, y + 1);
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    draw_grill ...                                                   */
/*---------------------------------------------------------------------*/
void
draw_grill(Xinfo_t *xinfo, Window win, int x, int y, int w, int h) {
   Display *disp = (xinfo)->disp;
   GC gc = (xinfo)->gcb;
   int xx;

   for (xx = 2; xx < w; xx += 3) {
      int yy = y + 1;

      while (yy <= h) {
	 draw_dot(disp, win, gc, x + xx, yy);
	 yy += 3;
      }
   }
}


