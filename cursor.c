/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/cursor.c                           */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Sun Jul 17 17:15:49 2022                          */
/*    Last change :  Sun Jul 17 20:29:02 2022 (serrano)                */
/*    Copyright   :  2022 Manuel Serrano                               */
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

#ifdef HAVE_XPM
#  include <X11/xpm.h>
#endif

#include "icccmpanel.h"

/*---------------------------------------------------------------------*/
/*    static Window                                                    */
/*    cursor_window ...                                                */
/*---------------------------------------------------------------------*/
Window cursor_window = 0;
static Xinfo_t *cursor_xinfo = 0;
static int cursor_text_y = 0;
static int cursor_height = 0;
static char *cursor_text = 0;
static int cursor_x, cursor_y;
static int cursor_color;

static int cursor_state = 0;

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    init_cursor ...                                                  */
/*---------------------------------------------------------------------*/
void
init_cursor(Xinfo_t *xinfo) {
   Display *disp = xinfo->disp;
   Window win;
   XSetWindowAttributes att;
   MWMHints mwm;
   XSizeHints size_hints;
   XWMHints wmhints;
   unsigned long strut[ 4 ];
   XGCValues gcv;
   GC gc;
    
   att.background_pixel = mstk_palette[TOOLTIPS];
   att.event_mask = ExposureMask;
   
   win = XCreateWindow(/* display */ disp,
			/* parent  */ xinfo->root_win,
			/* x       */ 10,
			/* y       */ 10,
			/* width   */ 100,
			/* height  */ 100,
			/* border  */ 0,
			/* depth   */ CopyFromParent,
			/* class   */ InputOutput,
			/* visual  */ CopyFromParent,
			/* vmask   */ CWBackPixel | CWEventMask,
			/* attribs */ &att);

   if (!win)
      return;
   
   gcv.graphics_exposures = False;
   
   /* reserve "WINHEIGHT" pixels at the bottom of the screen */
   strut[ 0 ] = 0;
   strut[ 1 ] = 0;
   strut[ 2 ] = 0;
   strut[ 3 ] = 100;
   XChangeProperty(disp, win, atom__NET_WM_STRUT, XA_CARDINAL, 32,
		    PropModeReplace, (unsigned char *) strut, 4);

   /* reside on ALL desktops */
   set_window_prop(disp, win, atom__NET_WM_DESKTOP, XA_CARDINAL, 0xFFFFFFFF);
   set_window_prop(disp, win, atom__NET_WM_WINDOW_TYPE, XA_ATOM,
		    atom__NET_WM_WINDOW_TYPE_DOCK);
   set_window_prop(disp, win, atom__NET_WM_STATE, XA_ATOM,
		    atom__NET_WM_STATE_STICKY);

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

   cursor_hide();
}


/*---------------------------------------------------------------------*/
/*    int                                                              */
/*    cursor_windowp ...                                               */
/*---------------------------------------------------------------------*/
int
cursor_windowp(Window win) {
   return win && (win == cursor_window);
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    cursor_setup ...                                                 */
/*---------------------------------------------------------------------*/
void
cursor_setup(char *text, int x, int y, int color) {
   Display *disp = cursor_xinfo->disp;
   int len = strlen(text);
      
   cursor_text = text;
   cursor_color = color;
      
   if (x > cursor_xinfo->screen_width) {
      x = cursor_xinfo->screen_width - 2;
   }

   XMoveResizeWindow(disp, cursor_window, x, y, 100, 100);

   if (cursor_state == 0) {
      XMapWindow(disp, cursor_window);
      cursor_state = 1;
   }
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    cursor_refresh ...                                               */
/*---------------------------------------------------------------------*/
void
cursor_refresh() {
   if (cursor_text) {
      char *text = cursor_text;
      Display *disp = cursor_xinfo->disp;

      draw_gradient(cursor_xinfo, cursor_window,
		     1, 1, 100 - 2, 100 - 2,
		     0, TOOLTIPS, 0, 0);
   } else {
      XUnmapWindow(cursor_xinfo->disp, cursor_window);
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
/*    void                                                             */
/*    show_cursor ...                                                  */
/*---------------------------------------------------------------------*/
void
show_cursor(taskbar_t *tbar) {
   static long count = 0;
   static long long init_time = 0;
   
   Display *disp = tbar->xinfo->disp;
   Window root_window = tbar->xinfo->root_win;
   Window root_return, child_return;
   int root_x_return, root_y_return;
   int win_x_return, win_y_return;
   unsigned int mask_return;
   long long cur_time;
   struct timeval tv;
   
   if (gettimeofday(&tv, 0) == 0) {
      cur_time = ((long long)(tv.tv_sec * 1000000) + (long long)tv.tv_usec);
   }

#define POINTER_SENSITIVITY 500000
   
   if (cur_time - init_time < POINTER_SENSITIVITY) {
      if (count > 55) {
	 int retval = XQueryPointer(disp, root_window, &root_return, &child_return,
				    &root_x_return, &root_y_return,
				    &win_x_return, &win_y_return,
				    &mask_return);
	 if (!retval) {
	    /* pointer is not in the same screen, ignore */
	    return;
	 }
	 
	 printf("ct:%lld, it:%lld d:%ld count:%d x: %d,  y:%d\n", cur_time, init_time , cur_time - init_time, count, root_x_return, root_y_return);
	 init_time = cur_time;
	 cursor_setup("foo", root_x_return, root_y_return, 3);
      } else {
	 cursor_hide();
	 count++;
      }
   } else {
      cursor_hide();
      init_time = cur_time;
      count = 0;
   }
}
   
