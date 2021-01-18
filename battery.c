/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/battery.c                          */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Mon Aug 11 10:37:34 2014                          */
/*    Last change :  Sat Nov 30 10:37:10 2019 (serrano)                */
/*    Copyright   :  2014-19 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    battery applet                                                   */
/*=====================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <values.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#ifdef HAVE_XPM
#  include <X11/xpm.h>
#endif

#include "icccmpanel.h"
#include "battery.h"

/*---------------------------------------------------------------------*/
/*    battery_t                                                        */
/*---------------------------------------------------------------------*/
#define STRLEN 30
typedef struct ipbattery {
   area_t area;
   char querying;
   char *query_command;
   char *halt_command;
   char infobuf[ 1024 ];
   char percent[ 4 ];
   int batteries_len;
   double charge_max;
   int *charges_fd;
   int online_fd;
   char *dir;
   int iconlen;
   Pixmap *icons, *masks;
   Pixmap online_img, online_mask;
   Pixmap current_img, current_mask;
/*    void (*refresh)( struct area * );                                */
/*    Pixmap *icons, *masks;                                           */
} ipbattery_t;


/*---------------------------------------------------------------------*/
/*    statics                                                          */
/*---------------------------------------------------------------------*/
static long read_battery( int );
static long read_current_now( ipbattery_t * );

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    refresh_battery ...                                              */
/*---------------------------------------------------------------------*/
static void
refresh_battery( area_t *ar ) {
   taskbar_t *tbar = ar->taskbar;
   int relief = tbar->config->relief;
   ipbattery_t *bat = (ipbattery_t *)ar;
   Xinfo_t *xinfo = tbar->xinfo;
   char *time_str = bat->percent;
   int text_y = xinfo->xfsb->ascent + ((ar->height - xinfo->xfsb->ascent) / 3)
      + 1 + (1 * relief);
   int text_x = XTextWidth( xinfo->xfsb, time_str, 3 );

   draw_gradient( xinfo, ar->win,
		  0, 0,
		  ar->width - 1,
		  ar->height,
		  0, ar->active ? GREY10 : GREY12, 0, 0 );

   draw_pixmap( xinfo, ar->win,
		bat->current_img, bat->current_mask,
		0, 0,
		ar->width, ar->height );

   draw_text_plain( tbar->xinfo, ar->win,
		    tbar->config->icon_size + 2, text_y,
		    time_str, 3, 0, ar->active ? ACTIVE : BLACK,
		    tbar->config->color_shadow, tbar->config->shadow_size );
   draw_text_plain( tbar->xinfo, ar->win,
		    tbar->config->icon_size + 3 + text_x, text_y,
		    "%", 1, 0, ar->active ? ACTIVE : BLACK,
		    tbar->config->color_shadow, tbar->config->shadow_size );
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    set_battery_info ...                                             */
/*---------------------------------------------------------------------*/
static void
set_battery_info( area_t *ar ) {
   ipbattery_t *bat = (ipbattery_t *)ar;
   long charge_now;
   long battery_tm;
   long cnow = read_current_now( bat );
   int i;
   
   // battery charge
   for( charge_now = 0, i = bat->batteries_len - 1; i >= 0; i-- ) {
      charge_now += read_battery( bat->charges_fd[ i ] );
   }

   if( cnow > 0 ) {
      battery_tm = (long)(60. * (double)charge_now / (double)cnow);
   } else {
      battery_tm = 0;
   }

   // the interface name
   snprintf( bat->infobuf, 1024,
	     "time left: %02d:%02d", battery_tm / 60, battery_tm % 60 );
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    suggest_shutdown ...                                             */
/*---------------------------------------------------------------------*/
static void
suggest_shutdown( ipbattery_t *bat ) {
   if( !fork() ) {
      if( system( bat->query_command ) == 1 ) {
	 system( bat->halt_command );
      } else {
	 exit( 0 );
      }
   }
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    set_battery ...                                                  */
/*---------------------------------------------------------------------*/
static void
set_battery( area_t *ar ) {
   ipbattery_t *bat = (ipbattery_t *)ar;
   char *buf = bat->percent;
   char online[1];
   int i;
   long c;
   double dc;
   
   // online info
   lseek( bat->online_fd, 0, SEEK_SET );
   read( bat->online_fd, online, 1 );

   // battery charge
   for( c = 0, i = bat->batteries_len - 1; i >= 0; i-- ) {
      c += read_battery( bat->charges_fd[ i ] );
   }

   dc = (double)c;
   if( dc >= bat->charge_max ) {
      strcpy( buf, "100" );
   } else {
      long p = (long)(dc * 100. / bat->charge_max);
      
      snprintf( buf, 4, " %02d", p );

      /* low battery level, suggest shutdown */
      if( p <= 5 ) {
	 if( !bat->querying ) {
	    bat->querying = 1;
	    suggest_shutdown( bat );
	 }
      } else {
	 bat->querying = 0;
      }
	    
   }

   // battery status
   if( online[ 0 ] == '1' ) {
      bat->current_mask = bat->online_mask;
      bat->current_img = bat->online_img;
   } else {
      i = (long)(dc * 10. / bat->charge_max);

      if( i == 10 ) i = 9;
      
      bat->current_mask = bat->masks[ i ];
      bat->current_img = bat->icons[ i ];
   }
}
   
/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    enter_notify_battery ...                                         */
/*---------------------------------------------------------------------*/
static void
enter_notify_battery( XEvent *ev, area_t *ar ) {
   taskbar_t *tbar = ar->taskbar;

   set_battery( ar );
   set_battery_info( ar );

   ar->active = 1;
   refresh_battery( ar );
   
   tooltips_setup( ((ipbattery_t *)ar)->infobuf,
		   ar->x,
		   tbar->top ? tbar->y + tbar->height : tbar->y - tbar->height,
		   TOOLTIPS );
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    leave_notify_battery ...                                         */
/*---------------------------------------------------------------------*/
static void
leave_notify_battery( XEvent *ev, area_t *ar ) {
   ar->active = 0;
   refresh_battery( ar );
   
   tooltips_hide();
}

/*---------------------------------------------------------------------*/
/*    static int                                                       */
/*    timeout_battery ...                                              */
/*---------------------------------------------------------------------*/
static int
timeout_battery( area_t *ar ) {
   set_battery( ar );
   refresh_battery( ar );

   return 1;
}

/*---------------------------------------------------------------------*/
/*    static pair_t *                                                  */
/*    find_batteries ...                                               */
/*---------------------------------------------------------------------*/
static pair_t *
find_batteries( char *dir ) {
   static char buffer[ 2048 ];
   pair_t *lst = NIL;
   int i;

   for( i = 0; ; i++ ) {
      sprintf( buffer, "%s/BAT%d", dir, i );

      if( access( buffer, R_OK ) ) {
	 return lst;
      } else {
	 lst = cons( make_string( buffer ), lst );
      }
   }
}

/*---------------------------------------------------------------------*/
/*    static long                                                      */
/*    read_battery ...                                                 */
/*---------------------------------------------------------------------*/
static long
read_battery( int fd ) {
   char buf[ 20 ];
   int i, j;

   lseek( fd, 0, SEEK_SET );
   i = read( fd, buf, 20 );
   buf[ i ] = 0;

   return atol( buf );
}

/*---------------------------------------------------------------------*/
/*    static long                                                      */
/*    read_battery_file ...                                            */
/*---------------------------------------------------------------------*/
static long
read_battery_file( char *name ) {
   int fd = open( name, O_RDONLY );
   long res;

   res = read_battery( fd );
   close( fd );

   return res;
}

/*---------------------------------------------------------------------*/
/*    static long                                                      */
/*    read_current_now ...                                             */
/*---------------------------------------------------------------------*/
static long
read_current_now( ipbattery_t *bat ) {
   static char buffer[ 2048 ];
   pair_t *lst = NIL;
   int i;
   long c;

   for( c = 0, i = bat->batteries_len - 1; i >= 0 ; i-- ) {
      sprintf( buffer, "%s/BAT%d/current_now", bat->dir, i );
      
      c += read_battery_file( buffer );
   }

   return c;
}

/*---------------------------------------------------------------------*/
/*    static long                                                      */
/*    open_battery_charges ...                                         */
/*---------------------------------------------------------------------*/
static long
open_battery_charges( pair_t *l, int *fds ) {
   int i;
   long m = 0;

   for( i = 0; PAIRP( l ); l = CDR( l ), i++ ) {
      char *name = STRING_CHARS( CAR( l ) );
      char buf[ 1024 ];

      snprintf( buf, 1024, "%s/charge_full", name );
      m += read_battery_file( buf );

      snprintf( buf, 1024, "%s/charge_now", name );
      fds[ i ] = open( buf, O_RDONLY );
   }

   return m;
}
   
/*---------------------------------------------------------------------*/
/*    static area_t *                                                  */
/*    make_battery ...                                                 */
/*---------------------------------------------------------------------*/
static area_t *
make_battery( taskbar_t *tbar, int width, int height,
	      string_t *icon_online, pair_t *icons,
	      string_t *query_command, string_t *halt_command,
	      string_t *directory ) {
   ipbattery_t *bat = calloc( 1, sizeof( ipbattery_t ) );
   area_t *ar = &(bat->area);
   int len = length( icons );
   int i;
   char *iconpath;
   char buffer[ 2048 ];
   char *dir;
   pair_t *batteries;
   
   if( !ar ) exit( 10 );

   /* initialize the battery */
   ar->win = make_area_window( tbar );
   ar->name = "battery";
   
   ar->uwidth = width;
   ar->uheight = height;

   /* battery directory */
   dir = STRING_CHARS( directory );

   snprintf( buffer, 2048, "%s/AC/online", dir );
   bat->online_fd = open( buffer, O_RDONLY );

   if( !bat->online_fd ) {
      fprintf( stderr,
	       "icccmpanel error: cannot find battery online info \"%s\"\n",
	       buffer );
      return 0L;
   }

   /* batteries discovery */
   batteries = find_batteries( dir );
   bat->batteries_len = length( batteries );
   bat->dir = dir;
   bat->querying = 0;
   bat->query_command = STRING_CHARS( query_command );
   bat->halt_command = STRING_CHARS( halt_command );

   bat->charges_fd = malloc( sizeof( int ) * bat->batteries_len );

   bat->charge_max =
      (double)open_battery_charges( batteries, bat->charges_fd );

   /* the icons */
   bat->iconlen = len;
   bat->icons = calloc( len, sizeof( Pixmap ) );
   bat->masks = calloc( len, sizeof( Pixmap ) );

   for( i = 0; i < len; i++, icons = CDR( icons ) ) {
       iconpath = find_icon( tbar->config, STRING_CHARS( CAR( icons ) ) );

       if( iconpath ) {
	  XpmReadFileToPixmap( tbar->xinfo->disp, ar->win, iconpath,
			       &(bat->icons[ i ]), &(bat->masks[ i ]),
			       NULL );
       }
   }

   /* online icon */
   iconpath = find_icon( tbar->config, STRING_CHARS( icon_online ) );
   if( iconpath ) {
      XpmReadFileToPixmap( tbar->xinfo->disp, ar->win, iconpath,
			   &(bat->online_img), &(bat->online_mask),
			   NULL );
   }

   
   /* bind the area in the taskbar */
   ar->taskbar = tbar;

   ar->refresh = &refresh_battery;
   ar->desktop_notify = &refresh_battery;
   ar->timeout = &timeout_battery;
   ar->timeout_delay = 600;

   ar->enter_notify = &enter_notify_battery;
   ar->leave_notify = &leave_notify_battery;
   
   /* register the timeout */
   set_battery( ar );
   evloop_timeout( ar );
   
   return ar;
}
   
/*---------------------------------------------------------------------*/
/*    void *                                                           */
/*    start_battery ...                                                */
/*---------------------------------------------------------------------*/
static void *
start_battery( void *tb, pair_t *args ) {
   taskbar_t *tbar = (taskbar_t *)tb;
   config_t *config = tbar->config;
   int width = INTEGER_VAL( CAR( args ) );
   string_t *icon_online = CAR( CDR( args ) );
   pair_t *icons = CAR( CDR( CDR( args ) ) );
   string_t *query_command = CAR( CDR( CDR( CDR( args ) ) ) );
   string_t *halt_command = CAR( CDR( CDR( CDR( CDR( args ) ) ) ) );
   string_t *directory = CAR( CDR( CDR( CDR( CDR( CDR( args ) ) ) ) ) );

   return make_battery( tbar, width, config->taskbar_height - 1,
			icon_online, icons, query_command, halt_command,
			directory );
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    parse_battery ...                                                */
/*---------------------------------------------------------------------*/
void
parse_battery( config_t *config, pair_t *lst ) {
   pair_t *l = CDR( lst );
   pair_t *args = NIL; 
   symbol_t *sym_query_command = make_symbol( ":query-command" );
   symbol_t *sym_halt_command = make_symbol( ":halt-command" );
   symbol_t *sym_icon_online = make_symbol( ":icon-online" );
   symbol_t *sym_icon = make_symbol( ":icon" );
   symbol_t *sym_directory = make_symbol( ":directory" );
   integer_t *width = make_integer( 50 );
   pair_t *icons = 0;
   string_t *query_command = 0L;
   string_t *halt_command = 0L;
   string_t *icon_online = 0L;
   string_t *directory = 0L;

   while( PAIRP( l ) ) {
      obj_t *car = CAR( l );
      if( SYMBOLP( car ) ) {
	 if( SYMBOL_EQ( (symbol_t *)car, sym_icon_online ) ) {
	    icon_online = parse_cadr_string( l );
	    l = CDR( l );
	 } else if( SYMBOL_EQ( (symbol_t *)car, sym_directory ) ) {
	    directory = parse_cadr_string( l );

	    if( !directory ) {
	       parse_error( "Illegal :directory", (obj_t *)lst );
	    } else {
	       l = CDR( l );
	    }
	 } else if( SYMBOL_EQ( (symbol_t *)car, sym_query_command ) ) {
	    query_command = parse_cadr_string( l );

	    if( !query_command ) {
	       parse_error( "Illegal :query-command", (obj_t *)lst );
	    } else {
	       l = CDR( l );
	    }
	 } else if( SYMBOL_EQ( (symbol_t *)car, sym_halt_command ) ) {
	    halt_command = parse_cadr_string( l );

	    if( !halt_command ) {
	       parse_error( "Illegal :halt-command", (obj_t *)lst );
	    } else {
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
	    icons = CAR( CDR( lst ) );

	    if( PAIRP( icons ) ) {
	       parse_error( "Illegal :icons", (obj_t *)lst );
	    } else {
	       l = CDR( l );
	    }
	 }
      } else {
	 parse_error( "Illegal battery", (obj_t *)lst );
      }
      l = CDR( l );
   }

   if( !icon_online )
      icon_online = make_string( "bat-online.xpm" );
   if( !directory )
      directory = make_string( "/sys/class/power_supply" );

   if( !query_command )
      query_command =
	 make_string( "gmessage \"Low Battery, shutdown?\" -buttons \"yes:1,no:0\"" );
   
   if( !halt_command )
      halt_command =
	 make_string( "sync && sudo systemctl hibernate" );
   
   
   if( !icons ) {
      icons =
	 cons( make_string( "bat0.xpm" ),
	       cons( make_string( "bat1.xpm" ),
		     cons( make_string( "bat2.xpm" ),
			   cons( make_string( "bat3.xpm" ),
				 cons( make_string( "bat4.xpm" ),
				       cons( make_string( "bat5.xpm" ),
					     cons( make_string( "bat6.xpm" ),
						   cons( make_string( "bat7.xpm" ),
							 cons( make_string( "bat8.xpm" ),
							       cons( make_string( "bat9.xpm" ),
								     NIL ) ) ) ) ) ) ) ) ) );
   }

   args = cons( width,
		cons( icon_online,
		      cons( icons,
			    cons( query_command,
				  cons( halt_command, 
					cons( directory, NIL ) ) ) ) ) );
   register_plugin( config, make_plugin( start_battery, args ) );
}
