/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/evloop.h                           */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Fri Jul 23 06:00:03 2004                          */
/*    Last change :  Mon Nov  8 10:16:31 2021 (serrano)                */
/*    Copyright   :  2004-21 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    Event loop                                                       */
/*=====================================================================*/
#ifndef _EVLOOP_H
#define _EVLOOP_H

/*---------------------------------------------------------------------*/
/*    export ...                                                       */
/*---------------------------------------------------------------------*/
extern void evloop_timeout(area_t *);
extern void evloop(taskbar_t *);
char *x_atom_name(Atom at);
char *x_event_name(XEvent *ev);
#endif
