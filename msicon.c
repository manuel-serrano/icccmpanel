/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/msicon.c                           */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Mon Oct 13 13:34:12 2003                          */
/*    Last change :  Mon Jan  3 14:43:26 2005 (serrano)                */
/*    Copyright   :  2003-05 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    MSpanel icon tools                                               */
/*=====================================================================*/
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#ifdef HAVE_XPM
#  include <X11/xpm.h>
#endif

#include "icccmpanel.h"
#include "msicon.h"

/*---------------------------------------------------------------------*/
/*    iconimg_t *                                                      */
/*    init_icon_table ...                                              */
/*---------------------------------------------------------------------*/
iconimg_t *
init_icon_table( FILE *fin ) {
   iconimg_t *res = 0;
   int size = 20;
   char buffer[ 2048 ];
   int n = 0, l;
   
   res = (iconimg_t *)calloc( size, sizeof( struct iconimg ) );

   while( fgets( buffer, 2048, fin ) ) {
      char *aux;

      if( n == size ) {
	 size *= 2;
	 res = (iconimg_t *)realloc( res, size );
      }

      /* we are done */
      if( !strncmp( buffer, "}", 1 ) )
	 break;
      
      /* fetch the icon file name */
      aux = strrchr( buffer, ':' );
      if( ! aux ) continue;

      *aux = 0;
      aux++;
      aux += strspn( aux, " \t" );
      aux[ strcspn( aux, " \t\n" ) ] = 0;
      res[ n ].filename = strdup( aux );

      /* fetch the instance name */
      aux = strrchr( buffer, ',' );
      if( aux  ) {
	 int l;
	 *aux = 0;
	 aux++;
	 aux += strspn( aux, " \t" );
	 l = strcspn( aux, " \t" );
	 aux[ l ] = 0;
	 if( aux[ l - 1 ] == '$' ) {
	    aux[ l - 1 ] = 0;
	    res[ n ].suffix = strdup( aux );
	 } else {
	    if( aux[ 0 ] == '^') {
	       res[ n ].prefix = strdup( aux + 1 );
	    }
	 }
      }

      /* fetch the class name */
      aux = buffer;
      l = strspn( buffer, " \t" );
      if( l ) aux += l;
      
      aux[ strcspn( aux, " \t" ) ] = 0;
      if( !strcmp( aux, "*" ) ) {
	 res[ n ].class = ICONIMG_STARCLASS;
      } else {
	 res[ n ].class = strdup( aux );
      }

      n++;
   }
   
   return res;
}
   
/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    load_pixmap ...                                                  */
/*---------------------------------------------------------------------*/
static void
load_pixmap( Display *disp, Window win, iconimg_t *ic ) {
   if( !ic->icon ) {
      XpmReadFileToPixmap( disp, win, ic->filename,
			   &(ic->icon), &(ic->mask),
			   NULL );
   }
}
   
/*---------------------------------------------------------------------*/
/*    char                                                             */
/*    find_user_icon ...                                               */
/*---------------------------------------------------------------------*/
char
find_user_icon( Xinfo_t *xinfo, Window win,
		iconimg_t *table, char *class, char *instance,
		Pixmap *icon, Pixmap *mask ) {
   Display *disp = xinfo->disp;
   iconimg_t *r = table;
   int li;
   
   if( !class || !table || !instance )
      return 0;
   
   li = strlen( instance );
   
   while( r->class != ICONIMG_EMPTYCLASS ) {
      if( r->class == ICONIMG_STARCLASS ) {
	 load_pixmap( disp, win, r );

	 *icon = r->icon;
	 *mask = r->mask;
	 
	 return 1;
      } else {
	 if( !strcasecmp( class, r->class ) ) {
	    if( !r->prefix && !r->suffix ) {
	       load_pixmap( disp, win, r );

	       *icon = r->icon;
	       *mask = r->mask;
	       
	       return 1;
	    } else {
	       if( r->suffix ) {
		  int lrs = strlen( r->suffix );

		  if( (li > lrs) &&
		      !(strcmp( &instance[ li - lrs ], r->suffix )) ) {
		     load_pixmap( disp, win, r );

		     *icon = r->icon;
		     *mask = r->mask;
		     
		     return 1;
		  }
	       } else {
		  if( r->prefix ) {
		     int lrp = strlen( r->prefix );
		     
		     if( (li > lrp) &&
			 !(strncmp( instance, r->prefix, lrp )) ) {
			load_pixmap( disp, win, r );

			*icon = r->icon;
			*mask = r->mask;
		     
			return 1;
		     }
		  }
	       }
	    }
	 }
      }

      r++;
   }
}
