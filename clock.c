/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/clock.c                            */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Fri Jul 23 22:15:38 2004                          */
/*    Last change :  Thu Apr 24 11:19:56 2025 (serrano)                */
/*    Copyright   :  2004-25 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    The clock                                                        */
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
#include "clock.h"

/*---------------------------------------------------------------------*/
/*    clock_t                                                          */
/*---------------------------------------------------------------------*/
#define STRLEN 30
typedef struct ipclock {
   area_t area;
   time_t sec;
   char str[ STRLEN ];
} ipclock_t;

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    refresh_clock ...                                                */
/*---------------------------------------------------------------------*/
static void
refresh_clock( area_t *ar ) {
   taskbar_t *tbar = ar->taskbar;
   int relief = tbar->config->relief;
   Xinfo_t *xinfo = tbar->xinfo;
   char *time_str = ((ipclock_t *)ar)->str + 11;
   int text_x = (ar->width - XTextWidth( xinfo->xfsb, time_str, 5 )) / 2;
   int text_y = xinfo->xfsb->ascent + ((ar->height - xinfo->xfsb->ascent) / 3)
      + (1 * relief);

   draw_gradient( xinfo, ar->win,
		  0, 0,
		  ar->width - 1,
		  ar->height + (relief ? (2 * tbar->aborder) : 0),
		  0, tbar->frame_gradient_color, 0, 0 );

   draw_text( tbar->xinfo, ar->win,
	      text_x, text_y,
	      time_str, 5, 0, ar->active ? ACTIVE : BLACK,
	      tbar->config->color_shadow, tbar->config->shadow_size );
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    button_press_clock ...                                           */
/*---------------------------------------------------------------------*/
static void
button_press_clock( XEvent *ev, area_t *ar ) {
   taskbar_t *tbar = ar->taskbar;
}
/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    enter_notify_clock ...                                           */
/*---------------------------------------------------------------------*/
static void
enter_notify_clock( XEvent *ev, area_t *ar ) {
   taskbar_t *tbar = ar->taskbar;
   tooltips_setup( ((ipclock_t *)ar)->str,
		   ar->x,
		   tbar->top ? tbar->y + tbar->height : tbar->y - tbar->height,
		   TOOLTIPS );
   ar->active = 1;
   refresh_clock( ar );
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    leave_notify_clock ...                                           */
/*---------------------------------------------------------------------*/
static void
leave_notify_clock( XEvent *ev, area_t *ar ) {
   tooltips_hide();
   ar->active = 0;
   refresh_clock( ar );
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    set_clock ...                                                    */
/*---------------------------------------------------------------------*/
static void
set_clock( area_t *ar ) {
   time_t now = time( 0 );
   time_t prev = ((ipclock_t *)ar)->sec;

   if( (now < prev) || (now - prev) >= 60 ) {
      char *str = ctime( &now );
      /* refresh the clock */
      ((ipclock_t *)ar)->sec = now;
      strncpy( ((ipclock_t *)ar)->str, ctime( &now ), STRLEN );
      ((ipclock_t *)ar)->str[ strlen( str ) - 1 ] = 0;
   }
}
   
/*---------------------------------------------------------------------*/
/*    static int                                                       */
/*    timeout_clock ...                                                */
/*---------------------------------------------------------------------*/
static int
timeout_clock( area_t *ar ) {
   set_clock( ar );
   refresh_clock( ar );

   return 1;
}

/*---------------------------------------------------------------------*/
/*    area_t *                                                         */
/*    make_clock ...                                                   */
/*---------------------------------------------------------------------*/
area_t *
make_clock( taskbar_t *tbar, int width, int height ) {
   ipclock_t *cl = calloc( 1, sizeof( ipclock_t ) );
   area_t *ar = &(cl->area);

   if( !ar ) exit( 10 );

   /* initialize the clock */
   ar->win = make_area_window( tbar );
   ar->name = "clock";
   
   ar->uwidth = width;
   ar->uheight = height;

   cl->sec = -1;
   
   /* register the timeout */
   set_clock( ar );
   
   ar->timeout_delay = 10 * 60;
   evloop_timeout( ar );
   
   /* bind the area in the taskbar */
   ar->taskbar = tbar;
/*    tbar->areas = cons( ar, tbar->areas );                           */

   ar->refresh = &refresh_clock;
   ar->desktop_notify = &refresh_clock;
   ar->timeout = &timeout_clock;
   ar->button_press = &button_press_clock;
   ar->enter_notify = &enter_notify_clock;
   ar->leave_notify = &leave_notify_clock;

   return ar;
}

/*---------------------------------------------------------------------*/
/*    void *                                                           */
/*    start_clock ...                                                  */
/*---------------------------------------------------------------------*/
static void *
start_clock( void *tb, pair_t *args ) {
   taskbar_t *tbar = (taskbar_t *)tb;
   config_t *config = tbar->config;
   int width = INTEGER_VAL( CAR( args ) );

   return make_clock( tbar, width, config->taskbar_height - 1 );
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    parse_clock ...                                                  */
/*---------------------------------------------------------------------*/
void
parse_clock( config_t *config, pair_t *lst ) {
   pair_t *l = CDR( lst );
   integer_t *width = make_integer( 40 );

   /* search of a command */
   while( PAIRP( l ) ) {
      obj_t *car = CAR( l );
      if( SYMBOLP( car ) ) {
	 if( SYMBOL_EQ( (symbol_t *)car, sym_width ) ) {
	    width = parse_cadr_integer( l );

	    if( !width ) {
	       parse_error( "Illegal :width", (obj_t *)lst );
	    } else {
	       l = CDR( l );
	    }
	 }
      } else {
	 parse_error( "Illegal clock", (obj_t *)lst );
      }
      l = CDR( l );
   }

   register_plugin( config, make_plugin( start_clock, cons( width, NIL ) ) );
}
