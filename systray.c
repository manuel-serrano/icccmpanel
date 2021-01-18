/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/systray.c                          */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Sun Mar 18 17:03:45 2007                          */
/*    Last change :  Sat Nov 30 10:44:28 2019 (serrano)                */
/*    Copyright   :  2007-19 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    System tray implementation                                       */
/*    The specification can be found at:                               */
/*      http://standards.freedesktop.org/systemtray-spec/              */
/*                                           systemtray-spec-0.2.html  */
/*=====================================================================*/
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_XPM
#  include <X11/xpm.h>
#endif

#include "icccmpanel.h"

/*---------------------------------------------------------------------*/
/*    systray spec constants                                           */
/*---------------------------------------------------------------------*/
#define SYSTRAY_REQUEST_DOCK 0
#define SYSTRAY_BEGIN_MESSAGE 1
#define SYSTRAY_CANCEL_MESSAGE 2

#define SYSTRAY_WINDOW_TYPE_KDE 1
#define SYSTRAY_WINDOW_TYPE_DEF 2

#define SYSTRAY_ORIENTATION_HORZ 0
#define SYSTRAY_ORIENTATION_VERT 1

#define SYSTRAY_ICON_SIZE 20

/*---------------------------------------------------------------------*/
/*    Global static variables                                          */
/*---------------------------------------------------------------------*/
static Atom net_opcode_atom;
static Atom net_sel_atom;
static Atom net_manager_atom;
static Atom net_msg_data_atom;

/*---------------------------------------------------------------------*/
/*    ipsystray_t                                                      */
/*---------------------------------------------------------------------*/
#define STRLEN 30
typedef struct ipsystray {
   area_t area;
   int len;
   pair_t *icons;
} ipsystray_t;

/*---------------------------------------------------------------------*/
/*    systrayicon_t                                                    */
/*---------------------------------------------------------------------*/
typedef struct systrayicon {
   area_t area;
   ipsystray_t *systray;
   Window parent;
   int width;
} systrayicon_t;

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    destroy_notify_systray ...                                       */
/*---------------------------------------------------------------------*/
static void
destroy_notify_systray( area_t *ar, xclient_t *xcl ) {
   /* fprintf( stderr, "destroy_notify_systray...xcl->win=%p\n", xcl->win ); */
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    refresh_systray_icon ...                                         */
/*---------------------------------------------------------------------*/
static void
refresh_systray_icon( area_t *ar ) {
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    systray_layout ...                                               */
/*---------------------------------------------------------------------*/
void
systray_layout( ipsystray_t *st ) {
   pair_t *lst = st->icons;
   taskbar_t *tbar = ((area_t *)st)->taskbar;
   int border = tbar->aborder;
   int x = tbar->aborder;
   Display *disp = tbar->xinfo->disp;
   
   while( PAIRP( lst ) ) {
      area_t *ar = (area_t *)CAR( lst );
      systrayicon_t *sti = (systrayicon_t *)ar;

      XMoveWindow( disp, sti->parent, x, 0 );

      x += (border + SYSTRAY_ICON_SIZE);
      
      lst = CDR( lst );
   }
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    destroy_systray_icon ...                                         */
/*---------------------------------------------------------------------*/
static void
destroy_systray_icon( area_t *ar, xclient_t *xcl ) {
   if( !xcl ) {
      systrayicon_t *sti = (systrayicon_t *)ar;
      ipsystray_t *st = sti->systray;
      taskbar_t *tbar = ar->taskbar;

      /* shrink the systray area */
      //((area_t *)st)->uwidth -= (tbar->border + SYSTRAY_ICON_SIZE);
      ((area_t *)st)->uwidth -= (tbar->border + tbar->config->app_width);

      /*
      fprintf( stderr, "destroy_systray_icon: name=%s win=%p parent=%p\n",
	       sti->area.name, ar->win, sti->parent );
      */
   
      XDestroyWindow( tbar->xinfo->disp, sti->parent );
      tbar->areas = remq( ar, tbar->areas );
      st->icons = remq( ar, st->icons );
      free( ar->name );
      free( ar );

      /* re-layout the remaining tray icons */
      systray_layout( st );
   
      /* redisplay the whole bar */
      taskbar_area_do_layout( tbar );
      taskbar_register_xclients( tbar );
      taskbar_refresh_all( tbar );
   }
}

/*---------------------------------------------------------------------*/
/*    static systrayicon_t *                                           */
/*    systray_icon_add ...                                             */
/*---------------------------------------------------------------------*/
static systrayicon_t *
systray_icon_add( ipsystray_t *st, Window id ) {
   systrayicon_t *sti = calloc( 1, sizeof( struct systrayicon ) );
   area_t *ar = (area_t *)st;
   taskbar_t *tbar = ar->taskbar;
   Display *disp = tbar->xinfo->disp;
   int border = tbar->aborder;
   Window win = make_area_window_parent( tbar, ar->win );
   int iconsz = ar->height;
   char *name = window_name( disp, id );
   int systray_icon_width = tbar->config->app_width;

   /* initialize the systray area */
   sti->area.win = id;
   sti->area.ignore_layout = 1;
   //sti->area.width = SYSTRAY_ICON_SIZE + border;
   sti->area.width = systray_icon_width + border;
   sti->area.height = iconsz;
   sti->area.refresh = &refresh_systray_icon;
   sti->area.destroy_notify = &destroy_systray_icon;
   sti->area.taskbar = tbar;
   sti->area.name = malloc( strlen( name ) + 1 );
   sti->systray = st;
   sti->parent = win;

   strcpy( sti->area.name, name );
   
   /* enlarge the tray area */
   //ar->uwidth += (SYSTRAY_ICON_SIZE + border);
   ar->uwidth += (systray_icon_width + border);
   
   /* put the new icon at its new place */
   //XMoveResizeWindow( disp, win, border, 0, SYSTRAY_ICON_SIZE, iconsz );
   XMoveResizeWindow( disp, win, border, 0, systray_icon_width, iconsz );
   
   /* reparent the systray icon */
   XReparentWindow( disp, id, win, border, 0 );
   
   XSync( disp, False );
   
   XSelectInput( disp, id, PropertyChangeMask | StructureNotifyMask );
   
   //XMoveResizeWindow( disp, id, border, 0, SYSTRAY_ICON_SIZE, iconsz );
   XMoveResizeWindow( disp, id, border, 0, systray_icon_width, iconsz );
   
   XFlush( disp );
   XClearWindow( disp, tbar->win );

   XMapRaised( disp, id );
   XMapRaised( disp, win );

   /* re-organize all the current systray */
   tbar->areas = cons( sti, tbar->areas );
   st->icons = cons( sti, st->icons );
   systray_layout( st );
   
   return sti;
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    client_message_systray ...                                       */
/*---------------------------------------------------------------------*/
void
client_message_systray( XEvent *ev, area_t *ar ) {
   if( ev->xclient.message_type == net_opcode_atom ) {
      XClientMessageEvent *xclient = &(ev->xclient);
      unsigned long opcode = xclient->data.l[ 1 ];
      taskbar_t *tbar = ar->taskbar;
      ipsystray_t *st = (ipsystray_t *)ar;
      Window id;
      systrayicon_t *tar;

      switch( opcode ) {
	 case SYSTRAY_REQUEST_DOCK:
	    id = xclient->data.l[ 2 ];

	    if( id && systray_icon_add( st, id ) ) {
	       XSelectInput( tbar->xinfo->disp, id, StructureNotifyMask );

	       taskbar_area_do_layout( tbar );
	       taskbar_register_xclients( tbar );
	       taskbar_refresh_all( tbar );
	    }
	    break;

	 case SYSTRAY_BEGIN_MESSAGE:
	    /*
	    fprintf( stderr, "Message From Dockapp\n" );
	    */
	    id = xclient->window;
	    break;

	 case SYSTRAY_CANCEL_MESSAGE:
	    /*
	    fprintf( stderr, "Message Cancelled\n" );
	    */
	    id = xclient->window;
	    break;

	 default:
	    if( opcode == net_msg_data_atom ) {
	       /*
	       fprintf( stderr, "Text For Message From Dockapp:\n%s\n",
			xclient->data.b );
	       */
	       id = xclient->window;
	       break;
	    }
    
	    /* unknown message type. not in the spec. */
	    fprintf( stderr,
		     "Warning: Received unknown client message to System Tray "
		     "selection window.\n" );
	    break;
      }
   }
}

/*---------------------------------------------------------------------*/
/*    SET_ATOM ...                                                     */
/*---------------------------------------------------------------------*/
#define SET_ATOM( var, key, message ) \
  if( !(var = XInternAtom( disp, key, False )) ) { \
     fprintf( stderr, "Can't get tray " #message ", disabling tray\n" ); \
     return 0; \
  }


/*---------------------------------------------------------------------*/
/*    static int                                                       */
/*    init_systray ...                                                 */
/*---------------------------------------------------------------------*/
static int
init_systray( Xinfo_t *xinfo, Window win ) {
   Display *disp = xinfo->disp;
   Window root = xinfo->root_win;
   static char init = 0;

   if( !init ) {
      char name[ 80 ];
      
      init = 1;
      
      /* system tray init */
      sprintf( name, "_NET_SYSTEM_TRAY_S%d", xinfo->screen );

      SET_ATOM( net_sel_atom, name, "sel atom" );
      SET_ATOM( net_opcode_atom, "_NET_SYSTEM_TRAY_OPCODE", "opcode atom" );
      SET_ATOM( net_manager_atom, "MANAGER", "manager atom" );
      SET_ATOM( net_msg_data_atom, "_NET_SYSTEM_TRAY_MESSAGE_DATA", "msg" );

      XSetSelectionOwner( disp, net_sel_atom, win, CurrentTime );
      if( XGetSelectionOwner( disp, net_sel_atom ) != win ) {
	 fprintf( stderr, "Can't get tray selection owner, disabling tray\n" );
	 /* we don't get the selection */
	 return 0L;
      } else {
	 XEvent ev;
	 
	 ev.type = ClientMessage;
	 ev.xclient.window = root;
	 ev.xclient.message_type = net_manager_atom;
	 ev.xclient.format = 32;
	 ev.xclient.data.l[ 0 ] = CurrentTime;
	 ev.xclient.data.l[ 1] = net_sel_atom;
	 ev.xclient.data.l[ 2 ] = win;
	 ev.xclient.data.l[ 3 ] = 0;
	 ev.xclient.data.l[ 4 ] = 0;
	 
	 XSendEvent( disp, root, False, StructureNotifyMask, &ev );
      }
      return 1;
   }
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    refresh_systray ...                                              */
/*---------------------------------------------------------------------*/
static void
refresh_systray( area_t *ar ) {
   taskbar_t *tbar = ar->taskbar;
   systrayicon_t *sti = (systrayicon_t *)ar;
   ipsystray_t *st = sti->systray;
   Xinfo_t *xinfo = tbar->xinfo;
   int relief = tbar->config->relief;

   draw_gradient( tbar->xinfo, ar->win,
		  0, 0, ar->width - 1, tbar->height - 1,
		  0,
		  GREY12, 0, 1 );
   
   if( !relief && ar->uwidth > 1 ) {
      if( relief ) {
	 draw_relief( tbar->xinfo, ar->win,
		      0, 0,
		      ar->width - 1, ar->height - 1,
		      0, WHITE, GREY9, tbar->aborder );
      } else {
	 draw_partial_relief( xinfo, ar->win, RELIEF_LEFT | RELIEF_RIGHT,
			      0, 0,
			      ar->width - 1, ar->height + 2,
			      0, WHITE, GREY9, tbar->aborder );
      }
   }
}

/*---------------------------------------------------------------------*/
/*    area_t *                                                         */
/*    make_systray ...                                                 */
/*---------------------------------------------------------------------*/
area_t *
make_systray( taskbar_t *tbar, int height ) {
   ipsystray_t *st = calloc( 1, sizeof( ipsystray_t ) );
   area_t *ar = &(st->area);

   /* initialize the tray */
   st->icons = NIL;
   ar->win = make_area_window( tbar );
   ar->name = "tray";

   ar->uwidth = 1;
   ar->uheight = height;
   ar->taskbar = tbar;

   /* initialize the systray */
   if( init_systray( tbar->xinfo, ar->win ) ) {
      tbar->areas = cons( ar, tbar->areas );
  
      ar->refresh = &refresh_systray;
      ar->desktop_notify = &refresh_systray;
      ar->destroy_notify = &destroy_notify_systray;
      ar->client_message = &client_message_systray;

      return ar;
   } else {
      return 0;
   }
}

/*---------------------------------------------------------------------*/
/*    void *                                                           */
/*    start_systray ...                                                */
/*---------------------------------------------------------------------*/
void *
start_systray( void *tb, pair_t *args ) {
   taskbar_t *tbar = (taskbar_t *)tb;
   config_t *config = tbar->config;

   return make_systray( tbar, config->taskbar_height - 1 );
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    parse_systray ...                                                */
/*---------------------------------------------------------------------*/
void
parse_systray( config_t *config, pair_t *lst ) {
   pair_t *l = CDR( lst );

   /* search of a command */
   while( PAIRP( l ) ) {
      obj_t *car = CAR( l );
      if( SYMBOLP( car ) ) {
	 ;
      } else {
	 parse_error( "Illegal systray", (obj_t *)lst );
      }
      l = CDR( l );
   }

   register_plugin( config, make_plugin( start_systray, NIL ) );
}
