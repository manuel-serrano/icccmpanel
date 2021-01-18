/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/hibernate.c                        */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Mon Aug 11 10:37:34 2014                          */
/*    Last change :  Sat Nov 30 10:38:20 2019 (serrano)                */
/*    Copyright   :  2014-19 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    hibernate applet                                                 */
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

#ifdef HAVE_LIBIW
#  include <iwlib.h>
#endif

#include "icccmpanel.h"
#include "hibernate.h"

/*---------------------------------------------------------------------*/
/*    hibernate_t                                                      */
/*---------------------------------------------------------------------*/
#define STRLEN 30
typedef struct hibernate {
   area_t area;
   char *command;
   Pixmap icon, mask;
   int active;
} hibernate_t;

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    refresh_hibernate ...                                            */
/*---------------------------------------------------------------------*/
static void
refresh_hibernate( area_t *ar ) {
   taskbar_t *tbar = ar->taskbar;
   hibernate_t *hi = (hibernate_t *)ar;
   Xinfo_t *xinfo = tbar->xinfo;
   int text_y = xinfo->xfsb->ascent + ((ar->height - xinfo->xfsb->ascent) / 3);

   draw_gradient( xinfo, ar->win,
		  0, 0,
		  ar->width - 1,
		  ar->height,
		  0, hi->active ? GREY10 : GREY12, 0, 0 );

   draw_pixmap( xinfo, ar->win,
		hi->icon, hi->mask,
		0, 0, 
		ar->width, ar->height );
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    button_press_hibernate ...                                       */
/*---------------------------------------------------------------------*/
static void
button_press_hibernate( XEvent *ev, area_t *ar ) {
   taskbar_t *tbar = ar->taskbar;
   char *cmd = ((hibernate_t *)ar)->command;

   if( cmd ) {
      if( !fork() ) {
	 setsid();
	 execl( "/bin/sh", "/bin/sh", "-c", cmd, 0 );
	 exit( 0 );
      }
   }
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    enter_notify_hibernate ...                                       */
/*---------------------------------------------------------------------*/
static void
enter_notify_hibernate( XEvent *ev, area_t *ar ) {
   taskbar_t *tbar = ar->taskbar;
   hibernate_t *hi = (hibernate_t *)ar;

   hi->active = 1;
   refresh_hibernate( ar );
   tooltips_setup( ((hibernate_t *)ar)->command,
		   ar->x,
		   tbar->top ? tbar->y + tbar->height : tbar->y - tbar->height,
		   TOOLTIPS );
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    leave_notify_hibernate ...                                       */
/*---------------------------------------------------------------------*/
static void
leave_notify_hibernate( XEvent *ev, area_t *ar ) {
   hibernate_t *hi = (hibernate_t *)ar;
   
   hi->active = 0;
   refresh_hibernate( ar );
   tooltips_hide();
}

/*---------------------------------------------------------------------*/
/*    static area_t *                                                  */
/*    make_hibernate ...                                               */
/*---------------------------------------------------------------------*/
static area_t *
make_hibernate( taskbar_t *tbar, int width, int height, char *icon, char *command ) {
   hibernate_t *hi = calloc( 1, sizeof( hibernate_t ) );
   area_t *ar = &(hi->area);
   char *iconpath = find_icon( tbar->config, icon );

   if( !ar ) exit( 10 );

   /* initialize the hibernate */
   ar->win = make_area_window( tbar );
   ar->name = "hibernate";
   
   ar->uwidth = width;
   ar->uheight = height;

   hi->command = command;
   
   /* bind the area in the taskbar */
   ar->taskbar = tbar;

   ar->refresh = &refresh_hibernate;
   ar->desktop_notify = &refresh_hibernate;

   ar->enter_notify = &enter_notify_hibernate;
   ar->leave_notify = &leave_notify_hibernate;
   ar->button_press = &button_press_hibernate;
   
   XpmReadFileToPixmap( tbar->xinfo->disp, ar->win, iconpath,
			&(hi->icon), &(hi->mask),
			NULL );
   
   return ar;
}
   
/*---------------------------------------------------------------------*/
/*    void *                                                           */
/*    start_hibernate ...                                              */
/*---------------------------------------------------------------------*/
static void *
start_hibernate( void *tb, pair_t *args ) {
   taskbar_t *tbar = (taskbar_t *)tb;
   config_t *config = tbar->config;
   int width = INTEGER_VAL( CAR( args ) );
   char *icon = STRING_CHARS( CAR( CDR( args ) ) );
   char *command = STRING_CHARS( CAR( CDR( CDR( args ) ) ) );

   return make_hibernate( tbar, width, config->taskbar_height - 1, icon, command );
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    parse_hibernate ...                                              */
/*---------------------------------------------------------------------*/
void
parse_hibernate( config_t *config, pair_t *lst ) {
   pair_t *l = CDR( lst );
   pair_t *args = NIL; 
   symbol_t *sym_intf = make_symbol( ":interface" );
   symbol_t *sym_command = make_symbol( ":command" );
   symbol_t *sym_icon = make_symbol( ":icon" );
   integer_t *width = make_integer( 16 );
   string_t *icon = make_string( "hibernate.xpm" );
   string_t *command = 0L;

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
	 } else if( SYMBOL_EQ( (symbol_t *)car, sym_icon ) ) {
	    icon = parse_cadr_string( l );

	    if( !icon ) {
	       parse_error( "Illegal :icon", (obj_t *)lst );
	    } else {
	       l = CDR( l );
	    }
	 } else if( SYMBOL_EQ( (symbol_t *)car, sym_command ) ) {
	    command = parse_cadr_string( l );

	    if( !command ) {
	       parse_error( "Illegal :command", (obj_t *)lst );
	    } else {
	       l = CDR( l );
	    }
	 }
      } else {
	 parse_error( "Illegal hibernate", (obj_t *)lst );
      }
      l = CDR( l );
   }


   args = cons( width, cons( icon, cons( command, NIL ) ) );
   register_plugin( config, make_plugin( start_hibernate, args ) );
}
