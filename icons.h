/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/icons.h                            */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Thu Jul 22 15:21:17 2004                          */
/*    Last change :  Fri Jul 19 09:23:40 2024 (serrano)                */
/*    Copyright   :  2004-24 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    The icons.                                                       */
/*=====================================================================*/
#ifndef _ICON_H
#define _ICON_H

/*---------------------------------------------------------------------*/
/*    Exports ...                                                      */
/*---------------------------------------------------------------------*/
area_t *make_icons(taskbar_t *, int, int, char, char);
extern void *start_icons(void *, pair_t *);
void parse_icons(config_t *, pair_t *);
char iconp(area_t *);

/*---------------------------------------------------------------------*/
/*    xclicon_t ...                                                    */
/*---------------------------------------------------------------------*/
typedef struct xclicon {
   /* the area associated with this task */
   area_t area;
   /* is the area window mapped */
   char mappedp;
   /* The X client associated with this task */
   xclient_t *xcl;
   /* the position of the task */
   int x, w;
   /* the position of the icon */
   int icon_x, icon_y, icon_w, icon_h;
} xclicon_t;
   
/*---------------------------------------------------------------------*/
/*    ipicons_t                                                        */
/*---------------------------------------------------------------------*/
typedef struct ipicons {
   /* the area */
   area_t area;
   /* the identifier of this ipicons */
   int id;
   /* property of the icon bar  */
   char icon_mapped;
   char icon_all_desktop;
   /* the list of xclient icons */
   pair_t *xclicons;
   pair_t *_freexclicons;
   pair_t *xclistack;
 } ipicons_t;

#endif
