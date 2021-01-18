/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/app.h                              */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Thu Jul 22 15:21:17 2004                          */
/*    Last change :  Mon Aug 11 10:58:19 2014 (serrano)                */
/*    Copyright   :  2004-14 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    The workspace.                                                   */
/*=====================================================================*/
#ifndef _APPS_H
#define _APPS_H

/*---------------------------------------------------------------------*/
/*    app_t                                                            */
/*---------------------------------------------------------------------*/
typedef struct app {
   /* the command */
   char *command;
   /* the file name */
   char *image;
} app_t;

/*---------------------------------------------------------------------*/
/*    Exportations ...                                                 */
/*---------------------------------------------------------------------*/
area_t *make_app( taskbar_t *, app_t *, int, int, int );
extern void *start_apps( void *, pair_t * );
void parse_apps( config_t *, pair_t * );

#endif
