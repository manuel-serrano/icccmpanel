/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/main.c                             */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Sat Oct 11 05:45:32 2003                          */
/*    Last change :  Thu Jun 12 13:26:25 2025 (serrano)                */
/*    Copyright   :  2003-25 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    ICCCMPANEL (from fspanel)                                        */
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
#include "grip.h"
#include "desktop.h"
#include "hfill.h"
#include "clock.h"
#include "app.h"
#include "icons.h"
#include "evloop.h"

/*---------------------------------------------------------------------*/
/*    int                                                              */
/*    main ...                                                         */
/*---------------------------------------------------------------------*/
int
#ifdef NOSTDLIB
_start(void) {
#else
main(int argc, char *argv[]) {
#endif
   Xinfo_t *xinfo;
   taskbar_t *tbar;
   config_t *config;
   pair_t *lst;
   
   /* configure ICCCMPANEL */
   if (!(config = make_config(argc, argv)))
      exit(10);

   /* initialize our simple X toolkit */
   xinfo = init_mstk(config);

#if DEBUG
   fprintf(stderr, "### mstk intialized disp=%p root=%p\n", xinfo->disp, xinfo->root_win);
#endif
   
   /* allocate the taskbar */
   tbar = make_taskbar(xinfo, config);

#if DEBUG
   fprintf(stderr, "### taskbar created.\n");
#endif
   
   if (!tbar) {
      fprintf(stderr, "*** ERROR: cannot create taskbar\nicccmap exit...\n");
      exit(11);
   }

   /* reset debug info */
   unlink("/tmp/icccmpanel.debug");

   /* start all the plugin */
   lst = config->plugins;
   while (PAIRP(lst)) {
      plugin_t *p = (plugin_t *)CAR(lst);

      p->start(tbar, p->args);

      lst = CDR(lst);
   }

#if DEBUG
   fprintf(stderr, "### plugins created.\n");
#endif
   
   /* build the taskbar */
   taskbar_area_do_layout(tbar);
   taskbar_register_xclients(tbar);
   taskbar_refresh_all(tbar);
   
#if DEBUG
   fprintf(stderr, "### taskbar ready.\n");
#endif
   
   /* refresh the taskbar */
   XSync(xinfo->disp, True);
   
#if DEBUG
   fprintf(stderr, "### Entering event loop.\n");
#endif
   
   /* the event loop */
   evloop(tbar);

   sleep(2);
   
   /* close before exit */
   closeup_mstk(xinfo);

   /* we are done */
   return 0;
}
