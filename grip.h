/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/grip.h                             */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Thu Jul 22 15:21:17 2004                          */
/*    Last change :  Mon Aug  2 14:20:23 2004 (serrano)                */
/*    Copyright   :  2004 Manuel Serrano                               */
/*    -------------------------------------------------------------    */
/*    The grip.                                                        */
/*=====================================================================*/
#ifndef _GRIP_H
#define _GRIP_H

/*---------------------------------------------------------------------*/
/*    Export ...                                                       */
/*---------------------------------------------------------------------*/
extern area_t *make_grip( taskbar_t *, int, int );
extern void *start_grip( void *, pair_t * );
void parse_grip( config_t *, pair_t * );

#endif
