/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/shaker.h                           */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Mon Oct 13 13:35:06 2003                          */
/*    Last change :  Sat Dec 21 06:28:22 2024 (serrano)                */
/*    Copyright   :  2003-24 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    MSpanel shaker                                                   */
/*=====================================================================*/
#if (!defined(MSPANEL_SHAKER_H))
#define MSPANEL_SHAKER_H

/*---------------------------------------------------------------------*/
/*    extern declarations                                              */
/*---------------------------------------------------------------------*/
extern void init_shaker(Xinfo_t *, char *);
extern int shaker_windowp(Window); 
extern void shaker_refresh();
extern void shaker_hide();
extern void show_shaker(long time, taskbar_t *);
#endif
