/*=====================================================================*/
/*    serrano/prgm/utils/mspanel2/tooltips.h                           */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Mon Oct 13 13:35:06 2003                          */
/*    Last change :  Fri Jul 23 21:59:42 2004 (serrano)                */
/*    Copyright   :  2003-04 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    MSpanel tooltips                                                 */
/*=====================================================================*/
#if( !defined( MSPANEL_TOOLTIPS_H ) )
#define MSPANEL_TOOLTIPS_H

/*---------------------------------------------------------------------*/
/*    extern declarations                                              */
/*---------------------------------------------------------------------*/
extern void init_tooltips( Xinfo_t * );
extern int tooltips_windowp( Window ); 
extern void tooltips_setup( char *, int, int, int );
extern void tooltips_refresh();
extern void tooltips_hide();
#endif
