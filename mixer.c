/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/mixer.c                            */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Thu Feb  3 15:25:23 2000                          */
/*    Last change :  Tue Jun 17 07:47:00 2025 (serrano)                */
/*    -------------------------------------------------------------    */
/*    A control over the audio mixer for Bigloo. To a large extent     */
/*    this file is inspired of mixctl.h by Sam Hawker                  */
/*    <shawkie@geocities.com>.                                         */
/*=====================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#ifdef __NetBSD__
#include <soundcard.h>
#endif
#ifdef __FreeBSD__
#include <machine/soundcard.h>
#endif
#ifdef __linux__
#include <linux/soundcard.h>
#endif
#include "mixer.h"

/*---------------------------------------------------------------------*/
/*    static functions                                                 */
/*---------------------------------------------------------------------*/
static void do_status(struct mixer *);

/*---------------------------------------------------------------------*/
/*    mixer_t *                                                        */
/*    open_mixer ...                                                   */
/*---------------------------------------------------------------------*/
mixer_t *
open_mixer(char *dname) {
   struct mixer *mixer = (struct mixer *)malloc(sizeof(struct mixer));

   mixer->device = (char *)malloc(strlen(dname) + 1);
   strcpy(mixer->device, dname);

   if (mixer->mixfdopen = (mixer->mixfd = open(dname, O_NONBLOCK))!=-1) {
      char *devnames[] = SOUND_DEVICE_NAMES;
      char *devlabels[] = SOUND_DEVICE_LABELS;
      int mixmask = 1, i;
      
      mixer->nrdevices = SOUND_MIXER_NRDEVICES;

      ioctl(mixer->mixfd, SOUND_MIXER_READ_DEVMASK, &(mixer->devmask));
      ioctl(mixer->mixfd, SOUND_MIXER_READ_STEREODEVS, &(mixer->stmask));
      ioctl(mixer->mixfd, SOUND_MIXER_READ_RECMASK, &(mixer->recmask));
      ioctl(mixer->mixfd, SOUND_MIXER_READ_CAPS, &(mixer->caps));

      mixer->mixdevs = (struct MixDev *)malloc(sizeof(struct MixDev) *
						mixer->nrdevices);
      
      for (i = 0; i < mixer->nrdevices; i++) {
	 mixer->mixdevs[ i ].support = mixer->devmask & mixmask;
	 mixer->mixdevs[ i ].stereo = mixer->stmask & mixmask;
	 mixer->mixdevs[ i ].records = mixer->recmask & mixmask;
	 mixer->mixdevs[ i ].mask = mixmask;
	 mixer->mixdevs[ i ].name = devnames[i];
	 mixer->mixdevs[ i ].label = devlabels[i];
	 mixmask *= 2;
      }

      do_status(mixer);

      return mixer;
   } else
      return 0L;
}

/*---------------------------------------------------------------------*/
/*    int                                                              */
/*    mixer_find_dev ...                                               */
/*---------------------------------------------------------------------*/
int
mixer_find_dev(mixer_t *mixer, char *name) {
   int i;
   
   for (i = 0; i < mixer->nrdevices; i++) {
      if (!strcmp(name, mixer->mixdevs[ i ].name)) {
	 return i;
      }
   }

   return -1;
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    close_mixer ...                                                  */
/*---------------------------------------------------------------------*/
void
close_mixer(mixer_t *mixer) {
   int i;

   ioctl(mixer->mixfd, SOUND_MIXER_READ_RECSRC, &(mixer->recsrc));
   for (i = 0; i < mixer->nrdevices; i++) {
      if (mixer->mixdevs[ i ].support)
	 ioctl(mixer->mixfd, MIXER_READ(i), &(mixer->mixdevs[ i ].value));
      mixer->mixdevs[ i ].recsrc = (mixer->recsrc & mixer->mixdevs[ i ].mask);
   }
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    do_status ...                                                    */
/*---------------------------------------------------------------------*/
static void
do_status(mixer_t *mixer) {
   int i;
   
   ioctl(mixer->mixfd, SOUND_MIXER_READ_RECSRC, &(mixer->recsrc));
   
   for (i = 0; i< mixer->nrdevices; i++) {
      if (mixer->mixdevs[ i ].support)
	 ioctl(mixer->mixfd, MIXER_READ(i),  &(mixer->mixdevs[ i ].value));
      mixer->mixdevs[ i ].recsrc = (mixer->recsrc & mixer->mixdevs[ i ].mask);
   }
}

/*---------------------------------------------------------------------*/
/*    int                                                              */
/*    mixer_read_vol ...                                               */
/*    -------------------------------------------------------------    */
/*    Read the volume for a device.                                    */
/*---------------------------------------------------------------------*/
int
mixer_read_vol(mixer_t *mixer, int dev, int read) {
   if (read)
      ioctl(mixer->mixfd, MIXER_READ(dev), &(mixer->mixdevs[ dev ].value));
   return mixer->mixdevs[ dev ].value;
}

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    mixer_write_vol ...                                              */
/*---------------------------------------------------------------------*/
void
mixer_write_vol(mixer_t *mixer, int dev, int value) {
   mixer->mixdevs[ dev ].value = value;
   ioctl(mixer->mixfd, MIXER_WRITE(dev), &(mixer->mixdevs[ dev ].value));
}

/*---------------------------------------------------------------------*/
/*    char *                                                           */
/*    mixer_dev_name ...                                               */
/*---------------------------------------------------------------------*/
char *
mixer_dev_name(mixer_t *mixer, int dev) {
   return mixer->mixdevs[ dev ].name;
}

/*---------------------------------------------------------------------*/
/*    int                                                              */
/*    mixer_dev_num ...                                                */
/*---------------------------------------------------------------------*/
int
mixer_dev_num(mixer_t *mixer) {
   return mixer->nrdevices;
}

/*---------------------------------------------------------------------*/
/*    int                                                              */
/*    mixer_supported_dev_p ...                                        */
/*---------------------------------------------------------------------*/
int
mixer_supported_dev_p(mixer_t *mixer, int dev) {
   return (dev < mixer->nrdevices) && (mixer->mixdevs[ dev ].support);
}
