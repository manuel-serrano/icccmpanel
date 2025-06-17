/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/cron.c                             */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Mon Aug 11 10:37:34 2014                          */
/*    Last change :  Tue Jun 17 08:03:28 2025 (serrano)                */
/*    Copyright   :  2014-25 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    cron applet                                                      */
/*=====================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <values.h>
#include <sys/wait.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#ifdef HAVE_XPM
#  include <X11/xpm.h>
#endif

#include "icccmpanel.h"
#include "cron.h"

/*---------------------------------------------------------------------*/
/*    cron_t                                                           */
/*---------------------------------------------------------------------*/
#define STRLEN 30
typedef struct ipcron {
   area_t area;
   char *intf;
   int skfd;
   char *command;
   char cronbuf[ 1024 ];
   char infobuf[ 1024 ];
   char *cmdbuf;
   Pixmap icon, mask;
   long count;
} ipcron_t;


/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    refresh_cron ...                                                 */
/*---------------------------------------------------------------------*/
static void
refresh_cron(area_t *ar) {
   taskbar_t *tbar = ar->taskbar;
   ipcron_t *im = (ipcron_t *)ar;
   Xinfo_t *xinfo = tbar->xinfo;
   int relief = tbar->config->relief;

   if (ar->width > 0) {
      draw_gradient(xinfo, ar->win,
		     0, 0,
		     ar->width - 1,
		     ar->height + (relief ? (2 * tbar->aborder) : 0),
		     0, ar->active ? GREY10 : tbar->frame_gradient_color, 0, 0);

      draw_pixmap(xinfo, ar->win,
		   im->icon, im->mask,
		   (ar->width - tbar->config->icon_size) / 2, (1 - relief), 
		   ar->width, ar->height);
   }
}

/*---------------------------------------------------------------------*/
/*    static int                                                       */
/*    timeout_cron ...                                                 */
/*---------------------------------------------------------------------*/
static int
timeout_cron(area_t *ar) {
   ipcron_t *im = (ipcron_t *)ar;

   im->count++;
   if (im->count < 0) im->count = 0;
   
   if (!fork()) {
      setsid();
      if (im->cmdbuf) {
	 sprintf(im->cmdbuf, im->command, im->count++);
	 execl("/bin/sh", "/bin/sh", "-c", im->cmdbuf, 0);
      } else {
	 fprintf(stderr, "command=%s\n", im->command);
	 execl("/bin/sh", "/bin/sh", "-c", im->command, 0);
      }
      exit(0);
   } else {
      while (waitpid(WAIT_ANY, NULL, WNOHANG) != 0);
   }
   
   return 1;
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    button_press_cron ...                                            */
/*---------------------------------------------------------------------*/
static void
button_press_cron(XEvent *ev, area_t *ar) {
   timeout_cron(ar);
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    enter_notify_cron ...                                            */
/*---------------------------------------------------------------------*/
static void
enter_notify_cron(XEvent *ev, area_t *ar) {
   taskbar_t *tbar = ar->taskbar;
   ipcron_t *im = (ipcron_t *)ar;

   ar->active = 1;
   refresh_cron(ar);
   
   /* info buf */
   if (im->cmdbuf) {
      sprintf(im->cmdbuf, im->command, im->count);
      sprintf(im->infobuf, "[%ds] %s", ar->timeout_count / 10, im->cmdbuf);
   } else {
      sprintf(im->infobuf, "[%ds] %s", ar->timeout_count / 10, im->command);
   }
   
   tooltips_setup(((ipcron_t *)ar)->infobuf,
		   ar->x,
		   tbar->top ? tbar->y + tbar->height : tbar->y - tbar->height,
		   TOOLTIPS);
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    leave_notify_cron ...                                            */
/*---------------------------------------------------------------------*/
static void
leave_notify_cron(XEvent *ev, area_t *ar) {
   ar->active = 0;
   refresh_cron(ar);
   
   tooltips_hide();
}

/*---------------------------------------------------------------------*/
/*    static area_t *                                                  */
/*    make_cron ...                                                    */
/*---------------------------------------------------------------------*/
static area_t *
make_cron(taskbar_t *tbar, int width, int height, int delay, char *icon, char *command) {
   ipcron_t *im = calloc(1, sizeof(ipcron_t));
   area_t *ar = &(im->area);
   int i;
   char *iconpath;
   
   if (!ar) exit(10);

   /* initialize the cron */
   ar->win = make_area_window(tbar);
   ar->name = "cron";
   
   ar->uwidth = width;
   ar->uheight = height;

   /* the icons */
   iconpath = find_icon(tbar->config, icon);

   if (iconpath) {
      XpmReadFileToPixmap(tbar->xinfo->disp, ar->win, iconpath,
			   &(im->icon), &(im->mask),
			   NULL);
   }
   
   /* bind the area in the taskbar */
   ar->taskbar = tbar;

   ar->refresh = &refresh_cron;
   ar->desktop_notify = &refresh_cron;
   ar->timeout = &timeout_cron;

   ar->enter_notify = &enter_notify_cron;
   ar->leave_notify = &leave_notify_cron;
   
   if (command) {
      ar->button_press = &button_press_cron;
      im->command = command;
   }

   /* register the timeout */
   ar->timeout_delay = delay * 10;
   evloop_timeout(ar);

   /* info buf */
   sprintf(im->infobuf, "[%ds] %s", delay, command);

   if (strchr(command, '%')) {
      im->cmdbuf = malloc(strlen(command) + 10);
   }
   
   return ar;
}
   
/*---------------------------------------------------------------------*/
/*    void *                                                           */
/*    start_cron ...                                                   */
/*---------------------------------------------------------------------*/
static void *
start_cron(void *tb, pair_t *args) {
   taskbar_t *tbar = (taskbar_t *)tb;
   config_t *config = tbar->config;
   int width = INTEGER_VAL(CAR(args));
   int delay = INTEGER_VAL(CAR(CDR(args)));
   char *icon = STRING_CHARS(CAR(CDR(CDR(args))));
   char *command = STRING_CHARS(CAR(CDR(CDR(CDR(args)))));

   return make_cron(tbar, width, config->taskbar_height - 1, delay, icon, command);
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    parse_cron ...                                                   */
/*---------------------------------------------------------------------*/
void
parse_cron(config_t *config, pair_t *lst) {
   pair_t *l = CDR(lst);
   pair_t *args = NIL; 
   symbol_t *sym_delay = make_symbol(":delay");
   symbol_t *sym_command = make_symbol(":command");
   symbol_t *sym_icon = make_symbol(":icon");
   integer_t *width = make_integer(30);
   integer_t *delay = make_integer(120);
   string_t *icon = 0;
   string_t *command = 0L;

   /* search of a command */
   while (PAIRP(l)) {
      obj_t *car = CAR(l);
      if (SYMBOLP(car)) {
	 if (SYMBOL_EQ((symbol_t *)car, sym_delay)) {
	    delay = parse_cadr_integer(l);

	    if (!delay) {
	       parse_error("Illegal :delay", (obj_t *)l);
	    } else {
	       args = cons(delay, args);
	       l = CDR(l);
	    }
	 } else if (SYMBOL_EQ((symbol_t *)car, sym_width)) {
	    width = parse_cadr_integer(l);

	    if (!width) {
	       parse_error("Illegal :width", (obj_t *)l);
	    } else {
	       l = CDR(l);
	    }
	 } else if (SYMBOL_EQ((symbol_t *)car, sym_icon)) {
	    icon = parse_cadr_string(l);

	    if (!icon) {
	       parse_error("Illegal :icon", (obj_t *)l);
	    } else {
	       l = CDR(l);
	    }
	 } else if (SYMBOL_EQ((symbol_t *)car, sym_command)) {
	    command = parse_cadr_string(l);

	    if (!command) {
	       parse_error("Illegal :command", (obj_t *)l);
	    } else {
	       l = CDR(l);
	    }
	 }
      } else {
	 parse_error("Illegal cron", (obj_t *)l);
      }
      l = CDR(l);
   }

   if (!command) {
      parse_error("Cron missing command", (obj_t *)lst);
   }

   if (!icon) {
      icon = make_string("cron.xpm");
   }
   
   args = cons(width, cons(delay, cons(icon, cons(command, NIL))));
   register_plugin(config, make_plugin(start_cron, args));
}
