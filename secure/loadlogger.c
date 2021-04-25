/* /secure/loadlogger
**
** Just makes every five minutes a one line log into a file.
** The line contains:
**   ctime() #total_users #players #wizards cmds/s lines/s #objects
** The numbers may be evaluated later with GNUplot or similar.
**
** Idea by Freaky (Unitopia).
**
**   11-Jan-94 [Mateese]
**   01-Aug-96 [Mateese] Added #objects count.
*/

#include <secure/config.h>
#include <driver/debug_info.h>

#define DELAY 600

#define PO previous_object()
#define TP this_player()
#define TI this_interactive()
#define TO this_object()

protected void do_log();                           /* Forward Declaration */

/*------------------------------------------------------------------------*/
static string Logfile()
{
  return "/log/load/"+query_host_name()+"."
	 +ctime()[<4..<1]+"-"+lower_case(ctime()[4..6]);
}

/*------------------------------------------------------------------------*/

void create() {
  seteuid(getuid(TO));
  if (getuid(TO) != ROOTID || clonep()) {
/*
  if (   getuid(TO) != ROOTID
      || -1 != member (object_name(TO), '#')) {
*/
    write(
 ctime(time())+": Illegal attempt!\n"
+"  this:        "+object_name(TO)+" ["+getuid(TO)+"]\n"
+"  previous:    "+(PO ? object_name(PO)+" ["+getuid(PO)+"]" : "<none>")+"\n"
+"  player:      "+(TP ? object_name(TP)+" ["+getuid(TP)+"]" : "<none>")+"\n"
+"  interactive: "+(TI ? object_name(TI)+" ["+getuid(TI)+"]" : "<none>")+"\n\n"
         );
    destruct (TO);
    return;
  }

  // Create the logfile name from current month and hostname,
  // and create the log directory if necessary.
  if (file_size("/log/load") != -2)
    mkdir("/log/load");
  do_log();
}

/*------------------------------------------------------------------------*/

protected void do_log() {
  string *s, rc;
  object *wiz, *pl;
  int nru, nro = 0;
  while (remove_call_out(#'do_log /*'*/) != -1);
  pl = users();
  nru = sizeof(pl);
  wiz = filter(pl, #'query_user_level /*'*/);
  pl = pl - wiz;
  s = explode(query_load_average(), " ");
#if __VERSION__ > "3.5.0"
  for (; sizeof(efun::objects(nro, 1)) > 0; nro++);
#else
  object o;
  for (nro = 0, o = 0; o = debug_info(2, o); nro++);
#endif
  rc = ctime()+"\t"
       +nru+"\t"
       +sizeof(pl)+"\t"
       +sizeof(wiz)+"\t"
       +s[0]+"\t"
       +s[2]+"\t"
       + nro+"\n";
  write_file(Logfile(), rc);
  call_out(#'do_log, DELAY /*'*/);
}

/*========================================================================*/
