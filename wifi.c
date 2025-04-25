/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/wifi.c                             */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Mon Aug 11 10:37:34 2014                          */
/*    Last change :  Thu Apr 24 11:20:54 2025 (serrano)                */
/*    Copyright   :  2014-25 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    wifi applet                                                      */
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
#include "taskbar.h"
#include "wifi.h"

#if( HAVE_LIBIW )
#  include <iwlib.h>

/*---------------------------------------------------------------------*/
/*    wifi_t                                                           */
/*---------------------------------------------------------------------*/
#define STRLEN 30
typedef struct ipwifi {
   area_t area;
   char *intf;
   int skfd;
   char *command;
   struct wireless_info *winfo;
   struct iwreq wrq;
   int quality, qualityidx;
   char qualbuf[ 3 ];
   char infobuf[ 1024 ];
   int iconlen;
   int iconstep;
   Pixmap *icons, *masks;
} ipwifi_t;


/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    refresh_wifi ...                                                 */
/*---------------------------------------------------------------------*/
static void
refresh_wifi( area_t *ar ) {
   taskbar_t *tbar = ar->taskbar;
   int relief = tbar->config->relief;
   ipwifi_t *iw = (ipwifi_t *)ar;
   Xinfo_t *xinfo = tbar->xinfo;
   int text_y = xinfo->xfsb->ascent + ((ar->height - xinfo->xfsb->ascent) / 3)
      + (1 * relief);

   draw_gradient( xinfo, ar->win,
		  0, 0,
		  ar->width - 1,
		  ar->height,
		  0, ar->active ? GREY10 : tbar->frame_gradient_color, 0, 0 );

   draw_pixmap( xinfo, ar->win,
		iw->icons[ iw->qualityidx ], iw->masks[ iw->qualityidx ],
		0, 0, 
		ar->width, ar->height );

   draw_text_plain( tbar->xinfo, ar->win,
		    tbar->config->icon_size + 2, text_y,
		    iw->qualbuf, 2, 0, ar->active ? ACTIVE : BLACK,
		    tbar->config->color_shadow, tbar->config->shadow_size );
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    set_wifi_quality ...                                             */
/*---------------------------------------------------------------------*/
static void
set_wifi_quality( ipwifi_t *iw, int sensibility ) {
   struct wireless_info *winfo = iw->winfo;
   
   if( winfo->has_range && winfo->has_stats
       && ((winfo->stats.qual.level != 0)
	   || (winfo->stats.qual.updated & IW_QUAL_DBM)) ) {
      if (!(winfo->stats.qual.updated & IW_QUAL_QUAL_INVALID)) {
	 int q = winfo->stats.qual.qual;
	 int qmax = winfo->range.max_qual.qual;
	 int d = q - iw->quality;

	 if( d == 0 || !sensibility || d < -sensibility || d > sensibility ) {
	    long r = (long)(((double)q)/((double)qmax) * 100.);
	    int s = (q * 100) / qmax;

	    iw->quality = q;
	    iw->qualityidx = r / iw->iconstep;
	    if( iw->qualityidx >= iw->iconlen ) {
	       iw->qualityidx = iw->iconlen - 1;
	    }

/* 	    fprintf( stderr, "qmax=%d q=%d idx=%d len=%d r=%d s=%d\n", */
/* 		     qmax, q, iw->qualityidx, iw->iconlen, r, s );     */
	    sprintf( iw->qualbuf, "%02d", s > 99 ? 99 : s );
	 }
      } else {
	 strcpy( iw->qualbuf, "--" );
      }
   }
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    set_wifi_info ...                                                */
/*---------------------------------------------------------------------*/
static void
set_wifi_info( area_t *ar ) {
   ipwifi_t *iw = (ipwifi_t *)ar;
   int skfd = iw->skfd;
   struct wireless_info *winfo = iw->winfo;
   struct iwreq wrq;
   int has_bitrate = 0;
   char *intf = iw->intf;
   char bitrate[ 64 ];
   char ap[ 64 ];
   char essid[ 64 ];
   char mode[ 17 ];

   // set present winfo variables
   if( iw_get_stats( skfd, intf, &(winfo->stats),
		     &winfo->range, winfo->has_range ) >= 0) {
      winfo->has_stats = 1;
   }
   if( iw_get_range_info( skfd, intf, &(winfo->range) ) >= 0 ) {
      winfo->has_range = 1;
   }
   if( iw_get_ext(skfd, intf, SIOCGIWAP, &wrq) >= 0 ) {
      winfo->has_ap_addr = 1;
      memcpy( &(winfo->ap_addr), &(wrq.u.ap_addr), sizeof( sockaddr ) );
   }

   // get bitrate
   if( iw_get_ext(skfd, intf, SIOCGIWRATE, &wrq) >= 0 ) {
      memcpy( &(winfo->bitrate), &(wrq.u.bitrate), sizeof( iwparam ) );
      iw_print_bitrate(bitrate, 16, winfo->bitrate.value);
      has_bitrate = 1;
   }

   // get ap mac
   if( winfo->has_ap_addr ) {
      iw_sawap_ntop( &winfo->ap_addr, ap );
   }

   // get essid
   if( winfo->b.has_essid ) {
      if( winfo->b.essid_on ) {
	 snprintf( essid, 32, "%s", winfo->b.essid );
      } else {
	 snprintf( essid, 32, "off/any" );
      }
   }

   // set link quality
   set_wifi_quality( iw, 0 );
   
   // the interface name
   snprintf( iw->infobuf, 1024, "%s %s %02d/%02d",
	     intf, essid,
	     iw->quality, winfo->range.max_qual.qual );
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    set_wifi ...                                                     */
/*---------------------------------------------------------------------*/
static void
set_wifi( area_t *ar ) {
   ipwifi_t *iw = (ipwifi_t *)ar;
   int skfd = iw->skfd;
   struct wireless_info *winfo = iw->winfo;
   int has_bitrate = 0;
   char *intf = iw->intf;

   // set present winfo variables
   if( iw_get_stats( skfd, intf, &(winfo->stats),
		     &winfo->range, winfo->has_range ) >= 0) {
      winfo->has_stats = 1;
   }
   if( iw_get_range_info( skfd, intf, &(winfo->range) ) >= 0 ) {
      winfo->has_range = 1;
   }

   // set link quality
   set_wifi_quality( iw, 5 );
}
   
/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    button_press_wifi ...                                            */
/*---------------------------------------------------------------------*/
static void
button_press_wifi( XEvent *ev, area_t *ar ) {
   taskbar_t *tbar = ar->taskbar;
   char *cmd = ((ipwifi_t *)ar)->command;

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
/*    enter_notify_wifi ...                                            */
/*---------------------------------------------------------------------*/
static void
enter_notify_wifi( XEvent *ev, area_t *ar ) {
   taskbar_t *tbar = ar->taskbar;

   set_wifi_info( ar );

   ar->active = 1;
   refresh_wifi( ar );
   
   tooltips_setup( ((ipwifi_t *)ar)->infobuf,
		   ar->x,
		   tbar->top ? tbar->y + tbar->height : tbar->y - tbar->height,
		   TOOLTIPS );
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    leave_notify_wifi ...                                            */
/*---------------------------------------------------------------------*/
static void
leave_notify_wifi( XEvent *ev, area_t *ar ) {
   ar->active = 0;
   refresh_wifi( ar );
   
   tooltips_hide();
}

/*---------------------------------------------------------------------*/
/*    static int                                                       */
/*    timeout_wifi ...                                                 */
/*---------------------------------------------------------------------*/
static int
timeout_wifi( area_t *ar ) {
   set_wifi( ar );
   refresh_wifi( ar );

   return 1;
}

/*---------------------------------------------------------------------*/
/*    static area_t *                                                  */
/*    make_wifi ...                                                    */
/*---------------------------------------------------------------------*/
static area_t *
make_wifi( taskbar_t *tbar, int width, int height, char *intf, pair_t *icons, string_t *command ) {
   ipwifi_t *iw = calloc( 1, sizeof( ipwifi_t ) );
   area_t *ar = &(iw->area);
   int len = length( icons );
   int i;
   
   if( !ar ) exit( 10 );

   /* initialize the wifi */
   ar->win = make_area_window( tbar );
   ar->name = "wifi";
   
   ar->uwidth = width;
   ar->uheight = height;

   iw->winfo = calloc( 1, sizeof( struct wireless_info ) );
   iw->skfd = iw_sockets_open();
   iw->intf = intf;
   strcpy( iw->qualbuf, "  " );
   
   if( iw_get_basic_config( iw->skfd, intf, &(iw->winfo->b)) < 0 ) {
      fprintf( stderr, "icccpanel: cannot get wifi info -- %s\n", intf );
      iw_sockets_close( iw->skfd );
      free( iw->winfo );
   }

   /* the icons */
   iw->iconlen = len;
   iw->iconstep = (100 / len);
   iw->icons = calloc( len, sizeof( Pixmap ) );
   iw->masks = calloc( len, sizeof( Pixmap ) );

   for( i = 0; i < len; i++, icons = CDR( icons ) ) {
      char *iconpath = find_icon( tbar->config, STRING_CHARS( CAR( icons ) ) );

      if( iconpath ) {
	 XpmReadFileToPixmap( tbar->xinfo->disp, ar->win, iconpath,
			      &(iw->icons[ i ]), &(iw->masks[ i ]),
			      NULL );
      }
   }
   
   /* bind the area in the taskbar */
   ar->taskbar = tbar;

   ar->refresh = &refresh_wifi;
   ar->desktop_notify = &refresh_wifi;
   ar->timeout = &timeout_wifi;
   ar->timeout_delay = 40;

   ar->enter_notify = &enter_notify_wifi;
   ar->leave_notify = &leave_notify_wifi;
   
   if( command ) {
      ar->button_press = &button_press_wifi;
      iw->command = STRING_CHARS( command );
   }

   /* register the timeout */
   set_wifi( ar );
   evloop_timeout( ar );
   
   return ar;
}
   
/*---------------------------------------------------------------------*/
/*    void *                                                           */
/*    start_wifi ...                                                   */
/*---------------------------------------------------------------------*/
static void *
start_wifi( void *tb, pair_t *args ) {
   taskbar_t *tbar = (taskbar_t *)tb;
   config_t *config = tbar->config;
   int width = INTEGER_VAL( CAR( args ) );
   char *intf = STRING_CHARS( CAR( CDR( args ) ) );
   pair_t *icons = CAR( CDR( CDR( args ) ) );
   string_t *command = CAR( CDR( CDR( CDR( args ) ) ) );

   return make_wifi( tbar, width, config->taskbar_height - 1, intf, icons, command );
}
#endif /* HAVE_LIBIW */

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    parse_wifi ...                                                   */
/*---------------------------------------------------------------------*/
void
parse_wifi( config_t *config, pair_t *lst ) {
   pair_t *l = CDR( lst );
   pair_t *args = NIL; 
   symbol_t *sym_intf = make_symbol( ":interface" );
   symbol_t *sym_command = make_symbol( ":command" );
   symbol_t *sym_icon = make_symbol( ":icons" );
   integer_t *width = make_integer( 40 );
   string_t *intf = make_string( "wlan0" );
   pair_t *icons = 0;
   string_t *command = 0L;

   /* search of a command */
   while( PAIRP( l ) ) {
      obj_t *car = CAR( l );
      if( SYMBOLP( car ) ) {
	 if( SYMBOL_EQ( (symbol_t *)car, sym_intf ) ) {
	    intf = parse_cadr_string( l );

	    if( !intf ) {
	       parse_error( "Illegal :interface", (obj_t *)lst );
	    } else {
	       args = cons( intf, args );
	       l = CDR( l );
	    }
	 } else if( SYMBOL_EQ( (symbol_t *)car, sym_width ) ) {
	    width = parse_cadr_integer( l );

	    if( !width ) {
	       parse_error( "Illegal :width", (obj_t *)lst );
	    } else {
	       l = CDR( l );
	    }
	 } else if( SYMBOL_EQ( (symbol_t *)car, sym_icon ) ) {
	    icons = CAR( CDR( l ) );
	    if( !PAIRP( icons ) ) {
	       parse_error( "Illegal :icons", (obj_t *)lst );
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
	 parse_error( "Illegal wifi", (obj_t *)lst );
      }
      l = CDR( l );
   }

#if( HAVE_LIBIW )   
   if( !PAIRP( icons ) ) {
      icons = cons( make_string( "wifi0.xpm" ),
		    cons( make_string( "wifi1.xpm" ),
			  cons( make_string( "wifi2.xpm" ),
				cons( make_string( "wifi3.xpm" ),
				      NIL ) ) ) );
   }
   
   args = cons( width, cons( intf, cons( icons, cons( command, NIL ) ) ) );
   register_plugin( config, make_plugin( start_wifi, args ) );
#endif   
}
