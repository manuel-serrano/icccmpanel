/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/hfill.h                            */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Thu Jul 22 15:21:17 2004                          */
/*    Last change :  Mon Aug  2 14:34:30 2004 (serrano)                */
/*    Copyright   :  2004 Manuel Serrano                               */
/*    -------------------------------------------------------------    */
/*    The workspace.                                                   */
/*=====================================================================*/
#ifndef _HFILL_H
#define _HFILL_H

/*---------------------------------------------------------------------*/
/*    Exportations ...                                                 */
/*---------------------------------------------------------------------*/
area_t *make_hfill( taskbar_t *, int, int );
extern void *start_hfill( void *, pair_t * );
void parse_hfill( config_t *, pair_t * );

#endif
