/*
* A magic (teleporting) die
* Created by Thragor, 25oct94
*/

#include <config.h>
#include <moving.h>
#include <magic.h>
#include <stdrooms.h>
#include <secure/wizlevels.h>
#include <search.h>
#include <properties.h>

#define TCOST 30

inherit "/std/thing";

private string *locations;
private int charges;

public string *QueryLocations()
{
  return (locations||({}))+({});
}

public string *SetLocations(string *s)
{
  if (!pointerp(s)) return (locations||({}))+({});
  if (!sizeof(s))
    {
      locations = 0;
      return ({});
    }
  return locations = s;
}

public string *AddLocation(string s)
{
  if (!stringp(s)||((file_size(s))<1&&(file_size(s+".c")<1))
      ||catch(load_object(s))) return (locations||({}))+({});
  return locations = (locations||({}))+({s});
}

public int QueryCharges()
{
  return charges;
}

public int SetCharges(int i)
{
  if (i<0) i = 0;
  return charges = i;
}

public int AddCharges(int i)
{
  return charges+=i;
}

public string QueryInfo()
{
  return "There are some slight runes of dimensions on its surface.\n"
         "Some other runes are from the kind of the magic of moving.\n"
         "The magic in this die seem to be limited. Currently there\n"
         "seem to be "+to_string(QueryCharges())+" charges left.\n"
         "Oh, this small rune, which looks like an 'A' says, that you\n"
         "may charge the die again.\n";
}

public void create()
{
  (::create());

  SetIds(({"die","cube","magicdie"}));
  SetAds(({"mithril","a","the","reflecting","glowing","magic","teleporting"}));
  SetShort("a mithril die");
  SetLong(
"The die is made out of pure mithril. It's surface reflects the light "
"and sometimes you even have the feeling it glows itself. "
"On each side of the die is a number from one to six, written in ancient "
"letters you only know from old scrolls.\n"
"You have a strange feeling holding the die in your hand, and you don't "
"know if it is a good idea to throw the die.\n");
  SetValue(1000);
  SetWeight(50);
  Set(P_MAGIC,1);
  SetHelpMsg("You could try to throw the die.\n");
  SetLight(5);
  SetReadMsg("1 - turn die - 2 - turn die - 3 - turn die - 4 - turn die - 5 - turn die - 6\n");
  SetSmell("It smells like mithril.\n");
  SetCharges(6);

  AddLocation(CHURCH);						// ok
  AddLocation("/d/archwiz/common/room/city/hump");		// ok
  AddLocation("/d/archwiz/common/room/city/fields");		// ok

  AddLocation("/d/orient/common/room/desert/d33");		// ok
  AddLocation("/d/orient/common/room/essaman/bazar2");		// ok

  AddLocation("/d/shadows/common/room/outskirts/oute12");	// ok
  AddLocation("/d/shadows/common/room/outskirts/southconnect4");// ok
  AddLocation("/d/shadows/common/room/outskirts/road9");	// ok

  AddLocation("/d/wasteland/common/room/plain/ruin");		// ok
  AddLocation("/d/wasteland/common/room/plain/plane6");		// ok
  AddLocation("/d/wasteland/common/room/plain/deep_forest2");	// ok

  AddLocation("/d/halfling/randyandy/tunnel13");		// ok
  AddLocation("/d/halfling/randyandy/dwarf/town_hall");		// ok
  AddLocation("/d/halfling/suamor/town/town_hall");		// ok
  
  AddLocation("/d/daydream/xilef/plane/plane1");		// ok

  AddLocation("/d/ireland/common/york/cliff");			// ok
  AddLocation("/d/ireland/common/1E6");				// ok
  AddLocation("/d/ireland/common/forest/2K7");			// ok

  AddLocation("/d/atlantis/common/room/jungle/44");		// ok
  AddLocation("/d/atlantis/common/room/pern/meadow2,2");	// ok
  AddLocation("/d/atlantis/common/room/underwater/u81,13");	// ok

  AddLocation("/d/avalon/common/paris/eiffel_top");		// ok
  AddLocation("/d/avalon/common/highland/spring");		// ok
  AddLocation("/d/avalon/common/viking/pub");			// ok

  AddLocation("/d/woodland/common/sandy/beach3");		// ok
  AddLocation("/d/woodland/common/tunnels/t1");			// ok
  AddLocation("/d/woodland/common/city/road11");		// ok

/*
  AddLocation("/d/valhalla/common/midgard/wilderness/northpath1");
  AddLocation("/d/valhalla/common/midgard/city/gates");
  AddLocation("/d/valhalla/common/midgard/city/mplace2");
*/
}

public string calculate_destination(string plname,int number)
{
  int dsize,startpos,stepsize,pos;
  string *dest;

  if (!stringp(plname)) return 0;
  if ((!dsize = sizeof(QueryLocations()))||dsize<6||number>6||number<1) return 0;
  startpos = to_int("/obj/lib/number"->a_avarage(filter(
		    explode(plname,""),lambda(({'v}),({#'[,'v,0})))))%dsize;
  stepsize = (plname[0]%5+1)*3;
  dest = ({QueryLocations()[startpos]});
  pos = startpos;
  do
    {
      pos=(pos+stepsize)%dsize;
      while(member(dest,QueryLocations()[pos])!=-1) pos++;
      dest+=({QueryLocations()[pos]});
    }
  while (sizeof(dest)<6);
  return dest[number-1];
}

private int teleport_check(object pl)
{
  object e;
  
  if (!pl||
      sizeof((pl->QueryEnemies()||({}))-({0}))||
      !(e = environment(pl))||
      !(e->QueryTPort()&TPORT_OUT)) return 0;
  return 1;
}

private int teleport(object pl,string dest,int number)
{
  if (!pl) return 0;
  write("You throw your die and it shows a "+to_string(number)+".\n");
  say(capitalize(pl->QueryName()||pl->QueryShort()||"someone")+
      " throws a die. It shows a "+to_string(number)+".\n");
  if (!teleport_check(pl)||!QueryCharges())
    return 1;
  AddCharges(-1);
  if (pl->QueryRealLevel()<6)
    {
      write("You have a dizzy feeling for a moment and you nearly fall "
            "unconscious. Then the feeling vanishes again. A strange "
            "experience.\n");
      return 1;
    }
  if (!pl->ReduceSP(TCOST))
    {
      pl->SetSP(0);
      write("Your die crumbles to dust.\n");
      call_out(#'remove,1);
    }
  return pl->move(dest,M_TELEPORT);
}

private int defines_verb(object pl,object ob,string verb)
{
  int start, pos;
  mixed * actions;

  actions = query_actions(pl, 1|8);
#if 0
  /* Ugly, old code, leading to 'Too long evaluations'.
   * It has a bug, too: if the searched verb is first in actions,
   * it will return 0 nevertheless. [Mateese]
   */
  for (start = 0; start < sizeof(actions); start = pos+2)
    {
      pos = member(actions[start..], verb);
      if (actions[pos+1] == ob)
        return pos;
      /* if (verb == actions[start] && ob == actions[start+1])
       *   return 1;
       * would have sufficed here.
       */
    }
#endif
  while (sizeof(actions) && -1 != (pos = member(actions, verb)))
  {
    if (actions[pos+1] == ob)
      return 1;
    actions = actions[pos+2..];
  }
  return 0;
}

public int cmd_throw(string what)
{
  object pl,ob;
  string dest;
  int number;

  if (!pl = this_player()) return 0;
  if (!what) return (int)notify_fail("Throw what?\n")&&0;
  if (!pl->CanSee()) 
    if (pl->CantSee()>0) return (int)notify_fail("It's too bright.\n")&&0;
    else return (int)notify_fail("It's too dark.\n")&&0;
  if (!ob=pl->Search(what,SEARCH_ENV_INV|SEARCH_OBJECT))
    return (int)notify_fail("You don't see that here.\n")&&0;
  if (ob!=this_object())
#if 0
    if (defines_verb(pl,ob,"throw")) return 0;
    else return (int)notify_fail("You can't throw this.\n")&&0;
#endif
  {
    notify_fail("You can't throw this.\n", NOTIFY_NOT_CMD);
    return 0;
  }
  if (!environment(ob)) return 0;
  if (environment(ob)==environment(pl))
    return (int)notify_fail("You have to get the die first.\n")&&0;
  if (environment(environment(ob))==pl)
    return (int)notify_fail("You first have to get the die from "+
                            (environment(ob)->QueryShort()||"something")+
                            ".\n")&&0;
  if (!dest = calculate_destination(getuid(pl),number = 1+random(6)))
    {
      write("You throw your die and it shows a "+to_string(number)+".\n");
      say(capitalize(pl->QueryName()||"someone")+" throws a "+
          to_string(number)+"on a die.\n");
      return 1;
    }
  return teleport(pl,dest,number);
}

public int cmd_charge(string what)
{
  object pl,ob;
  string dest;

  if (!pl = this_player()) return 0;
  if (!what) return (int)notify_fail("Charge what?\n")&&0;
  if (!pl->CanSee()) 
    if (pl->CantSee()>0) return (int)notify_fail("It's too bright.\n")&&0;
    else return (int)notify_fail("It's too dark.\n")&&0;
  if (!ob=pl->Search(what,SEARCH_ENV_INV,SEARCH_OBJECT))
    return (int)notify_fail("You don't see that here.\n")&&0;
  if (ob!=this_object())
#if 0
    if (defines_verb(pl,ob,"charge")) return 0;
    else return (int)notify_fail("You can't charge this.\n")&&0;
#endif
  {
    notify_fail("You can't charge this.\n", NOTIFY_NOT_CMD);
    return 0;
  }
  if (!environment(ob)) return 0;
  if (environment(ob)==environment(pl))
    return (int)notify_fail("You have to get the die first.\n")&&0;
  if (environment(environment(ob))==pl)
    return (int)notify_fail("You first have to get the die from "+
                            (environment(ob)->QueryShort()||"something")+
                            ".\n")&&0;
  if (!pl->ReduceSP(10))
    return (int)notify_fail("Your magical power is too low.\n")&&0;
  if (QueryCharges()==pl->QueryLevel())
    {
      call_out(#'remove,1);
      pl->DoDamage(5+random(5));
      return (int)notify_fail("Uh, oh. The die explodes.\n"
                              "The charge was too high for you to handle.\n")&&0;
    }
  AddCharges(1);
  write("Your magical power flows into the die.\n");
  return 1;
}

public void init()
{
  (::init());
  add_action("cmd_throw","throw");
  add_action("cmd_charge","charge");
}
