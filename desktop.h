/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/desktop.h                          */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Thu Jul 22 15:21:17 2004                          */
/*    Last change :  Mon Aug  2 14:39:37 2004 (serrano)                */
/*    Copyright   :  2004 Manuel Serrano                               */
/*    -------------------------------------------------------------    */
/*    The workspace.                                                   */
/*=====================================================================*/
#ifndef _DESKTOP_H
#define _DESKTOP_H

/*---------------------------------------------------------------------*/
/*    Exportations ...                                                 */
/*---------------------------------------------------------------------*/
area_t *make_desktop( taskbar_t *, int, int );
extern void *start_desktop( void *, pair_t * );
void parse_desktop( config_t *, pair_t * );

#endif
