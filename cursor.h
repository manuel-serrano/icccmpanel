/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/cursor.h                           */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Mon Oct 13 13:35:06 2003                          */
/*    Last change :  Sun Nov 24 06:49:07 2024 (serrano)                */
/*    Copyright   :  2003-24 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    MSpanel cursor                                                   */
/*=====================================================================*/
#if (!defined(MSPANEL_CURSOR_H))
#define MSPANEL_CURSOR_H

/*---------------------------------------------------------------------*/
/*    extern declarations                                              */
/*---------------------------------------------------------------------*/
extern void init_cursor(Xinfo_t *, char *);
extern int cursor_windowp(Window); 
extern void cursor_refresh();
extern void cursor_hide();
extern void show_cursor(long time, taskbar_t *);
#endif
