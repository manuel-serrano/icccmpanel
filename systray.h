/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel2/systray.h                         */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Thu Jul 22 15:21:17 2004                          */
/*    Last change :  Sun Mar 18 18:09:57 2007 (serrano)                */
/*    Copyright   :  2004-07 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    The system tray prototypes.                                      */
/*=====================================================================*/
#ifndef _SYSTRAY_H
#define _SYSTRAY_H

/*---------------------------------------------------------------------*/
/*    Export ...                                                       */
/*---------------------------------------------------------------------*/
void systray_message( XEvent * );
extern void *start_systray( void *, pair_t * );
void parse_systray( config_t *, pair_t * );

#endif
