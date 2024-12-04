/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/cursor.c                           */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Sun Jul 17 17:15:49 2022                          */
/*    Last change :  Wed Dec  4 09:55:33 2024 (serrano)                */
/*    Copyright   :  2022-24 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    ICCCMPanel big cursor (on mouse motion)                          */
/*=====================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <values.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>

#ifdef HAVE_XPM
#  include <X11/xpm.h>
#endif

#include "icccmpanel.h"

/*---------------------------------------------------------------------*/
/*    static Window                                                    */
/*    cursor_window ...                                                */
/*---------------------------------------------------------------------*/
static Window cursor_window = 0;

static Xinfo_t *cursor_xinfo = 0;
static Pixmap cursor_xpm, cursor_mask;
static int cursor_xpm_w, cursor_xpm_h;

static int cursor_x, cursor_y;
static int cursor_state = 0;

static void cursor_setup(int x, int y);
static void init_cursor_xpm(Xinfo_t *, Window, char *);

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    init_cursor ...                                                  */
/*---------------------------------------------------------------------*/
void
init_cursor(Xinfo_t *xinfo, char *xpm_path) {
   Display *disp = xinfo->disp;
   Window win;
   XSetWindowAttributes att;
   MWMHints mwm;
   XSizeHints size_hints;
   XWMHints wmhints;
   unsigned long strut[4];
   XGCValues gcv;
   
   att.override_redirect = 1;
   att.background_pixel = 0xffffffff;
   att.cursor = None;
   att.background_pixel = 0;

   win = XCreateWindow(/* display */ disp,
		       /* parent  */ xinfo->root_win,
		       /* x       */ 10,
		       /* y       */ 10,
		       /* width   */ 64,
		       /* height  */ 64,
		       /* border  */ 0,
		       /* depth   */ CopyFromParent,
		       /* class   */ InputOutput,
		       /* visual  */ CopyFromParent,
		       /* vmask   */ CWOverrideRedirect | CWCursor,
		       /* attribs */ &att);

   if (!win) return;
   
   /* reserve "WINHEIGHT" pixels at the bottom of the screen */
   strut[ 0 ] = 0;
   strut[ 1 ] = 0;
   strut[ 2 ] = 0;
   strut[ 3 ] = 64;
   XChangeProperty(disp, win, atom__NET_WM_STRUT, XA_CARDINAL, 32,
		   PropModeReplace, (unsigned char *) strut, 4);

   /* shadowless window */
   set_window_prop(disp, win, atom__NET_WM_WINDOW_TYPE, XA_ATOM,
		    atom__NET_WM_WINDOW_TYPE_DOCK);

   /* use old gnome hint since sawfish doesn't support _NET_WM_STRUT */
   set_window_prop(disp, win, atom__WIN_HINTS, XA_CARDINAL,
		    WIN_HINTS_SKIP_FOCUS | WIN_HINTS_SKIP_WINLIST |
		    WIN_HINTS_SKIP_TASKBAR | WIN_HINTS_DO_NOT_COVER);

   /* borderless motif hint */
   bzero(&mwm, sizeof(mwm));
   mwm.flags = MWM_HINTS_DECORATIONS;
   XChangeProperty(disp, win,
		    atom__MOTIF_WM_HINTS, atom__MOTIF_WM_HINTS, 32,
		    PropModeReplace,
		    (unsigned char *) &mwm, sizeof(MWMHints) / 4);

   /* make sure the WM obays our window position */
   size_hints.flags = PPosition;

   /* XSetWMNormalHints (disp, win, &size_hints); */
   XChangeProperty(disp, win,
		    XA_WM_NORMAL_HINTS, XA_WM_SIZE_HINTS, 32,
		    PropModeReplace,
		    (unsigned char *) &size_hints, sizeof(XSizeHints) / 4);

   /* make our window unfocusable */
   wmhints.flags = InputHint;
   wmhints.input = False;

   /* XSetWMHints (disp, win, &wmhints); */
   XChangeProperty(disp, win,
		    XA_WM_HINTS, XA_WM_HINTS, 32, PropModeReplace,
		    (unsigned char *) &wmhints, sizeof(XWMHints) / 4);

   cursor_window = win;
   cursor_xinfo = copy_xinfo(xinfo);
   cursor_xinfo->gcb = XCreateGC(disp, win, 0, &gcv);

   /* cursor xpm */
   init_cursor_xpm(cursor_xinfo, win, xpm_path);

   cursor_hide();
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    init_cursor_xpm ...                                              */
/*---------------------------------------------------------------------*/
static void
init_cursor_xpm(Xinfo_t *xinfo, Window win, char *xpm_path) {
   Pixmap pix;
   int x, y;
   unsigned int d, bw;
   XpmImage image;
   XpmInfo info;
   Cursor cursor;

   int res = XpmReadFileToPixmap(xinfo->disp, win, xpm_path,
				 &cursor_xpm, &cursor_mask,
				 NULL);
   XpmReadFileToXpmImage(xpm_path, &image, &info);

   /* the position of the image in the area */
   cursor_xpm_w = image.width;
   cursor_xpm_h = image.height;
   
   XMoveResizeWindow(xinfo->disp, cursor_window, x, y, cursor_xpm_w, cursor_xpm_h);

   if (cursor = XCreateFontCursor(xinfo->disp, XC_dot)) {
      XDefineCursor(xinfo->disp, win, cursor);
   }
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    cursor_setup ...                                                 */
/*---------------------------------------------------------------------*/
static void
cursor_setup(int x, int y) {
   Display *disp = cursor_xinfo->disp;

   XMoveResizeWindow(disp, cursor_window, x, y, cursor_xpm_w, cursor_xpm_h);

   if (cursor_state == 0) {
      cursor_state = 1;
      XMapWindow(disp, cursor_window);
      draw_pixmap(cursor_xinfo, cursor_window,
		  cursor_xpm, cursor_mask, 0, 0, cursor_xpm_w, cursor_xpm_h);
   }
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    hide_cursor ...                                                  */
/*---------------------------------------------------------------------*/
void
cursor_hide() {
   if (cursor_window && cursor_state) {
      cursor_state = 0;
      XUnmapWindow(cursor_xinfo->disp, cursor_window);
   }
}

/*---------------------------------------------------------------------*/
/*    Pointer sensitivity                                              */
/*---------------------------------------------------------------------*/
#define CURSOR_TIME_LAPSE 20000
#define CURSOR_SENSITIVITY 80

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    show_cursor ...                                                  */
/*---------------------------------------------------------------------*/
void
show_cursor(long etime, taskbar_t *tbar) {
   static unsigned long long last_time = 0;
   static unsigned long cnt = 0;
   unsigned long cur_time;
   struct timeval tv;

   if (gettimeofday(&tv, 0) == 0) {
      cur_time = ((unsigned long long)(tv.tv_sec * 1000000) + (long long)tv.tv_usec);

      //fprintf(stderr, "cur=%ld last=%ld diff=%ld cnt=%d\n", cur_time, last_time, cur_time - last_time, cnt);
      if ((cur_time - last_time) < CURSOR_TIME_LAPSE) {
	 // fast move detection
	 cnt++;
	 if (cnt > CURSOR_SENSITIVITY) {
	    Display *disp = tbar->xinfo->disp;
	    Window root_window = tbar->xinfo->root_win;
	    Window root, child;
	    int root_x, root_y;
	    int win_x, win_y;
	    unsigned int mask;
	    XQueryPointer(disp, root_window, &root, &child,
			  &root_x, &root_y,
			  &win_x, &win_y,
			  &mask);
	    cursor_setup(root_x, root_y);
	 }
      } else {
	 cursor_hide();
	 cnt = 0;
      }
      last_time = cur_time;
   } else {
      cursor_hide();
      cnt = 0;
   }
}
