/*
* Police Headquarter
* Office of the Police-AW
*/

inherit "/d/archwiz/common/std/aw_office";

#include <properties.h>
#include <secure/wizlevels.h>
#include <moving.h>
#include <nightday.h>
#include <rooms.h>
#include <doors.h>
#include "path.h"

#define ME this_object()
#define TP this_player()
#define STR "/obj/lib/string"

private static string *names = ({ "cadra", "grough" }); /* Arch first! */

QueryIntLong() {
 return "This is the main office of the Nightfall Police Department NFPD. "
"It's just a large office with a desk, a chair, a small window and "
"a lamp.\n"
"To the north is a strange gate, which seems to lead into another "
"dimension and right beside the gate, there's a sign entitled 'RULES'\n";
}

int query_day()
{
  return (QueryDayState() > ND_DAWN && QueryDayState() < ND_FALL);
}

string _lamp()
{
 return "It's just an ordinary lamp that swings at the room's ceiling.\n"+
	"Since it's "+(query_day()?"day":"night")+" it is "+
	(query_day()?"unlit":"lit")+ ".\n";
}

int IsCop(object pl)
{
  return (member(names,getuid(pl)) > -1);
}

string *QueryCop()
{
  return names;
}

int _read(string str)
{
  if (!TP) return 0;
  if (member(({"rules","sign"}),lower_case(str||""))==-1)
    return (int)notify_fail("Read WHAT?\n")&&0;
  show(TP->QueryName()+" reads the sign \"RULES\".\n");
  STR->smore("You read the sign \"RULES\"."+
             read_file("/etc/RULES"),TP->QueryPagesize());
  return 1;
}

string _window(mixed arg)
{
  string s;
  object sb;
  sb=load_object(CITY("boul2"));
  return "The window leads east on Sunset Boulevard. Outside "+
      ((string)QueryDayDescr()[0..<2])+ " and you see:\n"+
	((s=sb->Content())=="")?"Nothing.\n":s;
}

int _to_tower(string arg)
{
  if (arg)
    return (int)notify_fail("Do what with the north exit???\n")&&0;
  if (!IS_IMMORTAL(TP))
    return (int)notify_fail("You are not allowed to leave this way!\n")&&0;
  return TP->move(TOWER("first/south_corr"),M_SPECIAL,
                  ({"leaves through the mystic exit north",
                    "arrives from south"
                    "leave north through a wormhole in the 4th dimension"}));
}

void create()
{
  (::create());

  SetIntShort("Main Police Office of Nightfall");
  SetIndoors(1);
  SetIntLight(20);
  SetArchwiz(({"Cadra","Grough"}));

  AddDetail("lamp",#'_lamp);
  AddDetail("desk","It is a nice wooden desk\n");
  AddDetail("wood","Old wood.\n");
  AddDetail(({"window","small window"}),#'_window);
  AddDetail(({"office","main office","room","large office"}),
    "You are in it.\n");
  AddDetail("chair","It's behind the desk and reserved for the sheriff.\n");
  AddDetail(({"gate","strange gate"}),
    "This is a dimension-gate. But not everyone might enter it.\n");
  AddDetail(({"sign","rules"}),"You may read it.\n");
  AddReadMsg(({"sign","rules"}),#'_read);

  AddExit("north",#'_to_tower);
  HideExit("north");

  AddDoor("east",
          "./boul2",
          "small door",
          "This is a small door that leads out of the Police Office\n"
          "to Sunset Boulevard.\n",0,DOOR_STATUS_CLOSED);

  AddItem("/std/board",REFRESH_HOME,
          ([P_SHORT:"the Police Board",
            P_LONG:"This is the Police Board of NIghtfall.\n"
                   "You can leave complaints etc. here, but BEWARE:\n"
                   "This is NOT an anonymous board.\n",
            "Groups":({"police.players","police"})
	    ]) );
}
