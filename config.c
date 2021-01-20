/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/config.c                           */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Fri Oct 17 22:18:02 2003                          */
/*    Last change :  Sat Nov 30 10:30:08 2019 (serrano)                */
/*    Copyright   :  2003-21 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    The parsing of the MSpanel config file.                          */
/*=====================================================================*/
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <unistd.h>
#include <stdlib.h>

#ifdef HAVE_XPM
#  include <X11/xpm.h>
#endif

#include "configure.h"
#include "icccmpanel.h"
#include "read.h"
#include "grip.h"
#include "hfill.h"
#include "desktop.h"
#include "clock.h"
#include "app.h"
#include "icons.h"
#include "systray.h"
#include "applet.h"
#include "wifi.h"
#include "battery.h"
#include "hibernate.h"
#include "volume.h"
#include "cron.h"
#include "exec.h"

/*---------------------------------------------------------------------*/
/*    Cached symbols                                                   */
/*---------------------------------------------------------------------*/
symbol_t *sym_true, *sym_false;
symbol_t *sym_name;
symbol_t *sym_width;

/*---------------------------------------------------------------------*/
/*    Static variables                                                 */
/*---------------------------------------------------------------------*/
static pair_t *parsers = NIL;

/*---------------------------------------------------------------------*/
/*    char *                                                           */
/*    expand_env ...                                                   */
/*---------------------------------------------------------------------*/
char *
expand_env( char *path ) {
   char buf[ 1024 ];
   int l = strlen( path );
   int lr = l;
   char *res = malloc( l + 1 );
   int r = 0, w = 0;

   while( r < l ) {
      int c = path[ r ];
      if( c == '~' ) {
	 char *home = getenv( "HOME" );
	 int lh = strlen( home );

	 res = realloc( res, l + lh );
	 strcpy( &(res[ w ]), home );

	 w += lh;
	 r++;
	 continue;
      }
      if( c == '$' ) {
	 /* find the end of variable */
	 int r2 = r + 1;

	 while( (r2 < l) &&
		(path[ r2 ] != '/') &&
		(path[ r2 ] != '$') &&
		(path[ r2 ] != '~') )
	    r2++;

	 if( r2 > (r + 1) ) {
	    char *var;
	    
	    strncpy( buf, &path[ r + 1 ], r2 - r - 1 );
	    buf[ r2 - r - 1 ] = 0;
	    var = getenv( buf );
	    
	    if( var ) {
	       int lh = strlen( var );
	 
	       res = realloc( res, l + lh );
	       strcpy( &(res[ w ]), var );

	       w += lh;
	       r = r2;

	       continue;
	    } else {
	       fprintf( stderr, "Environment variable unbound `%s'\n", buf );
	    }
	 }
      }
      res[ w ] = c;
      w++;
      r++;
   }

   res[ w ] = 0;
   return res;
}

/*---------------------------------------------------------------------*/
/*    char *                                                           */
/*    find_icon ...                                                    */
/*---------------------------------------------------------------------*/
char *
find_icon( config_t *config, char *file ) {
   static char buffer[ 2048 ];
   pair_t *lst = config->icon_directories;
   
   while( PAIRP( lst ) ) {
      char *dir = (char *)CAR( lst );

      sprintf( buffer, "%s/%s", dir, file );

      if( !access( buffer, R_OK ) ) return buffer;

      lst = CDR( lst );
   }

   return 0L;
}

/*---------------------------------------------------------------------*/
/*    pair_t *                                                         */
/*    add_icondescr ...                                                */
/*---------------------------------------------------------------------*/
pair_t *
add_icondescr( config_t *config, char *class, char *name, char *fname ) {
   char *path;

   if( path = find_icon( config, fname ) ) {
      icondescr_t *id = (icondescr_t *)calloc( 1, sizeof( icondescr_t ) );
      id->class = class ? strdup( class ) : 0;
      id->name = name ? strdup( name ) : 0;
      id->filename = strdup( path );

      config->icondescrs = cons( id, config->icondescrs );

      return config->icondescrs;
   } else {
      return 0L;
   }
}
      
/*---------------------------------------------------------------------*/
/*    char *                                                           */
/*    prefix ...                                                       */
/*---------------------------------------------------------------------*/
char *
prefix( char *s ) {
   char *l = strrchr( s, '.' );

   if( l > s ) {
      *l = 0;
      return s;
   } else {
      return 0;
   }
}

/*---------------------------------------------------------------------*/
/*    char *                                                           */
/*    find_rc_file ...                                                 */
/*---------------------------------------------------------------------*/
char *
find_rc_file( char *layout ) {
   static char fname[ 2048 ];
   char host[ 512 ], buf[ 512 ];
   char *home = getenv( "HOME" );
   char *dir;
  
   gethostname( host, 512 );
   sprintf( buf, "icccmpanel.%s", host );
   dir = buf;
  
   while( dir ) {
      if( layout ) {
	 sprintf( fname, "%s/.config/%s/icccmpanelrc.%s.%s", home, dir, host, layout );
	 if( !access( fname, R_OK ) ) return fname;
      }
      
      sprintf( fname, "%s/.config/%s/icccmpanelrc.%s", home, dir, host );

      if( !access( fname, R_OK ) ) return fname;
      
      sprintf( fname, "%s/.config/%s/icccmpanelrc", home, dir );
      if( !access( fname, R_OK ) ) return fname;

      dir = prefix( dir );
   }
   
   sprintf( fname, "%s/.config/%s/%s/icccmpanelrc",
	    ICCCMPANEL_PREFIX,
	    ICCCMPANEL_DIR,
	    ICCCMPANEL_RELEASE );

   if( !access( fname, R_OK ) )
      return fname;
   else
      return 0L;
}

/*---------------------------------------------------------------------*/
/*    static config_t *                                                */
/*    default_config ...                                               */
/*---------------------------------------------------------------------*/
static config_t *
default_config( config_t *config ) {
   char *dir;
   int len;
   
   config->app_width = 22;
   config->taskbar_autohide = 0;
   config->taskbar_hidespeed = 1;
   config->taskbar_unhidespeed = 3;
   config->taskbar_linesep = 1;
   config->taskbar_border = 1;
   config->taskbar_x = -1;
   config->taskbar_y = -1;
   config->taskbar_width = -1;
   config->taskbar_height = 19;
   config->taskbar_always_on_top = 0;

   config->icon_mapped = 0;
   config->icon_all_desktop = 0;
   config->color_shadow = SHADOW;
   config->relief = 1;

   config->gradient_grey_min = 0xbb;
   config->gradient_grey_max = 0xdd;
   
   config->animspeed = 1;
   config->fontname_plain = "-*-lucidatypewriter-medium-r-normal-*-10-100-75-75-m-60-iso8859-*";
   config->fontname_bold = "-*-lucidatypewriter-bold-r-normal-*-10-100-75-75-m-60-iso8859-*";
   config->tooltips_fontname = "-*-lucidatypewriter-bold-r-normal-*-10-100-75-75-m-60-iso8859-*";

   config->icon_size = 16;
   config->update_netwmicon = 1;
   
   len = strlen( ICCCMPANEL_PREFIX ) +
      strlen( ICCCMPANEL_DIR ) + 
      strlen( ICCCMPANEL_ICON_DIR ) +
      strlen( "16x16" ) + 3 + 1;
   dir = malloc( len );
   sprintf( dir, "%s/%s/%s/%s",
	    ICCCMPANEL_PREFIX,
	    ICCCMPANEL_DIR,
	    ICCCMPANEL_ICON_DIR,
	    "16x16" );
   config->icon_directories = cons( dir, NIL );

   config->icondescrs = 0L;
   
   return config;
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    register_parser ...                                              */
/*---------------------------------------------------------------------*/
void
register_parser( symbol_t *sym, void (*parser)( config_t *, pair_t * ) ) {
   parsers = cons( cons( sym, ((pair_t *)parser) ), parsers );
}

/*---------------------------------------------------------------------*/
/*    static (void (*)( config_t *, pair_t * ))                        */
/*    find_parser ...                                                  */
/*---------------------------------------------------------------------*/
static void (*find_parser( symbol_t *sym ))( config_t *, pair_t * ) {
   pair_t *lst = parsers;

   while( PAIRP( lst ) ) {
      pair_t *car = (pair_t *)CAR( lst );
      
      if( car && SYMBOL_EQ( CAR( car ), sym ) ) {
	 return (void (*)( config_t *, pair_t * ))(CDR( car ));
      } else {
	 lst = CDR( lst );
      }
   }

   return 0L;
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    parse_error ...                                                  */
/*---------------------------------------------------------------------*/
void
parse_error( char *msg, obj_t *obj ) {
   fprintf( stderr, "%s -- ", msg );
   display( stderr, obj );
   fprintf( stderr, "\n" );
}

/*---------------------------------------------------------------------*/
/*    string_t *                                                       */
/*    parse_cadr_string ...                                            */
/*---------------------------------------------------------------------*/
string_t *
parse_cadr_string( pair_t *lst ) {
   if( !PAIRP( CDR( lst ) ) || !STRINGP( CAR( CDR( lst ) ) ) ) {
      return 0L;
   } else {
      return CAR( CDR( lst ) );
   }
}

/*---------------------------------------------------------------------*/
/*    symbol_t *                                                       */
/*    parse_cadr_symbol ...                                            */
/*---------------------------------------------------------------------*/
symbol_t *
parse_cadr_symbol( pair_t *lst ) {
   if( !PAIRP( CDR( lst ) ) || !SYMBOLP( CAR( CDR( lst ) ) ) ) {
      return 0L;
   } else {
      return CAR( CDR( lst ) );
   }
}

/*---------------------------------------------------------------------*/
/*    integer_t *                                                      */
/*    parse_cadr_integer ...                                           */
/*---------------------------------------------------------------------*/
integer_t *
parse_cadr_integer( pair_t *lst ) {
   if( !PAIRP( CDR( lst ) ) || !INTEGERP( CAR( CDR( lst ) ) ) ) {
      return 0L;
   } else {
      return CAR( CDR( lst ) );
   }
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    parse_icon_size ...                                              */
/*---------------------------------------------------------------------*/
static void
parse_icon_size( config_t *config, pair_t *lst ) {
   config->icon_size = INTEGER_VAL( parse_cadr_integer( lst ) );
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    parse_update_netwmicon ...                                       */
/*---------------------------------------------------------------------*/
static void
parse_update_netwmicon( config_t *config, pair_t *lst ) {
   symbol_t *s = parse_cadr_symbol( lst );
   
   if( !s ) {
      parse_error( "Illegal update-netwmicon", (obj_t *)lst );
   } else {
      config->update_netwmicon = SYMBOL_EQ( s, sym_true );
   }
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    parse_icon_path ...                                              */
/*---------------------------------------------------------------------*/
static void
parse_icon_path( config_t *config, pair_t *lst ) {
   string_t *s = parse_cadr_string( lst );
   
   if( !s ) {
      parse_error( "Illegal icon-path", (obj_t *)lst );
   } else {
      config->icon_directories =
	 cons( expand_env( STRING_CHARS( s ) ), config->icon_directories );
   }
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    parse_icon_theme ...                                             */
/*---------------------------------------------------------------------*/
static void
parse_icon_theme( config_t *config, pair_t *lst ) {
   string_t *s = parse_cadr_string( lst );
   
   if( !s ) {
      parse_error( "Illegal icon-theme", (obj_t *)lst );
   } else {
      char *name = STRING_CHARS( s );
      
      int len = strlen( ICCCMPANEL_PREFIX ) +
	 strlen( ICCCMPANEL_DIR ) + 
	 strlen( ICCCMPANEL_ICON_DIR ) +
	 strlen( name ) + 2 + 1;
      char *dir = malloc( len );
      sprintf( dir, "%s/%s/%s/%s",
	       ICCCMPANEL_PREFIX,
	       ICCCMPANEL_DIR,
	       ICCCMPANEL_ICON_DIR,
	       name );

      SET_CAR( last_pair( config->icon_directories ), dir );
   }
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    parse_shadow ...                                                 */
/*---------------------------------------------------------------------*/
static void
parse_shadow( config_t *config, pair_t *lst ) {
   config->color_shadow = INTEGER_VAL( parse_cadr_integer( lst ) );
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    parse_shadow_size ...                                            */
/*---------------------------------------------------------------------*/
static void
parse_shadow_size( config_t *config, pair_t *lst ) {
   config->shadow_size = INTEGER_VAL( parse_cadr_integer( lst ) );
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    parse_relief ...                                                 */
/*---------------------------------------------------------------------*/
static void
parse_relief( config_t *config, pair_t *lst ) {
   config->relief = INTEGER_VAL( parse_cadr_integer( lst ) );
   if( config->relief != 0 ) config->relief = 1;
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    parse_palette ...                                                */
/*---------------------------------------------------------------------*/
static void
parse_palette( config_t *config, pair_t *lst ) {
   int i = 0;

   lst = CDR( lst );
   
   while( PAIRP( lst ) && (i <= 28)) {
      obj_t *el = CAR( lst );

      if( !PAIRP( el ) ) {
	 fprintf( stderr, "return...\n" );
	 return;
      } else {
	 palette[ i ].red = INTEGER_VAL( CAR( el ) );
	 palette[ i ].green = INTEGER_VAL( CAR( CDR( el ) ) );
	 palette[ i ].blue = INTEGER_VAL( CAR( CDR( CDR( el ) ) ) );

	 i++;
	 lst = CDR( lst );
      }
   }
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    parse_font ...                                                   */
/*---------------------------------------------------------------------*/
static void
parse_font( config_t *config, pair_t *lst ) {
   string_t *s = parse_cadr_string( lst );
   
   if( !s ) {
      parse_error( "Illegal font", (obj_t *)lst );
   } else {
      config->fontname_plain = STRING_CHARS( s );
   }
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    parse_font_bold ...                                              */
/*---------------------------------------------------------------------*/
static void
parse_font_bold( config_t *config, pair_t *lst ) {
   string_t *s = parse_cadr_string( lst );
   
   if( !s ) {
      parse_error( "Illegal font-bold", (obj_t *)lst );
   } else {
      config->fontname_bold = STRING_CHARS( s );
   }
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    parse_font_tooltips ...                                          */
/*---------------------------------------------------------------------*/
static void
parse_font_tooltips( config_t *config, pair_t *lst ) {
   string_t *s = parse_cadr_string( lst );
   
   if( !s ) {
      parse_error( "Illegal font-tooltips", (obj_t *)lst );
   } else {
      config->tooltips_fontname = STRING_CHARS( s );
   }
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    parse_auto_hide ...                                              */
/*---------------------------------------------------------------------*/
static void
parse_auto_hide( config_t *config, pair_t *lst ) {
   symbol_t *s = parse_cadr_symbol( lst );
   
   if( !s ) {
      parse_error( "Illegal auto-hide", (obj_t *)lst );
   } else {
      config->taskbar_autohide = SYMBOL_EQ( s, sym_true );
   }
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    parse_x ...                                                      */
/*---------------------------------------------------------------------*/
static void
parse_x( config_t *config, pair_t *lst ) {
   integer_t *i = parse_cadr_integer( lst );
   
   if( !i ) {
      parse_error( "Illegal x", (obj_t *)lst );
   } else {
      config->taskbar_x = INTEGER_VAL( i );
   }
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    parse_y ...                                                      */
/*---------------------------------------------------------------------*/
static void
parse_y( config_t *config, pair_t *lst ) {
   integer_t *i = parse_cadr_integer( lst );
   
   if( !i ) {
      parse_error( "Illegal y", (obj_t *)lst );
   } else {
      config->taskbar_y = INTEGER_VAL( i );
   }
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    parse_height ...                                                 */
/*---------------------------------------------------------------------*/
static void
parse_height( config_t *config, pair_t *lst ) {
   integer_t *i = parse_cadr_integer( lst );
   
   if( !i ) {
      parse_error( "Illegal height", (obj_t *)lst );
   } else {
      config->taskbar_height = INTEGER_VAL( i );
   }
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    parse_width ...                                                  */
/*---------------------------------------------------------------------*/
static void
parse_width( config_t *config, pair_t *lst ) {
   integer_t *i = parse_cadr_integer( lst );
   
   if( !i ) {
      parse_error( "Illegal width", (obj_t *)lst );
   } else {
      config->taskbar_width = INTEGER_VAL( i );
   }
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    parse_gradient_grey ...                                          */
/*---------------------------------------------------------------------*/
static void
parse_gradient_grey( config_t *config, pair_t *lst ) {
   integer_t *min = parse_cadr_integer( lst );
   
   if( !min ) {
      parse_error( "Illegal gradient_grey_min", (obj_t *)lst );
   } else {
      lst = CDR( lst );
      integer_t *max = parse_cadr_integer( lst );
      
      config->gradient_grey_min = INTEGER_VAL( min );
      config->gradient_grey_max = INTEGER_VAL( max );
   }
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    parse_icon ...                                                   */
/*---------------------------------------------------------------------*/
static void
parse_icon( config_t *config, pair_t *lst ) {
   pair_t *l;
   char *class, *name = 0, *icon = 0;
   
   if( !PAIRP( lst ) ) goto err;
   if( !PAIRP( CDR( lst ) ) ) goto err;
   if( !STRINGP( CAR( lst ) ) ) goto err;
      
   class = STRING_CHARS( CAR( lst ) );

   l = CDR( lst );
   while( PAIRP( l ) ) {
      if( STRINGP( CAR( l ) ) ) {
	 if( icon )
	    goto err;
	 else {
	    icon = STRING_CHARS( CAR( l ) );
	    l = CDR( l );
	    continue;
	 }
      }

      if( SYMBOLP( CAR( l ) ) && SYMBOL_EQ( CAR( l ), sym_name ) ) {
	 l = CDR( l );

	 if( !PAIRP( l ) && STRINGP( CAR( l ) ) || name )
	    goto err;
	 else {
	    name = STRING_CHARS( CAR( l ) );
	    l = CDR( l );
	    continue;
	 }
      }

      goto err;
   }

   if( !class || !icon ) goto err;
   
   add_icondescr( config, class, name, icon );
   return;
   
 err:
   parse_error( "Illegal icon", (obj_t *)lst );
}

/*---------------------------------------------------------------------*/
/*    static void                                                      */
/*    parse_icons_descr ...                                            */
/*---------------------------------------------------------------------*/
static void
parse_icons_descr( config_t *config, pair_t *lst ) {
   pair_t *l = CDR( lst );

   while( PAIRP( l ) ) {
      parse_icon( config, CAR( l ) );
      l = CDR( l );
   }
}

/*---------------------------------------------------------------------*/
/*    static config_t *                                                */
/*    parse_config ...                                                 */
/*---------------------------------------------------------------------*/
static config_t *
parse_config( config_t *config, char *filename ) {
   FILE *fin = fopen( filename, "r" );
   sym_true = make_symbol( "#t" );
   sym_false = make_symbol( "#f" );
   sym_name = make_symbol( ":name" );
   sym_width = make_symbol( ":width" );
   
   register_parser( make_symbol( "icon-theme" ), parse_icon_theme );
   register_parser( make_symbol( "icon-size" ), parse_icon_size );
   register_parser( make_symbol( "update-netwmicon" ), parse_update_netwmicon );
   register_parser( make_symbol( "icon-path" ), parse_icon_path );
   register_parser( make_symbol( "palette" ), parse_palette );
   register_parser( make_symbol( "font" ), parse_font );
   register_parser( make_symbol( "font-bold" ), parse_font_bold );
   register_parser( make_symbol( "font-tooltips" ), parse_font_tooltips );
   register_parser( make_symbol( "shadow" ), parse_shadow );
   register_parser( make_symbol( "shadow-size" ), parse_shadow_size );
   register_parser( make_symbol( "relief" ), parse_relief );
   register_parser( make_symbol( "auto-hide" ), parse_auto_hide );
   register_parser( make_symbol( "taskbar-x" ), parse_x );
   register_parser( make_symbol( "taskbar-y" ), parse_y );
   register_parser( make_symbol( "taskbar-height" ), parse_height );
   register_parser( make_symbol( "taskbar-width" ), parse_width );
   register_parser( make_symbol( "gradient-grey" ), parse_gradient_grey );
   register_parser( make_symbol( "icons-descr" ), parse_icons_descr );
   register_parser( make_symbol( "grip" ), parse_grip );
   register_parser( make_symbol( "hfill" ), parse_hfill );
   register_parser( make_symbol( "desktop" ), parse_desktop );
   register_parser( make_symbol( "clock" ), parse_clock );
   register_parser( make_symbol( "apps" ), parse_apps );
   register_parser( make_symbol( "icons" ), parse_icons );
   register_parser( make_symbol( "tray" ), parse_systray );
   register_parser( make_symbol( "applets" ), parse_applets );
   register_parser( make_symbol( "wifi" ), parse_wifi );
   register_parser( make_symbol( "battery" ), parse_battery );
   register_parser( make_symbol( "hibernate" ), parse_hibernate );
   register_parser( make_symbol( "volume" ), parse_volume );
   register_parser( make_symbol( "cron" ), parse_cron );
   register_parser( make_symbol( "exec" ), parse_exec );
   
   printf( "Parsing config file `%s'\n", filename );
   
   if( !fin ) {
      fprintf( stderr, "Can't open config file `%s'\n", filename );
      exit( 12 );
   } else {
      obj_t *o;
      config_t *cfg = default_config( config );
      
      while( o = readobj( fin ) ) {
	 if( PAIRP( o ) ) {
	    if( !SYMBOLP( CAR( o ) ) ) {
	       fprintf( stderr, "Illegal list: " );
	       display( stderr, o );
	       fprintf( stderr, "\n" );
	    } else {
	       parse_pair( config, (pair_t *)o );
	    }
	 }
      }		
      return cfg;
   }
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    parse_pair ...                                                   */
/*---------------------------------------------------------------------*/
void
parse_pair( config_t *config, pair_t *o ) {
   void (*p)( config_t *, pair_t * ) = find_parser( CAR( o ) );

   if( p ) {
      p( config, (pair_t *)o );
   } else {
      fprintf( stderr, "Illegal symbol `" );
      display( stderr, CAR( o ) );
      fprintf( stderr, "', ignoring...\n" );
   }
}

/*---------------------------------------------------------------------*/
/*    static config_t *                                                */
/*    finalize_config ...                                              */
/*---------------------------------------------------------------------*/
static config_t *
finalize_config( config_t *config ) {
   int g;
   if( !config->icondescrs ) {
      add_icondescr( config, "Emacs", 0L, "emacs.xpm" );
      add_icondescr( config, "XTerm", 0L, "term.xpm" );
   } else {
      config->icondescrs = reverse( config->icondescrs );
   }

   g = config->gradient_grey_max - config->gradient_grey_min;

   if( g < 0 ) g = -g;
   
   if( g < config->taskbar_height ) {
      config->gradient_step = 1;
      config->gradient_substep = 1 + config->taskbar_height / g;
   } else {
      config->gradient_step = 1;
      config->gradient_substep = 1;
   }

   return config;
}

/*---------------------------------------------------------------------*/
/*    config_t *                                                       */
/*    make_config ...                                                  */
/*---------------------------------------------------------------------*/
config_t *
make_config( int argc, char *argv[] ) {
   config_t *config = (config_t *)calloc( 1, sizeof( config_t ) );
   char *rcfile;

   if( !config ) return config;

   if( !(rcfile = find_rc_file( argc > 1 ? argv[ 1 ] : (char *)0L )) )
      return finalize_config( default_config( config ) );
   else
      return finalize_config( parse_config( config, rcfile ) );
}

/*---------------------------------------------------------------------*/
/*    plugin_t *                                                       */
/*    make_plugin ...                                                  */
/*---------------------------------------------------------------------*/
plugin_t *
make_plugin( void *(*start)( void *, pair_t * ), pair_t *args ) {
   plugin_t *plug = calloc( 1, sizeof( plugin_t ) );

   if( !plug ) {
      fprintf( stderr, "Can't allocate plugin\n" );
      exit( 14 );
   }

   plug->start = start;
   plug->args = args;
   
   return plug;
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    register_plugin ...                                              */
/*---------------------------------------------------------------------*/
void
register_plugin( config_t *config, plugin_t *p ) {
   config->plugins = cons( p, config->plugins );
}
