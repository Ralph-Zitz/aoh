inherit "std/room";
inherit "/obj/lib/string";

#include <rooms.h>
#include <doors.h>
#include <properties.h>
#include "path.h"

#define SF(x) #'x //'
#define SYM(x) 'x //'
#define OCEAN "/room/ocean"

private static string* Pquays;

public string *SetQuays(string *q)
{
  if (pointerp(q))
    q = map(q,SF(resolve_file));
  return Pquays = q;
}
public string *QueryQuays() { return (Pquays||({}))+({}); }
public string *AddQuay(string file)
{
  file = resolve_file(file);
  return Pquays = (Pquays||({}))-({file})+({file});
}

public string detail_table(string what)
{
  mapping table;
  string *ships,*list,quay,*quays;
  int q,i,nr;
  q = sizeof(quays = sort_array(QueryQuays(),SF(<)));
  list = ({});
  while(q--)
    {
      sscanf(quays[q],PORT("quai%d.c"),nr);
      table = quays[q]->QueryTable();
      i = sizeof(ships = m_indices(table));
      while(i--)
        {
          string line;
          object ship;
          if (table[ships]==-1)
	    continue;
          line = ladjust(capitalize(ships[i]->Query(P_NAME)||
                                    strip_article(ships[i]->QueryShort())
                                   )+" (quay "+to_string(nr)+") ",35,".");
          if (!ship = ships[i]->QueryShip())
            line+=" sunken";
          else
            if (object_name(environment(ship))+".c"==quays[q])
              line+=" in harbour";
            else
	      {
	        if (object_name(environment(ship))==OCEAN)
                  line+=" to ";
	        else
                  line+=" at ";
	        line+=ship->Harbour();
	      }
	  list+=({line});
	}
    }
  return
    "Ships, which visit this harbour:\n"
    "--------------------------------\n\n"+implode(list,"\n")+"\n";
}

public void create()
{
  (::create());
  SetIntShort("the harbour of Nightfall City");
  SetIntLong(
    "You are in the harbour of Nightfall City. From here you can sail to "
    "many other harbours all over the world. The quays can be found to the "
    "east. A timetable shows all ships which support this harbour.\n"
    "To the north you can leave the harbour and go into the city and to the "
    "south is a small beach.\n"
    "A little kiosk is to the west, where you can buy some useful things. "
    "A door leads into the kiosk.\n");
  SetExplorable(1);
  SetIndoors(0);
  SetIntNoise("You hear the roaring of the Sea and the howling of the wind.");
  AddDetail(({"harbour","nightfall city harbour","city harbour"}),
    "You may board many ships here.\n");
  AddDetail(({"city","capital","nightfall city"}),
    "Nightfall City is the capital of this world.\n");
  AddDetail(({"quays","quais","quay"}),
    "The quays are further east.\n");
  AddDetail(({"beach","small beach"}),
    "It looks inviting.\n");
  AddDetail(({"kiosk","shop","little shop"}),
    "It's just a little shop which sells some unusual items.\n");
  AddExit("east", "./nf_mole1");
  AddExit("south",PIRATE("nearsea"));
  AddExit("north",CITY("vill_shore2"));
  AddDoor("west","./kiosk","shop door",
    "The door is made from wood. A window of coloured glass lets the sun in.\n",
    0,DOOR_STATUS_OPEN);
  SetQuays(map(get_dir(PORT("quai*.c")),
                     lambda(({SYM(s)}),({SF(+),"./",SYM(s)}))
                    )
          );
  AddDetail(({"table","timetable","time table","time-table"}),
    SF(detail_table));
  AddReadMsg(({"table","timetable","time table","time-table"}),
    SF(detail_table));
}
