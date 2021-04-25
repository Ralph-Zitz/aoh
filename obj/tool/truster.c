/*  Truster
**
** This object exports its uid onto other objects. If properly created, it
** may be used to trust tools of cloned from other wizards.
**
** Additionally this is an example for an Wizard Tool: protected against
** misusers, invisible for mortals (optionally even for the owner) and
** autoloading!
**
**   v.1 19-Jun-92 [m]
**       30-Jun-92 [m] Security Bug fixed
**       27-Jul-92 [m] Adapted for new autoloading mechanism
**       31-Aug-92 [m] Adapted for new property system
**
** [m] Mateese, Biontic Magics & Darkness
*/

#define PL this_player()
#define ME this_object()
#define PO previous_object()

#include <config.h>
#include <properties.h>

#define VERSION "920831"
#define VERSNR  1

inherit "/std/thing";

static int invis;      /* Flag if invisible */

/*-------------------------------------------------------------------------
** Initialization.
*/

create () {
  thing::create();
  AddId ("truster");
  SetShort ("a truster");
  SetLong (
"This object lets you 'trust' other objects by exporting its uid to them.\n"+
"You can also 'trust all' objects you carry, with 'trust here' trust all\n"+
"objects in the same room with you.\n"+
"Same is possible with 'chkuids' which tells you the uids of other objects.\n"+
"You can also create a 'stub' for an alien object with this truster.\n"+
"Being 'suspicious' makes the truster leave in to the next plane of\n"+
"existance. It can also be made (in)visible with '(in)vis truster'.\n"+
"Give '<cmd> ?' for special info.\n"+
"The truster is autoloading as long as carried.\n"
          );
  SetValue (0);
  SetWeight (0);
  SetReadMsg (
"A small sign reads:\n"+
"  Truster   ---   v"+VERSNR+"."+VERSION+"\n"+
"  Biontic Magics & Darkness\n"
          );
  SetInfo("Truster v"+VERSNR+"."+VERSION+": Mateese");
  invis = 0;
}

/*-------------------------------------------------------------------------
** No mere mortal shall be able to use or see us.
** Also handles invisibility.
*/

QueryShort() {
  if (!PL || invis || !query_user_level(PL)) return 0;
  return thing::QueryShort();
}

id(string str) {
  if (!PL || !query_user_level(PL)) return 0;
  return thing::id(str);
}

/*-------------------------------------------------------------------------
** Handle the autoloading stuff.
*/

QueryAutoObject() {
  return ({ VERSNR, invis });
}

SetAutoObject( mixed *args) {

    /* Can we understand the args ? */
  if (!pointerp(args) || sizeof(args) < 2) {
    write ("Can't initialize truster.\n");
    return;
  }

    /* Check version */
  if (args[0] != VERSNR)
    write ("The truster has been improved since your last visit!\n");

    /* Get arguments */
  invis = args[1];

    /* Did we understand all? */
  if (sizeof(args) > 2)
    write ("Truster couldn't understand all init data.\n");
  return;
}

/*-------------------------------------------------------------------------
** Check the euid of this_player() against ours.
** If we have none, try to set it to this_players().
** If this fails, the player can't use us, and our euid is reset.
*/

int check_euid() {
  if (!PO || geteuid(PO) != geteuid(ME)) {
    write ("Sorry, this truster is not yours.\n");
    return 0;
  }

  if (!geteuid(ME) && !seteuid(geteuid(PL))) {
    write ("Sorry, this truster is not yours.\n");
    return 0;
  }

  if (!geteuid(ME) || geteuid(ME) != geteuid(PL)) {
    seteuid(0);
    write ("You have no right to do that!\n");
    return 0;
  }
  return 1;
}

/*-------------------------------------------------------------------------
** Init our commands, but only if the living will be allowed to use them.
** At this opportunity we update the living's auto_load list.
**
** Note the various abbreviations, and shared usages of command funs.
*/

init() {

    /* May the living use us at all ? */
  if (!PL || !query_user_level(PL) || PL != environment(ME)) return;

    /* known stuff... */
  thing::init();
  add_action ("ftrust", "trust");
  add_action ("fstub", "stub");
  add_action ("fuid"  , "chkuids");
  add_action ("fsusp" , "suspicious");
  add_action ("fsusp" , "susp");
  add_action ("fvis"  , "invis");
  add_action ("fvis"  , "invisible");
  add_action ("fvis"  , "vis");
  add_action ("fvis"  , "visible");
}

/*-------------------------------------------------------------------------
** Command 'trust'
**
** Export our uid to all object(s) willing to take it.
** Since this is a potentially dangerous command, we check the euid
** of the commander.
*/

int ftrust (string arg) {
  object *inv, ob, env;
  string short;
  int i;

  if (!arg || arg == "?") {
    write ("trust <object>\n"+
           "  Tries to export the uid '"+getuid(ME)+"' to <object>,\n"+
           "  which is searched in your inventory or your environment.\n"+
           "trust all\n"+
           "  Tries to export the uid to all objects in your inventory.\n"+
           "trust here\n"+
           "  Tries to export the uid to all objects in your environment.\n"
          );
    return 1;
  }
  if (!check_euid()) return 1;

  env = environment(PL);
  if (arg == "all") inv = all_inventory(PL);
  else if (arg == "here" && env) inv = all_inventory(env);
  else {
    ob = present(arg, PL);
    if (!ob && env) ob = present(arg, env);
    if (!ob) inv = ({});
    else inv = ({ ob });
  }

  if (!sizeof(inv)) write ("Nothing found to trust to.\n");
  else
    for (i = 0; i < sizeof(inv); i++) {
      ob = inv[i];
      if (ob == PL || ob == ME) continue;
      short = (string) ob->QueryShort();
      if (!short) short = object_name(ob);

      if (geteuid(ob))
        write ("Can't trust "+short+": euid is set.\n");
      else {
        export_uid(ob);
        if (getuid(ob) != getuid(ME))
          write ("Can't trust "+short+", but don't know why.\n");
        else write ("Trusting "+short+".\n");
      }
    }

  return 1;
}

/*-------------------------------------------------------------------------
** Command 'stub'
**
** Create a stub file for an alien object in own directory.
*/

int fstub (string arg) {
  object env, ob;
  string src, base, dest, foo, *bar;

  if (!arg || arg == "?") {
    write (
"stub <object> [<path>]\n"+
"  Creates a small stubfile for <object> in <path>. This file just consists\n"+
"  of the line 'inherit \"<object_name(<object>)>\";', thus giving you full\n"+
"  access to someone else's tool if you clone the stub.\n"+
"  If <path> is a directory, the file will get the <object>s basename.\n"+
"  If <path> is not given, it defaults to your homedirectory.\n"+
"  <object> may be given also as its complete path, else it is searched in\n"+
"  your inventory and in your environment.\n"
          );
    return 1;
  }
  if (!check_euid()) return 1;

  if (sscanf (arg, "%s %s", src, dest) != 2) src = arg, dest = "~/";

  env = environment(PL);
  ob = present(src, PL);
  if (!ob && env) ob = present(src, env);
  if (!ob) {
    src = (string) MASTER->full_path(src,PL->QueryRealName());
    if (file_size (src) == -1 && file_size(src+".c") == -1) {
      write ("Can't find '"+src+"'.\n");
      return 1;
    }
    if (sscanf(src, "%s.c", foo)) src = foo;
  } 
  else src = object_name(ob);
  src += "#";
  bar = explode(src, "/");
  sscanf (bar[sizeof(bar)-1], "%s#%s", base, foo);
  sscanf (src, "%s#%s", src, foo);

  dest = (string) MASTER->full_path(dest,PL->QueryRealName());
  if (file_size(dest) == -2) dest += "/"+base;
  if (!sscanf(dest, "%s.c", foo)) dest += ".c";

  if (file_size(dest) == -2) {
    write ("Destination "+dest+" is a directory.\n");
    return 1;
  }
  if (file_size(dest) >= 0 && !rm(dest)) {
    write ("Can't remove old "+dest+".\n");
    return 1;
  }

  if (!write_file (dest, "inherit \""+src+"\";\n")) {
    write ("Can't write "+dest+".\n");
    return 1;
  }
  
  write ("Created stub '"+dest+"'\n  for object '"+src+"'.\n");
  return 1;
}

/*-------------------------------------------------------------------------
** Command 'chkuids'
**
** Give the uids of the object(s) of interest.
*/

int fuid (string arg) {
  object *inv, ob, env;
  string short, userid, euserid;
  string spaces;
  int i;

  spaces = "                                        ";

  if (!arg || arg == "?") {
    write ("chkuids <object>\n"+
        "  Prints the uids of <object>, which is searched in your inventory\n"+
        "  or your environment.\n"+
        "chkuids all\n"+
        "  Prints the uids of all objects in your inventory.\n"+
        "chkuids here\n"+
        "  Prints the uids of all objects in your environment.\n"
          );
    return 1;
  }

  env = environment(PL);
  if (arg == "all") inv = all_inventory(PL);
  else if (arg == "here" && env) inv = all_inventory(env);
  else {
    ob = present(arg, PL);
    if (!ob && env) ob = present(arg, env);
    if (!ob) inv = ({});
    else inv = ({ ob });
  }

  if (!sizeof(inv)) write ("Nothing found to check.\n");
  else
    for (i = 0; i < sizeof(inv); i++) {
      ob = inv[i];
      short = (string) ob->QueryShort();
      if (!short) short = object_name(ob);

      userid = getuid(ob);
      if (!userid) userid = "<none>";
      else if (userid == BACKBONEID) userid = "<Backbone>";
      else if (userid == ROOTID) userid = "<Root>";
      else userid = "'"+userid+"'";

      euserid = geteuid(ob);
      if (!euserid) euserid = "<none>";
      else if (euserid == BACKBONEID) euserid = "<Backbone>";
      else if (euserid == ROOTID) euserid = "<Root>";
      else euserid = "'"+euserid+"'";

      write ((short+spaces)[0..20]);
      write (": uid = ");
      write ((userid+spaces)[0..15]);
      write (" euid = ");
      write ((euserid+spaces)[0..15]);
      write ("\n");
    }

  return 1;
}

/*-------------------------------------------------------------------------
** Command 'Suspicious'
**
** Destruct ourself, but update the player's auto_load list.
*/

int fsusp (string arg) {

  if (arg == "?") {
    write ("susp/icious\n"+
           "  Destructs this truster.\n"
          );
    return 1;
  }
  if (arg) return 0;
/*  if (!check_euid()) return 1; */

  write ("Your truster leaves this world...\n");
    /* Try being nice before destructing me the hard way */
  if (!thing::remove()) destruct(ME);

  return 1;
}

/*-------------------------------------------------------------------------
** Commands 'Invisible' and 'Visible'
**
** Toggles the visibility, This also needs to be reflected in the
** auto_load list.
*/

int fvis (string arg) {
  string action;

  if (query_verb() == "invis" || query_verb() == "invisible") action = "in";
  else action = "";

  if (arg == "?") {
    write (action+"vis truster\n"+
           "  Makes the truster "+action+"visible.\n"
          );
    return 1;
  }
  if (arg != "truster") return 0;

  invis = action == "in";
  write ("Your truster is now "+action+"visible.\n");
  return 1;
}

/*************************************************************************/
