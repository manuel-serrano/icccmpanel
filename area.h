/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/area.h                             */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Sat Jul 24 14:10:36 2004                          */
/*    Last change :  Wed Mar 27 07:04:56 2019 (serrano)                */
/*    Copyright   :  2004-19 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    Areas                                                            */
/*=====================================================================*/
#ifndef _AREA_H
#define _AREA_H

/*---------------------------------------------------------------------*/
/*    area_t                                                           */
/*---------------------------------------------------------------------*/
typedef struct area {
   /* the name of the area */
   char *name;
   /* its parent area */
   struct area *parent;
   /* its subareas */
   pair_t *subareas;
   /* the taskbar the are belongs to */
   struct taskbar *taskbar;
   /* its X window */
   Window win;
   /* a boolean that is true if the area is ignore by the taskbar */
   int ignore_layout;
   /* the user width and height of the area */
   int uwidth, uheight;
   /* the computed position of the area (relative to the taskbar) */
   int x, y, width, height;
   /* active area */
   int active;
   /* the timeout delay */
   int timeout_delay;
   int timeout_count;
   /* the callbacks */
   int (*timeout)( struct area * );
   void (*layout)( struct area * );
   void (*refresh)( struct area * );
   void (*desktop_notify)( struct area * );
   void (*enter_notify)( XEvent *, struct area * );
   void (*leave_notify)( XEvent *, struct area * );
   void (*motion_notify)( XEvent *, struct area * );
   void (*button_press)( XEvent *, struct area * );
   void (*key_press)( XEvent *, struct area * );
   void (*create_notify)( struct area *, xclient_t * );
   void (*destroy_notify)( struct area *, xclient_t * );
   void (*state_notify)( struct area *, xclient_t * );
   void (*client_message)( XEvent *, struct area * );
} area_t;

/*---------------------------------------------------------------------*/
/*    Export ...                                                       */
/*---------------------------------------------------------------------*/
extern area_t *find_area( taskbar_t *, Window );
extern Window make_area_window_parent( taskbar_t *, Window );
extern Window make_area_window( taskbar_t * );

#endif
