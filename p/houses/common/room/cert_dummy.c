/* This object makes a directory, writes some files in it, and gives a
 * property certificate to the player.
 */

#include <moving.h>
#include <properties.h>
#include "/p/houses/common/sys/prices.h" 
inherit "/std/thing";

string homedir;
string exitverb;
string exitpath;

create()
{
  ::create();
  seteuid(getuid());
  SetShort("a property certificate for a house somewhere");
  SetLong("This is the certificate for your property.\n\
If you have enough money you can 'build' a house there.\n\
For to expand your house, 'build' or 'dig' in a direction.\n\
You can 'name' every room, and 'describe' them.\n\
You can 'allow' or 'forbid' players to enter your house.\n");
  SetIds(({"certificate","cert"}));
  SetHomedir("/players/sique");
  SetExitverb("west");
  SetExitpath("/players/sique/workroom");
  SetValue(GROUND_PRICE);
}

init()
{
  object  cert;
  string  owner;

  ::init();
  if(!interactive(this_player()||this_object())) return;
  if(query_verb() != "buy") return;

  owner = getuid(this_player());
  cert = clone_object("/p/houses/common/obj/certificate");
  cert->SetShort(QueryShort());
  cert->SetLong(QueryLong());
  cert->Set(P_AUTOOBJECT,({QueryShort(),QueryLong()}));
  cert->move(this_player(),M_SILENT);
  destruct(this_object());  
}

MakeProperty(string owner)
{
  string  file;
  string  out;
  string *exits;
  int     exitnumber;
  object  cert;

  if(!mkdir(QueryHomedir() + "/" + owner))
    return 0;
  switch(QueryExitverb()) {
  case "north": file = "011"; break;
  case "south": file = "211"; break;
  case "west": file = "101"; break;
  case "east": file = "121"; break;
  default: return 0;
  }
  write_file(QueryHomedir() + "/" + owner + "/" + file + ".c",
"inherit \"/p/houses/common/room/buildplace\";\ncreate(){  ::create();\n\
  AddExit(\"" + QueryExitverb() + "\",\"" + QueryHomedir() + "/" + owner +
          "/garden1\");\n  SetIndoors(0);\n}\n");
  exits = ({"north","west","south","east"});
  exitnumber = member(exits, QueryExitverb());

  write_file(QueryHomedir() + "/" + owner + "/garden1.c",
	     "/* The garden of " + capitalize(owner) + " */\n\
inherit \"/p/houses/common/room/meadow\";\ncreate() {\n  ::create();\n\
  SetIntShort(\"in the garden of " + capitalize(owner) + "\");\n\
  SetIntLong(\"You are standing in the garden of " + capitalize(owner) +
	     ".\\n\\\nYou can leave it to the " + QueryExitverb() + ".\\n\\\n\
To the " + exits[(exitnumber + 2)%4] + " " + capitalize(owner) +
	     " is building a house.\\n\\\n\
To the " + exits[(exitnumber + 1)%4] + " and " + exits[(exitnumber + 3)%4] +
	     " you can go deeper into the garden.\\n\\\n\
But maybe you can search for herbs in this garden?\\n\");\n\
  AddExit(\"" + QueryExitverb() + "\",\"" + QueryExitpath() + "\");\n\
  AddExit(\"" + exits[(exitnumber + 2)%4] + "\",\"./" + file + "\");\n\
  AddExit(\"" + exits[(exitnumber + 1)%4] + "\",\"./garden2\");\n\
  AddExit(\"" + exits[(exitnumber + 3)%4] + "\",\"./garden8\");\n\
  AddHerb(\"/d/avalon/common/obj/fruits/dandilion\",2);\n}\n\n\
init()\n{\n  ::init();\n  call_other(\"" + QueryHomedir() + "/" + owner + "/" +
  file + "\",\"???\");\n}\n");

  write_file(QueryHomedir() + "/" + owner + "/garden2.c",
	     "/* The garden of " + capitalize(owner) + " */\n\
inherit \"/p/houses/common/room/meadow\";\ncreate() {\n  ::create();\n\
  SetIntShort(\"in the garden of " + capitalize(owner) + "\");\n\
  SetIntLong(\"You are standing in the garden of " + capitalize(owner) +
	     ".\\n\\\n\
To the " + exits[(exitnumber + 2)%4] + " and " + exits[(exitnumber + 3)%4] +
	     " you can go deeper into the garden.\\n\\\n\
But maybe you can search for herbs in this garden?\\n\");\n\
  AddExit(\"" + exits[(exitnumber + 2)%4] + "\",\"./garden3\");\n\
  AddExit(\"" + exits[(exitnumber + 3)%4] + "\",\"./garden1\");\n\
  AddHerb(\"/d/avalon/common/obj/fruits/dandilion\",2);\n}\n");

  write_file(QueryHomedir() + "/" + owner + "/garden3.c",
	     "/* The garden of " + capitalize(owner) + " */\n\
inherit \"/p/houses/common/room/meadow\";\ncreate() {\n  ::create();\n\
  SetIntShort(\"in the garden of " + capitalize(owner) + "\");\n\
  SetIntLong(\"You are standing in the garden of " + capitalize(owner) +
	     ".\\n\\\n\
To the " + exits[(exitnumber + 3)%4] + " " + capitalize(owner) +
	     " is building a house.\\n\\\n\
To the " + exits[(exitnumber + 2)%4] + " and " + exits[exitnumber] +
	     " you can go deeper into the garden.\\n\\\n\
But maybe you can search for herbs in this garden?\\n\");\n\
  AddExit(\"" + exits[(exitnumber + 2)%4] + "\",\"./garden4\");\n\
  AddExit(\"" + exits[exitnumber] + "\",\"./garden2\");\n\
  AddHerb(\"/d/avalon/common/obj/fruits/dandilion\",2);\n}\n");

  write_file(QueryHomedir() + "/" + owner + "/garden4.c",
	     "/* The garden of " + capitalize(owner) + " */\n\
inherit \"/p/houses/common/room/meadow\";\ncreate() {\n  ::create();\n\
  SetIntShort(\"in the garden of " + capitalize(owner) + "\");\n\
  SetIntLong(\"You are standing in the garden of " + capitalize(owner) +
	     ".\\n\\\n\
To the " + exits[(exitnumber + 3)%4] + " and " + exits[exitnumber] +
	     " you can go deeper into the garden.\\n\\\n\
But maybe you can search for herbs in this garden?\\n\");\n\
  AddExit(\"" + exits[exitnumber] + "\",\"./garden3\");\n\
  AddExit(\"" + exits[(exitnumber + 3)%4] + "\",\"./garden5\");\n\
  AddHerb(\"/d/avalon/common/obj/fruits/dandilion\",2);\n}\n");

  write_file(QueryHomedir() + "/" + owner + "/garden5.c",
	     "/* The garden of " + capitalize(owner) + " */\n\
inherit \"/p/houses/common/room/meadow\";\ncreate() {\n  ::create();\n\
  SetIntShort(\"in the garden of " + capitalize(owner) + "\");\n\
  SetIntLong(\"You are standing in the garden of " + capitalize(owner) +
	     ".\\n\\\n\
To the " + exits[exitnumber] + " " + capitalize(owner) +
	     " is building a house.\\n\\\n\
To the " + exits[(exitnumber + 3)%4] + " and " + exits[(exitnumber + 1)%4] +
	     " you can go deeper into the garden.\\n\\\n\
But maybe you can search for herbs in this garden?\\n\");\n\
  AddExit(\"" + exits[(exitnumber + 1)%4] + "\",\"./garden4\");\n\
  AddExit(\"" + exits[(exitnumber + 3)%4] + "\",\"./garden6\");\n\
  AddHerb(\"/d/avalon/common/obj/fruits/dandilion\",2);\n}\n");

  write_file(QueryHomedir() + "/" + owner + "/garden6.c",
	     "/* The garden of " + capitalize(owner) + " */\n\
inherit \"/p/houses/common/room/meadow\";\ncreate() {\n  ::create();\n\
  SetIntShort(\"in the garden of " + capitalize(owner) + "\");\n\
  SetIntLong(\"You are standing in the garden of " + capitalize(owner) +
	     ".\\n\\\n\
To the " + exits[(exitnumber + 1)%4] + " and " + exits[exitnumber] +
	     " you can go deeper into the garden.\\n\\\n\
But maybe you can search for herbs in this garden?\\n\");\n\
  AddExit(\"" + exits[exitnumber] + "\",\"./garden7\");\n\
  AddExit(\"" + exits[(exitnumber + 1)%4] + "\",\"./garden5\");\n\
  AddHerb(\"/d/avalon/common/obj/fruits/dandilion\",2);\n}\n");

  write_file(QueryHomedir() + "/" + owner + "/garden7.c",
	     "/* The garden of " + capitalize(owner) + " */\n\
inherit \"/p/houses/common/room/meadow\";\ncreate() {\n  ::create();\n\
  SetIntShort(\"in the garden of " + capitalize(owner) + "\");\n\
  SetIntLong(\"You are standing in the garden of " + capitalize(owner) +
	     ".\\n\\\n\
To the " + exits[(exitnumber + 1)%4] + " " + capitalize(owner) +
	     " is building a house.\\n\\\n\
To the " + exits[(exitnumber + 2)%4] + " and " + exits[exitnumber] +
	     " you can go deeper into the garden.\\n\\\n\
But maybe you can search for herbs in this garden?\\n\");\n\
  AddExit(\"" + exits[(exitnumber + 2)%4] + "\",\"./garden6\");\n\
  AddExit(\"" + exits[exitnumber] + "\",\"./garden8\");\n\
  AddHerb(\"/d/avalon/common/obj/fruits/dandilion\",2);\n}\n");

  write_file(QueryHomedir() + "/" + owner + "/garden8.c",
	     "/* The garden of " + capitalize(owner) + " */\n\
inherit \"/p/houses/common/room/meadow\";\ncreate() {\n  ::create();\n\
  SetIntShort(\"in the garden of " + capitalize(owner) + "\");\n\
  SetIntLong(\"You are standing in the garden of " + capitalize(owner) +
	     ".\\n\\\n\
To the " + exits[(exitnumber + 2)%4] + " and " + exits[(exitnumber + 1)%4] +
	     " you can go deeper into the garden.\\n\\\n\
But maybe you can search for herbs in this garden?\\n\");\n\
  AddExit(\"" + exits[(exitnumber + 1)%4] + "\",\"./garden1\");\n\
  AddExit(\"" + exits[(exitnumber + 2)%4] + "\",\"./garden7\");\n\
  AddHerb(\"/d/avalon/common/obj/fruits/dandilion\",2);\n}\n");
  
  return 1;
}

ConnectProperty(owner)
{
  if(!QueryExitpath()->AddHome(opposite(QueryExitverb()),
			       QueryHomedir() + "/" + owner + "/garden1",
			       owner))
    return 0;
  return 1;
}

SetExitverb(string verb) { return exitverb = verb; }

SetExitpath(string path) { return exitpath = path; }

SetHomedir(string dir) { return homedir = dir; }

QueryExitverb() { return exitverb; }

QueryExitpath() { return exitpath; }

QueryHomedir() { return homedir; }

opposite(string dest)
{
  switch (dest) {
  case "north": return "south";
  case "south": return "north";
  case "west": return "east";
  case "east": return "west";
  }
  return 0;
}
