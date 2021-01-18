/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/evloop.c                           */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Fri Jul 23 05:59:11 2004                          */
/*    Last change :  Sat Apr 20 16:10:36 2019 (serrano)                */
/*    Copyright   :  2004-19 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    Event loop                                                       */
/*=====================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <values.h>

#ifdef HAVE_XPM
#  include <X11/xpm.h>
#endif

#include "icccmpanel.h"
#include "taskbar.h"

/*---------------------------------------------------------------------*/
/*    static pair_t *                                                  */
/*    timeout_areas ...                                                */
/*---------------------------------------------------------------------*/
static pair_t *timeout_areas = NIL;
static long timeout_gcd = 10;

/*---------------------------------------------------------------------*/
/*    static long                                                      */
/*    gcd ...                                                          */
/*---------------------------------------------------------------------*/
static long
gcd( long a, long b ) {
   while( b != 0 ) {
      long t = b;
      b = a % b;
      a = t;
   }

   return a;
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    evloop_timeout ...                                               */
/*---------------------------------------------------------------------*/
void
evloop_timeout( area_t *ar ) {
   timeout_areas = cons( ar, timeout_areas );

   if( ar->timeout_delay < timeout_gcd ) {
      timeout_gcd = ar->timeout_delay;
   } else {
      timeout_gcd = gcd( timeout_gcd, ar->timeout_delay );
   }
}

/*---------------------------------------------------------------------*/
/*    static area_t *                                                  */
/*    enter_area ...                                                   */
/*---------------------------------------------------------------------*/
static area_t *
enter_area( XEvent *ev, area_t *newa, area_t *olda ) {
   if( newa != olda ) {
      if( olda && olda->leave_notify ) olda->leave_notify( ev, olda );
      if( newa && newa->enter_notify ) newa->enter_notify( ev, newa );
   }
   return newa;
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    evloop ...                                                       */
/*---------------------------------------------------------------------*/
void
evloop( taskbar_t *tbar ) {
   area_t *iar = 0;
   XEvent ev;
   int evt;
   XEvent elev;
   fd_set fd;
   int xfd;
   long count = timeout_gcd;
   
   xfd = ConnectionNumber( tbar->xinfo->disp );

   while( 1 ) {
      area_t *ar;

      if( NULLP( timeout_areas ) ) {
	 FD_ZERO( &fd );
	 FD_SET( xfd, &fd );
	 select( xfd + 1, &fd, 0, 0, 0 );
      } else {
	 struct timeval tv;

	 tv.tv_usec = 100000;
	 tv.tv_sec = 0;
	 FD_ZERO( &fd );
	 FD_SET( xfd, &fd );
	 
	 if( select (xfd + 1, &fd, 0, 0, &tv ) == 0 ) {
	    pair_t *lst = timeout_areas;
	    pair_t *prev = 0;

	    if( --count <= 0 ) {
	       count = timeout_gcd;
	       
	       while( PAIRP( lst ) ) {
		  area_t *ar = (area_t *)CAR( lst );

		  if( ar->timeout_delay > 0 ) {
		     if( ar->timeout_count > 0 ) {
			ar->timeout_count -= timeout_gcd;
			prev = lst;
			lst = CDR( lst );
			continue;
		     } else {
			ar->timeout_count = (ar->timeout_delay - 10);
		     }
		  }

		  if( ar->timeout( ar ) ) {
		     prev = lst;
		     lst = CDR( lst );
		  } else {
		     lst = CDR( lst );
		     if( !prev ) {
			timeout_areas = lst;
		     } else {
			CDR( prev ) = lst;
		     }
		     free( lst );
		  }
	       }
	    }
	 }
      }

      // the evt variable is used to delay Leave/Notify event until
      // no pending events are to be processed
      evt = 0;

      while( XPending( tbar->xinfo->disp ) ) {
	 XNextEvent( tbar->xinfo->disp, &ev );
	 
	 switch( ev.type ) {
	    case ButtonPress:
	       // reset the possible timeout
	       ar = find_area( tbar, ev.xbutton.window );
	       if( ar ) {
		  if( ar->timeout_delay > 0 ) {
		     ar->timeout_count = (ar->timeout_delay - 10);
		  }
		  if( ar->button_press ) ar->button_press( &ev, ar );
	       }
	       break;

	    case KeyPress:
	       ar = find_area( tbar, ev.xkey.window );
	       if( ar && ar->key_press ) ar->key_press( &ev, ar );
	       break;

	    case DestroyNotify:
	       taskbar_destroy_notify( tbar, &ev );
	       break;

	    case Expose:
	       if( tbar->win == ev.xexpose.window ) {
		  taskbar_refresh( tbar );
	       } else {
		  if( tooltips_windowp( ev.xexpose.window ) ) {
		     tooltips_refresh();
		  } else {
		     ar = find_area( tbar, ev.xexpose.window );
		     if( ar ) ar->refresh( ar );
		  }
	       }
	       break;
	       
	    case PropertyNotify:
	       taskbar_property_notify( tbar, &ev );
	       break;

	    case EnterNotify:
	       evt = EnterNotify;
	       elev = ev;
	       break;

	    case LeaveNotify:
	       evt = LeaveNotify;
	       elev = ev;
	       break;

	    case MotionNotify:
	       ar = find_area( tbar, ev.xmotion.window );
	       iar = enter_area( &ev, ar, iar );
	       break;

	    case ClientMessage:
	       ar = find_area( tbar, ev.xclient.window );
	       if( ar && ar->client_message ) ar->client_message( &ev, ar );
	       break;

	    default:
	       break;
	 }
      }

      // once all the pending events have been handled, we process
      // the last leave/enter event for the tbar window
      switch( evt ) {
	 case EnterNotify:
	    if( tbar->hiddenp ) {
	       taskbar_unhide( tbar );
	    } else {
	       ar = find_area( tbar, ev.xcrossing.window );
	       iar = enter_area( &elev, ar, iar );
	    }
	    
	    break;

	 case LeaveNotify: 
	    iar = enter_area( &elev, 0, iar );
	    
	    tooltips_hide();
	    if( tbar->autohide ) taskbar_hide( tbar );
	    break;
      }
   }
}
