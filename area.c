/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/area.c                             */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Sat Jul 24 14:20:57 2004                          */
/*    Last change :  Mon Aug 11 18:32:11 2014 (serrano)                */
/*    Copyright   :  2004-14 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    Area management                                                  */
/*=====================================================================*/
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_XPM
#  include <X11/xpm.h>
#endif

#include "icccmpanel.h"

/*---------------------------------------------------------------------*/
/*    static area_t *                                                  */
/*    find_area_list ...                                               */
/*---------------------------------------------------------------------*/
static area_t *
find_area_list( pair_t *lst, Window win ) {
   while( PAIRP( lst ) ) {
      area_t *ar = CAR( lst );
      if( ar->win == win ) return ar;
      lst = CDR( lst );

      if( ar->subareas ) {
	 area_t *res = find_area_list( ar->subareas, win );

	 if( res ) {
	    return res;
	 }
      }
   }

   return 0L;
}

/*---------------------------------------------------------------------*/
/*    area_t *                                                         */
/*    find_area ...                                                    */
/*    -------------------------------------------------------------    */
/*    Find an area given an Xwindow.                                   */
/*---------------------------------------------------------------------*/
area_t *
find_area( taskbar_t *tbar, Window win ) {
   return find_area_list( tbar->areas, win );
}

/*---------------------------------------------------------------------*/
/*    Window                                                           */
/*    make_area_window_parent ...                                      */
/*---------------------------------------------------------------------*/
Window
make_area_window_parent( taskbar_t *tbar, Window parent ) {
   Display *disp = tbar->xinfo->disp;
   Window win;
   MWMHints mwm;
   XSizeHints size_hints;
   XWMHints wmhints;
   XSetWindowAttributes att;
   taskbar_t *tb;

   if( !(tb = calloc( 1, sizeof( taskbar_t ) )) )
      return 0;

   /* enable other windows to go above the panel */
   att.override_redirect = 1;
   att.background_pixel = mstk_palette[ LIGHTGREY ];
   att.event_mask = ButtonPressMask
      | ExposureMask
      | LeaveWindowMask
      | EnterWindowMask
      | PointerMotionMask;

   win = XCreateWindow( /* display */ disp,
			/* parent  */ parent,
			/* x       */ 0,
			/* y       */ 0,
			/* width   */ 1,
			/* height  */ 1,
			/* border  */ 0,
			/* depth   */ CopyFromParent,
			/* class   */ InputOutput,
			/* visual  */ CopyFromParent,
			/* vmask   */ CWBackPixel | CWEventMask | CWOverrideRedirect,
			/* attribs */ &att );

   /* use old gnome hint since sawfish doesn't support _NET_WM_STRUT */
   set_window_prop( disp, win, atom__WIN_HINTS, XA_CARDINAL,
		    WIN_HINTS_SKIP_FOCUS | WIN_HINTS_SKIP_WINLIST |
		    WIN_HINTS_SKIP_TASKBAR | WIN_HINTS_DO_NOT_COVER );

   /* borderless motif hint */
   memset( &mwm, 0, sizeof( mwm ) );
   mwm.flags = MWM_HINTS_DECORATIONS;
   XChangeProperty( disp, win, atom__MOTIF_WM_HINTS, atom__MOTIF_WM_HINTS, 32,
		    PropModeReplace,
		    (unsigned char *)&mwm, sizeof( MWMHints ) / 4 );

   /* make sure the WM obays our window position */
   size_hints.flags = PPosition;

   /*XSetWMNormalHints (disp, win, &size_hints); */
   XChangeProperty( disp, win, XA_WM_NORMAL_HINTS, XA_WM_SIZE_HINTS, 32,
		    PropModeReplace,
		    (unsigned char *)&size_hints, sizeof( XSizeHints ) / 4 );
   
   /* make our window unfocusable */
   wmhints.flags = InputHint;
   wmhints.input = False;

   /*XSetWMHints (disp, win, &wmhints); */
   XChangeProperty( disp, win, XA_WM_HINTS, XA_WM_HINTS, 32, PropModeReplace,
		    (unsigned char *)&wmhints, sizeof(XWMHints) / 4 );

   /* receive the window event */
   XMapWindow( disp, win );

   return win;
}

/*---------------------------------------------------------------------*/
/*    Window                                                           */
/*    make_area_window ...                                             */
/*---------------------------------------------------------------------*/
Window
make_area_window( taskbar_t *tbar ) {
   return make_area_window_parent( tbar, tbar->win );
}

