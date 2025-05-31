/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/debug.h                            */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Thu Jul 22 15:21:17 2004                          */
/*    Last change :  Sat May 24 13:03:40 2025 (serrano)                */
/*    Copyright   :  2004-25 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    The workspace.                                                   */
/*=====================================================================*/
#ifndef _DEBUG_H
#define _DEBUG_H

/*---------------------------------------------------------------------*/
/*    DEBUG_EVENT                                                      */
/*---------------------------------------------------------------------*/
#define DEBUG_EVENT_WINDOW_CREATED 1
#define DEBUG_EVENT_WINDOW_DESTROYED 2
#define DEBUG_EVENT_AREA_REGISTERED 3

/*---------------------------------------------------------------------*/
/*    Exports ...                                                      */
/*---------------------------------------------------------------------*/
void debug_window_event(taskbar_t *tbar, Window, int);
void assert_window_list(taskbar_t *tbar);
void debug(taskbar_t *tbar, char *msg, long);

#endif
