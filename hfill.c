/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/hfill.c                            */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Fri Jul 23 22:15:38 2004                          */
/*    Last change :  Thu May 22 13:08:30 2025 (serrano)                */
/*    Copyright   :  2004-25 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    The hfill                                                        */
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
#include "hfill.h"

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    refresh_hfill ...                                                */
/*---------------------------------------------------------------------*/
static void
refresh_hfill(area_t *ar) {
}

/*---------------------------------------------------------------------*/
/*    area_t *                                                         */
/*    make_hfill ...                                                   */
/*---------------------------------------------------------------------*/
area_t *
make_hfill(taskbar_t *tbar, int width, int height) {
   area_t *ar = calloc(1, sizeof(area_t));

   if (!ar) exit(10);

   ar->name = "hfill";
   ar->uwidth = width;
   ar->uheight = height;

   ar->refresh = &refresh_hfill;

   /* bind the area in the taskbar */
   ar->taskbar = tbar;
   tbar->areas = cons(ar, tbar->areas);
   
   return ar;
}

/*---------------------------------------------------------------------*/
/*    void *                                                           */
/*    start_hfill ...                                                  */
/*---------------------------------------------------------------------*/
void *
start_hfill(void *tb, pair_t *args) {
   taskbar_t *tbar = (taskbar_t *)tb;
   config_t *config = tbar->config;
   int width = INTEGER_VAL(CAR(args));


   return make_hfill(tbar, width, config->taskbar_height - 1);
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    parse_hfill ...                                                  */
/*---------------------------------------------------------------------*/
void
parse_hfill(config_t *config, pair_t *lst) {
   pair_t *l = CDR(lst);
   integer_t *width = make_integer(9);

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
	 }
      } else {
	 parse_error("Illegal hfill", (obj_t *)lst);
      }
      l = CDR(l);
   }

   register_plugin(config, make_plugin(start_hfill, cons(width, NIL)));
}
