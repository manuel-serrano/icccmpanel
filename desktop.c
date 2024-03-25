/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/desktop.c                          */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Fri Jul 23 22:15:38 2004                          */
/*    Last change :  Wed Mar 20 08:40:50 2024 (serrano)                */
/*    Copyright   :  2004-24 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    The desktop                                                      */
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
#include "desktop.h"

/*---------------------------------------------------------------------*/
/*    Global controls and parameters                                   */
/*---------------------------------------------------------------------*/
static char *desktop_names[] = { "one", "two", "three",
				 "four", "five", "six",
				 "seven", "eight", "nine",
				 "then", "eleven", "twelve"
};

#define DESKTOPNAMES_COUNT \
  (sizeof(desktop_names) / sizeof(desktop_names[ 0 ]))

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    refresh_desktop ...                                              */
/*---------------------------------------------------------------------*/
static void
refresh_desktop(area_t *ar) {
   taskbar_t *tbar = ar->taskbar;
   int relief = tbar->config->relief;
   Xinfo_t *xinfo = tbar->xinfo;
   int text_x, text_y, text_w;
   char *dname;
   int alen = 0;
   int i;
   int desktop = current_desktop(xinfo->disp, xinfo->root_win);

   /* compute the desktop info */
   if (desktop > DESKTOPNAMES_COUNT) {
      dname = "???";

      for (i = 0; i < DESKTOPNAMES_COUNT; i++) {
	 int l = strlen(desktop_names[ i ]);
	 if (l > alen) alen = l;
      }
   } else {
      dname = desktop_names[ desktop ];
      for (i = 0; i <= desktop; i++) {
	 int l = strlen(desktop_names[ i ]);
	 if (l > alen) alen = l;
      }
   }
   
   /* the position of the desktop info */
   text_w = ar->width;
   text_y = xinfo->xfsb->ascent + ((ar->height - xinfo->xfsb->ascent) / 3);

   /* draw the desktop info */
   while ((XTextWidth(xinfo->xfsb, dname, alen) >= (text_w - 2)) &&
	  (alen > 0))
      alen--;

   text_x = (text_w - XTextWidth(xinfo->xfsb, dname, alen)) / 2;

   draw_gradient(tbar->xinfo, ar->win,
		  0, relief,
		  ar->width,
		  ar->height + (relief ? (2 * tbar->aborder) : 0),
		  0, GREY12, 0, 0);
   
   draw_text(xinfo, ar->win, text_x, text_y, dname, alen, 0,
	      ar->active ? ACTIVE : BLACK,
	      tbar->config->color_shadow,
	      tbar->config->shadow_size);

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

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    button_press_desktop ...                                         */
/*---------------------------------------------------------------------*/
static void
button_press_desktop(XEvent *ev, area_t *ar) {
   taskbar_t *tbar = ar->taskbar;
   Xinfo_t *xinfo = tbar->xinfo;
   int c = current_desktop(xinfo->disp, xinfo->root_win);
   int nb = number_of_desktops(xinfo->disp, xinfo->root_win);

   switch_desktop(xinfo->disp, xinfo->root_win, c == (nb - 1) ? 0 : c + 1);
   refresh_desktop(ar);
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    enter_notify_desktop ...                                         */
/*---------------------------------------------------------------------*/
static void
enter_notify_desktop(XEvent *ev, area_t *ar) {
   taskbar_t *tbar = ar->taskbar;
   ar->active = 1;
   tooltips_setup("Switch workspace",
		   ar->x,
		   tbar->top ? tbar->y + tbar->height : tbar->y - tbar->height,
		   TOOLTIPS);
   refresh_desktop(ar);
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    leave_notify_desktop ...                                         */
/*---------------------------------------------------------------------*/
static void
leave_notify_desktop(XEvent *ev, area_t *ar) {
   tooltips_hide();
   ar->active = 0;
   refresh_desktop(ar);
}

/*---------------------------------------------------------------------*/
/*    area_t *                                                         */
/*    make_desktop ...                                                 */
/*---------------------------------------------------------------------*/
area_t *
make_desktop(taskbar_t *tbar, int width, int height) {
   area_t *ar = calloc(1, sizeof(area_t));

   if (!ar) exit(10);

   /* initialize the desktop */
   ar->win = make_area_window(tbar);
   ar->name = "desktop";
   
   ar->uwidth = width;
   ar->uheight = height;

   /* bind the area in the taskbar */
   ar->taskbar = tbar;
   tbar->areas = cons(ar, tbar->areas);
   
   ar->refresh = &refresh_desktop;
   ar->desktop_notify = &refresh_desktop;
   ar->button_press = &button_press_desktop;
   ar->enter_notify = &enter_notify_desktop;
   ar->leave_notify = &leave_notify_desktop;

   return ar;
}

/*---------------------------------------------------------------------*/
/*    void *                                                           */
/*    start_desktop ...                                                */
/*---------------------------------------------------------------------*/
void *
start_desktop(void *tb, pair_t *args) {
   taskbar_t *tbar = (taskbar_t *)tb;
   config_t *config = tbar->config;
   int width = INTEGER_VAL(CAR(args));

   return make_desktop(tbar, width, config->taskbar_height - 1);
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    parse_desktop ...                                                */
/*---------------------------------------------------------------------*/
void
parse_desktop(config_t *config, pair_t *lst) {
   pair_t *l = CDR(lst);
   integer_t *width = make_integer(40);

   /* parse desktop width */
   while (PAIRP(l)) {
      obj_t *car = CAR(l);
      if (SYMBOLP(car)) {
	 if (SYMBOL_EQ((symbol_t *)car, sym_width)) {
	    width = parse_cadr_integer(l);

	    if (!width) {
	       parse_error("Illegal :width", (obj_t *)lst);
	    } else {
	       l = CDR(l);
	    }
	 }
      } else {
	 parse_error("Illegal desktop", (obj_t *)lst);
      }
      l = CDR(l);
   }

   register_plugin(config, make_plugin(start_desktop, cons(width, NIL)));
}
