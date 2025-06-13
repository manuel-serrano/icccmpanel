/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/taskbar.c                          */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Thu Jul 22 14:32:38 2004                          */
/*    Last change :  Fri Jun 13 16:30:17 2025 (serrano)                */
/*    Copyright   :  2004-25 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    Taskbar management                                               */
/*=====================================================================*/
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifdef HAVE_XPM
#  include <X11/xpm.h>
#endif

#include "icccmpanel.h"

/*---------------------------------------------------------------------*/
/*    xclient_t *                                                      */
/*    window_xclient ...                                               */
/*    -------------------------------------------------------------    */
/*    Retreive a xclient structure from a X window.                    */
/*---------------------------------------------------------------------*/
xclient_t *
window_xclient(taskbar_t *tbar, Window w) {
   pair_t *ts = tbar->xclients;

   while (PAIRP(ts)) {
      xclient_t *t = (xclient_t *)CAR(ts);

      if (t->win == w)
	 return t;

      ts = CDR(ts);
   }

   return 0L;
}

/*---------------------------------------------------------------------*/
/*    static char                                                      */
/*    find_user_icon ...                                               */
/*    -------------------------------------------------------------    */
/*    Search an icon image according to the xclient class and name.    */
/*---------------------------------------------------------------------*/
static char
find_user_icon(xclient_t *xcl, taskbar_t *tbar) {
   char *class = xcl->class;
   char *name = xcl->name;
   pair_t *lst = tbar->iconcache;

   while (PAIRP(lst)) {
      iconentry_t *en = CAR(lst);

      if (class && !regexec(&(en->classreg), class, 0, 0, 0)) {
	 if (!en->icondescr->name ||
	     (name && !regexec(&(en->namereg), name, 0, 0, 0))) {
	    if (!en->loaded) {
	       en->loaded = 1;
	       XpmReadFileToPixmap(tbar->xinfo->disp, tbar->win,
				   en->icondescr->filename,
				   &(en->icon), &(en->mask),
				   NULL);
 	    }

	    xcl->icon = en->icon;
	    xcl->mask = en->mask;

	    if (xcl->icon) {
	       Pixmap pix;
	       int x, y;
	       unsigned int w, h, d, bw;

	       XGetGeometry(tbar->xinfo->disp,
			    xcl->icon, &pix, &x, &y, &w, &h, &bw, &d);
	       xcl->icon_width = w;
	       xcl->icon_height = h;
	       xcl->icon_shared = 1;
	    }

	    return 1;
	 }
      }

      lst = CDR(lst);
   }

   return 0;
}

/*---------------------------------------------------------------------*/
/*    static xclient_t *                                               */
/*    update_xclient_icon ...                                          */
/*---------------------------------------------------------------------*/
static xclient_t *
update_xclient_icon(xclient_t *xcl, taskbar_t *tbar, Window win) {
   char *in;
   int icon_size = tbar->config->icon_size;

   if (xcl->icon == 0) {
      if (!find_user_icon(xcl, tbar)) {
	 if (!window_netwm_icon(tbar->xinfo, xcl->win,
				&(xcl->icon), &(xcl->mask),
				icon_size)) {
	    if (!window_hint_icon(tbar->xinfo, xcl->win,
				  &(xcl->icon), &(xcl->mask),
				  icon_size)) {
	       xcl->icon = xcl->mask = 0;
	    }
	 }
      } else {
	 if (tbar->config->update_netwmicon) {
	    window_update_netwm_icon(tbar->xinfo, xcl->win, xcl->name, 
				     &(xcl->icon), &(xcl->mask),
				     icon_size);
	 }
      }
   }
   return xcl;
}

/*---------------------------------------------------------------------*/
/*    static xclient_t *                                               */
/*    update_xclient_state ...                                         */
/*---------------------------------------------------------------------*/
static xclient_t *
update_xclient_state(xclient_t *xcl, taskbar_t *tbar, Window win) {
   xcl->desktop = window_desktop(tbar->xinfo->disp, win);
   xcl->unmappedp = window_iconifiedp(tbar->xinfo->disp, win);

   if (tbar->config->update_netwmicon) {
      if (xcl->unmappedp) update_xclient_icon(xcl, tbar, win);
   }
   
   return xcl;
}

/*---------------------------------------------------------------------*/
/*    static xclient_t *                                               */
/*    fill_xclient ...                                                 */
/*---------------------------------------------------------------------*/
static xclient_t *
fill_xclient(xclient_t *xcl, taskbar_t *tbar, Window w) {
   xcl->win = w;
   xcl->live = 1;
   xcl->class = window_class(tbar->xinfo->disp, w);
   xcl->name = window_name(tbar->xinfo->disp, w);
   xcl->desktop = window_desktop(tbar->xinfo->disp, w);
   xcl->unmappedp = window_iconifiedp(tbar->xinfo->disp, w);
   
   if (!xcl->class || *(xcl->class) == 0) {
      xcl->class = strdup("???");
   }
   
   if (!xcl->name || *(xcl->name) == 0) {
      xcl->name = strdup(xcl->class);
   }

   return update_xclient_icon(xcl, tbar, w);
}

/*---------------------------------------------------------------------*/
/*    static xclient_t *                                               */
/*    make_xclient ...                                                 */
/*---------------------------------------------------------------------*/
static xclient_t *
make_xclient(taskbar_t *tbar, Window w) {
   static int count = 0;
   xclient_t *xcl = malloc(sizeof(xclient_t));
   memset(xcl, 0, sizeof(xclient_t));

   if (!xcl) {
      fprintf(stderr, "*** ERROR(%s:%d): cannot allocation client\n", 
	      __FILE__, __LINE__);
      return 0;
   }

   /* the xclient identifier */
   xcl->id = count++;
   
   /* general X information */
   fill_xclient(xcl, tbar, w);
   
   return xcl;
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    free_xclient ...                                                 */
/*---------------------------------------------------------------------*/
void
free_xclient(xclient_t *xcl, taskbar_t *tbar) {
   if (xcl->class) free(xcl->class);
   if (xcl->name) free(xcl->name);

   if (!xcl->icon_shared) {
      if (xcl->icon != None) XFreePixmap(tbar->xinfo->disp, xcl->icon);
      if (xcl->mask != None) XFreePixmap(tbar->xinfo->disp, xcl->mask);
   }

   xcl->class = 0;
   xcl->name = 0;
   xcl->icon = None;
   xcl->mask = None;
   
   xcl->live = 0;
   
#if ICCCMPANEL_FREE_LIST
   tbar->_freexclients = cons(xcl, tbar->_freexclients);
#else
   free(xcl);
#endif
}

/*---------------------------------------------------------------------*/
/*    xclient_t *                                                      */
/*    get_xclient ...                                                  */
/*---------------------------------------------------------------------*/
xclient_t *
get_xclient(taskbar_t *tbar, Window w) {
   if (NULLP(tbar->_freexclients)) {
      xclient_t *xcl = make_xclient(tbar, w);
      
      return xcl;
   } else {
      pair_t *lst = tbar->_freexclients;
      xclient_t *xcl = (xclient_t *)CAR(lst);

      fill_xclient(xcl, tbar, w);
      tbar->_freexclients = CDR(lst);
      free(lst);

      return xcl;
   }
}

/*---------------------------------------------------------------------*/
/*    pair_t *                                                         */
/*    make_iconcache ...                                               */
/*---------------------------------------------------------------------*/
pair_t *
make_iconcache(taskbar_t *tbar) {
   config_t *config = tbar->config;
   pair_t *lr = config->icondescrs;
   pair_t *lw = NIL;

   while (PAIRP(lr)) {
      icondescr_t *id = (icondescr_t *)CAR(lr);

      if (id->filename) {
	 iconentry_t *ie = calloc(1, sizeof(iconentry_t));

	 ie->icondescr = id;
	 ie->loaded = 0;
	  
	 if (!regcomp(&(ie->classreg), id->class, REG_NOSUB | REG_EXTENDED)) {
	    if (id->name) {
	       if (!regcomp(&(ie->namereg), id->name, REG_NOSUB | REG_EXTENDED)) {
		  lw = cons(ie, lw);
	       } else {
		  fprintf(stderr, "icccmpanel: Illegal regexp name `%s'\n",
			   id->name);
		  free(ie);
	       }
	    } else {
	       lw = cons(ie, lw);
	    }
	 } else {
	    fprintf(stderr, "icccmpanel: Illegal regexp class `%s'\n",
		     id->class);
	    free(ie);
	 }
      } else {
	 fprintf(stderr, "icccmpanel: Illegal can't find icon in path `%s'\n",
		  id->filename);
      }

      lr = CDR(lr);
   }

   return reverse(lw);
}
   
/*---------------------------------------------------------------------*/
/*    taskbar_t *                                                      */
/*    make_taskbar ...                                                 */
/*---------------------------------------------------------------------*/
taskbar_t *
make_taskbar(Xinfo_t *xinfo, config_t *config) {
   Window win;
   MWMHints mwm;
   XSizeHints size_hints;
   XWMHints wmhints;
   XSetWindowAttributes att;
   unsigned long strut[ 4 ];
   Display *disp = xinfo->disp;
   taskbar_t *tb;
   int tbar_x, tbar_y, tbar_w, tbar_h;

   if (!(tb = calloc(1, sizeof(taskbar_t))))
      return 0;

   /* enable other windows to go above the panel */
   att.override_redirect = 1;
   att.background_pixel = mstk_palette[ LIGHTGREY ];
   att.event_mask = ButtonPressMask
      | ExposureMask
      | LeaveWindowMask
      | EnterWindowMask
      | PointerMotionMask
      | KeyPressMask;

   /* taskbar x and width */
   if (config->taskbar_x >= 0) {
      tbar_x = config->taskbar_x;
      tbar_w = config->taskbar_width > 0 ? config->taskbar_width : xinfo->screen_width;

      if (tbar_x >  xinfo->screen_width)
	 tbar_x = 0;
      
      if ((tbar_w + tbar_x) > xinfo->screen_width)
	 tbar_w = xinfo->screen_width - tbar_x;
   } else {
      if (config->taskbar_width >= 0) {
	 if (config->taskbar_width <= xinfo->screen_width) {
	    tbar_w = config->taskbar_width;
	    tbar_x = (xinfo->screen_width - tbar_w) / 2;
	 }
	 else {
	    tbar_w = xinfo->screen_width;
	    tbar_x = 0;
	 }
      } else {
	    tbar_w = xinfo->screen_width;
	    tbar_x = 0;
      }
   }
   
   /* taskbar y and heght */
   if (config->taskbar_y >= 0) {
      /* top of screen */
      if (xinfo->screen_height <= config->taskbar_height) {
	 tbar_h = xinfo->screen_height;
      } else {
	 tbar_h = config->taskbar_height;
      }
      if ((xinfo->screen_height + tbar_h) <= config->taskbar_y) {
	 tbar_y >= 0;
      } else {
	 tbar_y = config->taskbar_y;
      }
   } else {
      /* bottom of screen */
      if (xinfo->screen_height <= config->taskbar_height) {
	 tbar_h = xinfo->screen_height;
	 tbar_y = 0;
      } else {
	 tbar_h = config->taskbar_height;
	 tbar_y = xinfo->screen_height - tbar_h;
      }
   }

   /* the X window */
   win = XCreateWindow(/* display */ disp,
		       /* parent  */ xinfo->root_win,
		       /* x       */ tbar_x,
		       /* y       */ tbar_y,
		       /* width   */ tbar_w,
		       /* height  */ tbar_h,
		       /* border  */ 0,
		       /* depth   */ CopyFromParent,
		       /* class   */ InputOutput,
		       /* visual  */ CopyFromParent,
		       /* vmask   */ CWBackPixel | CWEventMask | CWOverrideRedirect,
		       /* attribs */ &att);

#if DEBUG
   fprintf(stderr, "%s:%d taskbar initialized.\n", __FILE__, __LINE__);
#endif
   
   /* reserve "WINHEIGHT" pixels at the bottom of the screen */
   strut[ 0 ] = 0;
   strut[ 1 ] = 0;
   strut[ 2 ] = 0;
   strut[ 3 ] = tbar_h;
   XChangeProperty(disp, win, atom__NET_WM_STRUT, XA_CARDINAL, 32,
		    PropModeReplace, (unsigned char *) strut, 4);
#if DEBUG
   fprintf(stderr, "%s:%d props set.\n", __FILE__, __LINE__);
#endif

   /* reside on ALL desktops */
   set_window_prop(disp, win, atom__NET_WM_DESKTOP, XA_CARDINAL, 0xFFFFFFFF);
   set_window_prop(disp, win, atom__NET_WM_WINDOW_TYPE, XA_ATOM,
		    atom__NET_WM_WINDOW_TYPE_DOCK);
   set_window_prop(disp, win, atom__NET_WM_STATE, XA_ATOM,
		    atom__NET_WM_STATE_STICKY);

   /* use old gnome hint since sawfish doesn't support _NET_WM_STRUT */
   set_window_prop(disp, win, atom__WIN_HINTS, XA_CARDINAL,
		    WIN_HINTS_SKIP_FOCUS | WIN_HINTS_SKIP_WINLIST |
		    WIN_HINTS_SKIP_TASKBAR | WIN_HINTS_DO_NOT_COVER);

   /* borderless motif hint */
   memset(&mwm, 0, sizeof(mwm));
   mwm.flags = MWM_HINTS_DECORATIONS;
   XChangeProperty(disp, win, atom__MOTIF_WM_HINTS, atom__MOTIF_WM_HINTS, 32,
		    PropModeReplace,
		    (unsigned char *)&mwm, sizeof(MWMHints) / 4);
#if DEBUG
   fprintf(stderr, "%s:%d props set (2).\n", __FILE__, __LINE__);
#endif

   /* make sure the WM obays our window position */
   size_hints.flags = PPosition;

   /*XSetWMNormalHints (disp, win, &size_hints); */
   XChangeProperty(disp, win, XA_WM_NORMAL_HINTS, XA_WM_SIZE_HINTS, 32,
		    PropModeReplace,
		    (unsigned char *)&size_hints, sizeof(XSizeHints) / 4);
   
   XMoveWindow(disp, win, tbar_x, tbar_y);

#if DEBUG
   fprintf(stderr, "%s:%d window moves.\n", __FILE__, __LINE__);
#endif
   
   /* make our window unfocusable */
   wmhints.flags = InputHint;
   wmhints.input = False;

   /*XSetWMHints (disp, win, &wmhints); */
   XChangeProperty(disp, win, XA_WM_HINTS, XA_WM_HINTS, 32, PropModeReplace,
		    (unsigned char *)&wmhints, sizeof(XWMHints) / 4);

#if DEBUG
   fprintf(stderr, "%s:%d props set (3).\n", __FILE__, __LINE__);
#endif
   
   /* receive the window event */
   XMapWindow(disp, win);
   
#if DEBUG
   fprintf(stderr, "%s:%d window mapped.\n", __FILE__, __LINE__);
#endif
   
   if (!config->taskbar_always_on_top) XLowerWindow(disp, win);

#if DEBUG
   fprintf(stderr, "%s:%d taskbar lowered.\n", __FILE__, __LINE__);
#endif
   
   tb->xinfo = xinfo;
   tb->config = config;
   tb->win = win;

   tb->autohide = config->taskbar_autohide;
   tb->hidespeed = config->taskbar_hidespeed;
   tb->unhidespeed = config->taskbar_unhidespeed;
   tb->hiddenp = 0;
   
   tb->border = config->taskbar_border;
   tb->aborder = config->taskbar_border;
   tb->linesep = config->taskbar_linesep;
   
   tb->x = tbar_x;
   tb->y = tbar_y; 
   tb->width = tbar_w;
   tb->height = tbar_h;
   
   tb->desktop = current_desktop(xinfo->disp, xinfo->root_win);
#if DEBUG
   fprintf(stderr, "%s:%d current desktop %d.\n", __FILE__, __LINE__, tb->desktop);
#endif
   
   tb->areas = NIL;
   
   tb->xclients = NIL;
   tb->_freexclients = NIL;

   tb->xclient_manager_number = 0;

   tb->iconcache = make_iconcache(tb);

#if (DEBUG == 0)
   taskbar_set_frame_colors(tb, GREY12, WHITE, GREY9);
#else
   taskbar_set_frame_colors(tb, GREY12, GREEN, RED);
#endif

#if DEBUG
   fprintf(stderr, "%s:%d taskbar color set.\n", __FILE__, __LINE__);
#endif
   
   return tb;
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    taskbar_set_frame_colors ...                                     */
/*---------------------------------------------------------------------*/
void
taskbar_set_frame_colors(taskbar_t *tb, int gradient, int top, int bottom) {
   tb->frame_gradient_color = gradient;
   tb->frame_top_color = top;
   tb->frame_bottom_color = bottom;
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    taskbar_register_xclients ...                                    */
/*---------------------------------------------------------------------*/
void
taskbar_register_xclients(taskbar_t *tbar) {
   Xinfo_t *xinfo = tbar->xinfo;
   Display *disp = xinfo->disp;
   Window root_win = xinfo->root_win;
   Window *wins;
   long num, i;

   /* store the new desktop value */
   tbar->desktop = current_desktop(xinfo->disp, xinfo->root_win);
   
   /* get the window list */
   wins = get_window_prop_data(disp, root_win,
			       atom__NET_CLIENT_LIST, XA_WINDOW,
			       &num);

   /* check all the windows */
   for(i = 0; i < num; i++) {
      Window w = wins[ i ];

      if (w != tbar->win && !tooltips_windowp(w) && !find_area(tbar, w)) {
	 xclient_t *xcl = window_xclient(tbar, w);

	 if (!xcl) {
	    pair_t *lst = tbar->areas;

	    XSelectInput(disp, w, PropertyChangeMask | StructureNotifyMask);

	    xcl = get_xclient(tbar, w);

	    tbar->xclients = cons(xcl, tbar->xclients);
	    
	    /* notify all the interested areas */
	    while (PAIRP(lst)) {
	       area_t *ar = (area_t *)CAR(lst);

	       if (ar->create_notify) ar->create_notify(ar, xcl);
	       lst = CDR(lst);
	    }
	 }
      }
   }

   XFree(wins);
   assert_window_list(tbar);
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    taskbar_area_do_layout ...                                       */
/*    -------------------------------------------------------------    */
/*    Compute the taskbar area layout for areas currently added        */
/*    to the taskbar.                                                  */
/*---------------------------------------------------------------------*/
void
taskbar_area_do_layout(taskbar_t *tbar) {
   pair_t *lst = tbar->areas;
   int fixed_width = 0;
   int float_width;
   int nfloat = 0;
   int relief = tbar->config->relief;
   int twidth = tbar->width - (2 * relief * tbar->border);
   int arx = relief ? tbar->border : 0;
   Display *disp = tbar->xinfo->disp;

   /* compute the sum of fixed area widths */
   while (PAIRP(lst)) {
      area_t *ar = (area_t *)CAR(lst);

      if (!ar->ignore_layout) {
	 if (ar->uwidth) {
	    fixed_width += ar->uwidth;
	 } else {
	    nfloat++;
	 }
      }
      
      lst = CDR(lst);
   }

   /* the float_width */
   if (nfloat) float_width = (twidth - fixed_width) / nfloat;

   /* compute the horizontal layout of the area widths */
   lst = tbar->areas;
   while (PAIRP(lst)) {
      area_t *ar = (area_t *)CAR(lst);

      if (!ar->ignore_layout) {
	 ar->x = arx;
	 ar->y = tbar->linesep + tbar->border;
	 ar->width = (ar->uwidth ? ar->uwidth : float_width);
	 ar->height = tbar->height - tbar->linesep - (2 * tbar->border);

	 arx = ar->x + ar->width;

	 if (ar->win) {
	    XMoveResizeWindow(disp, ar->win,
			       ar->x, ar->y, ar->width, ar->height);
	 }
      }

      if (ar->layout) {
	 ar->layout(ar);
      }
      
      lst = CDR(lst);
   }
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    taskbar_hide ...                                                 */
/*---------------------------------------------------------------------*/
void
taskbar_hide(taskbar_t *tbar) {
   int y = tbar->y;

   tooltips_hide();
   while (y < tbar->y + tbar->height - 1) {
      y++;
      XMoveWindow(tbar->xinfo->disp, tbar->win, 0, y);

      if (!(y % tbar->hidespeed)) {
	 usleep(tbar->config->animspeed);
	 XSync(tbar->xinfo->disp, True);
      }
   }
   tbar->hiddenp = 1;
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    taskbar_unhide ...                                               */
/*---------------------------------------------------------------------*/
void
taskbar_unhide(taskbar_t *tbar) {
   int y = tbar->y + tbar->height - 1;

   taskbar_refresh_all(tbar);
   
   while (y > tbar->y) {
      y--;
      
      if (!(y % tbar->unhidespeed)) {
	 XMoveWindow(tbar->xinfo->disp, tbar->win, 0, y);
	 taskbar_refresh_all(tbar);
	 XSync(tbar->xinfo->disp, True);
	 usleep(tbar->config->animspeed);
      }
   }
   tbar->hiddenp = 0;
   
   XSync(tbar->xinfo->disp, True);
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    taskbar_refresh ...                                              */
/*---------------------------------------------------------------------*/
void
taskbar_refresh(taskbar_t *tbar) {
   int width = tbar->width;
   int height = tbar->height;
   int linesep = tbar->linesep;
   int i;

   /* background */
   draw_gradient(tbar->xinfo, tbar->win,
		  1, linesep, width - 1, height - linesep - 1,
		  0,
		  tbar->frame_gradient_color, 0, 0);

   /* refief framing */
   draw_relief(tbar->xinfo, tbar->win,
	       0, linesep, width - 1, height - linesep - 1,
	       0,
	       tbar->frame_top_color, tbar->frame_bottom_color,
	       tbar->border);
   
   /* separation line */
   for(i = 0; i < linesep; i++) {
      draw_line(tbar->xinfo, tbar->win,
		0, i, width - 1, 0,
		0, 25);
   }
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    taskbar_refresh_all ...                                          */
/*---------------------------------------------------------------------*/
void
taskbar_refresh_all(taskbar_t *tbar) {
   pair_t *lst = tbar->areas;
   
   while (PAIRP(lst)) {
      area_t *ar = (area_t *)CAR(lst);

      ar->refresh(ar);
      lst = CDR(lst);
   }
   
   taskbar_refresh(tbar);
}

/*---------------------------------------------------------------------*/
/*    int                                                              */
/*    taskbar_info ...                                                 */
/*---------------------------------------------------------------------*/
int
taskbar_info(taskbar_t *tbar, int src) {
   long num, numi, numid, dt, i;
   Window *wins;
   
   dt = current_desktop(tbar->xinfo->disp, tbar->xinfo->root_win);
   wins = get_window_prop_data(tbar->xinfo->disp,
				tbar->xinfo->root_win,
				atom__NET_CLIENT_LIST, XA_WINDOW,
				&num);

   for(i = 0, numi = 0, numid = 0; i < num; i++) {
      if (window_iconifiedp(tbar->xinfo->disp, wins[ i ])) {
	 numi++;
	 if (window_desktop(tbar->xinfo->disp, wins[ i ]) == dt) numid++;
      }
   }
	    
   XFree(wins);
}   

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    taskbar_property_notify ...                                      */
/*---------------------------------------------------------------------*/
void
taskbar_property_notify(taskbar_t *tbar, XEvent *ev) {
   Window win = ev->xproperty.window;
   Xinfo_t *xinfo = tbar->xinfo;
   Display *disp = xinfo->disp;
   Atom at = ev->xproperty.atom;
   /* store the new desktop value */
   tbar->desktop = current_desktop(disp, xinfo->root_win);

   fprintf(stderr, "taskbar_prop_notify win=%p root_win=%p\n", win, tbar->xinfo->root_win);
   if (win == tbar->xinfo->root_win) {
      if (at == atom__NET_CURRENT_DESKTOP) {
	 /* the desktop has changed */
	 pair_t *lst = tbar->areas;

	 taskbar_refresh(tbar);

	 while (PAIRP(lst)) {
	    area_t *ar = (area_t *)CAR(lst);
	    
	    if (ar->desktop_notify) ar->desktop_notify(ar);
	    lst = CDR(lst);
	 }
      } else {
	 if (at == atom__NET_CLIENT_LIST) {
	    /* a client has been added or destroyed */
	    taskbar_register_xclients(tbar);
	 }
      }
   } else {
      area_t *ar = find_area(tbar, win);

      if (at == XA_WM_NAME) {
	 xclient_t *xcl = window_xclient(tbar, win);

	 if (xcl) {
	    if (xcl->name) free(xcl->name);
	    xcl->name = window_name(disp, win);

	    update_xclient_icon(xcl, tbar, win);
	 }
      } else {
	 if (at == atom_WM_STATE) {
	    xclient_t *xcl = window_xclient(tbar, win);

	    if (xcl) {
	       pair_t *lst = tbar->areas;

	       /* update the xclient in order to get its new state */
	       update_xclient_state(xcl, tbar, win);
	       
	       /* notify all the interested areas */
	       while (PAIRP(lst)) {
		  area_t *ar = (area_t *)CAR(lst);

		  if (ar->state_notify) ar->state_notify(ar, xcl);
		  lst = CDR(lst);
	       }
	    }
	 } else {
	    if (at == XA_WM_HINTS) {
	       ;
	    }
	 }
      }

      if (ar) ar->refresh(ar);
   }
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    taskbar_destroy_notify ...                                       */
/*---------------------------------------------------------------------*/
void
taskbar_destroy_notify(taskbar_t *tbar, XEvent *ev) {
   Window win = ev->xdestroywindow.window;

   /* store the new desktop value */
   tbar->desktop = current_desktop(tbar->xinfo->disp, tbar->xinfo->root_win);
   
   if (win != tbar->xinfo->root_win
       && win != tbar->win
       && !tooltips_windowp(win)) {
      xclient_t *xcl = window_xclient(tbar, win);

      debug_window_event(tbar, win, DEBUG_EVENT_WINDOW_DESTROYED);

      if (xcl) {
	 pair_t *lst = tbar->areas;

	 /* mark that the xclient is now dead */
	 xcl->live = 0;

	 /* notify all the interested areas */
	 while (PAIRP(lst)) {
	    area_t *ar = (area_t *)CAR(lst);

	    if (ar->destroy_notify) ar->destroy_notify(ar, xcl);
	    lst = CDR(lst);
	 }
	 
	 /* remove the client from the active list */
	 tbar->xclients = remq(xcl, tbar->xclients);
	 free_xclient(xcl, tbar);
	 
      } else {
	 pair_t *lst = tbar->areas;

	 /* notify all the interested areas */
	 while (PAIRP(lst)) {
	    area_t *ar = (area_t *)CAR(lst);

	    if (ar->win == win) {
	       if (ar->destroy_notify) ar->destroy_notify(ar, 0L);
	       break;
	    }
	    lst = CDR(lst);
	 }
      }
   }
}

/*---------------------------------------------------------------------*/
/*    int                                                              */
/*    taskbar_get_xclient_manager_number ...                           */
/*---------------------------------------------------------------------*/
int
taskbar_get_xclient_manager_number(taskbar_t *tbar) {
   tbar->xclient_manager_number++;

   return tbar->xclient_manager_number;
}
