/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/debug.c                            */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Fri Jul 19 08:43:57 2024                          */
/*    Last change :  Wed Apr 23 07:38:29 2025 (serrano)                */
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
   int fail = 0;

   /* get the window list */
   wins = get_window_prop_data(disp, root_win,
			       atom__NET_CLIENT_LIST, XA_WINDOW,
			       &num);

   /* check that all windows are registered */
   for(i = 0; i < num; i++) {
      Window w = wins[ i ];

      if (w != tbar->win && !tooltips_windowp(w)) {
	 xclient_t *xcl = window_xclient(tbar, w);
	 char *name = window_name(disp, w);
	 char *class = window_class(disp, w);
	 char unmappedp = window_iconifiedp(disp, w);
	 int desktop = window_desktop(tbar->xinfo->disp, w);
	 
	 cnt++;

	 if (!xcl) {
	    fprintf(stderr, "icccmpanel: cannot find client %d %p %s (%s) %d\n", w,
		    desktop, name, class, unmappedp);
	    fail = 1;
	 } else {
	    if (strcmp(xcl->name, name)
		|| strcmp(xcl->class, class)
		|| xcl->unmappedp != unmappedp
		|| xcl->desktop != desktop) {
	       fprintf(stderr, "icccmpanel: client status mismatch %d/%d %p [%s]/[%s] ([%s]/[%s]) %d/%d\n", 
		       desktop, xcl->desktop,
		       w, name, 
		       xcl->name, class, xcl->class,
		       unmappedp, xcl->unmappedp);
		       
	       fail = 1;
	    }
	 }
      }
   }

   /* check that no icccmpanel window is left pending */
   if (cnt != length(tbar->xclients)) {
      fprintf(stderr, "icccmpane: client list corrupted\n");
      fail = 1;
   }

   if (fail) {
      taskbar_set_frame_colors(tbar, RED, RED);
      taskbar_refresh(tbar);
   }
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    debug ...                                                        */
/*---------------------------------------------------------------------*/
void
debug(area_t *ar) {
   taskbar_t *tbar = ar->taskbar;
   Xinfo_t *xinfo = tbar->xinfo;
   pair_t *areas = tbar->areas;
   int desktop = current_desktop(xinfo->disp, xinfo->root_win);
   Window root_win = xinfo->root_win;
   Window *wins;
   Display *disp = xinfo->disp;
   long num, i;
   
   /* get the window list */
   wins = get_window_prop_data(disp, root_win,
			       atom__NET_CLIENT_LIST, XA_WINDOW,
			       &num);


   FILE *fd = fopen("/tmp/icccmpanel.debug", "w");

   // desktop
   fprintf(fd, "===============================\n");
   fprintf(fd, "current-desktop: %d\n", desktop);

   // areas
   fprintf(fd, "\nareas len: %d\n", length(areas));
   areas = tbar->areas;
   for (int i = 0; PAIRP(areas); areas = CDR(areas), i++) {
      area_t *ar = (area_t *)CAR(areas);

      fprintf(fd, "%3d: %s win: %8x ignore: %d\n",
	      i, ar->name, ar->win, ar->ignore_layout);
   }

   // icons
   fprintf(fd, "\nicons\n");
   areas = tbar->areas;
   for (int i = 0; PAIRP(areas); areas = CDR(areas)) {
      area_t *ar = (area_t *)CAR(areas);

      if (iconp(ar)) {
	 xclicon_t *xcli = (xclicon_t *)ar;
	 ipicons_t *ip = (ipicons_t *)(ar->parent);
	 
	 fprintf(fd, "%3d: desktop: %2d id: %8x %s:%s [%p] live: %d mapped: %d parent-mapped: %d\n",
		 i++, xcli->xcl->desktop, xcli->xcl->win,
		 xcli->xcl->name, xcli->xcl->class, xcli->xcl->name, 
		 xcli->xcl->live, xcli->mappedp,
		 ip->icon_mapped);
      }
   }

   // actual windows
   fprintf(fd, "\nwindows\n");
   for(i = 0; i < num; i++) {
      Window w = wins[ i ];
      int desktop = window_desktop(tbar->xinfo->disp, w);

      if (w != tbar->win && !tooltips_windowp(w)) {
	 char *name = window_name(disp, w);
	 char *class = window_class(disp, w);
	 char unmappedp = window_iconifiedp(disp, w);
	 fprintf(fd, "icccmpanel: %d %8x %s:%s mappend: %d\n",
		 desktop, w, name, class, unmappedp);
      }
   }

   fclose(fd);
}


