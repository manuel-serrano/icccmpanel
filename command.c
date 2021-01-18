/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/command.c                          */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Thu Jul 22 15:21:17 2004                          */
/*    Last change :  Sat Nov 30 10:37:27 2019 (serrano)                */
/*    Copyright   :  2004-19 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    The command launcher.                                            */
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
#include <X11/keysym.h>

#ifdef HAVE_XPM
#  include <X11/xpm.h>
#endif

#include "icccmpanel.h"
#include "command.h"

/*---------------------------------------------------------------------*/
/*    carea                                                            */
/*---------------------------------------------------------------------*/
typedef struct carea {
   /* the area */
   area_t area;
   /* the command */
   char *string;
   int len;
   int current_len;
   int visible_offset;
   int visible_len;
} carea_t;
   
/*---------------------------------------------------------------------*/
/*    ipcommand_t ...                                                  */
/*---------------------------------------------------------------------*/
typedef struct ipcmd {
   /* the area */
   area_t area;
   /* the private command area */
   area_t *carea;
   /* the command window width */
   int cwidth;
   /* the state of the carea window */
   int mappedp;
   /* the partial relief mask */
   int relief_mask;
   /* the image associated with the application */
   Pixmap icon, mask;
   /* the positionn of the icon */
   int iconx, icony;
} ipcmd_t;

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    refresh_command ...                                              */
/*---------------------------------------------------------------------*/
static void
refresh_command( area_t *ar ) {
   ipcmd_t *ic = (ipcmd_t *)ar;
   taskbar_t *tbar = ar->taskbar;
   Xinfo_t *xinfo = tbar->xinfo;
   int border = tbar->aborder;
   int relief = tbar->config->relief;
   
   /* the icon */
   if( ic->icon || ic->mask ) {
      draw_pixmap( xinfo, ar->win,
		   ic->icon, ic->mask,
		   ic->iconx, ic->icony,
		   ar->width - ((1 + relief) * border),
		   ar->height - ((1 + relief) * border) );
   }

   /* the border */
   draw_partial_relief( xinfo, ar->win, ic->relief_mask,
			0, 0,
			ar->width - relief, ar->height - relief,
			0, WHITE, GREY9, border );
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    ipcmd_map_window ...                                             */
/*---------------------------------------------------------------------*/
void
ipcmd_map_window( taskbar_t *tbar, ipcmd_t *ipcmd ) {
   Display *disp = tbar->xinfo->disp;
   Window win = ipcmd->carea->win;
   int desk = current_desktop( disp, tbar->xinfo->root_win );
      
   set_window_prop( disp, win, atom__NET_WM_DESKTOP, XA_CARDINAL, desk );
   
   if( !ipcmd->mappedp ) {
      ipcmd->mappedp = 1;
      XMapWindow( disp, win );
   }
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    ipcmd_unmap_window ...                                           */
/*---------------------------------------------------------------------*/
void
ipcmd_unmap_window( taskbar_t *tbar, ipcmd_t *ipcmd ) {
   if( ipcmd->mappedp ) {
      ipcmd->mappedp = 0;
      XUnmapWindow( tbar->xinfo->disp, ipcmd->carea->win );
   }
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    refresh_cmdarea ...                                              */
/*---------------------------------------------------------------------*/
static void
refresh_cmdarea( area_t *ar ) {
   carea_t *ca = (carea_t *)ar;
   taskbar_t *tbar = ar->taskbar;
   Xinfo_t *xinfo = tbar->xinfo;
   int texta_x, texta_y, texta_w, texta_h;
   int text_x, text_y, text_w, text_h;
   char *text = &ca->string[ ca->visible_offset ];
   int tlen = ca->current_len - ca->visible_offset;
   texta_x = tbar->aborder + 2;
   texta_y = tbar->aborder + 2;
   texta_w = ar->width - (4 * tbar->aborder) - 3;
   texta_h = ar->height - (4 * tbar->aborder) - 2;

   /* Move the command wuindow */
   XMoveResizeWindow( xinfo->disp,
		      ar->win, ar->x, ar->y,
		      ar->width, ar->height );
   
   draw_gradient( tbar->xinfo, ar->win,
		  0, 0,
		  ar->width - 1,
		  ar->height - 1,
		  0, WHITE, 0, 0 );
   
   draw_relief( tbar->xinfo, ar->win,
                0, 0,
                ar->width - 1,
                ar->height - 1,
                0, WHITE, GREY9, tbar->aborder );

   /* the text */
   text_y = xinfo->xfsp->ascent + (texta_h - xinfo->xfsp->ascent) + 2;
   text_w = texta_w - 4;
   text_h = texta_h - 4;
   
   while( (XTextWidth( xinfo->xfsp, text, tlen ) >= text_w) && (tlen > 0) )
      tlen--;
   text_x = texta_x + 2;
   
   draw_text_plain( xinfo, ar->win, text_x, text_y, text, tlen, 0,
		    BLACK,
		    tbar->config->color_shadow, tbar->config->shadow_size );
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    enter_notify_cmdarea ...                                         */
/*---------------------------------------------------------------------*/
static void
enter_notify_cmdarea( XEvent *ev, area_t *ar ) {
   taskbar_t *tbar = ar->taskbar;

   XSetInputFocus( tbar->xinfo->disp, ar->win, RevertToNone, CurrentTime );
}
   
/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    leave_notify_cmdarea ...                                         */
/*---------------------------------------------------------------------*/
static void
leave_notify_cmdarea( XEvent *ev, area_t *ar ) {
   ipcmd_t *ip = (ipcmd_t *)ar->parent;
   taskbar_t *tbar = ar->taskbar;

   ipcmd_unmap_window( tbar, ip );
}

/*---------------------------------------------------------------------*/
/*    static Window                                                    */
/*    make_carea_window ...                                            */
/*---------------------------------------------------------------------*/
static Window
make_carea_window( taskbar_t *tbar ) {
   Display *disp = tbar->xinfo->disp;
   Window parent = tbar->xinfo->root_win;
   Window win;
   MWMHints mwm;
   XSizeHints size_hints;
   XWMHints wmhints;
   XSetWindowAttributes att;
   taskbar_t *tb;

   if( !(tb = calloc( 1, sizeof( taskbar_t ) )) )
      return 0;

   win = XCreateSimpleWindow( /* display */ disp,
			      /* parent  */ parent,
			      /* x       */ 0,
			      /* y       */ 0,
			      /* width   */ 1,
			      /* height  */ 1,
			      /* bwidth  */ 0,
			      /* border  */ 0,
			      /* bg      */ 0 );

   XSelectInput( disp,
		 win,
		 ExposureMask |
		 EnterWindowMask |
		 ButtonPressMask |
		 KeyPressMask );

   /* borderless motif hint */
   bzero( &mwm, sizeof( mwm ) );
   mwm.flags = MWM_HINTS_DECORATIONS;
   XChangeProperty( disp, win, atom__MOTIF_WM_HINTS, atom__MOTIF_WM_HINTS, 32,
		    PropModeReplace,
		    (unsigned char *)&mwm, sizeof( MWMHints ) / 4 );

   /* make sure the WM obays our window position */
   size_hints.flags = PPosition;
   XChangeProperty( disp, win, XA_WM_NORMAL_HINTS, XA_WM_SIZE_HINTS, 32,
		    PropModeReplace,
		    (unsigned char *)&size_hints, sizeof( XSizeHints ) / 4 );
   
   /* reside on ALL desktops */
   set_window_prop( disp, win, atom__NET_WM_DESKTOP, XA_CARDINAL, 0xFFFFFFFF );
   set_window_prop( disp, win, atom__NET_WM_WINDOW_TYPE, XA_ATOM,
		    atom__NET_WM_WINDOW_TYPE_DOCK );
   set_window_prop( disp, win, atom__NET_WM_STATE, XA_ATOM,
 		    atom__NET_WM_STATE_STICKY );

   return win;
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    key_press_cmdarea ...                                            */
/*---------------------------------------------------------------------*/
static void
key_press_cmdarea( XEvent *ev, area_t *ar ) {
   carea_t *ca = (carea_t *)ar;
   KeySym ks;
   char keychar[ 1 ];
   
   XLookupString( (XKeyEvent *)ev, keychar, 1, &ks, 0 );
   
   switch( ks ) {
      case XK_Return:
	 if( ca->current_len > 1 ) {
	    if( !fork() ) {
	       setsid();
	       execl( "/bin/sh", "/bin/sh", "-c", &ca->string[ 1 ], 0 );
	       exit( 0 );
	    } else {
	       leave_notify_cmdarea( ev, ar );
	    }
	 }
	 break;
      case XK_BackSpace:
	 if( ca->current_len > 1 ) {
	    ca->current_len--;
	    ca->string[ ca->current_len ] = 0;
	    if( ca->visible_offset > 0 )
	       ca->visible_offset--;
	 }
	 break;

      default:
	 if( !IsModifierKey( ks ) && !IsCursorKey( ks ) ) {
	    if( ca->current_len == ca->len ) {
	       ca->len *= 2;
	       ca->string = realloc( ca->string, ca->len );
	    }

	    ca->string[ ca->current_len ] = keychar[ 0 ];
	    ca->current_len++;
	    ca->string[ ca->current_len ] = 0;

	    if( ca->current_len > ca->visible_len )
	       ca->visible_offset++;
	 }
      }
   
   refresh_cmdarea( ar );	 
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    button_press_command ...                                         */
/*---------------------------------------------------------------------*/
static void
button_press_command( XEvent *ev, area_t *ar ) {
   taskbar_t *tbar = ar->taskbar;
   ipcmd_t *ic = (ipcmd_t *)ar;

   if( !ic->carea ) {
      area_t *area = calloc( 1, sizeof( carea_t ) );
      carea_t *carea = (carea_t *)area;
      int text_w, tlen; 
      char *text;
      
      area->win = make_carea_window( tbar );
      area->name = "command-window";
      area->ignore_layout = 1;
      area->taskbar = tbar;
      area->parent = ar;

      area->x = tbar->x + ar->x;
      area->y = tbar->y + ar->y;
      area->width = ic->cwidth;
      area->height = ar->height;

      ic->carea = area;
      
      carea->len = 100;
      carea->string = calloc( 1, carea->len );
      carea->string[ 0 ] = '>';
      carea->current_len = 1;
      carea->visible_offset = 0;

      tlen = tbar->width / 4;
      text = alloca( tlen + 1 );
      memset( text, 'm', tlen );
      text_w = ic->cwidth - (4 * tbar->aborder) - 3 - 4;
      
      while( (XTextWidth( tbar->xinfo->xfsp, text, tlen ) >= text_w)
	     && (tlen > 0) )
	 tlen--;
      carea->visible_len = tlen;

      ic->mappedp = 0;
      ipcmd_map_window( tbar, ic );

      tbar->areas = cons( area, tbar->areas );
      
      area->refresh = &refresh_cmdarea;
      area->desktop_notify = &refresh_cmdarea;
      area->key_press = &key_press_cmdarea;
      area->enter_notify = &enter_notify_cmdarea;
      area->button_press = &leave_notify_cmdarea;
   } else {
      ipcmd_map_window( tbar, ic );
   }
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    enter_notify_command ...                                         */
/*---------------------------------------------------------------------*/
static void
enter_notify_command( XEvent *ev, area_t *ar ) {
   taskbar_t *tbar = ar->taskbar;

   tooltips_setup( "Run user command",
		   ar->x,
		   tbar->top ? tbar->y + tbar->height : tbar->y - tbar->height,
		   TOOLTIPS );
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    leave_notify_command ...                                         */
/*---------------------------------------------------------------------*/
static void
leave_notify_command( XEvent *ev, area_t *ar ) {
   tooltips_hide();
}

/*---------------------------------------------------------------------*/
/*    area_t *                                                         */
/*    make_command ...                                                 */
/*---------------------------------------------------------------------*/
area_t *
make_command( taskbar_t *tbar, char *image, int pos, int width, int height, int cwidth ) {
   ipcmd_t *ic = calloc( 1, sizeof( ipcmd_t ) );
   int relief = tbar->config->relief;
   area_t *ar = &(ic->area);
   char *path;
   Pixmap pix;
   int x, y;
   unsigned int w, h, d, bw;
   
   if( !ar ) exit( 10 );

   /* initialize the command */
   ar->win = make_area_window( tbar );
   ar->name = "command";

   ar->uwidth = width;
   ar->uheight = height;

   ic->relief_mask = relief ? RELIEF_TOP | RELIEF_BOTTOM | pos : pos;
   ic->carea = 0L;
   ic->cwidth = cwidth;
   
   /* the image */
   if( path = find_icon( tbar->config, image ) ) {
      XpmReadFileToPixmap( tbar->xinfo->disp, ar->win, path,
			   &(ic->icon), &(ic->mask),
			   NULL );
   }

   /* the position of the image in the area */
   XGetGeometry( tbar->xinfo->disp, ic->icon, &pix, &x, &y, &w, &h, &bw, &d );

   ic->iconx = width <= w ? 0 : (width - w) / 2;
   ic->icony = height <= h ? 0 : (height - h) / 2 - 1;

   /* bind the area in the taskbar */
   ar->taskbar = tbar;
   tbar->areas = cons( ar, tbar->areas );
   
   ar->refresh = &refresh_command;
   ar->desktop_notify = &refresh_command;
   ar->button_press = &button_press_command;
   ar->enter_notify = &enter_notify_command;
   ar->leave_notify = &leave_notify_command;

   return ar;
}

