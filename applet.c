/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/applet.c                           */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Fri Jul 23 22:15:38 2004                          */
/*    Last change :  Sat Nov 30 10:37:01 2019 (serrano)                */
/*    Copyright   :  2004-19 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    The applets                                                      */
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
#include "applet.h"
#include "command.h"

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    refresh_applets ...                                              */
/*---------------------------------------------------------------------*/
static void
refresh_applets( area_t *ar ) {
   taskbar_t *tbar = ar->taskbar;
   int relief = tbar->config->relief;
   Xinfo_t *xinfo = tbar->xinfo;
   pair_t *lst = (pair_t *)ar->subareas;

   draw_gradient( xinfo, ar->win,
		  0, relief,
		  ar->width,
		  ar->height + (relief ? (2 * tbar->aborder) : 0),
		  0, GREY12, 0, 0 );
   
   while( PAIRP( lst ) ) {
      area_t *ar = (area_t *)CAR( lst );

      ar->refresh( ar );
      lst = CDR( lst );
   }

   if( relief ) {
      draw_relief( xinfo, ar->win,
		   0, 0,
		   ar->width - 1, ar->height - 1,
		   0, WHITE, GREY9, tbar->aborder );
   } else {
      draw_partial_relief( xinfo, ar->win, RELIEF_LEFT | RELIEF_RIGHT,
			   0, 0,
			   ar->width - 1, ar->height + 1,
			   0, WHITE, GREY9, tbar->aborder );
   }
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    layout_applets ...                                               */
/*---------------------------------------------------------------------*/
static void
layout_applets( area_t *ar ) {
   pair_t *lst = (pair_t *)ar->subareas;
   struct taskbar *tbar = ar->taskbar;
   Display *disp = tbar->xinfo->disp;
   int relief = tbar->config->relief;
   int arx = ar->x + 1 + ar->width - 2 - (2 * relief);
   
   while( PAIRP( lst ) ) {
      area_t *sar = (area_t *)CAR( lst );

      if( !sar->ignore_layout ) {
	 sar->width = sar->uwidth;
	 sar->x = arx - sar->uwidth;
	 sar->y = tbar->linesep + 1 + relief * tbar->border ;
	 sar->height = tbar->height - tbar->linesep - ((2 * relief + 2) * tbar->border);

	 arx = sar->x;

	 if( sar->win )
	    XMoveResizeWindow( disp, sar->win,
			       sar->x, sar->y, sar->width, sar->height );
      }

      if( sar->layout ) {
	 sar->layout( sar );
      }
      
      lst = CDR( lst );
   }
}

/*---------------------------------------------------------------------*/
/*    area_t *                                                         */
/*    make_applets ...                                                 */
/*---------------------------------------------------------------------*/
static area_t *
make_applets( taskbar_t *tbar, int width, int height, pair_t *plugins ) {
   area_t *ar = calloc( 1, sizeof( area_t ) );
   pair_t *subareas = NIL;

   if( !ar ) exit( 10 );

   /* initialize the applets */
   ar->win = make_area_window( tbar );
   ar->name = "applets";
   
   ar->uwidth = width;
   ar->uheight = height;

   /* bind the area in the taskbar */
   ar->taskbar = tbar;
   tbar->areas = cons( ar, tbar->areas );
   
   ar->refresh = &refresh_applets;
   ar->layout = &layout_applets;
   
   while( PAIRP( plugins ) ) {
      plugin_t *p = CAR( plugins );
      subareas = cons( p->start( tbar, p->args ), subareas );

      plugins = CDR( plugins );
   }
      
   ar->subareas = reverse( subareas );

   return ar;
}

/*---------------------------------------------------------------------*/
/*    void *                                                           */
/*    start_applets ...                                                */
/*---------------------------------------------------------------------*/
void *
start_applets( void *tb, pair_t *args ) {
   taskbar_t *tbar = (taskbar_t *)tb;
   config_t *config = tbar->config;
   int width = INTEGER_VAL( CAR( args ) );
   pair_t *plugins = CDR( args );

   return make_applets( tbar, width, config->taskbar_height - 1, plugins );
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    parse_applets ...                                                */
/*---------------------------------------------------------------------*/
void
parse_applets( config_t *config, pair_t *lst ) {
   pair_t *l = CDR( lst );
   symbol_t *sym_width = make_symbol( ":width" );
   integer_t *width;
   config_t *conf = calloc( 1, sizeof( struct config ) );
   pair_t *subplugins;
   pair_t *args = NIL;

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
	 if( PAIRP( car ) ) {
	    parse_pair( conf, (pair_t *)car );
	 } else {
	    parse_error( "Illegal applet", (obj_t *)lst );
	 }
      }
      l = CDR( l );
   }

   subplugins = conf->plugins;
   args = cons( width, subplugins );
   
   register_plugin( config, make_plugin( start_applets, args ) );
}
