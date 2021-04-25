/*:ts=2
 * object handling
 * Version 4.2
 *
 *   1.1 with file search mechanism
 *   4.0 new NF I mudlib
 *   4.2 Pepel fixed many things
 */

#include <config.h>
#include <secure/wizlevels.h>
#include <driver/object_info.h>
#include <properties.h>
#include <moving.h>
#include <search.h>
#include <combat.h>

#define THIS this_object()

#define OK { write("OK.\n"); return 1; }

string QueryName();
public varargs mixed Search (mixed what, int mode);    // std/living/description
varargs string find_file(mixed thing, string suffix);  // std/player/filesys

void add_object_commands() {
  if (this_object() != this_player()) return;
  if (!IS_IMMORTAL(this_player())) return;
  add_action("zapobject", "zap");
  if (!IS_WIZARD(this_player())) return;
  add_action("clone", "clone");
  add_action("soft_destruct_object", "destruct");
  add_action("destruct_object", "Destruct");
  add_action("load", "load");
  add_action("soft_update_object", "update");
  add_action("update_object", "Update");
}

// TODO: The parsing and output is crude. Should also handle clones,
// TODO:: local object specs and all that.
int update_object(string str) {
  object ob;
  if (!str) { notify_fail("Usage: Update <object_path>\n"); return 0; }
  ob = find_object(str) || find_object(str[0..<3]);
  if (!ob)
  {
    str = find_file(str) || find_file(str, ".c");
    ob = str && find_object(str);
  }
  if (!ob) { notify_fail(str+": No such file or object.\n"); return 0; }
  destruct(ob);
  write(str + ": will be reloaded at next reference.\n");
  return 1;
}

/*
 * "soft" means that the object is given the chance to self-destruct, thus
 * allowing it to do necessary cleanup like subtracting from the carried
 * weight of the environment(). We call remove() in the object to be
 * destructed.
 */
int soft_update_object(string str) {
  object ob;
  if (!str) { notify_fail("Usage: update <object_path>\n"); return 0; }
  ob = find_object(str) || find_object(str[0..<3]);
  if (!ob)
  {
    str = find_file(str) || find_file(str, ".c");
    ob = str && find_object(str);
  }
  if (!ob) { notify_fail(str+": No such file or object.\n"); return 0; }
  ob->remove();
  if (ob) {
    write(str+": didn't want to be destructed, doing it the hard way!\n");
    destruct(ob);
  }
  write(str + ": will be reloaded at next reference.\n");
  return 1;
}

int clone(string str) {
  object ob;
  string clone_file;
  int err;

  if (!str) { notify_fail("Usage: clone <object_path>\n", NOTIFY_ILL_ARG); return 0; }
  clone_file = find_file(str,".c") || find_file(str);
  if (!clone_file) { notify_fail(str+": No such file.\n"); return 0; }
  ob = clone_object(clone_file);
  if (!ob) { write("Error on cloning.\n"); return 1; }
  /* try to move the object to my environment */
  if ((err = ({int})ob->move(environment(), M_NOCHECK)) != ME_OK) {
    say(QueryName()+
           " wants to fetch something from another dimension, but failed.\n");
    write(clone_file+": failed to move: `"+err+"'\n");
    destruct(ob);
    return 1;
  }
  say(QueryName()+" fetches something from another dimension.\n");
  write(clone_file+"\n");
  /* Try to move it into the player */
  ob->move(this_object(),M_SILENT);
  return 1;
}

/*
 * "soft" means that the object is given the chance to self-destruct, thus
 * allowing it to do necessary cleanup like subtracting from the carried
 * weight of the environment(). We call remove() in the object to be
 * destructed.
 */
int soft_destruct_object(string str) {
  object ob;
  string lstr;

  if (!str) { notify_fail("Usage: destruct <objectname>\n"); return 0; }
  ob = WSearchInvEnv(lower_case(str));
  if (!ob) {
      notify_fail("No " + str + " found.\n");
      return 0;
  }
  lstr = capitalize(({string})ob->QueryShort() || "Something");
  if (getuid(this_object()) != "joan" || !query_once_interactive(ob))
  {
      ob->remove();
      if (ob) {
        write(str+": didn't want to be destructed, doing it the hard way!\n");
        destruct(ob);
      }
  }
  say(lstr+" is disintegrated by "+QueryName()+".\n");
  OK;
}

int destruct_object(string str) {
  object ob;

  if (!str) { notify_fail("Usage: Destruct <objectname>\n"); return 0; }
  ob = WSearchInvEnv(lower_case(str));
  if (!ob) {
      notify_fail("No " + str + " found.\n");
      return 0;
  }
  say(capitalize(({string})ob->QueryShort() || "Something")+" is disintegrated by "
      +QueryName()+".\n");
  if (getuid(this_object()) != "joan" || !query_once_interactive(ob))
  {
      destruct(ob);
  }
  OK;
}

int zapobject(string str) {
  object ob;
  int    alive;

  if (!str) { notify_fail("Usage: zap <objectname>\n"); return 0; }
  ob = WSearchInvEnv(lower_case(str));
  if (!ob) {
      notify_fail("No " + str + " found.\n");
      return 0;
  }
  say(QueryName()+" summons a flash from the sky.\n");
  write("You summon a flash from the sky.\n");
  alive = living(ob);
  if (getuid(this_object()) != "joan" || !query_once_interactive(ob))
  {
      ob->Defend(100000, DT_ZAP, 0);
      write("There is a big clap of thunder.\n");
      if (alive && (!ob || environment(ob) != environment(THIS)))
      {
        ob = clone_object("/std/armour");
        ob->SetStandard(AT_BOOTS, 0); // this first!
        ob->SetShort("a pair of smoky boots");
        ob->SetLong(
    "A pair of boots, burned and charred as if some unearthly powerful got angry\n"
    "with the former owner.\n"
                   );
        ob->SetSmell(
    "The boots smell of burned leather, the still evaporing smoke tickles\n"
    "your nose.\n"
                    );
        ob->AddAdjective("smoky");
        ob->SetQuality(-1);
        ob->SetResetQuality(-1);
        if (ME_OK != ({int})ob->move(environment(THIS), M_SILENT))
          ob->remove();
      }
  }
  return 1;
}

int load(string str) {
  string file;

  if (!str) { notify_fail("Usage: load <object_path>\n"); return 0; }
  file = find_file(str,".c");
  if (!file) file=find_file(str);
  if (!file) { notify_fail(str+": No such file.\n"); return 0; }
  // Someone put a catch() in here, with the neat effect that the wizard
  // just got a single message if something went wrong during the implicite
  // call to create(), but NOTHING ELSE. Very helpful indeed :-(
  load_object(file);
  write(file+"\n");
  return 1;
}
