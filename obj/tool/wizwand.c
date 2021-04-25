/* Wand of Wizardhood
**
** A tool for setting wiz_levels, i.e. promotion of wizards.
**
**   14-Oct-92 [m] 
**   19-Nov-92 [m] Now invisible for non-wiz.
**   26-Nov-92 [m] 'promote seer' added
**   10-Jan-93 [m] Bug in seer promotion fixed.
**   31-Jan-93 [m] Learners can now have directories.
**   15-Feb-93 [m] New rules for promotions.
**   16-Feb-93 [m] The wizscroll is given out.
**   23-Aug-93 [m] Wait-time for learners now 1 hour
**   01-Oct-93 [m] Names are now treated case-insensitive.
**   13-Oct-03 [m] Adapted for new quest_control
**   08-Dec-93 [s] Support for Blackheart's Geneology Chart added
**   15-Feb-94 [m] SEERhood removed.
**   30-Mar-94 [t] Support for adoptives (GenChart) added and
**                 option to add domainmember removed (task of domaintool)
**   19-Jul-94 [m] Promotion to learnerhood must be done by someone else
**                 than promotion to wizardhood.
**   04-Jan-95 [m] A learners parent may demote its child again.
**   21-Mar-98 [m] New level system.
**
**  [m] Mateese, Biontic Magics & Darkness
**  [s] Sique
**  [t] Thragor
*/

#include <properties.h>
#include <secure/wizlevels.h>
#include <secure/userinfo.h>

#include "/obj/lib/stddefs.h"

#include <string.h>
#include "/obj/tool/geneology.h"

inherit "/std/thing";
inherit "/lib/string";

/*-------------------------------------------------------------------------*/

#define REVTAG   "980321"
#define VERSION  1
#define REVISION 8
#define VER_REV  VERSION+"."+REVISION

#define USAGE(con,txt) if (con) return notify_fail(txt) && 0;else
#define ERROR(msg) return (write(msg) || 1)
#define FAIL(msg) return (notify_fail(msg) && 0)

varargs int check_euid (int silently, object pl);

/*-------------------------------------------------------------------------*/

private static mixed *uinfo;

/*-------------------------------------------------------------------------*/

create()
{
  string name, foo;

  if (!is_clone(ME)) return;

  thing::create();
  SetShort("the Wand of Wizardhood");
  SetIds(({"wand","wizwand"}));
  SetReadMsg(
"A small sign reads:\n"
"  Wand of Wizardhood -- v "+VER_REV+" ["+REVTAG+"]\n"
"            Biontic Magics & Darkness\n"
             );
  SetInfo("Wand of Wizardhood "+VER_REV+" ["+REVTAG+"] by Mateese");
  SetNoDrop("Dropping such a mighty wand? Funny idea!\n");
  SetNoGive(1);
  SetNoGet(1);
}

QueryShort()
{
  return (!PL || IS_PLAYER(PL)) ? "something useless" : ::QueryShort();
}

QueryHelpMsg()
{
  string rc;
  if (!PL || IS_PLAYER(PL)) return "You don't know anything about it.\n";
  rc = " -- Wand of Wizardhood "+VER_REV+" ["+REVTAG+"] --\n"
       "This wand is for promotion of immortals and wizards and for direct\n"
       "setting of levels.\n"
       "The commands are 'promote', 'demote' and 'ulevel'.\n";
  rc += "\nGive '?' as parameter for help.\n"
        "The wand identifies itself as 'wand' and 'wizwand'.\n";
  return rc;
}

QueryLong()
{
  string rc;
  if (!PL || IS_PLAYER(PL)) return "It's dirty, old and useless.\n";
  return QueryHelpMsg();
}

id(str)
{
  if (!PL || IS_PLAYER(PL)) return "something useless";
  return thing::id(str);
}

QueryNoGet()
{
  if (!check_euid(1)) return "This wand is not yours.\n";
  return 0;
}

/*-------------------------------------------------------------------------
** Check the euid of this_player() against ours.
** If we have none, try to set it to this_players().
** If this fails, the player can't use us, our euid is reset and tried
** to be set to the players one.
*/

varargs int check_euid (int silently, object pl)
{
  if (!pl) pl = PL;
  if (!pl || !living(pl)) return 0;
  if (!previous_object() || geteuid(previous_object()) != geteuid(ME))
  {
    if (!silently) write ("Sorry, this wand is not yours.\n");
    return 0;
  }

  if (!geteuid(ME) && !seteuid(geteuid(pl)))
  {
    if (!silently) write ("Sorry, this wand is not yours.\n");
    return 0;
  }

  if (!geteuid(ME) || geteuid(ME) != geteuid(pl))
  {
    seteuid(0);
    if (!seteuid(geteuid(pl)))
    {
      if (!silently) write ("You have no right to do that!\n");
      return 0;
    }
  }
 return 1;
}

/*-------------------------------------------------------------------------
** check_owner() checks if the right player is trying to use us.
*/

check_owner()
{
  return PL && IS_IMMORTAL(PL) && PL == environment(ME) && PL == IPL;
}

/*-------------------------------------------------------------------------
** Add and decode our commands.
*/

init()
{
  if (!check_owner()) return 0;
  thing::init();
  add_action ("read"      , "inspect");
  add_action ("_dispatch" , "", 1);
}

int _dispatch(string arg)
{
  string verb, foo;
  verb = query_verb();
  if (verb && verb != "" && function_exists("f"+verb, ME)) 
    return (int) call_other(ME, "f"+verb, arg);
  return 0;
}

/*-------------------------------------------------------------------------
** Shout to all except PL and given pl;
*/

void xshout (string msg, object pl)
{
  object *u;
  int i;
  u = users() - ({ PL, pl });
  for (i = 0; i < sizeof(u); i++) tell_object(u[i], msg);
}

/*-------------------------------------------------------------------------
** Equip a 'victim' with the wizscroll.
*/

static void give_scroll (object pl)
{
  object scr;

  if (present("wizscroll", pl))
  {
    tell_object (pl,"Your scroll receives additional information. Read it.\n");
    return;
  }
  clone_object ("/obj/wizscroll")->move(pl);
  tell_object(pl,"You receive a scroll with valuable information. Read it.\n");
}

/*-------------------------------------------------------------------------
** Promote a player to be a ancient.
*/

static int pro_ancient (string name)
{
  object pl;
  string wname;
  wname = capitalize((string)PL->QueryName());
  name = capitalize(name);
  if (!IS_IMMORTAL(PL)) ERROR("You can't do that.\n");
  if (IS_IMMORTAL(name)) ERROR("Somebody already did that.\n");
  if (!(pl = find_player(lower_case(name))) 
      || (environment(pl) != environment(PL))
      ) ERROR(name+" is not here.\n");
  if ((int)pl->QueryRealLevel() < 20 || uinfo[USER_LEVEL] >= LVL_ANCIENT)
    ERROR(name+" is not of high enough level.\n");
#if 0
  if ((int)pl->QueryXP() < LEARNER_MIN_EXP)
    ERROR(name+" has not enough experience.\n");
  if (!QUESTMASTER->QueryWizQuests (pl))
    ERROR(name+" has not solved enough quests.\n");
#endif

  write ("You wave your wand at "+name+". The air starts to glow...\n");
  show_object (pl, wname+" waves a wand at you. The air starts to glow...\n\n"
	       , "Something happens, and the air starts to glow...\n\n");
  show ( ({ wname+" waves a wand at "+name+". The air starts to glow...\n\n"
	      , "Something happens, and the air starts to glow...\n\n"
	      }), ({ PL, pl }) );

  call_out ("pro_ancient2", 1, ({ pl, name, wname }));
  xshout ("Something important happens...\n", pl);
  return 1;
}

void pro_ancient2 (mixed *data)
{
  object pl;
  string wname, name;
  pl = data[0]; name = data[1]; wname = data[2];
  tell_object (pl, 
"The glow fades away, but you feel new powers flowing through you...\n"
"You are now recognized as seasoned player of OSB!\n"
"You are given the following new commands:\n"
"  title  people    invis     vis       goto    trans     echoto\n"
"  echo   echoall   force     heal      in      review\n"
"  snoop  earmuffs  man       shutdown  wizmode\n"
"They are activated on your next login. Use them wisely.\n"
              );
  pl->add_ancient_commands();
  give_scroll (pl);
  say ("The glow fades away.\n", ({ pl, PL }) );
  write ("\nThe glow fades away. "+name+" is now an Elder.\n> ");
  xshout (name+" is an Elder now!\n", pl);
}

/*-------------------------------------------------------------------------
** Promote a player or ancient to be a wizard.
*/

static int pro_wizard (string name)
{
  object pl;
  string wname;
  wname = capitalize((string)PL->QueryName());
  name = capitalize(name);
  if (!IS_WIZARD(PL)) ERROR("You can't do that.\n");
  if (IS_WIZARD(name)) ERROR("Somebody already did that.\n");
  if (!(pl = find_player(lower_case(name))) 
      || (environment(pl) != environment(PL))
      ) ERROR(name+" is not here.\n");
#if 0
  if ((int)pl->QueryRealLevel() < 20 || uinfo[USER_LEVEL] >= LEARNER_LVL)
    ERROR(name+" is not of high enough level.\n");
  if ((int)pl->QueryXP() < LEARNER_MIN_EXP)
    ERROR(name+" has not enough experience.\n");
  if (!QUESTMASTER->QueryWizQuests (pl))
    ERROR(name+" has not solved enough quests.\n");
#endif

  write ("You wave your wand at "+name+". The air starts to glow...\n");
  show_object (pl, wname+" waves a wand at you. The air starts to glow...\n\n"
	       , "Something happens, and the air starts to glow...\n\n");
  show ( ({ wname+" waves a wand at "+name+". The air starts to glow...\n\n"
	      , "Something happens, and the air starts to glow...\n\n"
	      }), ({ PL, pl }) );

  if (!MASTER->create_wizard (lower_case(name), -1, PL))
    ERROR ("Funny, that failed. Ask somebody knowledgable about it.\n");

  call_out ("pro_wizard2", 1, ({ pl, name, wname }));
  xshout ("You hear a low rumble...\n", pl);
  // TODO: Reactivate Geneology
#if 0
  GENEOLOGY->NewWizard(lower_case(name),getuid(PL));
  GENEOLOGY->PromoteWizard(lower_case(name),getuid(PL));
#endif
  return 1;
}

void pro_wizard2 (mixed *data)
{
  object pl;
  string wname, name;
  pl = data[0]; name = data[1]; wname = data[2];
  tell_object (pl, 
"The glow fades away, but you feel new powers flowing through you...\n"
"You are accepted as Wizard!\n"
"You are given the following new commands:\n"
"  title  people    invis     vis       goto    trans     echoto\n"
"  echo   echoall   localcmd  force     heal    in        review\n"
"  snoop  earmuffs  man       ls        type    more      cd\n"
"  pwd    falias    clone     destruct  update  shutdown  cp\n"
"  mv     rm        ed        mkdir     rmdir\n"
"They are activated on your next login. Use them wisely.\n"
"You also now have a 'home' to go to.\n"
              );
  pl->add_wizard_commands();
  give_scroll (pl);
  say ("The glow fades away.\n", ({ pl, PL }) );
  write ("\nThe glow fades away. "+name+" is now a Wizard.\n> ");
  xshout (name+" is a Wizard now!\n", pl);
}

/*-------------------------------------------------------------------------
** Command 'promote'
*/

int fpromote (string arg)
{
  string *args, dname;

  if (!check_owner()) return 0;
  if (!arg || arg == "?")
  {
    write (
"promote ancient <name>\n"
"  Promote player <name> as ancient.\n"
"promote wizard <name>\n"
"  Promote <name> as wizard.\n"
    );
    return 1;
  }

  arg = lower_case(arg);
  if (sizeof(args = explode(arg, " ")) < 1)
    FAIL("Not enough arguments.\n");
  if (sizeof(args)>2) FAIL("Too much or wrong arguments.\n");

  if (member(({"ancient","wizard"}),args[0]) == -1) 
    args = ({ "__free__" }) + args;

  if (!(uinfo = (mixed *)MASTER->find_userinfo(args[1]))) 
    FAIL("There's no player '"+args[1]+"'.\n");

  switch (args[0])
  {
    case "ancient" : return pro_ancient(args[1]);
    case "wizard"  : return pro_wizard(args[1]);
  }
  FAIL("Promote whom?\n");
}

/*-------------------------------------------------------------------------
** Command 'demote'
*/

int fdemote (string arg)
{
  object pl;

  if (!check_owner()) return 0;
  if (!arg || arg == "?")
  {
    write(
"demote <name>\n"
"  Demotes wizard <name> back to playerhood.\n"
	  );
    return 1;
  }

  arg = lower_case(arg);
  if (!(uinfo = (mixed *)MASTER->find_userinfo(arg))) 
    FAIL("There's no player '"+arg+"'.\n");

  if (uinfo[USER_LEVEL] != LVL_WIZARD)
    FAIL(capitalize(arg)+" is not a wizard.\n");

  if (!MASTER->change_user_level(arg, LVL_PLAYER))
    ERROR ("Funny, that failed. Ask someone knowledgable about it.\n");

  write("You demote "+capitalize(arg)+" from being wizard.\n"
        "Now somebody has to remove /players/"+arg+" and all that jazz.\n"
       );

  // TODO: Reactivate Geneology here, too
#if 0
  GENEOLOGY->ChangeParent(arg, "**");
#endif
    
  if (pl = find_player(arg))
    tell_object(pl, "--- "+capitalize(PL->QueryRealName())+" demoted you. "
                    +"You are now player again. ---\n"
               );
  return 1;
}

/*-------------------------------------------------------------------------
** Command 'ulevel'
*/

int fulevel (string arg)
{
  string *args;
  int level;

  if (!check_owner()) return 0;

 USAGE (!arg || arg == "?",
 "ulevel <name>\n"
+" Displays the user level of <name>.\n"
+"ulevel <name> <level>\n"
+"  Sets the user level of <name> to <level>.\n"
+"  <level> can be: 'player' or 0, 'ancient' or 1, 'wizard' or 2.\n"
       );

  if (sizeof(args = explode(arg, " ")) > 2) FAIL("Too much arguments.\n");

  args[0] = lower_case(args[0]);
  if (!(uinfo = (mixed *)MASTER->find_userinfo(args[0]))) 
    FAIL("There's no player '"+args[0]+"'.\n");

  if (sizeof(args) != 1)
  {
    // TODO: Allows names instead of numbers.
    level = atoi(args[1]);
    if (!intp(level) || level < LVL_PLAYER || level > LVL_WIZARD)
      switch (lower_case(args[1])) {
      case "player":
      case "p":
        level = LVL_PLAYER;
        break;
      case "ancient":
      case "a":
        level = LVL_ANCIENT;
        break;
      case "wizard":
      case "w":
        level = LVL_WIZARD;
        break;
      default:
        FAIL ("Illegal argument '"+args[1]+"'.\n");
      }
  
    if (!MASTER->change_user_level (args[0], level))
      ERROR ("Funny, that failed. Ask someone knowledgable about it.\n");
  }
  write (capitalize(args[0])+": "+query_user_level(args[0])+"\n");  
  return 1;
}

/*************************************************************************/


