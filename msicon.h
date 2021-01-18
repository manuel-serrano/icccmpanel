/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/msicon.h                           */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Mon Oct 13 13:35:06 2003                          */
/*    Last change :  Sun Jul 25 07:05:49 2004 (serrano)                */
/*    Copyright   :  2003-04 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    MSpanel icon tools                                               */
/*=====================================================================*/
#if( !defined( MSPANEL_ICON_H ) )
#define MSPANEL_ICON_H

/*---------------------------------------------------------------------*/
/*    Constants                                                        */
/*---------------------------------------------------------------------*/
#define ICONIMG_EMPTYCLASS (char *)0
#define ICONIMG_STARCLASS (char *)1

/*---------------------------------------------------------------------*/
/*    iconimg                                                          */
/*---------------------------------------------------------------------*/
typedef struct iconimg {
   char *class;
   char *suffix;
   char *prefix;
   char *filename;
   Pixmap icon;
   Pixmap mask;
} iconimg_t;

/*---------------------------------------------------------------------*/
/*    extern declarations                                              */
/*---------------------------------------------------------------------*/
iconimg_t *init_icon_table( FILE * );
char find_user_icon( Xinfo_t *, Window,
		     iconimg_t *, char *, char *, Pixmap *, Pixmap * );
#endif
