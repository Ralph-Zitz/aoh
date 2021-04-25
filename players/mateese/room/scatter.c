/* Scatter Room                                                     
**
** This room will scatter all things lying here around the world by
** calling their auto_scatter() function.
** It also saves the actual things lying here.
**
** The setting of scatterlist statically via #include is also kludgy as
** incomfortable, but it seems to be the only way to read sth from a 
** file in a routine called (indirectly) by the reset daemon *sigh*.
**
** For the docs see SCATTER.DOC
**
**   30-Jan-92 [m] Adapted to the NFII-reset()-schedule
**   04-Feb-92 [m] Setting of scatterlist now via #include
**   10-Feb-92 [m] query_scatter_room()
**   23-Apr-92 [m] Targetloads are now catched
**   14-Jun-92 [m] Adapted for native mode.
*/

#include "/players/mateese/mydefs.h"

#include <properties.h>
#include <rooms.h>

#define DATFILE  MP+"data/setscatterlist.c"

#define SCATTER_TIME 1800

#define SAY(str) tell_room(this_room,str)

inherit "/std/room";

string scatterlist;
static object this_room, last_object;
static int last_scatter;

create() {
  room::create();
  SetIntShort ("The Scatter Room");
  SetIntLong (
"The Scatter Room. Everything placed here will be scattered over\n"+
"Nightfall on every reset, as long as the things support auto_scattering.\n"+
"The current contents will also be saved automagically on dropping,\n"+
"and are reloaded on reboot.\n"+
"The manual commands are 'scatter', 'scsave', 'scload [list]' and 'scstat'.\n"
              );
  AddExit ("leave", MP+"workroom");
  AddRoomCmd ("drop", "fdrop");
  AddRoomCmd ("save", "fsave");
  AddRoomCmd ("scload", "freload");
  AddRoomCmd ("scatter", "fscatter");
  AddRoomCmd ("scstat", "fstatus");
  /* SetIntLight(1); */
  Set(P_INDOORS, 1);
  Set(P_TPORT, TPORT_OUT);

#include "/players/mateese/data/setscatterlist.c"
  call_out ("do_auto_scattering", 5, 0);

  this_room = ME;
}

/* Our automatic routine, arg analog to reset */

do_auto_scattering(arg) {
  call_out ("do_auto_scattering", SCATTER_TIME, 1);
  if (!arg) reloadthem(arg);
  scatterthem (arg);
}

/* Scatter the objects all around */

fscatter (str) {
  if (str) {
    notify_fail ("The room doesn't understand you.\n");
    return 0;
  }
  call_out ("scatterthem", 1, 1);
  return 1;
}

scatterthem (arg) {
  object ob;
  int nrsc, nrob;

  last_scatter = time();
  SAY ("The room starts glowing in an intense light...\n");
  nrob = 0;
  ob = first_inventory(ME);
  while (ob) {
    if (call_other(ob, "query_auto_scatter") > 0) {
      if ((nrsc = call_other (ob, "auto_scatter", ME, arg))) {
        nrob += nrsc; 
      }
    }
    ob = next_inventory(ob);
  }
  SAY ("...the light fades away.\n");
  if (nrob) SAY (nrob+" objects scattered.\n");
  else SAY ("Nothing to scatter.\n");
}

/* save the contents of this room into our datafile */

fdrop(str) {
  call_out ("savethem", 2, 1);
  return 0;
}

fsave(str) {
  if (str) {
    notify_fail ("The room doesn't understand you.\n");
    return 0;
  }
  call_out ("savethem", 1, 1);
  return 1;
}

savethem (arg) {
  object ob;
  string s, foo;
  int nrob;

  SAY ("The room touches each item with a beam of light.\n");
  nrob = 0;
  scatterlist = "";
  ob = first_inventory(ME);
  while (ob) {
    if (call_other(ob, "query_auto_scatter") != 0) {
      SAY ("  Saving "+ob->query_name()+".\n");
      sscanf (file_name(ob), "%s#%s", s, foo);
      scatterlist = scatterlist + s + "!";
      nrob++;
    }
    ob = next_inventory(ob);
  }
  if (file_size(DATFILE) >= 0) rm(DATFILE);
  write_file (DATFILE, "scatterlist = \""+scatterlist+"\";\n");
  if (nrob) SAY (nrob+" objects saved.\n");
  else SAY ("Nothing to save.\n");
}

/* Reload this room out of scatterlist. */

freload(str) {
  string foo, bar, baz;
  if (str && str != "list") {
    notify_fail ("The room doesn't understand you.\n");
    return 0;
  }
  if (str && str == "list") {
    foo = read_file(DATFILE);
    if (sscanf (foo, "scatterlist = \"%s\"%s", bar, baz) != 2) {
      write ("The list is cluttered!\n");
      write ("List: '"+scatterlist+"'\n");
      return 1;
    }
    scatterlist = bar;
    write ("List: '"+scatterlist+"'\n");
  }
  call_out ("reloadthem", 1, 1);
  return 1;
}

reloadthem(arg) {
  object ob;
  string slist, s, foo;
  int nrob, objhere;

  if (!scatterlist || scatterlist == "") {
    SAY ("Nothing to reload.\n");
    return;
  }
  SAY ("A stream of light zeroes out everything superfluous!\n");
  cleanupall();
  objhere = 0;
  nrob = 0;
  slist = scatterlist;
  while (slist) {
    if (sscanf (slist, "%s!%s", s, foo) != 2) break;
    slist = foo;
    if (!objhere) {
      SAY ("Some objects appear.\n");
      objhere = 1;
    }
    ob = clone_object(s);
    move_object (ob, ME);
    SAY ("  Loaded "+ob->query_name()+".\n");
    nrob++;
  }
  if (nrob) SAY (nrob+" objects loaded.\n");
  else SAY ("Nothing to load.\n"); 
}

/* Show room's status */

fstatus(str) {
  int size, i, size2;
  string info, rc;
  if (str) {
    notify_fail ("The room doesn't understand you.\n");
    return 0;
  }
  write ("Scatterlist: '"+scatterlist+"'\n");
  write ("Last scatter at "+ctime(last_scatter)+", ");
  write (time()-last_scatter+" Seconds ago.\n");
  info = call_out_info();
  size = sizeof(info);
  write ("Pending call_out()s:\n");
  for (i = 0; i < size; i++) {
    rc = catch(sizeof(info[i]));
    if (!rc) {
      size2 = sizeof(info[i]);
      if (file_name(info[i][0]) == file_name(ME)) {
        write("  "+info[i][1]+" (");
        write(info[i][2]+", ");
        write(info[i][3]+")\n");
      }
    }
  }
  return 1;
}

/* empties this room from anything - needed for manual resets */

cleanupall() {
  object ob;
  int livs, npc;
  int i;

  ob = first_inventory(ME);
  while (ob) {
    livs = living(ob); npc = 0;
    if (livs) npc = call_other(ob, "query_npc");
    if (livs && !npc) { ob = next_inventory(ob); continue; }
    destruct (ob);
    ob = first_inventory(this_object());
  }
  return 1;
}

query_scatter_room() { return 1; } 

/* The routines to ease scattering */

lscatter (scobj, dest, avail, scmsg) {
  int rc;
  string msg, lmsg;

  rc = scatter (scobj, dest, avail, scmsg);
  SAY ("  " +scobj->query_name()+"  "+dest+": ");
  msg = "Failed ";
  if      (rc == -4) msg = msg + "IsThere.";
  else if (rc == -3) msg = msg + "IllDest.";
  else if (rc == -2) msg = msg + "PartDest.";
  else if (rc == -1) msg = msg + "NoDest.";
  else if (rc ==  0) msg = "Ok.";
  else if (rc !=  0) msg = msg + rc + ".";
  SAY (msg+"\n");
  return rc ;
}

scatter (scobj, dest, avail, scmsg) {
  object me, dst, dst2;
  string fname, oname, foo;
  int rc, found;
  int notcomplete;

  last_object = 0;
  if (!dest) return -3;
  if (sscanf (dest, "%s:", foo) != 1) dest = dest+":";
    /* Check for room */
  sscanf (dest, "%s:%s", oname, foo); dest = foo;
  if (!(dst = find_object(oname))) {
    if (catch(call_other (oname, "???"))) return -3;
    if (!(dst = find_object(oname))) return -3;
  }
  found = 1;
  notcomplete = 0;
  while (dest && dest != "") {
    if (sscanf (dest, "%s:", foo) != 1) dest = dest+":";
    if (sscanf (dest, "%s:%s", oname, foo) != 2) break;
    dest = foo;  
    if (!(dst2 = present (oname, dst))) {
      if (avail == 0 || found < avail) return -1;
      notcomplete = 1; 
      break;
    } 
    dst = dst2;
    found++;
  }
  if (present(scobj->query_name(), dst)) return -4;
  sscanf (file_name(scobj), "%s#%s", fname, foo);
  me = clone_object(fname);
  rc = transfer (me, dst);
  if (rc) destruct(me);
  else {
    last_object = me;
    if (found == 1 && scmsg) tell_room (dst, scmsg);
    if (notcomplete) rc = -2;
  }
  return rc;
}

query_scobject() { return last_object; }

