/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/exec.c                             */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Wed Nov 12 07:52:21 2014                          */
/*    Last change :  Wed Nov 12 08:18:00 2014 (serrano)                */
/*    Copyright   :  2014 Manuel Serrano                               */
/*    -------------------------------------------------------------    */
/*    binary execs                                                     */
/*=====================================================================*/
#include <unistd.h>
#include <stdlib.h>

#include "config.h"
#include "exec.h"

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    parse_exec ...                                                   */
/*---------------------------------------------------------------------*/
void
parse_exec( config_t *config, pair_t *lst ) {
   pair_t *l = CDR( lst );
   pair_t *args = NIL; 

   /* executed all the commands */
    while( PAIRP( l ) ) {
      obj_t *car = CAR( l );

      if( STRINGP( car ) ) {
	 if( !fork() ) {
	    setsid();
	    execl( "/bin/sh", "/bin/sh", "-c", STRING_CHARS( car ), 0 );
	    exit( 0 );
	 }
      }
      
      l = CDR( l );
    }
}
