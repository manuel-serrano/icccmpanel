/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/config.h                           */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Mon Oct 13 13:35:06 2003                          */
/*    Last change :  Tue Dec  3 07:50:54 2024 (serrano)                */
/*    Copyright   :  2003-24 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    Icccmpanel config parsing                                        */
/*=====================================================================*/
#if (!defined(ICCCMPANEL_CONFIG_H))

#define ICCCMPANEL_CONFIG_H
#include "list.h"

#define ICCCMPANEL_FREE_LIST 0

/*---------------------------------------------------------------------*/
/*    icondescr_t ...                                                  */
/*---------------------------------------------------------------------*/
typedef struct icondescr {
   char *class;
   char *name;
   char *filename;
} icondescr_t;

/*---------------------------------------------------------------------*/
/*    plugin_t ...                                                     */
/*---------------------------------------------------------------------*/
typedef struct plugin {
   void *(*start)(void *, pair_t *);
   pair_t *args;
} plugin_t;

/*---------------------------------------------------------------------*/
/*    config_t ...                                                     */
/*---------------------------------------------------------------------*/
typedef struct config {
   int animspeed;
   char *fontname_plain;
   char *fontname_bold;
   char taskbar_autohide;
   int taskbar_hidespeed;
   int taskbar_unhidespeed;
   int taskbar_linesep;
   int taskbar_border;
   int taskbar_x;
   int taskbar_y;
   int taskbar_width;
   int taskbar_height;
   int gradient_grey_min;
   int gradient_grey_max;
   int gradient_grey_len;
   int gradient_step;
   int gradient_substep;
   int color_shadow;
   int shadow_size;
   int relief;
   char taskbar_always_on_top;
   char icon_mapped;
   char icon_all_desktop;
   char update_netwmicon;
   int icon_size;
   pair_t *icon_directories;
   pair_t *icondescrs;
   int app_width;
   char *tooltips_fontname;
   pair_t *plugins;
   char *bigcursor_path;
} config_t;

/*---------------------------------------------------------------------*/
/*    extern declarations                                              */
/*---------------------------------------------------------------------*/
extern config_t *make_config();
extern plugin_t *make_plugin(void *(*)(void *, pair_t *), pair_t *);
extern char *find_icon(config_t *, char *);
extern char *find_rc_file();
extern char *expand_env(char *);

extern void parse_error(char *, obj_t *);
extern void parse_pair(config_t *, pair_t *);
extern string_t *parse_cadr_string(pair_t *);
extern symbol_t *parse_cadr_symbol(pair_t *);
extern integer_t *parse_cadr_integer(pair_t *);

extern void register_plugin(config_t *, plugin_t *);

/* cached symbols */
extern symbol_t *sym_true, *sym_false;
extern symbol_t *sym_name;
extern symbol_t *sym_width;

#endif
