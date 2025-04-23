/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/shaker.c                           */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Sun Jul 17 17:15:49 2022                          */
/*    Last change :  Wed Apr 23 07:27:27 2025 (serrano)                */
/*    Copyright   :  2022-25 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    ICCCMPanel mouse shaker                                          */
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
/*    shaker_window ...                                                */
/*---------------------------------------------------------------------*/
static Window shaker_window = 0;

static Xinfo_t *shaker_xinfo = 0;
static Pixmap shaker_xpm, shaker_mask;
static int shaker_xpm_w, shaker_xpm_h;

static int shaker_x, shaker_y;
static int shaker_state = 0;

static void shaker_setup(int x, int y);
static void init_shaker_xpm(Xinfo_t *, Window, char *);

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    init_shaker ...                                                  */
/*---------------------------------------------------------------------*/
void
init_shaker(Xinfo_t *xinfo, char *xpm_path) {
#if ICCCMPANEL_MOUSE_SHAKER
   return ;
#else   
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
   att.event_mask = ButtonPressMask;
   
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

   shaker_window = win;
   shaker_xinfo = copy_xinfo(xinfo);
   shaker_xinfo->gcb = XCreateGC(disp, win, 0, &gcv);

   /* shaker xpm */
   init_shaker_xpm(shaker_xinfo, win, xpm_path);

   shaker_hide();

   /* shaker window events */
   XSelectInput(disp, shaker_window, ButtonPressMask);
#endif   
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    init_shaker_xpm ...                                              */
/*---------------------------------------------------------------------*/
static void
init_shaker_xpm(Xinfo_t *xinfo, Window win, char *xpm_path) {
   Pixmap pix;
   int x, y;
   unsigned int d, bw;
   XpmImage image;
   XpmInfo info;
   Cursor shaker;

   int res = XpmReadFileToPixmap(xinfo->disp, win, xpm_path,
				 &shaker_xpm, &shaker_mask,
				 NULL);
   XpmReadFileToXpmImage(xpm_path, &image, &info);

   /* the position of the image in the area */
   shaker_xpm_w = image.width;
   shaker_xpm_h = image.height;
   
   XMoveResizeWindow(xinfo->disp, shaker_window, x, y, shaker_xpm_w, shaker_xpm_h);

   if (shaker = XCreateFontCursor(xinfo->disp, XC_dot)) {
      XDefineCursor(xinfo->disp, win, shaker);
   }
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    shaker_setup ...                                                 */
/*---------------------------------------------------------------------*/
static void
shaker_setup(int x, int y) {
   Display *disp = shaker_xinfo->disp;

   XMoveResizeWindow(disp, shaker_window, x, y, shaker_xpm_w, shaker_xpm_h);

   if (shaker_state == 0) {
      shaker_state = 1;
      XMapWindow(disp, shaker_window);
      draw_pixmap(shaker_xinfo, shaker_window,
	       shaker_xpm, shaker_mask,
	       0, 0, shaker_xpm_w, shaker_xpm_h);
   }
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    hide_shaker ...                                                  */
/*---------------------------------------------------------------------*/
void
shaker_hide() {
   if (shaker_window && shaker_state) {
      shaker_state = 0;
      XUnmapWindow(shaker_xinfo->disp, shaker_window);
   }
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    show_shaker ...                                                  */
/*---------------------------------------------------------------------*/
void
show_shaker(long etime, taskbar_t *tbar) {
   static unsigned long long last_time = 0;
   static unsigned long cnt = 0;
   unsigned long long cur_time;
   struct timespec tv;
   struct config *config = tbar->config;

   if (clock_gettime(CLOCK_REALTIME_COARSE, &tv) == 0) {
      cur_time = ((unsigned long long)(tv.tv_sec * 1000) + (tv.tv_nsec / 1000000));

      // fprintf(stderr, "cur=%ld last=%ld diff=%ld cnt=%d\n", cur_time, last_time, cur_time - last_time, cnt);
      if ((cur_time - last_time) < config->mouse_shaker_speed) {
	 cnt++;
	 if (cnt > config->mouse_shaker_sensitivity) {
	    // fast move detection
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

	    shaker_setup(root_x, root_y);
	 }
      } else {
	 shaker_hide();
	 cnt = 0;
      }
      last_time = cur_time;
   } else {
      shaker_hide();
      cnt = 0;
   }
}
