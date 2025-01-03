/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/debug.c                            */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Fri Jul 19 08:43:57 2024                          */
/*    Last change :  Fri Aug 30 06:52:31 2024 (serrano)                */
/*    Copyright   :  2024 Manuel Serrano                               */
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
/*    debug ...                                                        */
/*---------------------------------------------------------------------*/
void
debug(area_t *ar) {
   taskbar_t *tbar = ar->taskbar;
   Xinfo_t *xinfo = tbar->xinfo;
   pair_t *areas = tbar->areas;
   int desktop = current_desktop(xinfo->disp, xinfo->root_win);

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

   fclose(fd);
}
