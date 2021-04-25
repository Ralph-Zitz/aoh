/* LPDemon
**
** This objects enables the evaluation of LPC-expressions and the execution
** of short LPC-code interactively.
** I got the idea by Ergberg's calc, but liked to implement it myself.
**
**   01-May-92 [m]
**   14-Jun-92 [m] Adapted for native mode.
**   16-Jul-92 [m] Now uses mydefs.h in the target.
**   13-Aug-92 [m] Target got an own .h for better abbrevs.
**
**  [m] Mateese
*/

#include "/players/mateese/mydefs.h"
#include <config.h>
#include <properties.h>

#include <string.h>
inherit "/lib/string";

#define LOGFILE MPL+"calc"

#define VERSION "920813"

inherit "/std/thing";

object cobj;
mixed *vars;

create () {
  ::create();
  AddId      ("demon");
  AddId      ("lpdemon");
  SetShort   ("a LPdemon");
  SetLong    ("A small buzzing demon.\n");
  SetValue   (0);
  SetWeight  (0);
  Set(P_READ_MSG,
"A small sign reads:\n"+
"  LPDemon    ---    v"+VERSION+"\n"+
"  Biontic Magics & Darkness\n"
          );
  Set(P_INFO, "LPDemon, v"+VERSION+" by Mateese");
  vars = ({({}),({})});
}

init() {
  thing::init();
  add_action ("read", "inspect");
  add_action ("help", "help");
  add_action ("execlpc", "lpc");
  add_action ("execlpc", "eval");
}

SetAutoObject (mixed *args) {}
QueryAutoObject () {  return ({ VERSION }); }


help(str) {
  if (PL != environment(ME) || !id(str) || !PL_WLEVEL) return 0;
  write (
"The LPDemon is able to evaluate LPC-expressions or to run a short piece\n"+
"of LPC-code.\n"+
"  lpc <expression>      : evaluate the expression\n"+
"  lpc [<decl>] @ <code> : execute a function containing <code>\n"+
"  lpc                   : redo the last lpc-action\n"+
"'eval' may be used instead of 'lpc'.\n"
        );
  return 1;
}

execlpc(str) {
  string fname, decl, code, text;
  int i;
  mixed rc, val;
  if (!PL || PL != environment(ME) || !PL_WLEVEL) return 0;
  if (!str) {
    if (!cobj) {
      write ("There is no previous action to redo.\n");
      return 1;
    }
    rc = cobj->magic();
  } else {
    fname = WPATH+PL_EUID+"/.lpd.c";
    if (file_size(fname) >= 0 && !rm(fname)) {
      write ("Can't remove previous file '"+fname+"'\n");
      return 1;
    }
    if (cobj) { destruct(cobj); cobj = 0; }
    if (cobj = find_object(fname)) { destruct(cobj); cobj = 0; }
    if (sscanf (str, "%s@%s", decl, code))
      code = "magic() {"+code+"}";
    else
      decl = "", code = "magic() { return ("+str+"); }";
    text = "#include \""+MO+"lpd.h\"\n" + decl + code;
    write_file(fname, text);
    if (catch(call_other(fname, "???"))) {
      write ("Error loading '"+fname+"'.\n");
      return 1;
    }
    if (!(cobj = find_object(fname))) {
      write ("Can't find '"+fname+"' though loaded.\n");
      return 1;
    }
    cobj->set_demon(ME);
    rc = cobj->magic();
  }
  write ("Result: "+val_to_str(rc)+"\n");
  return 1;
}

set_var(name,value) {
  vars = insert_alist(name, value, vars);
}

get_var(name) {
  if (assoc(name,vars[0]) == -1) return 0;
  return assoc (name,vars);
}

rm_var(name) {
  if (assoc(name,vars[0]) != -1) vars = remove_alist(name,vars);
}
