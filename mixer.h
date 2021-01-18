/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/mixer.h                            */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Thu Jul 22 15:21:17 2004                          */
/*    Last change :  Sat Oct 10 06:41:09 2015 (serrano)                */
/*    Copyright   :  2004-15 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    The workspace.                                                   */
/*=====================================================================*/
#ifndef _MIXER_H
#define _MIXER_H

/*---------------------------------------------------------------------*/
/*    A private mixer structure                                        */
/*---------------------------------------------------------------------*/
typedef struct mixer {
   int mixfd;
   int mixfdopen;
   char *device;

   int nrdevices;       // maximum number of devices
   int devmask;         // supported devices
   int stmask;          // stereo devices
   int recmask;         // devices which can be recorded from
   int caps;            // capabilities
   int recsrc;          // devices which are being recorded from
   
   struct MixDev {
      int support;
      int stereo;
      int recsrc;
      int records;
      char *name;
      char *label;
      int value;
      int mask;
   } *mixdevs;
} mixer_t;

/*---------------------------------------------------------------------*/
/*    Export ...                                                       */
/*---------------------------------------------------------------------*/
extern mixer_t *open_mixer( char *dname );
int mixer_find_dev( mixer_t *mixer, char *name );
extern int mixer_read_vol( mixer_t *, int, int );
extern void close_mixer( mixer_t * );
extern void mixer_write_vol( mixer_t *, int, int );
extern char *mixer_dev_name( mixer_t *, int );
extern int mixer_dev_num( mixer_t * );
extern int mixer_supported_dev_p( mixer_t *, int );

#endif
