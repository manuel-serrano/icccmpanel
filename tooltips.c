/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/tooltips.c                         */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Sat Oct 18 05:57:32 2003                          */
/*    Last change :  Sat Nov 30 10:38:52 2019 (serrano)                */
/*    Copyright   :  2003-19 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    MSpanel tooltips                                                 */
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
/*    tooltips_window ...                                              */
/*---------------------------------------------------------------------*/
Window tooltips_window = 0;
static Xinfo_t *tooltips_xinfo = 0;
static XFontStruct *ttfs;
static int tooltips_text_y = 0;
static int tooltips_height = 0;
static char *tooltips_text = 0;
static int tooltips_x, tooltips_y;
static int tooltips_color;

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    init_tooltips ...                                                */
/*---------------------------------------------------------------------*/
void
init_tooltips( Xinfo_t *xinfo ) {
   Display *disp = xinfo->disp;
   Window win;
   XSetWindowAttributes att;
   MWMHints mwm;
   XSizeHints size_hints;
   XWMHints wmhints;
   unsigned long strut[ 4 ];
   XGCValues gcv;
   GC gc;
   char *fontname = xinfo->tooltips_fontname;
    
   att.background_pixel = mstk_palette[ TOOLTIPS ];
   att.event_mask = ExposureMask;
   
   win = XCreateWindow( /* display */ disp,
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
			/* attribs */ &att );

   if( !win )
      return;
   
   /* the X graphic context */
   do {
      ttfs = XLoadQueryFont( disp, xinfo->tooltips_fontname );
      xinfo->tooltips_fontname = "fixed";
   } while( !ttfs );

   gcv.graphics_exposures = False;
   gcv.font = ttfs->fid;
   gc = XCreateGC( disp, win, GCFont | GCGraphicsExposures, &gcv );
   
   /* reserve "WINHEIGHT" pixels at the bottom of the screen */
   strut[ 0 ] = 0;
   strut[ 1 ] = 0;
   strut[ 2 ] = 0;
   strut[ 3 ] = 100;
   XChangeProperty( disp, win, atom__NET_WM_STRUT, XA_CARDINAL, 32,
		    PropModeReplace, (unsigned char *) strut, 4 );

   /* reside on ALL desktops */
   set_window_prop( disp, win, atom__NET_WM_DESKTOP, XA_CARDINAL, 0xFFFFFFFF );
   set_window_prop( disp, win, atom__NET_WM_WINDOW_TYPE, XA_ATOM,
		    atom__NET_WM_WINDOW_TYPE_DOCK );
   set_window_prop( disp, win, atom__NET_WM_STATE, XA_ATOM,
		    atom__NET_WM_STATE_STICKY );

   /* use old gnome hint since sawfish doesn't support _NET_WM_STRUT */
   set_window_prop( disp, win, atom__WIN_HINTS, XA_CARDINAL,
		    WIN_HINTS_SKIP_FOCUS | WIN_HINTS_SKIP_WINLIST |
		    WIN_HINTS_SKIP_TASKBAR | WIN_HINTS_DO_NOT_COVER );

   /* borderless motif hint */
   bzero( &mwm, sizeof( mwm ) );
   mwm.flags = MWM_HINTS_DECORATIONS;
   XChangeProperty( disp, win,
		    atom__MOTIF_WM_HINTS, atom__MOTIF_WM_HINTS, 32,
		    PropModeReplace,
		    (unsigned char *) &mwm, sizeof(MWMHints) / 4 );

   /* make sure the WM obays our window position */
   size_hints.flags = PPosition;

   /* XSetWMNormalHints (disp, win, &size_hints); */
   XChangeProperty( disp, win,
		    XA_WM_NORMAL_HINTS, XA_WM_SIZE_HINTS, 32,
		    PropModeReplace,
		    (unsigned char *) &size_hints, sizeof(XSizeHints) / 4 );


   /* make our window unfocusable */
   wmhints.flags = InputHint;
   wmhints.input = False;

   /* XSetWMHints (disp, win, &wmhints); */
   XChangeProperty( disp, win,
		    XA_WM_HINTS, XA_WM_HINTS, 32, PropModeReplace,
		    (unsigned char *) &wmhints, sizeof(XWMHints) / 4 );

   tooltips_window = win;
   tooltips_xinfo = copy_xinfo( xinfo );
   tooltips_xinfo->gcb = gc;
   tooltips_height = ttfs->ascent * 2;
   tooltips_text_y = ttfs->ascent + (ttfs->ascent / 2);

   tooltips_hide();
}


/*---------------------------------------------------------------------*/
/*    int                                                              */
/*    tooltips_windowp ...                                             */
/*---------------------------------------------------------------------*/
int
tooltips_windowp( Window win ) {
   return win && (win == tooltips_window);
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    tooltips_setup ...                                               */
/*---------------------------------------------------------------------*/
void
tooltips_setup( char *text, int x, int y, int color ) {
   if( tooltips_window && (tooltips_text != text) ) {
      Display *disp = tooltips_xinfo->disp;
      int len = strlen( text );
      int w = XTextWidth( ttfs, text, len ) + 8;
      int h = tooltips_height;
      
      tooltips_text = text;
      tooltips_color = color;
      
      if( x + w > tooltips_xinfo->screen_width ) {
	 x = tooltips_xinfo->screen_width - w - 2;
      }

      XMoveResizeWindow( disp, tooltips_window, x, y, w, h );

      XMapWindow( disp, tooltips_window );
   }
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    tooltips_refresh ...                                             */
/*---------------------------------------------------------------------*/
void
tooltips_refresh() {
   if( tooltips_text ) {
      char *text = tooltips_text;
      Display *disp = tooltips_xinfo->disp;
      int len = strlen( text );
      int w = XTextWidth( ttfs, text, len ) + 8;
      int h = tooltips_height;

      draw_gradient( tooltips_xinfo, tooltips_window,
		     1, 1, w - 2, h - 2,
		     0, TOOLTIPS, 0, 0 );
      draw_relief( tooltips_xinfo, tooltips_window,
		   0 , 0, w - 1, h - 1,
		   0, TOOLTIPSBD, TOOLTIPSBD, 1 );
      draw_text( tooltips_xinfo, tooltips_window,
		 3, tooltips_text_y,
		 text, len,
		 0, TOOLTIPSFG, -1, 0 );
   } else {
      XUnmapWindow( tooltips_xinfo->disp, tooltips_window );
   }
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    hide_tooltips ...                                                */
/*---------------------------------------------------------------------*/
void
tooltips_hide() {
   if( tooltips_window ) {
      tooltips_text = 0;
      XUnmapWindow( tooltips_xinfo->disp, tooltips_window );
   }
}

