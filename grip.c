/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/grip.c                             */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Thu Jul 22 15:21:17 2004                          */
/*    Last change :  Fri Sep 19 08:29:28 2014 (serrano)                */
/*    Copyright   :  2004-14 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    The grip.                                                        */
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
#include "grip.h"

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    refresh_grip ...                                                 */
/*---------------------------------------------------------------------*/
static void
refresh_grip( area_t *ar ) {
   taskbar_t *tbar = ar->taskbar;
   int relief = tbar->config->relief;

   /* the surrounding relief */
   if( relief ) { 
      draw_relief( tbar->xinfo, ar->win,
		   0, 0,
		   ar->width - 1, ar->height - 1,
		   0, WHITE, GREY9, tbar->aborder );
   } else {
      draw_partial_relief( tbar->xinfo, ar->win, RELIEF_LEFT | RELIEF_RIGHT,
		0, 0,
		ar->width - 1, ar->height,
		0, WHITE, GREY9, tbar->aborder );
   }

   /* the grill */
   draw_grill( tbar->xinfo, ar->win,
	       0, 0,
	       ar->width - 1 - relief, ar->height - relief );
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    button_press_grip ...                                            */
/*---------------------------------------------------------------------*/
static void
button_press_grip( XEvent *ev, area_t *ar ) {
   taskbar_t *tbar = ar->taskbar;
   
   if( !tbar->autohide ) {
      tbar->autohide = 1;
      taskbar_hide( tbar );
   } else {
      tbar->autohide = 0;
   }
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    enter_notify_grip ...                                            */
/*---------------------------------------------------------------------*/
static void
enter_notify_grip( XEvent *ev, area_t *ar ) {
   taskbar_t *tbar = ar->taskbar;

   tooltips_setup( "Hide panel",
		   ar->x,
		   tbar->top ? tbar->y + tbar->height : tbar->y - tbar->height,
		   TOOLTIPS );
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    leave_notify_grip ...                                            */
/*---------------------------------------------------------------------*/
static void
leave_notify_grip( XEvent *ev, area_t *ar ) {
   tooltips_hide();
}

/*---------------------------------------------------------------------*/
/*    area_t *                                                         */
/*    make_grip ...                                                    */
/*---------------------------------------------------------------------*/
area_t *
make_grip( taskbar_t *tbar, int width, int height ) {
   area_t *ar = calloc( 1, sizeof( area_t ) );

   if( !ar ) exit( 10 );

   /* initialize the grip */
   ar->win = make_area_window( tbar );
   ar->name = "grip";

   ar->uwidth = width;
   ar->uheight = height;

   /* bind the area in the taskbar */
   ar->taskbar = tbar;
   tbar->areas = cons( ar, tbar->areas );
   
   ar->refresh = &refresh_grip;
   ar->desktop_notify = &refresh_grip;
   ar->button_press = &button_press_grip;
   ar->enter_notify = &enter_notify_grip;
   ar->leave_notify = &leave_notify_grip;

   return ar;
}

/*---------------------------------------------------------------------*/
/*    void *                                                           */
/*    start_grip ...                                                   */
/*---------------------------------------------------------------------*/
void *
start_grip( void *tb, pair_t *args ) {
   taskbar_t *tbar = (taskbar_t *)tb;
   config_t *config = tbar->config;
   int width = INTEGER_VAL( CAR( args ) );


   return make_grip( tbar, width, config->taskbar_height - 1 );
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    parse_grip ...                                                   */
/*---------------------------------------------------------------------*/
void
parse_grip( config_t *config, pair_t *lst ) {
   pair_t *l = CDR( lst );
   integer_t *width = make_integer( 9 );

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
	 parse_error( "Illegal grip", (obj_t *)lst );
      }
      l = CDR( l );
   }

   register_plugin( config, make_plugin( start_grip, cons( width, NIL ) ) );
}
