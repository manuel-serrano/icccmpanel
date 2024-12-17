/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/mstk.h                             */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Sat Oct 11 05:44:23 2003                          */
/*    Last change :  Mon Dec 16 10:11:55 2024 (serrano)                */
/*    Copyright   :  2003-24 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    MS X toolkit                                                     */
/*=====================================================================*/
#if(!defined(_MSTK_H))
#define _MSTK_H

/*---------------------------------------------------------------------*/
/*    Xinfo_t ...                                                      */
/*---------------------------------------------------------------------*/
typedef struct Xinfo {
   Display *disp;
   Window root_win;
   int screen;
   int screen_depth, screen_width, screen_height;
   int text_y;
   GC gcb;
   GC gcp;
   char *fontname_plain;
   char *fontname_bold;
   char *tooltips_fontname;
   XFontStruct *xfsp;
   XFontStruct *xfsb;
   Atom net_wm_name;
   Atom net_wm_icon;
} Xinfo_t;

/*---------------------------------------------------------------------*/
/*    color                                                            */
/*---------------------------------------------------------------------*/
typedef struct color {
   unsigned short red, green, blue;
} color_t;

extern color_t palette[];

#define MSTK_PALETTE_COUNT (sizeof(palette) / sizeof (palette[ 0 ].red) / 3)

/*---------------------------------------------------------------------*/
/*    Import/export                                                    */
/*---------------------------------------------------------------------*/
extern unsigned long mstk_palette[];
extern unsigned long mstk_grey_palette[];

extern Xinfo_t *init_mstk(config_t *);
extern Xinfo_t *copy_xinfo(Xinfo_t *);
extern void closeup_mstk(Xinfo_t *);

extern void *get_window_prop_data(Display *, Window, Atom, Atom, long *);
extern void set_window_prop(Display *,  Window, Atom, Atom, long);

extern char *window_name(Display *, Window);
extern int window_desktop(Display *, Window);
extern char *window_class(Display *, Window);
extern char window_hint_icon(Xinfo_t *, Window, Pixmap *, Pixmap *, int);
extern char window_netwm_icon(Xinfo_t *, Window, Pixmap *, Pixmap *, int);
extern void window_update_netwm_icon(Xinfo_t *, Window, char *, Pixmap *, Pixmap *, int);

extern Window *desktop_windows(Xinfo_t *, long *);
extern int current_desktop(Display *, Window);
extern int number_of_desktops(Display *, Window);
extern void switch_desktop(Display *, Window, int);

extern int window_iconifiedp(Display *, Window);
extern int window_hiddenp(Display *, Window);
extern void window_deiconify(Display *, Window);

extern void scale_icon(Xinfo_t *, Window, Pixmap, Pixmap,
			Pixmap *, Pixmap *, int, int);
extern void draw_text(Xinfo_t *, Window, int, int, char *, int,
		       unsigned long[], int c, int r, int s);
extern void draw_text_plain(Xinfo_t *, Window, int, int, char *, int,
			     unsigned long[], int c, int r, int s);
extern void draw_line(Xinfo_t *, Window, int, int, int, int,
			 unsigned long [], int);
extern void draw_gradient(Xinfo_t *, Window, int, int, int, int,
			   unsigned long [], int, int, int);
extern void draw_relief(Xinfo_t *, Window, int, int, int, int,
			 unsigned long [], int, int, int);
extern void draw_partial_relief(Xinfo_t *, Window, int,
				 int, int, int, int,
				 unsigned long [], int, int, int);
extern void draw_pixmap(Xinfo_t *, Window, Pixmap, Pixmap,
			 int, int, int, int);
extern void draw_grill(Xinfo_t *, Window, int, int, int, int);

extern Atom atoms[];

#define atom_KWM_WIN_ICON atoms[ 0 ]
#define atom_WM_STATE atoms[ 1 ]
#define atom__MOTIF_WM_HINTS atoms[ 2 ]
#define atom__NET_WM_STATE atoms[ 3 ]
#define atom__NET_WM_STATE_SKIP_TASKBAR atoms[ 4 ]
#define atom__NET_WM_STATE_SHADED atoms[ 5 ]
#define atom__NET_WM_DESKTOP atoms[ 6 ]
#define atom__NET_WM_WINDOW_TYPE atoms[ 7 ]
#define atom__NET_WM_WINDOW_TYPE_DOCK atoms[ 8 ]
#define atom__NET_WM_WINDOW_TYPE_TOOLBAR atoms[ 9 ]
#define atom__NET_WM_STRUT atoms[ 10 ]
#define atom__NET_WM_NAME atoms[ 11 ]
#define atom__NET_WM_ICON atoms[ 12 ]
#define atom__WIN_HINTS atoms[ 13 ]
#define atom__NET_CLIENT_LIST atoms[ 14 ]
#define atom__NET_NUMBER_OF_DESKTOPS atoms[ 15 ]
#define atom__NET_CURRENT_DESKTOP atoms[ 16 ]
#define atom__NET_WM_STATE_STICKY atoms[ 17 ]
#define atom__CARDINAL atoms[ 18 ]

/*---------------------------------------------------------------------*/
/*    Colors                                                           */
/*---------------------------------------------------------------------*/
#define WHITE       6
#define LIGHTGREY   0
#define GREY10      11
#define GREY11      10
#define GREY12      9
#define GREY9       12
#define BLACK       21
#define RED         22
#define BLUE        26
#define GREEN       27
#define ACTIVE      20
#define TOOLTIPS    23
#define TOOLTIPSFG  24
#define TOOLTIPSBD  25
#define SHADOW      24

/*---------------------------------------------------------------------*/
/*    Partial relief                                                   */
/*---------------------------------------------------------------------*/
#define RELIEF_NONE   0
#define RELIEF_TOP    1
#define RELIEF_BOTTOM 2
#define RELIEF_LEFT   4
#define RELIEF_RIGHT  8

/*---------------------------------------------------------------------*/
/*    Window manager ...                                               */
/*---------------------------------------------------------------------*/
#define MWM_HINTS_DECORATIONS (1L << 1)
typedef struct _mwmhints {
   unsigned long flags;
   unsigned long functions;
   unsigned long decorations;
   long inputMode;
   unsigned long status;
} MWMHints;

#define WIN_STATE_STICKY          (1<<0) /* everyone knows sticky */
#define WIN_STATE_MINIMIZED       (1<<1) /* ??? */
#define WIN_STATE_MAXIMIZED_VERT  (1<<2) /* window in maximized V state */
#define WIN_STATE_MAXIMIZED_HORIZ (1<<3) /* window in maximized H state */
#define WIN_STATE_HIDDEN          (1<<4) /* not on taskbar but win visible */
#define WIN_STATE_SHADED          (1<<5) /* shaded (NeXT style) */
#define WIN_STATE_HID_DESKTOP     (1<<6) /* not on current desktop */
#define WIN_STATE_HID_TRANSIENT   (1<<7) /* owner of transient is hidden */
#define WIN_STATE_FIXED_POSITION  (1<<8) /* window is fixed in position even */
#define WIN_STATE_ARRANGE_IGNORE  (1<<9) /* ignore for auto arranging */

#define WIN_HINTS_SKIP_FOCUS      (1<<0) /* "alt-tab" skips this win */
#define WIN_HINTS_SKIP_WINLIST    (1<<1) /* not in win list */
#define WIN_HINTS_SKIP_TASKBAR    (1<<2) /* not on taskbar */
#define WIN_HINTS_GROUP_TRANSIENT (1<<3) /* ??????? */
#define WIN_HINTS_FOCUS_ON_CLICK  (1<<4) /* only accepts focus when clicked */
#define WIN_HINTS_DO_NOT_COVER    (1<<5) /* attempt to not cover this window */

#endif
