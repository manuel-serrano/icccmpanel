/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/app.c                              */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Fri Jul 23 22:15:38 2004                          */
/*    Last change :  Wed Apr 30 14:15:09 2025 (serrano)                */
/*    Copyright   :  2004-25 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    The applications                                                 */
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
#include "app.h"
#include "command.h"
#include "debug.h"

/*---------------------------------------------------------------------*/
/*    ipapp_t                                                          */
/*---------------------------------------------------------------------*/
typedef struct ipapp {
   /* the area */
   area_t area;
   /* the partial relief mask */
   int relief_mask;
   /* the application */
   app_t *app;
   /* the image associated with the application */
   Pixmap icon, mask;
   /* the position of the icon */
   int iconx, icony;
} ipapp_t;

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    refresh_app ...                                                  */
/*---------------------------------------------------------------------*/
static void
refresh_app(area_t *ar) {
   ipapp_t *ia = (ipapp_t *)ar;
   taskbar_t *tbar = ar->taskbar;
   Xinfo_t *xinfo = tbar->xinfo;
   int border = tbar->aborder;
   int relief = tbar->config->relief;
   
   /* the icon */
   if (ia->icon || ia->mask) {
      draw_pixmap(xinfo, ar->win,
		   ia->icon, ia->mask,
		   ia->iconx, ia->icony,
		   ar->width - (2 * border), ar->height - (2 * relief));
   }

   /* the border */
   if (relief) {
      draw_partial_relief(xinfo, ar->win, ia->relief_mask,
			0, 0,
			ar->width - 1, ar->height - 1,
			0, WHITE, GREY9, border);
   } else {
      draw_partial_relief(xinfo, ar->win, ia->relief_mask,
			   0, 0,
			   ar->width - 1, ar->height - relief,
			   0, WHITE, GREY9, tbar->aborder);
   }
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    button_press_app ...                                             */
/*---------------------------------------------------------------------*/
static void
button_press_app(XEvent *ev, area_t *ar) {
   taskbar_t *tbar = ar->taskbar;
   char *cmd = ((ipapp_t *)ar)->app->command;
   
   if (cmd) {
      if (!strcmp(cmd, "__debug__")) {
	 debug(tbar, 0L);
	 taskbar_set_frame_colors(tbar, GREY12, WHITE, GREY9);
      } else if (!fork()) {
	 setsid();
	 execl("/bin/sh", "/bin/sh", "-c", cmd, 0);
	 exit(0);
      }
   }
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    enter_notify_app ...                                             */
/*---------------------------------------------------------------------*/
static void
enter_notify_app(XEvent *ev, area_t *ar) {
   taskbar_t *tbar = ar->taskbar;
   
   tooltips_setup(((ipapp_t *)ar)->app->command,
		   ar->x,
		   tbar->top ? tbar->y + tbar->height : tbar->y - tbar->height,
		   TOOLTIPS);
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    leave_notify_app ...                                             */
/*---------------------------------------------------------------------*/
static void
leave_notify_app(XEvent *ev, area_t *ar) {
   tooltips_hide();
}

/*---------------------------------------------------------------------*/
/*    area_t *                                                         */
/*    make_app ...                                                     */
/*---------------------------------------------------------------------*/
area_t *
make_app(taskbar_t *tbar, app_t *app, int pos, int width, int height) {
   ipapp_t *ia = calloc(1, sizeof(ipapp_t));
   area_t *ar = &(ia->area);
   int relief = tbar->config->relief;
   char *path;
   int len;
   Pixmap pix;
   int x, y;
   unsigned int w, h, d, bw;
   
   if (!ar) exit(10);

   /* initialize the app */
   ar->win = make_area_window(tbar);
   ar->name = "app";
   
   ar->uwidth = width;
   ar->uheight = height;

   ia->relief_mask = relief ? RELIEF_TOP | RELIEF_BOTTOM | pos : pos;
   ia->app = app;

   /* the image */
   if (path = find_icon(tbar->config, app->image)) {
      XpmReadFileToPixmap(tbar->xinfo->disp, ar->win, path,
			   &(ia->icon), &(ia->mask),
			   NULL);
   }

   /* the position of the image in the area */
   XGetGeometry(tbar->xinfo->disp, ia->icon, &pix, &x, &y, &w, &h, &bw, &d);

   ia->iconx = width <= w ? 0 : (width - w) / 2;
   ia->icony = (height <= h ? 0 : (height - h) / 2) - 1;

   /* bind the area in the taskbar */
   ar->taskbar = tbar;
   tbar->areas = cons(ar, tbar->areas);

   ar->refresh = &refresh_app;
   ar->button_press = &button_press_app;
   ar->enter_notify = &enter_notify_app;
   ar->leave_notify = &leave_notify_app;

   return ar;
}

/*---------------------------------------------------------------------*/
/*    void *                                                           */
/*    start_apps ...                                                   */
/*---------------------------------------------------------------------*/
void *
start_apps(void *tb, pair_t *args) {
   taskbar_t *tbar = (taskbar_t *)tb;
   config_t *config = tbar->config;
   char cmdp = !SYMBOL_EQ(CAR(args), sym_false);
   pair_t *apps = CAR(CDR(CDDR(args)));
   area_t *l;
   int rel = cmdp ? RELIEF_RIGHT : RELIEF_RIGHT | RELIEF_LEFT;
   int relief = tbar->config->relief;

   while (PAIRP(apps)) {
      app_t *app = calloc(1, sizeof(app_t));

      app->command = STRING_CHARS(CAR(CAR(apps)));
      app->image = STRING_CHARS(CADR(CAR(apps)));

      l = make_app(tbar, app,
		   rel, 
		   config->app_width, config->taskbar_height - relief);
      rel = RELIEF_NONE;
      apps = CDR(apps);
   }

   if (cmdp) {
      char *icon = STRING_CHARS(CADR(args));
      int alen = INTEGER_VAL(CAR(CDDR(args)));
      l = make_command(tbar, icon,
		       RELIEF_LEFT,
		       config->app_width,
		       config->taskbar_height,
		       config->app_width * alen);
   }

   return l;
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    parse_apps ...                                                   */
/*---------------------------------------------------------------------*/
void
parse_apps(config_t *config, pair_t *lst) {
   pair_t *l = CDR(lst);
   char cmd = 0;
   char *cmdicon = "exec.xpm";
   symbol_t *sym_command = make_symbol(":command");
   symbol_t *sym_command_icon = make_symbol(":command-icon");
   symbol_t *sym_icon = make_symbol(":icon");
   symbol_t *sym_width = make_symbol(":width");
   int applen = 0;
   pair_t *apps = 0;
   pair_t *args;

   while (PAIRP(l)) {
      obj_t *car = CAR(l);
      if (SYMBOLP(car)) {
	 if (SYMBOL_EQ((symbol_t *)car, sym_command)) {
	    applen++;
	    cmd = 1;
	 } else {
	    if (SYMBOL_EQ((symbol_t *)car, sym_command_icon)) {
	       string_t *s = parse_cadr_string(l);

	       if (!s) {
		  parse_error("Illegal :command-icon", (obj_t *)lst);
	       } else {
		  cmdicon = expand_env(STRING_CHARS(s));
		  l = CDR(l);
	       }
	    } else {
	       if (SYMBOL_EQ((symbol_t *)car, sym_width)) {
		  integer_t *w = parse_cadr_integer(l);
	    
		  if (!w) {
		     parse_error("Illegal :width", (obj_t *)lst);
		  } else {
		     config->app_width = INTEGER_VAL(w);
		     l = CDR(l);
		  }
	       }
	    }
	 }
      } else {
	 if (PAIRP(car)) {
	    if (!STRINGP(CAR(car)) ||
		!PAIRP(CDR(car)) ||
		!SYMBOLP(CADR(car)) ||
		!SYMBOL_EQ(CADR(car), sym_icon) ||
		!STRINGP(CAR(CDDR(car))) ||
		!NULLP(CDR(CDDR(car)))) {
	       parse_error("Illegal icon", (obj_t *)car);
	    } else {
	       pair_t *app;
	       app = cons(CAR(car), cons(CAR(CDDR(car)), NIL));
	       apps = cons(app, apps);
	       applen++;
	    }
	 }
      }
      l = CDR(l);
   }

   args = cons(apps, NIL);
   if (cmd)
      args = cons(sym_true,
		   cons(make_string(cmdicon),
			 cons(make_integer(applen), args)));
   else
      args = cons(sym_false,
		   cons(sym_false,
			 cons(make_integer(0), args)));
   
   register_plugin(config, make_plugin(start_apps, args));
}
