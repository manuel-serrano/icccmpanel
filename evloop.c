/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/evloop.c                           */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Fri Jul 23 05:59:11 2004                          */
/*    Last change :  Sat Dec 21 06:28:48 2024 (serrano)                */
/*    Copyright   :  2004-24 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    Event loop                                                       */
/*=====================================================================*/
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
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
/*    debug                                                            */
/*---------------------------------------------------------------------*/
static long debug_cnst = 0;

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
gcd(long a, long b) {
   while (b != 0) {
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
evloop_timeout(area_t *ar) {
   timeout_areas = cons(ar, timeout_areas);

   if (ar->timeout_delay < timeout_gcd) {
      timeout_gcd = ar->timeout_delay;
   } else {
      timeout_gcd = gcd(timeout_gcd, ar->timeout_delay);
   }
}

/*---------------------------------------------------------------------*/
/*    static area_t *                                                  */
/*    enter_area ...                                                   */
/*---------------------------------------------------------------------*/
static area_t *
enter_area(XEvent *ev, area_t *newa, area_t *olda) {
   if (newa != olda) {
      if (olda && olda->leave_notify) olda->leave_notify(ev, olda);
      if (newa && newa->enter_notify) newa->enter_notify(ev, newa);
   }
   return newa;
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    evloop ...                                                       */
/*---------------------------------------------------------------------*/
void
evloop(taskbar_t *tbar) {
   area_t *iar = 0;
   XEvent ev;
   int evt;
   XEvent elev;
   fd_set fd;
   int xfd;
   long count = timeout_gcd;
   struct timeval tv;
   
   KeyCode altr_keycode = XKeysymToKeycode(tbar->xinfo->disp,XK_Alt_R);
   KeyCode altl_keycode = XKeysymToKeycode(tbar->xinfo->disp,XK_Alt_L);
   unsigned int ignored_modmask = 0; // stub
    
   xfd = ConnectionNumber(tbar->xinfo->disp);
   XSelectInput(tbar->xinfo->disp, tbar->xinfo->root_win,
		//ButtonPressMask
		KeyPressMask | KeyReleaseMask
		| ExposureMask | PropertyChangeMask
		| EnterWindowMask | LeaveWindowMask | PointerMotionMask
		| StructureNotifyMask);
   
   XGrabKey(tbar->xinfo->disp,
            altr_keycode,
            Mod1Mask,
	    tbar->xinfo->root_win, True,
	    GrabModeAsync, GrabModeAsync);
   XGrabKey(tbar->xinfo->disp,
            altl_keycode,
            Mod1Mask,
	    tbar->xinfo->root_win, True,
	    GrabModeAsync, GrabModeAsync);
   
   while (1) {
      area_t *ar;

      if (NULLP(timeout_areas)) {
	 FD_ZERO(&fd);
	 FD_SET(xfd, &fd);
	 select(xfd + 1, &fd, 0, 0, 0);
      } else {
	 struct timeval tv;

	 tv.tv_usec = 100000;
	 tv.tv_sec = 0;
	 FD_ZERO(&fd);
	 FD_SET(xfd, &fd);
	 
	 if (select (xfd + 1, &fd, 0, 0, &tv) == 0) {
	    pair_t *lst = timeout_areas;
	    pair_t *prev = 0;

	    if (--count <= 0) {
	       count = timeout_gcd;
	       
	       while (PAIRP(lst)) {
		  area_t *ar = (area_t *)CAR(lst);

		  if (ar->timeout_delay > 0) {
		     if (ar->timeout_count > 0) {
			ar->timeout_count -= timeout_gcd;
			prev = lst;
			lst = CDR(lst);
			continue;
		     } else {
			ar->timeout_count = (ar->timeout_delay - 10);
		     }
		  }

		  if (ar->timeout(ar)) {
		     prev = lst;
		     lst = CDR(lst);
		  } else {
		     lst = CDR(lst);
		     if (!prev) {
			timeout_areas = lst;
		     } else {
			CDR(prev) = lst;
		     }
		     free(lst);
		  }
	       }
	    }
	 }
      }

      // the evt variable is used to delay Leave/Notify event until
      // no pending events are to be processed
      evt = 0;

      while (XPending(tbar->xinfo->disp)) {
	 XNextEvent(tbar->xinfo->disp, &ev);

	 switch(ev.type) {
	    case ButtonPress:
	       // reset the possible timeout
	       ar = find_area(tbar, ev.xbutton.window);
	       if (ar) {
		  if (ar->timeout_delay > 0) {
		     ar->timeout_count = (ar->timeout_delay - 10);
		  }
		  if (ar->button_press) ar->button_press(&ev, ar);
	       }
	       break;

	    case KeyPress:
	       ar = find_area(tbar, ev.xkey.window);
	       if (ar && ar->key_press) {
		  ar->key_press(&ev, ar);
	       }
	       break;

	    case KeyRelease:
	       break;
	       
	    case DestroyNotify:
	       taskbar_destroy_notify(tbar, &ev);
	       break;

	    case Expose:
	    case UnmapNotify: {
	       Window win =
		  ev.type == Expose ? ev.xexpose.window : ev.xunmap.window;

	       if (tbar->win == win) {
		  taskbar_refresh(tbar);
	       } else {
		  if (tooltips_windowp(win)) {
		     tooltips_refresh();
		  } else {
		     ar = find_area(tbar, win);
		     if (ar) ar->refresh(ar);
		  }
	       }
	       break;
	    }

	    case PropertyNotify:
	       taskbar_property_notify(tbar, &ev);
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
	       ar = find_area(tbar, ev.xmotion.window);
	       iar = enter_area(&ev, ar, iar);
	       if (tbar->config->mouse_shaker_speed > 0) {
		  show_shaker(((XMotionEvent *)&ev)->time, tbar);
	       }
	       break;

	    case ClientMessage:
	       ar = find_area(tbar, ev.xclient.window);
	       if (ar && ar->client_message) ar->client_message(&ev, ar);
	       break;

	    default:
	       ;
	 }
      }

      // once all pending events have been handled, we process
      // the last leave/enter event for the tbar window
      switch(evt) {
	 case EnterNotify:
	    if (tbar->hiddenp) {
	       taskbar_unhide(tbar);
	    } else {
	       ar = find_area(tbar, ev.xcrossing.window);
	       iar = enter_area(&elev, ar, iar);
	    }
	    
	    break;

	 case LeaveNotify: 
	    iar = enter_area(&elev, 0, iar);
	    
	    tooltips_hide();
	    if (tbar->autohide) taskbar_hide(tbar);
	    break;
      }
   }
}

/*---------------------------------------------------------------------*/
/*    char *                                                           */
/*    x_atom_name ...                                                  */
/*---------------------------------------------------------------------*/
char *
x_atom_name(Atom at) {
   static char buf[20];
   
   switch((long)at) {
      case XA_PRIMARY: return "XA_PRIMARY";
      case XA_SECONDARY: return "XA_SECONDARY";
      case XA_ARC: return "XA_ARC";
      case XA_ATOM: return "XA_ATOM";
      case XA_BITMAP: return "XA_BITMAP";
      case XA_CARDINAL: return "XA_CARDINAL";
      case XA_COLORMAP: return "XA_COLORMAP";
      case XA_CURSOR: return "XA_CURSOR";
      case XA_CUT_BUFFER0: return "XA_CUT_BUFFER0";
      case XA_CUT_BUFFER1: return "XA_CUT_BUFFER1";
      case XA_CUT_BUFFER2: return "XA_CUT_BUFFER2";
      case XA_CUT_BUFFER3: return "XA_CUT_BUFFER3";
      case XA_CUT_BUFFER4: return "XA_CUT_BUFFER4";
      case XA_CUT_BUFFER5: return "XA_CUT_BUFFER5";
      case XA_CUT_BUFFER6: return "XA_CUT_BUFFER6";
      case XA_CUT_BUFFER7: return "XA_CUT_BUFFER7";
      case XA_DRAWABLE: return "XA_DRAWABLE";
      case XA_FONT: return "XA_FONT";
      case XA_INTEGER: return "XA_INTEGER";
      case XA_PIXMAP: return "XA_PIXMAP";
      case XA_POINT: return "XA_POINT";
      case XA_RECTANGLE: return "XA_RECTANGLE";
      case XA_RESOURCE_MANAGER: return "XA_RESOURCE_MANAGER";
      case XA_RGB_COLOR_MAP: return "XA_RGB_COLOR_MAP";
      case XA_RGB_BEST_MAP: return "XA_RGB_BEST_MAP";
      case XA_RGB_BLUE_MAP: return "XA_RGB_BLUE_MAP";
      case XA_RGB_DEFAULT_MAP: return "XA_RGB_DEFAULT_MAP";
      case XA_RGB_GRAY_MAP: return "XA_RGB_GRAY_MAP";
      case XA_RGB_GREEN_MAP: return "XA_RGB_GREEN_MAP";
      case XA_RGB_RED_MAP: return "XA_RGB_RED_MAP";
      case XA_STRING: return "XA_STRING";
      case XA_VISUALID: return "XA_VISUALID";
      case XA_WINDOW: return "XA_WINDOW";
      case XA_WM_COMMAND: return "XA_WM_COMMAND";
      case XA_WM_HINTS: return "XA_WM_HINTS";
      case XA_WM_CLIENT_MACHINE: return "XA_WM_CLIENT_MACHINE";
      case XA_WM_ICON_NAME: return "XA_WM_ICON_NAME";
      case XA_WM_ICON_SIZE: return "XA_WM_ICON_SIZE";
      case XA_WM_NAME: return "XA_WM_NAME";
      case XA_WM_NORMAL_HINTS: return "XA_WM_NORMAL_HINTS";
      case XA_WM_SIZE_HINTS: return "XA_WM_SIZE_HINTS";
      case XA_WM_ZOOM_HINTS: return "XA_WM_ZOOM_HINTS";
      case XA_MIN_SPACE: return "XA_MIN_SPACE";
      case XA_NORM_SPACE: return "XA_NORM_SPACE";
      case XA_MAX_SPACE: return "XA_MAX_SPACE";
      case XA_END_SPACE: return "XA_END_SPACE";
      case XA_SUPERSCRIPT_X: return "XA_SUPERSCRIPT_X";
      case XA_SUPERSCRIPT_Y: return "XA_SUPERSCRIPT_Y";
      case XA_SUBSCRIPT_X: return "XA_SUBSCRIPT_X";
      case XA_SUBSCRIPT_Y: return "XA_SUBSCRIPT_Y";
      case XA_UNDERLINE_POSITION: return "XA_UNDERLINE_POSITION";
      case XA_UNDERLINE_THICKNESS: return "XA_UNDERLINE_THICKNESS";
      case XA_STRIKEOUT_ASCENT: return "XA_STRIKEOUT_ASCENT";
      case XA_STRIKEOUT_DESCENT: return "XA_STRIKEOUT_DESCENT";
      case XA_ITALIC_ANGLE: return "XA_ITALIC_ANGLE";
      case XA_X_HEIGHT: return "XA_X_HEIGHT";
      case XA_QUAD_WIDTH: return "XA_QUAD_WIDTH";
      case XA_WEIGHT: return "XA_WEIGHT";
      case XA_POINT_SIZE: return "XA_POINT_SIZE";
      case XA_RESOLUTION: return "XA_RESOLUTION";
      case XA_COPYRIGHT: return "XA_COPYRIGHT";
      case XA_NOTICE: return "XA_NOTICE";
      case XA_FONT_NAME: return "XA_FONT_NAME";
      case XA_FAMILY_NAME: return "XA_FAMILY_NAME";
      case XA_FULL_NAME: return "XA_FULL_NAME";
      case XA_CAP_HEIGHT: return "XA_CAP_HEIGHT";
      case XA_WM_CLASS: return "XA_WM_CLASS";
      case XA_WM_TRANSIENT_FOR: return "XA_WM_TRANSIENT_FOR";
      default: sprintf(buf, "XA_UNKNOWN (%d)", (long)at); return buf;
   }
}

/*---------------------------------------------------------------------*/
/*    char *                                                           */
/*    x_event_name ...                                                 */
/*---------------------------------------------------------------------*/
char *
x_event_name(XEvent *ev) {
   static char buf[30];
   
   switch(ev->type) {
      case KeyPress: return "KeyPress";
      case KeyRelease: return "KeyRelease";
      case ButtonPress: return "ButtonPress";
      case ButtonRelease: return "ButtonRelease";
      case MotionNotify: return "MotionNotify";
      case EnterNotify: return "EnterNotify";
      case LeaveNotify: return "LeaveNotify";
      case FocusIn: return "FocusIn";
      case FocusOut: return "FocusOut";
      case KeymapNotify: return "KeymapNotify";
      case Expose: return "Expose";
      case GraphicsExpose: return "GraphicsExpose";
      case NoExpose: return "NoExpose";
      case VisibilityNotify: return "VisibilityNotify";
      case CreateNotify: return "CreateNotify";
      case DestroyNotify: return "DestroyNotify";
      case UnmapNotify: return "UnmapNotify";
      case MapNotify: return "MapNotify";
      case MapRequest: return "MapRequest";
      case ReparentNotify: return "ReparentNotify";
      case ConfigureNotify: return "ConfigureNotify";
      case ConfigureRequest: return "ConfigureRequest";
      case GravityNotify: return "GravityNotify";
      case ResizeRequest: return "ResizeRequest";
      case CirculateNotify: return "CirculateNotify";
      case CirculateRequest: return "CirculateRequest";
      case PropertyNotify: return "PropertyNotify";
      case SelectionClear: return "SelectionClear";
      case SelectionRequest: return "SelectionRequest";
      case SelectionNotify: return "SelectionNotify";
      case ColormapNotify: return "ColormapNotify";
      case ClientMessage: return "ClientMessage";
      case MappingNotify: return "MappingNotify";
      case GenericEvent: return "GenericEvent";
      case LASTEvent: return "LASTEvent";
      default: sprintf(buf, "UNKNOWNevent (%d)", (long)ev->type); return buf;
   }
}
