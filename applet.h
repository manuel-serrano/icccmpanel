/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/applet.h                           */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Thu Jul 22 15:21:17 2004                          */
/*    Last change :  Mon Aug 11 11:32:15 2014 (serrano)                */
/*    Copyright   :  2004-14 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    The workspace.                                                   */
/*=====================================================================*/
#ifndef _APPLETS_H
#define _APPLETS_H

/*---------------------------------------------------------------------*/
/*    applet_t                                                         */
/*---------------------------------------------------------------------*/
typedef struct applet {
   /* the local config */
   config_t *config;
} applet_t;

/*---------------------------------------------------------------------*/
/*    Exportations ...                                                 */
/*---------------------------------------------------------------------*/
area_t *make_applet( taskbar_t *, applet_t *, int, int, int );
extern void *start_applets( void *, pair_t * );
void parse_applets( config_t *, pair_t * );

#endif
