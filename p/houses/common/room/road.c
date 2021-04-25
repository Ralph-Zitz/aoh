/* The road for to add homes */

#include <moving.h>
inherit "/std/room";

mixed *homes;

create()
{
  string *dummy;
  int     i;

  ::create();
  homes = ({});
/*
  if(file_size(object_name() + ".rc") != -1)
*/
  if(file_size(object_name() + ".rc") > 0)
  {
    for(i = sizeof(dummy = explode(read_file(object_name() + ".rc"),"\n")-({""}));
        i--; ) {
      homes += ({explode(dummy[i]," ")});
      AddExit(homes[<1][0],homes[<1][1]);
    }
    /* Check existance of homes */
    for (i = sizeof(homes); i--; )
    {
      if (file_size(homes[i][1]) <= 0 && file_size(homes[i][1]+".c") <= 0)
	DeleteHome(homes[i][2]);
    }
  }
}

QueryIntLong(mixed arg)
{
  string the_long;

  return ::QueryIntLong(arg) + AddLong();
}

AddHome(string exit, string path, string owner)
{
  object *inv;
  int     i;

  if(QueryExitsDest()[exit]) return 0;
  write_file(object_name() + ".rc",exit + " " + path + " " + owner + "\n");
  AddExit(exit,path);
  if(!homes) homes = ({({exit,path,owner})});
  else homes += ({({exit,path,owner})});
  for(i = sizeof(inv = all_inventory()); i--; )
    if(living(inv[i])) inv[i]->move(this_object(),M_SILENT);
  tell_room(this_object(),"To the " + exit + " the home of " +
    capitalize(owner) + " has opened.\n");
  return 1;
}

AddLong()
{
  int    i;
  string out;

  out = "";
  for(i = sizeof(homes); i--; ) {
    switch (i) {
    case 0:
      out += "To the " + homes[i][0] + " the house of " +
        capitalize(homes[i][2]) + " can be found.\n";
      break;
    case 1:
      out += "The house of " + capitalize(homes[i][2]) +
        " lies to the " + homes[i][0] + ".\n";
      break;
   case 2:
      out += "To the " + homes[i][0] +
        " you see the entrance to the house of " +
          capitalize(homes[i][2]) + ".\n";
      break;
    }
  }
  return out;
}

DeleteHome(string owner)
{
  string out;
  int    i;
  object *inv;

  if(!owner) return 0;
  if(!homes || !sizeof(homes)) return 0;
  for (i = 0; i < sizeof(homes); i++)
    if (homes[i][2] == owner)
      break;
  if (i >= sizeof(homes))
    return 0;
  RemoveExit(homes[i][0]);
  rm(object_name() + ".rc");
  homes = homes[0..i-1] + homes[i+1..];
  if(i = sizeof(homes)) {
    out = "";
    for( ; i--; )
      out += implode(homes[i]," ") + "\n";
    write_file(object_name() + ".rc", out);
  }
  for(i = sizeof(inv = all_inventory()); i--; )
    if(living(inv[i])) inv[i]->move(this_object(),M_SILENT);
  tell_room(this_object(),"The home of " + capitalize(owner) +
            " is closed now.\n");
  return 1;
}

exist(mixed home, string owner) { return home[2] == owner; }

QueryHomes() { return homes; }







