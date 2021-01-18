/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/taskbar.h                          */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Thu Jul 22 14:32:38 2004                          */
/*    Last change :  Sat Oct 10 06:36:02 2015 (serrano)                */
/*    Copyright   :  2004-15 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    Taskbar management                                               */
/*=====================================================================*/
#ifndef _TASKBAR_H
#define _TASKBAR_H
#include <sys/types.h>
#include <regex.h>

/*---------------------------------------------------------------------*/
/*    THe maximun number of users (areas) that can plug into           */
/*    an xclient_t structure.                                          */
/*---------------------------------------------------------------------*/
#define XCLIENT_MAX_NUMBER 3

/*---------------------------------------------------------------------*/
/*    iconentry ...                                                    */
/*---------------------------------------------------------------------*/
typedef struct iconentry {
   icondescr_t *icondescr;
   regex_t classreg;
   regex_t namereg;
   char loaded;
   Pixmap icon, mask;
} iconentry_t;

/*---------------------------------------------------------------------*/
/*    xclient                                                          */
/*    -------------------------------------------------------------    */
/*    A xclient is the structure representing an X iconified window    */
/*    in the taskbar.                                                  */
/*---------------------------------------------------------------------*/
typedef struct xclient {
   /* The unique X client identifier */
   int id;
   /* is this xclient still live */
   char live;
   /* The X Window associated with this xclient */
   Window win;
   /* the name of the application and its class */
   char *name, *class;
   /* the desktop number where it is uniconfied */
   int desktop;
   /* is it iconified */
   int unmappedp;
   /* user pointer */
   void *user[ XCLIENT_MAX_NUMBER ];
   /* the icon associated with the xclient */
   Pixmap icon, mask;
   /* the dimension of the icon */
   int icon_width, icon_height;
} xclient_t;

/*---------------------------------------------------------------------*/
/*    taskbar                                                          */
/*    -------------------------------------------------------------    */
/*    A taskbar is represented by exactly one window. The icons        */
/*    are not implemented by means of X windows. They are plain        */
/*    area in the taskbar window.                                      */
/*---------------------------------------------------------------------*/
typedef struct taskbar {
   /* X information */
   Xinfo_t *xinfo;
   Window win;
   /* global configuration */
   config_t *config;
   /* cached configuration */
   int top;
   int autohide;
   int hidespeed;
   int unhidespeed;
   int hiddenp;
   /* the X coordinates of the taskbar window */
   int x, y, width, height;
   int desktop;
   /* the pixel number for the border */
   int border, aborder, linesep;
   /* the list of areas */
   pair_t *areas;
   /* the list of xclients  */
   pair_t *xclients;
   pair_t *_freexclients;
   /* the number of connected xclient managers */
   int xclient_manager_number;
   /* the icon cache */
   pair_t *iconcache;
} taskbar_t;

/*---------------------------------------------------------------------*/
/*    Export ...                                                       */
/*---------------------------------------------------------------------*/
extern taskbar_t *make_taskbar( Xinfo_t *, config_t * );
extern void taskbar_hide( taskbar_t * ); 
extern void taskbar_unhide( taskbar_t * ); 
extern void taskbar_refresh( taskbar_t * );
extern void taskbar_refresh_all( taskbar_t * );
extern void taskbar_property_notify( taskbar_t *, XEvent * );
extern void taskbar_destroy_notify( taskbar_t *, XEvent * );
extern void taskbar_register_xclients( taskbar_t * );
extern void taskbar_area_do_layout( taskbar_t * );
extern int taskbar_get_xclient_manager_number( taskbar_t * );

#endif
