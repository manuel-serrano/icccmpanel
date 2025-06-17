/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/debug.c                            */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Fri Jul 19 08:43:57 2024                          */
/*    Last change :  Mon Jun 16 07:59:28 2025 (serrano)                */
/*    Copyright   :  2024-25 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    Icccmap debug                                                    */
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
#include "list.h"
#include "area.h"
#include "icons.h"
#include "debug.h"

/*---------------------------------------------------------------------*/
/*    pair_t *                                                         */
/*    windows ...                                                      */
/*---------------------------------------------------------------------*/
pair_t *windows = NIL;

/*---------------------------------------------------------------------*/
/*    char **                                                          */
/*    debug_event_names ...                                            */
/*---------------------------------------------------------------------*/
static char *debug_event_names[] = {
   "",
   "created",
   "destroyed",
   "registered"
};

static char event_fail[1024];

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    debug_window_event ...                                           */
/*---------------------------------------------------------------------*/
void
debug_window_event(taskbar_t *tbar, Window win, int event) {
   Xinfo_t *xinfo = tbar->xinfo;
   Display *disp = xinfo->disp;
   pair_t *old = assq((void *)win, windows);

   switch (event) {
      case DEBUG_EVENT_WINDOW_CREATED: {
	 if ((old) && (INTEGER_VAL(CADR(old))) != DEBUG_EVENT_WINDOW_DESTROYED) {
	    sprintf(event_fail, "double window creation %p\n", win);
	    fprintf(stderr, "*** ICCCMPANEL ERROR: %s\n", event_fail);
	    debug(tbar, event_fail, RED);
	 } else {
	    pair_t *cell = cons(make_integer(event), (pair_t *)window_name(disp, win));
	    windows = cons(cons((void *)win, cell), windows);
	 }
	 break;
      }

      case DEBUG_EVENT_WINDOW_DESTROYED: {
	 if (!old) {
	    char *name = window_name(disp, win);
	    char *class = window_class(disp, win);
	    sprintf(event_fail, "destroying unregistred window %p (%s:%s)\n", win, name, class);
	    fprintf(stderr, "*** ICCCMPANEL ERROR: %s\n", event_fail);
	    free(name);
	    free(class);
	    debug(tbar, event_fail, RED);
	 } else if (INTEGER_VAL(CADR(old)) != DEBUG_EVENT_WINDOW_CREATED
		    && INTEGER_VAL(CADR(old)) != DEBUG_EVENT_AREA_REGISTERED) {
	    char *name = window_name(disp, win);
	    char *class = window_class(disp, win);
	    sprintf(event_fail, "double window destruction %p (%s:%s)\n", win, name, class);
	    fprintf(stderr, "*** ICCCMPANEL ERROR: %s\n", event_fail);
	    free(name);
	    free(class);
	    debug(tbar, event_fail, RED);
	 } else {
	    pair_t *cell = cons(make_integer(event), (void *)win);
	    windows = cons(cons((void *)win, cell), windows);
	 }
	 break;
      }

/*       case DEBUG_EVENT_AREA_REGISTERED: {                           */
/* 	 if (!old) {                                                   */
/* 	    sprintf(event_fail, "registering area of unregistred window %p [%s:%s]\n", win, window_name(disp, win), window_class(disp, win)); */
/* 	    fprintf(stderr, "*** ICCCMPANEL ERROR: %s\n", event_fail); */
/* 	    debug(tbar, event_fail, RED);                              */
/* 	 } else {                                                      */
/* 	    SET_CAR(CDR(old), (pair_t *)make_integer(event));          */
/* 	 }                                                             */
/* 	 break;                                                        */
/*       }                                                             */
	 
      default:
	 fprintf(stderr, "Illegal debug_window_event %d\n", event);
	 exit(1);
   }
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    assert_window_list ...                                           */
/*---------------------------------------------------------------------*/
void
assert_window_list(taskbar_t *tbar) {
   Xinfo_t *xinfo = tbar->xinfo;
   Display *disp = xinfo->disp;
   Window root_win = xinfo->root_win;
   Window *wins;
   long num, i;
   long cnt = 0;
   char *fail = 0;
   int failcolor = RED;
   
   /* get the window list */
   wins = get_window_prop_data(disp, root_win,
			       atom__NET_CLIENT_LIST, XA_WINDOW,
			       &num);

   /* check that all windows are registered */
   for(i = 0; i < num; i++) {
      Window w = wins[i];

      if (w != tbar->win && !tooltips_windowp(w)) {
	 xclient_t *xcl = window_xclient(tbar, w);
	 char *name = window_name(disp, w);
	 char *class = window_class(disp, w);
	 char unmappedp = window_iconifiedp(disp, w);
	 int desktop = window_desktop(tbar->xinfo->disp, w);
	 
	 cnt++;

	 if (!xcl) {
	    fail = alloca(2048);
	    snprintf(fail, 2048, "cannot find client desktop: %2d win: %p %s:%s mapped: %d",
		     desktop, w, name, class, unmappedp);
	    break;
	 } else {
	    if (!name || !class
		|| strcmp(xcl->class, class)
		//|| xcl->unmappedp != unmappedp
		|| xcl->desktop != desktop) {
	       fail = alloca(2048);
	       failcolor = ORANGE;
	       snprintf(fail, 2048,
			"client status mismatch desktop: %d/%d win: %p [%s]/[%s] ([%s]/[%s]) mapped: %d/%d", 
			desktop, xcl->desktop,
			w,
			name, xcl->name,
			class, xcl->class,
			unmappedp, xcl->unmappedp);
	       break;
	    } else if (strlen(xcl->name) > 2048) {
	       fail = alloca(2048);
	       snprintf(fail, 2048,
			"client name corrupted: %d/%d win: %p [%s]/[%s] ([%s]/[%s]) mapped: %d/%d", 
			desktop, xcl->desktop,
			w, name, 
			xcl->name, class, xcl->class,
			unmappedp, xcl->unmappedp);
	       break;
	    }
	 }

	 if (name) free(name);
	 if (class) free(class);
      }
   }

   if (fail) {
      fprintf(stderr, "*** ICCCMPANEL ERROR: %s\n", fail);
      debug(tbar, fail, failcolor);
   }

   XFree(wins);
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    debug ...                                                        */
/*---------------------------------------------------------------------*/
void
debug(taskbar_t *tbar, char *msg, long color) {
   Xinfo_t *xinfo = tbar->xinfo;
   pair_t *areas = tbar->areas;
   int desktop = current_desktop(xinfo->disp, xinfo->root_win);
   Window root_win = xinfo->root_win;
   Window *wins;
   Display *disp = xinfo->disp;
   long num, cnt = 0;
   time_t tm;
   
   /* get the window list */
   wins = get_window_prop_data(disp, root_win,
			       atom__NET_CLIENT_LIST, XA_WINDOW,
			       &num);


   FILE *fd = fopen("/tmp/icccmpanel.debug", "a");

   fprintf(fd, "===============================\n");
   time(&tm);
   fprintf(fd, ctime(&tm));
   fprintf(fd, "-------------------------------\n\n");

   // msg
   if (msg) {
      fprintf(fd, "*** ERROR: %s\n\n", msg);
   }
   
   // desktop
   fprintf(fd, "current-desktop: %d\n", desktop);

   // areas
   fprintf(fd, "\nareas len: %d\n", length(areas));
   areas = tbar->areas;
   for (int i = 0; PAIRP(areas); areas = CDR(areas), i++) {
      area_t *ar = (area_t *)CAR(areas);

      if (iconp(ar)) {
	 xclicon_t *xcli = (xclicon_t *)ar;
	 fprintf(fd, " %3d: %s win: %p ignore: %d active: %d %p %s:%s \n",
		 i, ar->name, ar->win, ar->ignore_layout, ar->active,
		 xcli->xcl->win, 
		 xcli->xcl->name, xcli->xcl->class);
      } else {
	 fprintf(fd, " %3d: %s win: %p ignore: %d\n",
		 i, ar->name, ar->win, ar->ignore_layout);
      }
   }

   // icons
   fprintf(fd, "\nicons\n");
   areas = tbar->areas;
   for (int i = 0; PAIRP(areas); areas = CDR(areas)) {
      area_t *ar = (area_t *)CAR(areas);

      if (iconp(ar)) {
	 xclicon_t *xcli = (xclicon_t *)ar;
	 ipicons_t *ip = (ipicons_t *)(ar->parent);
	 char inwin = 0;

	 for (int j = 0; j < num; j++) {
	    if (wins[j] == xcli->xcl->win) {
	       inwin = 1;
	       break;
	    }
	 }
	 
	 fprintf(fd, "%c%3d: desktop: %2d win: %p %s:%s live: %d mapped: %d parent-mapped: %d\n",
		 (inwin ? ' ' : '!'),
		 i++, xcli->xcl->desktop, xcli->xcl->win,
		 xcli->xcl->name, xcli->xcl->class,
		 xcli->xcl->live, xcli->mappedp,
		 ip->icon_mapped);
      }
   }

   // actual windows
   fprintf(fd, "\nwindows\n");
   for (int i = 0; i < num; i++) {
      Window w = wins[i];
      int desktop = window_desktop(tbar->xinfo->disp, w);
      char inarea = 0;

      areas = tbar->areas;

      while (PAIRP(areas)) {
	 area_t *ar = (area_t *)CAR(areas);
	 if (iconp(ar)) {
	    xclicon_t *xcli = (xclicon_t *)ar;
	    if (xcli->xcl->win == w) {
	       inarea = 1;
	       break;
	    }
	 }
	 areas = CDR(areas);
      }
      
      if (w != tbar->win && !tooltips_windowp(w)) {
	 char *name = window_name(disp, w);
	 char *class = window_class(disp, w);
	 char unmappedp = window_iconifiedp(disp, w);
	 fprintf(fd, "%c%3d: desktop: %2d %p %s:%s mapped: %d\n",
		 (inarea ? ' ': '!'),
		 cnt++, desktop, w,
		 name, class,
		 unmappedp);
	 if (name) free(name);
	 if (class) free(class);
      }
   }

   // window events
   fprintf(fd, "\nwindow events %d\n", length(windows));
   
   pair_t *prev = NIL;
   pair_t *ws = windows;
   
   while (PAIRP(ws)) {
      pair_t *car = CAR(ws);
      
      switch (INTEGER_VAL(CADR(car))) {
	 case DEBUG_EVENT_WINDOW_CREATED:
	    fprintf(fd, "  %p: created [%s]\n", CAR(car), CDDR(car));
	    break;

	 case DEBUG_EVENT_AREA_REGISTERED:
	    fprintf(fd, "  %p: registered [%s]\n", CAR(car), CDDR(car));
	    break;
	    
	 case DEBUG_EVENT_WINDOW_DESTROYED:
	    fprintf(fd, "  %p: destroyed\n", CAR(car));
	    break;
      }
      ws = CDR(ws);
   }
   
   fprintf(fd, "\n\n");
	   
   fclose(fd);
   XFree(wins);

   if (msg) {
      taskbar_set_frame_colors(tbar, color, color, color);
   } else {
      taskbar_set_frame_colors(tbar, GREY12, WHITE, GREY9);
   }
   taskbar_refresh_all(tbar);
}
