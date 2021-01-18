/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/main.c                             */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Sat Oct 11 05:45:32 2003                          */
/*    Last change :  Sat Nov 30 09:39:20 2019 (serrano)                */
/*    Copyright   :  2003-19 Manuel Serrano                            */
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
main( int argc, char *argv[] ) {
#endif
   Xinfo_t *xinfo;
   taskbar_t *tbar;
   config_t *config;
   pair_t *lst;
   
   /* configure ICCCMPANEL */
   if( !(config = make_config( argc, argv )) )
      exit( 10 );

   /* initialize our simple X toolkit */
   xinfo = init_mstk( config );

   /* allocate the taskbar */
   tbar = make_taskbar( xinfo, config );

   if( !tbar ) exit( 11 );

   /* start all the plugin */
   lst = config->plugins;
   while( PAIRP( lst ) ) {
      plugin_t *p = (plugin_t *)CAR( lst );

      p->start( tbar, p->args );
      
      lst = CDR( lst );
   }

   /* build the taskbar */
   taskbar_area_do_layout( tbar );
   taskbar_register_xclients( tbar );
   taskbar_refresh_all( tbar );
   
   /* refresh the taskbar */
   XSync( xinfo->disp, True );
   
   /* the event loop */
   evloop( tbar );
   
   /* close up out simple X toolkit */
   closeup_mstk( xinfo );

   /* we are done */
   return 0;
}
