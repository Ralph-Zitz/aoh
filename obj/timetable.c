/*  -*- LPC -*-
 * The great timetable in the harbours */

#include <properties.h>

#define DEFAULT_PATH (implode(explode(object_name(environment()||this_object()),\
				      "/")[0..<2],"/") + "/")

inherit "/std/thing";

string *Pquays, Ppath, Pquayid, Pharbour;

public mapping QueryTable();

string SetPath(string str) { return Ppath = str; }
string QueryPath()         { return Ppath||DEFAULT_PATH; }

string SetQuayId(string str) { return Pquayid = str; }
string QueryQuayId()         { return Pquayid; }

string *SetQuays(string *str) { return Pquays = str; }
string *QueryQuays()          { return Pquays; }

string SetHarbour(string str) { return Pharbour = str; }
string QueryHarbour()         { return Pharbour; }

string CheckQuay(string str)
{
  if(str[0] != '/') str = QueryPath() + str;
  if(catch(call_other(str,"???"))) return str;
  if(!({mapping})str->QueryTable()) return 0;
  return str;
}

varargs void create()
{
  ::create();
  seteuid(getuid(this_object()));
  Set(P_IDS,({"table","timetable","time table","time-table"}));
  SetShort("the central time-table");
  Pquays = ({});
  Set(P_NOGET,"It is secured to the ground.\n");
}

string *AddQuay(string str)
{
  if(!(str = CheckQuay(str))) return 0;
  SetQuays(QueryQuays() - ({str}) + ({str}));
  return QueryQuays();
}

varargs string *AddAllQuays(mixed m)
{
  SetQuays((QueryQuays()||({})) +
           map(get_dir(QueryPath() + (QueryQuayId()||"*") + ".c"),
                     #'CheckQuay /*'*/)-({0}));
  return QueryQuays();
}

private string get_single_quay(string ship, int *param)
{
  return sprintf("%s %-35s -> %s\n",
		 param[1]++?"         ":sprintf("Quay %3d:",param[0]),
		 (({string})ship->QueryShort())[0..34],
     ({string})ship->QueryPortName((({mapping})(QueryQuays()[param[0]-1])->
				      QueryTable())[1]
				     [member( QueryQuays()[param[0]-1],
						   QueryTable()[0])]+1), ship);
}

private string get_ships(string path, int number)
{
  int *j;

  j = ({++number,0});
  return implode(map((({mapping})path->QueryTable()||({({})}))[0],
			   #'get_single_quay /*'*/,&j),"");
}
  
varargs string QueryLong()
{
  int i;
  string str;
  
  if(!sizeof(QueryQuays())) AddAllQuays();
  if(!sizeof(QueryQuays())) return "The table is empty.\n";

  str = "The central time-table of all ships servicing the " + QueryHarbour();
  str += "\n------------------------------------------------------------------\
---------------------------"[0..sizeof(str)] + "\n";
  return str + implode(map(QueryQuays(),#'get_ships /*'*/,&i),"");
}
