/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/volume.c                           */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Mon Aug 11 10:37:34 2014                          */
/*    Last change :  Tue Jun 17 07:47:48 2025 (serrano)                */
/*    Copyright   :  2014-25 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    volume applet                                                    */
/*=====================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <values.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#ifdef HAVE_XPM
#  include <X11/xpm.h>
#endif

#include "icccmpanel.h"
#include "mixer.h"
#include "volume.h"

/*---------------------------------------------------------------------*/
/*    volume_t                                                         */
/*---------------------------------------------------------------------*/
#define STRLEN 30
typedef struct volume {
   area_t area;
   char *command;
   Pixmap icon, mask;
   mixer_t *mixer;
   char *devname;
   int devindex;
   long volume;
   char tpbuf[ 200 ];
   int iconlen, iconidx;
   Pixmap *icons, *masks;
} volume_t;

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    refresh_none ...                                                 */
/*---------------------------------------------------------------------*/
static void 
refresh_none(area_t *ar) {
   return;
}

/*---------------------------------------------------------------------*/
/*    static int                                                       */
/*    timeout_none ...                                                 */
/*---------------------------------------------------------------------*/
static int 
timeout_none(area_t *ar) {
   return 1;
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    refresh_volume ...                                               */
/*---------------------------------------------------------------------*/
static void
refresh_volume(area_t *ar) {
   taskbar_t *tbar = ar->taskbar;
   volume_t *hi = (volume_t *)ar;
   Xinfo_t *xinfo = tbar->xinfo;
   int text_y = xinfo->xfsb->ascent + ((ar->height - xinfo->xfsb->ascent) / 3);

   draw_gradient(xinfo, ar->win,
		  0, 0,
		  ar->width - 1,
		  ar->height,
		  0, ar->active ? GREY10 : tbar->frame_gradient_color, 0, 0);

   draw_pixmap(xinfo, ar->win,
		hi->icons[ hi->iconidx ], hi->masks[ hi->iconidx ],
		(ar->width - tbar->config->icon_size) / 2, 0, 
		ar->width, ar->height);
}

/*---------------------------------------------------------------------*/
/*    static int                                                       */
/*    timeout_volume ...                                               */
/*---------------------------------------------------------------------*/
static int
timeout_volume(area_t *ar) {
   taskbar_t *tbar = ar->taskbar;
   volume_t *mi = (volume_t *)ar;
   int vol = mixer_read_vol(mi->mixer, mi->devindex, 1) % 256;

   if (mi->volume != vol) {
      long i = 100 / (mi->iconlen - 1);

      mi->volume = vol;
      mi->iconidx = vol / i;

      refresh_volume(ar);
   }

   return 1;
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    enter_notify_volume ...                                          */
/*---------------------------------------------------------------------*/
static void
enter_notify_volume(XEvent *ev, area_t *ar) {
   taskbar_t *tbar = ar->taskbar;
   volume_t *mi = (volume_t *)ar;
   int vol = mixer_read_vol(mi->mixer, mi->devindex, 1) % 256;
   
   timeout_volume(ar);

   snprintf(mi->tpbuf, 200, "%s: %d\000", mi->devname, vol);
   ar->active = 1;
   refresh_volume(ar);

   tooltips_setup(mi->tpbuf,
		   ar->x,
		   tbar->top ? tbar->y + tbar->height : tbar->y - tbar->height,
		   TOOLTIPS);
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    leave_notify_volume ...                                          */
/*---------------------------------------------------------------------*/
static void
leave_notify_volume(XEvent *ev, area_t *ar) {
   volume_t *hi = (volume_t *)ar;
   
   ar->active = 0;
   refresh_volume(ar);
   tooltips_hide();
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    button_press_fork ...                                            */
/*---------------------------------------------------------------------*/
static void
button_press_fork(XEvent *ev, area_t *ar) {
   taskbar_t *tbar = ar->taskbar;
   volume_t *vo = ((volume_t *)ar);
   char *cmd = vo->command;
   int vol;
   int x;

   if (cmd) {
      if (!fork()) {
	 setsid();
	 execl("/bin/sh", "/bin/sh", "-c", cmd, 0);
	 exit(0);
      }
   }
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    button_press_volume ...                                          */
/*---------------------------------------------------------------------*/
static void
button_press_volume(XEvent *ev, area_t *ar) {
   taskbar_t *tbar = ar->taskbar;
   volume_t *vo = ((volume_t *)ar);
   int vol;
   int x;

   switch(((XButtonEvent *)ev)->button) {
      case 1:
	 x = ((XButtonEvent *)ev)->x;
	 vol = (int)(100. * ((double)x/(double)ar->width));

	 mixer_write_vol(vo->mixer, vo->devindex, vol + (vol * 256));
	 snprintf(vo->tpbuf, 200, "%s: %d\000", vo->devname, vol);

	 tooltips_setup(vo->tpbuf,
			 ar->x,
			 tbar->top ? tbar->y + tbar->height : tbar->y - tbar->height,
			 TOOLTIPS);
	 tooltips_refresh();
	 
	 refresh_volume(ar);
	 break;

      case 2:
	 button_press_fork(ev, ar);
   }
}

/*---------------------------------------------------------------------*/
/*    static area_t *                                                  */
/*    make_volume ...                                                  */
/*---------------------------------------------------------------------*/
static area_t *
make_volume(taskbar_t *tbar, int width, int height, pair_t *icons, char *command, char *device, char *devname) {
   volume_t *mi = calloc(1, sizeof(volume_t));
   area_t *ar = &(mi->area);
   int len = length(icons);
   int i;

   if (!ar) exit(10);

   /* initialize the volume */
   ar->win = make_area_window(tbar);
   ar->name = "volume";
   
   ar->uwidth = width;
   ar->uheight = height;

   mi->command = command;
   mi->mixer = open_mixer(device);
   mi->devname = devname;

   /* the icons */
   mi->iconlen = len;
   mi->icons = calloc(len, sizeof(Pixmap));
   mi->masks = calloc(len, sizeof(Pixmap));

   for (i = 0; i < len; i++, icons = CDR(icons)) {
      char *iconpath = find_icon(tbar->config, STRING_CHARS(CAR(icons)));

      if (iconpath) {
	 XpmReadFileToPixmap(tbar->xinfo->disp, ar->win, iconpath,
			     &(mi->icons[i]), &(mi->masks[i]),
			     NULL);
      }
   }

   /* bind the area in the taskbar */
   ar->taskbar = tbar;
   
   if (mi->mixer) {
      mi->devindex = mixer_find_dev(mi->mixer, devname);
      ar->desktop_notify = &refresh_volume;
      ar->timeout = &timeout_volume;
      ar->timeout_delay = 10;
      ar->enter_notify = &enter_notify_volume;
      ar->leave_notify = &leave_notify_volume;
      ar->button_press = &button_press_volume;
   } else {
      ar->refresh = &refresh_none;
      ar->desktop_notify = &refresh_none;
      ar->timeout = &timeout_none;
      ar->timeout_delay = 10000;
      ar->button_press = &button_press_fork;
   }

   ar->refresh = &refresh_volume;
   
   /* register the timeout */
   evloop_timeout(ar);
   
   return ar;
}
   
/*---------------------------------------------------------------------*/
/*    void *                                                           */
/*    start_volume ...                                                 */
/*---------------------------------------------------------------------*/
static void *
start_volume(void *tb, pair_t *args) {
   taskbar_t *tbar = (taskbar_t *)tb;
   config_t *config = tbar->config;
   int width = INTEGER_VAL(CAR(args));
   pair_t *icons = CAR(CDR(args));
   char *command = STRING_CHARS(CAR(CDR(CDR(args))));
   char *device = STRING_CHARS(CAR(CDR(CDR(CDR(args)))));
   char *devname = STRING_CHARS(CAR(CDR(CDR(CDR(CDR(args))))));

   return make_volume(tbar, width, config->taskbar_height - 1, icons, command, device, devname);
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    parse_volume ...                                                 */
/*---------------------------------------------------------------------*/
void
parse_volume(config_t *config, pair_t *lst) {
   pair_t *l = CDR(lst);
   pair_t *args = NIL; 
   symbol_t *sym_intf = make_symbol(":interface");
   symbol_t *sym_command = make_symbol(":command");
   symbol_t *sym_icon = make_symbol(":icon");
   symbol_t *sym_device = make_symbol(":device");
   symbol_t *sym_devname = make_symbol(":devname");
   integer_t *width = make_integer(24);
   pair_t *icons = NIL;
   string_t *device = make_string("/dev/mixer");
   string_t *devname = make_string("vol");
   string_t *command = 0L;

   /* search of a command */
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
	 } else if (SYMBOL_EQ((symbol_t *)car, sym_icon)) {
	    icons = CAR(CDR(lst));

	    if (!PAIRP(icons)) {
	       parse_error("Illegal :icons", (obj_t *)lst);
	    } else {
	       l = CDR(l);
	    }
	 } else if (SYMBOL_EQ((symbol_t *)car, sym_device)) {
	    device = parse_cadr_string(l);

	    if (!device) {
	       parse_error("Illegal :device", (obj_t *)lst);
	    } else {
	       l = CDR(l);
	    }
	 } else if (SYMBOL_EQ((symbol_t *)car, sym_devname)) {
	    devname = parse_cadr_string(l);

	    if (!devname) {
	       parse_error("Illegal :devname", (obj_t *)lst);
	    } else {
	       l = CDR(l);
	    }
	 } else if (SYMBOL_EQ((symbol_t *)car, sym_command)) {
	    command = parse_cadr_string(l);

	    if (!command) {
	       parse_error("Illegal :command", (obj_t *)lst);
	    } else {
	       l = CDR(l);
	    }
	 }
      } else {
	 parse_error("Illegal volume", (obj_t *)lst);
      }
      l = CDR(l);
   }

   if (!PAIRP(icons)) {
      int i;
      for (i = 10; i >= 0; i--) {
	 char buf[ 20 ];
	 sprintf(buf, "volume%d.xpm", i);
	 icons = cons(make_string(buf), icons);
      }
   }

   args = cons(width, cons(icons, cons(command, cons(device, cons(devname, NIL)))));
   register_plugin(config, make_plugin(start_volume, args));
}
