/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/icons.h                            */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Thu Jul 22 15:21:17 2004                          */
/*    Last change :  Mon Aug  2 15:46:49 2004 (serrano)                */
/*    Copyright   :  2004 Manuel Serrano                               */
/*    -------------------------------------------------------------    */
/*    The icons.                                                       */
/*=====================================================================*/
#ifndef _ICON_H
#define _ICON_H

/*---------------------------------------------------------------------*/
/*    Exportations ...                                                 */
/*---------------------------------------------------------------------*/
area_t *make_icons( taskbar_t *, int, int, char, char );
extern void *start_icons( void *, pair_t * );
void parse_icons( config_t *, pair_t * );

#endif
