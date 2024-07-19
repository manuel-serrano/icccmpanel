/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/icons.c                            */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Thu Jul 22 15:21:17 2004                          */
/*    Last change :  Fri Jul 19 09:24:29 2024 (serrano)                */
/*    Copyright   :  2004-24 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    The icons.                                                       */
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
#include "icons.h"

/*---------------------------------------------------------------------*/
/*    XCLICON_IMAGE_PADDING ...                                        */
/*---------------------------------------------------------------------*/
#define XCLICON_IMAGE_PADDING 2

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    xcli_map_window ...                                              */
/*---------------------------------------------------------------------*/
void
xcli_map_window(taskbar_t *tbar, xclicon_t *xcli) {
   if (!xcli->mappedp) {
      xcli->mappedp = 1;
      XMapWindow(tbar->xinfo->disp, ((area_t *)xcli)->win);
   }
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    xcli_unmap_window ...                                            */
/*---------------------------------------------------------------------*/
void
xcli_unmap_window(taskbar_t *tbar, xclicon_t *xcli) {
   if (xcli->mappedp) {
      xcli->mappedp = 0;
      XUnmapWindow(tbar->xinfo->disp, ((area_t *)xcli)->win);
   }
}

/*---------------------------------------------------------------------*/
/*    char                                                             */
/*    xclicon_showp ...                                                */
/*---------------------------------------------------------------------*/
char
xclicon_showp(xclicon_t *xcli, ipicons_t *ip, taskbar_t *tbar) {
   return ((ip->icon_all_desktop || (tbar->desktop == xcli->xcl->desktop))
	   &&
	   (ip->icon_mapped || xcli->xcl->unmappedp));
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    refresh_xclicon ...                                              */
/*---------------------------------------------------------------------*/
void
refresh_xclicon(area_t *ar) {
   taskbar_t *tbar = ar->taskbar;
   Xinfo_t *xinfo = tbar->xinfo;
   xclicon_t *xcli = (xclicon_t *)ar;
   config_t *config = tbar->config;
   int relief = config->relief;

   if (xclicon_showp(xcli, (ipicons_t *)(ar->parent), tbar)) {
      char *name = xcli->xcl->name;
      int len = strlen(name);
      Pixmap icon = xcli->xcl->icon;
      int text_x, text_y, text_w;
      int image_width = ar->height;
      int image_height = ar->height;
      int w = ar->width - 2 * tbar->aborder - 1;
      int h = ar->height + (relief ? (2 * tbar->aborder) : 0);
      int step = 6;
      int grey = (0xa0 + step*h) > 255 ? 255 - step*h : 0xa0;

      /* the new dimension of the window area */
      XMoveResizeWindow(xinfo->disp, ar->win, ar->x, 0, ar->width, ar->height);
   
      /* the background */
      draw_gradient(xinfo, ar->win,
		    tbar->aborder, relief,
		    w, h,
		    mstk_grey_palette, 0,
		    config->gradient_step, config->gradient_substep);

      /* the position of the texts */
      text_w = ar->width;
      text_y = xinfo->xfsb->ascent + ((ar->height - xinfo->xfsb->ascent) / 3);

      /* draw the icon and compute the position of the icon name */
      if (icon &&
	  (text_w > (image_width+2*XCLICON_IMAGE_PADDING+(2*tbar->aborder)))) {
	 Pixmap mask = xcli->xcl->mask;
	 int w = xcli->xcl->icon_width;
	 int h = xcli->xcl->icon_height;
      
	 draw_pixmap(xinfo, ar->win, icon, mask,
		     xcli->icon_x, xcli->icon_y, xcli->icon_w, xcli->icon_h);

	 text_x = image_width + 2 * XCLICON_IMAGE_PADDING + tbar->aborder;
	 text_w = ar->width -
	    (image_width + 2 * XCLICON_IMAGE_PADDING + (2 * tbar->aborder));
      } else {
	 text_x = XCLICON_IMAGE_PADDING + tbar->aborder;
	 text_w = ar->width - (XCLICON_IMAGE_PADDING + (2 * tbar->aborder));
      }

      /* compute and draw the name of the icon */
      while ((XTextWidth(xinfo->xfsb, name, len) >=
	      (text_w - 2)) && (len > 0))
	 len--;

      /* draw the name of the xclient */
      if (xcli->xcl->unmappedp) {
	 draw_text(xinfo, ar->win, text_x, text_y, name, len, 0,
		   ar->active ? ACTIVE : BLACK,
		   tbar->config->color_shadow, tbar->config->shadow_size);
      } else {
	 draw_text(xinfo, ar->win, text_x, text_y, name, len, 0,
		   GREY9,
		   tbar->config->color_shadow, tbar->config->shadow_size);
      }
   
      /* the icon border */
      if (relief) {
	 draw_relief(xinfo, ar->win,
		     0, 0,
		     ar->width - 1, ar->height - 1,
		     0, WHITE, GREY9, tbar->aborder);
      } else {
	 draw_partial_relief(xinfo, ar->win, RELIEF_LEFT | RELIEF_RIGHT,
			      0, 0,
			      ar->width - 1, ar->height + 2,
			      0, WHITE, GREY9, tbar->aborder);
      }
   }
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    state_notify_xclicon ...                                         */
/*---------------------------------------------------------------------*/
static void
state_notify_xclicon(area_t *ar, xclient_t *xcl) {
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    button_press_xclicon ...                                         */
/*---------------------------------------------------------------------*/
static void
button_press_xclicon(XEvent *ev, area_t *ar) {
   taskbar_t *tbar = ar->taskbar;
   xclicon_t *xcli = (xclicon_t *)ar;
   Window win = xcli->xcl->win;
   Display *disp = tbar->xinfo->disp;

   if (xcli->xcl->unmappedp)
      window_deiconify(disp, win);
   else
      XIconifyWindow(disp, win, tbar->xinfo->screen);
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    enter_notify_xclicon ...                                         */
/*---------------------------------------------------------------------*/
static void
enter_notify_xclicon(XEvent *ev, area_t *ar) {
   taskbar_t *tbar = ar->taskbar;
   xclicon_t *xcli = (xclicon_t *)ar;
   ar->active = 1;
   refresh_xclicon(ar);
   
   tooltips_setup(xcli->xcl->name,
		   ar->x + ar->parent->x, 
		   tbar->top ? tbar->y + tbar->height : tbar->y - tbar->height,
		   TOOLTIPS);
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    leave_notify_xclicon ...                                         */
/*---------------------------------------------------------------------*/
static void
leave_notify_xclicon(XEvent *ev, area_t *ar) {
   xclicon_t *xcli = (xclicon_t *)ar;
   ar->active = 0;
   refresh_xclicon(ar);
   tooltips_hide();
}

/*---------------------------------------------------------------------*/
/*    static xclicon_t *                                               */
/*    fill_xclicon ...                                                 */
/*---------------------------------------------------------------------*/
static xclicon_t *
fill_xclicon(xclicon_t *xcli, area_t *parent, xclient_t *xcl) {
   area_t *ar = (area_t *)xcli;
   taskbar_t *tbar = parent->taskbar;
   Pixmap icon = xcl->icon;
   
   /* the parent of the icon */
   ar->parent = parent;
   
   /* connect the xclicon and xcl */
   ((xclicon_t *)ar)->xcl = xcl;
   xcl->user[ ((ipicons_t *)parent)->id ] = xcli;
   
   ar->ignore_layout = 1;
   ar->uwidth = 0;
   ar->uheight = 0;

   /* bind the area in the taskbar */
   ar->taskbar = tbar;
   tbar->areas = cons(ar, tbar->areas);

   ar->refresh = &refresh_xclicon;
   ar->state_notify = &state_notify_xclicon;
   ar->button_press = &button_press_xclicon;
   ar->enter_notify = &enter_notify_xclicon;
   ar->leave_notify = &leave_notify_xclicon;

   if (icon) {
      Pixmap pix;
      int x, y;
      unsigned int w, h, d, bw;
      
      XGetGeometry(tbar->xinfo->disp, icon, &pix, &x, &y, &w, &h, &bw, &d);

      xcli->icon_x = tbar->aborder + XCLICON_IMAGE_PADDING;
      xcli->icon_y = parent->height <= h ? 0 : (parent->height - h) / 2;
      xcli->icon_w = w;
      xcli->icon_h = h;
   }
   
   return xcli;
}

/*---------------------------------------------------------------------*/
/*    static area_t *                                                  */
/*    make_xclicon ...                                                 */
/*---------------------------------------------------------------------*/
static xclicon_t *
make_xclicon(area_t *parent, xclient_t *xcl) {
   area_t *ar = calloc(1, sizeof(xclicon_t));
   xclicon_t *xcli = ((xclicon_t *)ar);
   taskbar_t *tbar = parent->taskbar;
   
   ar->win = make_area_window_parent(tbar, parent->win);
   ar->name = "xclicon";
   
   xcli_map_window(tbar, xcli);

   if (!xcli) exit(10);

   return fill_xclicon(xcli, parent, xcl);
}

/*---------------------------------------------------------------------*/
/*    static xclicon_t *                                               */
/*    bind_xclicon ...                                                 */
/*---------------------------------------------------------------------*/
static xclicon_t *
bind_xclicon(ipicons_t *ip, xclient_t *xcl) {
   if (NULLP(ip->_freexclicons)) {
      xclicon_t *xcli = make_xclicon((area_t *)ip, xcl);

      ip->xclicons = cons(xcli, ip->xclicons);

      return xcli;
   } else {
      pair_t *lst = ip->_freexclicons;
      xclicon_t *xcli = (xclicon_t *)CAR(ip->_freexclicons);
      taskbar_t *tbar = ((area_t *)ip)->taskbar;
   
      fill_xclicon(xcli, (area_t *)ip, xcl);
      xcli_map_window(tbar, xcli);
      
      ip->_freexclicons = CDR(lst);
      SET_CDR(lst, ip->xclicons);
      ip->xclicons = lst;

      return xcli;
   }
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    refresh_icons ...                                                */
/*---------------------------------------------------------------------*/
static void
refresh_icons(area_t *ar) {
   taskbar_t *tbar = ar->taskbar;

   /* empty space gradient */
   draw_gradient(tbar->xinfo, ar->win,
		  0, 0, ar->width - 1, ar->height,
		  0,
		  GREY12, 0, 1);
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    refresh_xclients ...                                             */
/*---------------------------------------------------------------------*/
static void
refresh_xclients(area_t *ar) {
   ipicons_t *ip = (ipicons_t *)ar;
   taskbar_t *tbar = ar->taskbar;
   config_t *config = tbar->config;
   int xclwidth;
   int x;
   int xclnum = 0;
   pair_t *lst = ip->xclicons;
	    
   while(PAIRP(lst)) {
      xclicon_t *xcli = (xclicon_t *)CAR(lst);

      if (xclicon_showp(xcli, ip, tbar)) {
	 xclnum++;
	 if (!xcli->mappedp) {
	    xcli_map_window(tbar, xcli);
	 }
      } else {
	 if (xcli->mappedp) {
	    xcli_unmap_window(tbar, xcli);
	 }
      }
      lst = CDR(lst);
   }

   if (xclnum) {
      int maxwidth = tbar->width / 10;
      xclwidth = ar->width / xclnum;

      if (xclwidth > maxwidth) xclwidth = maxwidth;
      x = (xclwidth * xclnum);

      lst = (ip->icon_mapped ? ip->xclicons : ip->xclistack);
      
      while(PAIRP(lst)) {
	 area_t *xcli = (area_t *)CAR(lst);
      
	 if (xclicon_showp((xclicon_t *)CAR(lst), ip, tbar)) {
	    x -= xclwidth;

	    xcli->width = xclwidth;
	    xcli->height = ar->height;
	    xcli->x = x;
	    xcli->y = 0;

	    xcli->refresh(xcli);
	 }
      
	 lst = CDR(lst);
      }
   }
}
   
/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    create_notify_icons ...                                          */
/*---------------------------------------------------------------------*/
static void
create_notify_icons(area_t *ar, xclient_t *xcl) {
   ipicons_t *ip = (ipicons_t *)ar;
   taskbar_t *tbar = ar->taskbar;
   Window win = xcl->win;
   xclicon_t *xcli = bind_xclicon(ip, xcl);

   if (xcl->unmappedp) {
      ip->xclistack = cons(xcli, ip->xclistack);
   }
   
   refresh_xclients(ar);
}   

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    destroy_notify_icons ...                                         */
/*---------------------------------------------------------------------*/
static void
destroy_notify_icons(area_t *ar, xclient_t *xcl) {
   taskbar_t *tbar = ar->taskbar;
   ipicons_t *ip = (ipicons_t *)ar;
   xclicon_t *xcli =  xcl->user[ip->id];

   xcli_unmap_window(tbar, xcli);
   
   /* remove the client from the active list */
   ip->xclicons = remq(xcli, ip->xclicons);
   ip->_freexclicons = cons(xcli, ip->_freexclicons);

   if (xcl->unmappedp) {
      ip->xclistack = remq(xcli, ip->xclistack);
   }

   /* refresh the icon area */
   refresh_xclients(ar);
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    state_notify_icons ...                                           */
/*---------------------------------------------------------------------*/
static void
state_notify_icons(area_t *ar, xclient_t *xcl) {
   ipicons_t *ip = (ipicons_t *)ar;
   pair_t *lst = ip->xclicons;
   taskbar_t *tbar = ar->taskbar;
   xclicon_t *xcli =  xcl->user[ ip->id ];

   if (xcl->unmappedp) {
      if (!memq(xcli, ip->xclistack)) {
	 ip->xclistack = cons(xcli, ip->xclistack);
      }
   }
   else
      ip->xclistack = remq(xcli, ip->xclistack);
   
   if (ip->icon_mapped) {
      while(PAIRP(lst)) {
	 area_t *xcli = (area_t *)CAR(lst);
      
	 if (((xclicon_t *)xcli)->xcl == xcl) {
	    xcli->refresh(xcli);
	    break;
	 }
	 lst = CDR(lst);
      }
   } else {
      refresh_xclients(ar);
   }
}

/*---------------------------------------------------------------------*/
/*    area_t *                                                         */
/*    make_icons ...                                                   */
/*---------------------------------------------------------------------*/
area_t *
make_icons(taskbar_t *tbar, int width, int height, char im, char iad) {
   area_t *ar = calloc(1, sizeof(ipicons_t));
   ipicons_t *ip = (ipicons_t *)ar;

   if (!ar) exit(10);

   /* initialize the icons */
   ar->win = make_area_window(tbar);
   XMapWindow(tbar->xinfo->disp, ar->win);
   
   ar->name = "icons";

   ar->uwidth = width;
   ar->uheight = height;

   /* bind the area in the taskbar */
   ar->taskbar = tbar;
   tbar->areas = cons(ar, tbar->areas);

   ar->refresh = &refresh_icons;
   ar->desktop_notify = &refresh_xclients;
   ar->create_notify = &create_notify_icons;
   ar->destroy_notify = &destroy_notify_icons;
   ar->state_notify = &state_notify_icons;

   ip->id = taskbar_get_xclient_manager_number(tbar);

   ip->xclicons = NIL;
   ip->_freexclicons = NIL;
   ip->xclistack = NIL;
   
   ip->icon_mapped = im;
   ip->icon_all_desktop = iad;
   
   return ar;
}

/*---------------------------------------------------------------------*/
/*    void *                                                           */
/*    start_icons ...                                                  */
/*---------------------------------------------------------------------*/
void *
start_icons(void *tb, pair_t *args) {
   taskbar_t *tbar = (taskbar_t *)tb;
   config_t *config = tbar->config;
   int width = INTEGER_VAL(CAR(args));
   char im =  !SYMBOL_EQ(CADR(args), sym_false);
   char iad =  !SYMBOL_EQ(CAR(CDDR(args)), sym_false);

   return make_icons(tbar, width, config->taskbar_height - 1, im, iad);
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    parse_icons ...                                                  */
/*---------------------------------------------------------------------*/
void
parse_icons(config_t *config, pair_t *lst) {
   pair_t *l = CDR(lst);
   integer_t *width = make_integer(40);
   pair_t *args;
   symbol_t *sym_mapped = make_symbol(":mapped");
   symbol_t *sym_all_desktop = make_symbol(":all-desktop");
   symbol_t *im = sym_false, *iad = sym_false;

   /* search of a command */
   while(PAIRP(l)) {
      obj_t *car = CAR(l);
      if (SYMBOLP(car)) {
	 if (SYMBOL_EQ((symbol_t *)car, sym_width)) {
	    width = parse_cadr_integer(l);

	    if (!width) {
	       parse_error("Illegal :width", (obj_t *)lst);
	    } else {
	       l = CDR(l);
	    }
	 } else {
	    if (SYMBOL_EQ((symbol_t *)car, sym_mapped)) {
	       im = parse_cadr_symbol(l);
	       l = CDR(l);
	    } else {
	       if (SYMBOL_EQ((symbol_t *)car, sym_all_desktop)) {
		  iad = parse_cadr_symbol(l);
		  l = CDR(l);
	       } else {
		  parse_error("Illegal icons", (obj_t *)lst);
	       }
	    }
	 }
	 l = CDR(l);
      } else {
	 parse_error("Illegal icons", (obj_t *)lst);
      }
   }

   args = cons(iad, NIL);
   args = cons(im, args);
   args = cons(width, args);
			     
   register_plugin(config, make_plugin(start_icons, args));
}

/*---------------------------------------------------------------------*/
/*    char                                                             */
/*    iconp ...                                                        */
/*---------------------------------------------------------------------*/
char
iconp(area_t *ar) {
   return ar->refresh == refresh_xclicon;
}
