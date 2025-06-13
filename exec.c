/*=====================================================================*/
/*    serrano/prgm/utils/icccmpanel/exec.c                             */
/*    -------------------------------------------------------------    */
/*    Author      :  Manuel Serrano                                    */
/*    Creation    :  Wed Nov 12 07:52:21 2014                          */
/*    Last change :  Fri Jun 13 08:26:46 2025 (serrano)                */
/*    Copyright   :  2014-25 Manuel Serrano                            */
/*    -------------------------------------------------------------    */
/*    binary execs                                                     */
/*=====================================================================*/
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "config.h"
#include "exec.h"

/*---------------------------------------------------------------------*/
/*    void                                                             */
/*    parse_exec ...                                                   */
/*---------------------------------------------------------------------*/
void
parse_exec(config_t *config, pair_t *lst) {
   symbol_t *sym_setsid = make_symbol(":setsid");
   symbol_t *sym_true = make_symbol("#t");
   symbol_t *sym_false = make_symbol("#f");
   pair_t *l = CDR(lst);
   pair_t *args = NIL;
   int sid;

   /* executed all the commands */
    while (PAIRP(l)) {
      obj_t *car = CAR(l);
      
      if (SYMBOLP(car)) {
	 if (SYMBOL_EQ((symbol_t *)car, sym_setsid)) {
	    obj_t *val = CAR(CDR(l));
	    if ((!SYMBOLP(val)) &&
		!(SYMBOL_EQ((symbol_t *)val, sym_true)
		  || SYMBOL_EQ((symbol_t *)val, sym_false))) {
	       parse_error("Illegal setsid value", (obj_t *)l);
	       
	    }
	    sid = SYMBOL_EQ((symbol_t *)val, sym_true);
	    l = CDR(CDR(l));
	    car = CAR(l);
	 } else {
	    parse_error("Illegal exec symbol", (obj_t *)l);
	 }
      }

      if (STRINGP(car)) {
	 if (!fork()) {
	    if (sid) setsid();
	    execl("/bin/sh", "/bin/sh", "-c", STRING_CHARS(car), 0);
	    exit(0);
	 }
      }
      
      l = CDR(l);
    }
}
