/*
 * A roommaker like we all need it
 *
 * Version 0.991
 *
 * Author: Sique 2/93
 *
 * Last Update: Sep 15 1994
 *
 * Features: - Can create roomfiles with normal exits, items and details -
 * Little text editor with the possibility to delete any lines - Escape with
 * "~!" - The possibility to "see" the room befor the creation of the file -
 * Display of the actual file - Rename of old file.c into file.c~ and into
 * file.old, if file.c changed in the session
 *
 * V 0.94
 *  Adding the possibility to comment the file interactively
 *
 * V 0.95
 *  Including of textes from files
 *  Abbreviation of standart directions with 'n', 's' etc.
 *  Fixing of bugs nobody ever mentioned :)
 *
 * V 0.96
 *  Support of the AddDoor() function of /obj/room
 *  Fixing of other bugs nobody ever mentioned
 *
 * V 0.97
 *  Change the current filename while creating new rooms
 *  Reset all variables after aborting the file
 *
 * V 0.98
 *  looks for an old room with the same Filename, tries to load them
 *  and initialises, if succeed, the maker variables with the values
 *  of the old room
 *
 * V 0.981
 *  The fattest bugs are removed.
 *
 * V 0.982
 *  The roommaker now shorts the paths of exits and doors (see the specifica-
 *  tions of AddExit() and AddDoor() in /doc/std/room)
 *
 * V 0.99
 *  Support of general properties
 *
 * V 0.991
 *  Now makes rooms from different inherit files
 *
 * V 0.992
 *  Several bugfixes, uses now mappings instead of alists for the communication
 *  Registers the creator of the file.
 *
 * V 0.995
 *  Compatibility with new /lib/string.c
 *
 * V 0.996
 *  Upgraded for new /std/room [Mateese]
 *
 * V 0.99.7
 *  /std/room as standard inherit file
 *  version control
 *
 * V 0.99.8
 *  Support of /std/shop.c (partly)
 *  Support of mappings and closures (even partly)
 *
 * V 0.99.9
 *  More support of /std/shop.c (articles)
 */

#include <properties.h>
#include <secure/wizlevels.h>
#include <driver/rtlimits.h>
#include <moving.h>
#include <rooms.h>
#include <string.h>
#include "/obj/tool/roommaker/sizes.h"

#define VERSION "0.99.9"

inherit "/std/thing";
inherit "/obj/tool/roommaker/stdfuns";
inherit "/obj/tool/roommaker/properties";
inherit "/obj/tool/roommaker/items";
inherit "/obj/tool/roommaker/details";
inherit "/obj/tool/roommaker/exits";
inherit "/obj/tool/roommaker/descriptions";
inherit "/obj/tool/roommaker/doors";
inherit "/obj/tool/roommaker/inherit";
inherit "/obj/tool/roommaker/articles";

nosave string  rlong2;		/* contains a string that creates the long
				 * descrip- tion string if it will be printed
				 * out */
nosave string *detail2; 	/* contains the details and the arrays of
				 * details with the same descriptions */
nosave string *desc2;		/* contains all description string creation
				 * strings */
nosave string *dlong2;          /* contains the long descriptions of the
                                 * doors */
nosave mixed  *specialprop;     /* all properties with a default != 0 */
nosave string autoload;         /* Autoload informations */

string SetAutoObject(mixed prop)
{
  if(this_interactive() &&
     blueprint(this_interactive()) == "/secure/login" &&
     prop != VERSION)
    tell_object(this_player(),"New version of RoomMaker: " + VERSION + ".\n");
  return autoload;
}

string QueryAutoObject() { return autoload; }

string QueryMenuString()
{
  return "You can change your file now. Possibilities are:\n\
sh[ort] ...................... change the short description of the room\n\
l[ong] ....................... change the long description of the room\n\
a[dd] ex[it] ................. add exits to your room\n\
a[dd] de[tail] ............... add details to your room\n\
a[dd] it[em] ................. add items to your room\n\
a[dd] do[or] ................. add doors to your room\n" +
  (shopflag?"a[dd] ar[ticle] .............. add articles to your shop\n":"") +
  "d[elete] ex[it] [<exit>] ..... delete an exit (the exit named <exit>)\n\
d[elete] de[tail] [<detail>] . delete a detail (the detail named <detail>)\n\
d[elete] it[em] [<item>] ..... delete an item (the item named <item>)\n\
d[elete] do[or] [<door>] ..... delete a door (the door in directon <door>)\n" +
  (shopflag?
   "d[elete] ar[ticle] [<article>] delete an article (the article <article>\n":
   "") +
  "s[et] [<property>] ........... set any property (the property <property>)\n\
c[omment] .................... comment the file\n\
h[ints] ...................... gives you some hints how to use the roommaker\n\
f[ilename] ................... change the current filename\n\
i[nherit] [<file>] ........... change the inherited file\n\
~h ...... shows this text            ~q ...... aborts the execution\n\
~u ...... undo the last action       ~f ...... shows the roomfile\n\
~s ...... saves the object in the    ~!<cmd>.. escape and command <cmd>\n\
          current form               ~! ...... interrupts. restart with \"~m\"\n\
~g ...... go into the new room\n]";
}

void create()
{
  (::create());
  Set(P_AUTOOBJECT, VERSION);
  Set(P_IDS, ({"maker", "roommaker"}));
  SetShort("a roommaker");
  SetLong("This is a roommaker. It tries to help you to create new rooms.\n" +
	  "You start the maker with a simple \"make <filename>\". It will lead you\n" +
	  "by making the room.\n");
  SetHelpMsg(QueryLong());
  comm = ([]);
  edittext = ({});
  written = 0;
  Set("making", 0);
  refresh = ({"REFRESH_NONE", "REFRESH_DESTRUCT", "REFRESH_REMOVE", "REFRESH_ALWAYS"});
  statlist = ({"DOOR_STATUS_OPEN","DOOR_STATUS_CLOSED","DOOR_STATUS_LOCKED"});
  hardcoded = ({"P_INT_LONG","P_INT_SHORT","P_DETAILS","P_DOORS","P_EXITS",
		"P_ITEMS","P_EXITS_DEST"});
  specialprop = order_alist(({({"P_INDOORS","P_SUNBRIGHT","P_INT_LIGHT",
                                "P_LIGHT","P_OUTDOORS","P_SUNLIGHT"}),
			      ({"indoors","sunbright","light","light",
				"outdoors","sunlight"})}));
  builtin = ({});
  propies = ({({}),({})});
  create_prop_list((hardcoded + ({"P_PROPERTIES"})));
  SetNoGet(1);
  SetNoDrop(1);
  SetNoGive(1);
}

void init()
{
  ::init();
  add_action("make", "make");
  add_action("restart", "~m");
}

/* Returns 1 if the security check is succesful, 0 otherwise */

static int security()
{
  if (this_player() != this_interactive()) {	/* Unforced make command? */
    notify_fail("Attention! " + this_interactive()->QueryName()
		+ " tried to abuse your roommaker!\n");
    tell_object(this_interactive(),
		"Please don't try to force anybody to \"make\" something!\n");
    return 0;
  }
  if (!IS_IMMORTAL(this_player())) {	/* Nonwizards can't make anything */
    notify_fail("You aren't allowed to write new rooms. Try to become a wizard soon, then you\n" +
		"will be able to write so many rooms as you like.\n");
    return 0;
  }
  return 1;
}

static void maker_reset()
{
  edittext = ({});
  rshort = 0;
  rlong = 0;
  exit = ({});
  path = ({});
  detail = ({});
  desc = ({({})});
  detail2 = ({});
  desc2 = ({});
  item = ({({}),({})});
  article = ({({}),({})});
  kommentar = ({});
  door = ({});
  dpath = ({});
  dshort = ({});
  dlong = ({});
  dkey = ({});
  dstatus = ({});
  dlong2 = ({});
  propies = ({({}),({})});
  written = 0;
  inheritfile = "/std/room";
}

/* Lets check the security, resets all variables and start the rommaker */

int make(string str)
{
  if (!security())
    return 0;
  if (!str) {			/* a filename was given? */
    notify_fail("What will you make?\n");
    return 0;
  }
  /* RESET of all variables */

  maker_reset();
  rshort = "<nameless room>";
  rlong = ({"<You stand in a nameless room.>"});

  /* Start to make anything */

  say(this_player()->QueryName()
      + " starts to write a file. Please don't disturb!\n");
  if(str[<2..<1] != ".c") str += ".c";
  Filename = (string) this_player()->get_path(str);
  write("We now make the file named: " + Filename + "\n");
  write("Is that correct? (y/n) ");
  input_to("get_correct");
  return 1;
}

/* Gets a path and shorted them after the specifikation of /std/room */

static string shorted(string longpath, string *currentdir)
{
  string *patharr;

  patharr = (explode(longpath,"/")-({""}));
  if(implode(patharr[0..sizeof(currentdir)-1],"/") ==
     implode(currentdir,"/"))
    return "./" + implode(patharr[sizeof(currentdir)..],"/");
  if(patharr[0] == "w" || patharr[0] == "players")
    return "~" + implode(patharr[1..],"/");
  if(patharr[0] == "d")
    return "+" + implode(patharr[1..],"/");
  return longpath;
}

/* These function reads all important variables from the given object */

static void dump(object room)
{
  mixed  propy;
  int i;
  string *currentdir;
  string *patharr;
  string rweight,rweightcontents;
  mixed dummy;
  mapping map;

  write("Reading information of old " + Filename + "...\n");
  map = (mapping)room->QueryExits();
  if(sizeof(map)) {
    exit = m_indices(map);
    path = ({});
    currentdir = (explode(Filename,"/")-({""}))[0..<2];
    for(i = 0; i < sizeof(exit); i++) {
      mixed dest;
      dest = map[exit[i], EXIT_DEST];
      if (stringp(dest))
        path += ({shorted(dest,currentdir)});
      else
        exit[i] = 0;
    }
    exit -= ({ 0 });
  }
  propy = (mixed *)room->QueryItems();
  item = ({ ({}), ({}) });
  for (i = sizeof(propy); i--; ) {
    item[0] += ({ propy[i][RITEM_FILE] });
    item[1] += ({ propy[i][RITEM_REFRESH] });
  }
  map = (mapping)room->QueryDoors();
  if(i = sizeof(map)) {
    if(!sizeof(currentdir))
      currentdir = explode(Filename,"/")[0..<2];
    for(; i--;) {
      object door;
      door = m_indices(map)[i];
      dpath = ({shorted(object_name(door),currentdir)}) + dpath;
      dshort = ({door->QueryShort()}) + dshort;
      dlong = ({door->QueryLong(),"\n"}) + dlong;
      dkey = ({door->QueryKeyIds()[0]}) + dkey;
      dstatus = ({door->QueryLockState()}) + dstatus;
    }
  }
  if(dummy = find_object(inheritfile)) dummy->remove();
  if(dummy) destruct(dummy);
  inheritfile = "/" + inherit_list(room)[1][0..<3];
  if(inheritfile == "/obj/room") inheritfile = "/std/room";
  if(member(inherit_list(room),"std/shop.c") == -1 ||
     function_exists("QueryIntLong",room) != "/std/shop") {
    rshort = (string)room->QueryIntShort();
    if((propy = room->QueryIntLong()) && stringp(propy)) {
      rlong = explode(propy,"\n");
      if(rlong[<1] == "") rlong == rlong[0..<2];
    }
  } else {
    shopflag = 1;
    rshort = (string)room->QueryShopName();
    if((propy = room->QueryAddDesc()) && stringp(propy)) {
      rlong = explode(propy,"\n");
      if(rlong[<1] == "") rlong = rlong[0..<2];
    }
    propy = (mixed *)room->AddNArticle();
    article = ({ ({}), ({}) });
    for (i = sizeof(propy[0]); i--; ) {
      article[0] += ({ propy[RITEM_FILE][i]});
      article[1] += ({ propy[RITEM_REFRESH][i]});
    }
  }
  call_other(inheritfile,"create");
  dump_object(room,find_object(inheritfile));
  map = (mapping)room->QueryDetails();
  filter(m_indices(find_object(inheritfile)->QueryDetails()||([])),
               lambda(({'a,'b}),({#'m_delete,'b,'a})),&map);
  if(i = sizeof(map)) {
    desc = ({});
    detail = m_indices(map);
    for(; i--;)
      if(closurep(map[detail[i]]))
	desc += ({implode(explode(sprintf("%O",map[detail[i]]),
				  Filename[1..<3] + "->"),
			  "")});

      else
	desc += ({explode(map[detail[i]]||"","\n")[0..<2]});
  }
  if(room->QueryIndoors()) propies = insert_alist("P_INDOORS",1,propies);
  propy = explode((string)room->QueryLong()||"","\n");
  if((i = member(propy,"It contains:")) != -1) {
    if(i)
      propies = insert_alist("P_LONG",
        mixedtostring(implode(propy[0..i-1],"\n")),propies);
    else
      propies = remove_alist("P_LONG",propies);
  }
  if(rweight = assoc("P_WEIGHT",propies) &&
     rweightcontents = assoc("P_WEIGHT_CONTENT",propies)) {
    if(i = to_int(rweight[1..<2])  - to_int(rweightcontents[1..<2]))
      propies = insert_alist("P_WEIGHT",i,propies);
    else
      propies = remove_alist("P_WEIGHT",propies);
  }
  propies = remove_alist("P_WEIGHT_CONTENT",propies);
  propies = remove_alist("P_SUNBRIGHT",propies);
}

/* Gets the answer if the path was right, check them and starts the input loop
 */

static void get_correct(string str)
{
  int i;
  object room;
  object *inv;
  object *inv2;
  int size;

  if(str == "n") {
    write("Enter the right filename: ");
    input_to("getrightname");
    return;
  }
  if(str != "y") {
    write("Is that correct?? (y/n) ");
    input_to("get_correct");
    return;
  }
  if(size = file_size(Filename) == -2) {
    write(Filename + " is a directory. Please choose another filename!\n" +
           "New filename: ");
    input_to("getrightname");
    return;
  }
  if(size == -1)
    return getshort();
  if(!(room = find_object(Filename[0..<3]))) {
    catch(call_other(Filename[0..<3],"???"));
    if(!(room = find_object(Filename[0..<3])))
      return getshort();
    if(!(inheritfile = build_from_instance(room))) {
      write(Filename +
            " seems to be no roomfile. Please choose another filename!\n" +
           "New filename: ");
      input_to("getrightname");
      return;
    }
    dump(room);
    return gotomenu();
  }
  if(!(inheritfile = build_from_instance(room))) {
    write(Filename +
      " seems to be no roomfile. Please enter the right filename!\n" +
      "New filename: ");
    input_to("getrightname");
    return;
  }
  inv = all_inventory(room);
  for(size = 0; size < sizeof(inv); size++)
    inv[size]->move("/std/void",M_SILENT);
  room->remove();
  if(room) destruct(room);
  catch(call_other(Filename[0..<3],"???"));
  room = find_object(Filename[0..<3]);
  if(!room)
    return getshort();
  inv2 = all_inventory(room);
  for(size = 0; size < sizeof(inv2); size++) {
    inv2[size]->remove();
    if(inv2[size]) destruct(inv2[size]);
  }
  for(size = 0; size < sizeof(inv); size++)
    inv[size]->move(room,M_SILENT);
  dump(room);
  return gotomenu();
}

int restart(string str)
{
  if (!security())
    return 0;
  if (str)
    return 0;
  if (!Query("making")) {
    notify_fail("You can't \"restart\" something that you didn't start!\n");
    return 0;
  }
  Set("making", 0);
  write("Restart to make " + Filename + " ...\n");
  say(this_player()->QueryName() + " continues to write a file.\n");
  call_other(this_object(), (string) QueryLabel("nextfun"));
  return 1;
}

varargs int remove()
{
  return thing::remove();
}

/*
 * Converts all datas to a printable form: specially it searches all details
 * whit the same description and makes then a string like
 * "({\"<detail1>\",\"<detail2>\",\"<detail3>\",...})" converts all string
 * arrays to strings
 */

static void convert()
{
  int             i, j;
  string          dummy;
  detail2 = ({});
  desc2 = ({});
  dlong2 = ({});
  for (i = 0; i < sizeof(detail); i++) {
    dummy = arrayconvert(desc[i]);
    j = member(desc2,dummy);
    if (j == -1) {
      detail2 += ({"\"" + detail[i] + "\""});
      desc2 += ({dummy});
    } else {
      if (detail2[j][0] != 40)
	detail2[j] = "({" + detail2[j] + ",\"" + detail[i] + "\"})";
      else
	detail2[j] = detail2[j][0..<3] + ",\"" + detail[i] + "\"})";
    }
  }
  rlong2 = arrayconvert(rlong);
  for (i = 0; i < sizeof(dlong); i++)
    dlong2 += ({arrayconvert(dlong[i])});
}

/* Creates the filestring that will be printed to Filename */

static string file_print()
{
  int    i;
  string out;
  string fun;

  convert();
  out = comment(kommentar);	/* First we write a comment */
  if (out == "")
    out += "/* made by " + capitalize(getuid(this_player())) +
      " using RoomMaker V" + VERSION + " (c) Sique 2/1993 */\n";
  else
    out += " * made by " + capitalize(getuid(this_player())) +
      " using RoomMaker V" + VERSION + " (c) Sique 2/1993\n */\n";
  out += "\n";
  if(sizeof(propies[0] - builtin) > 1)
    out += "#include <properties.h>\n";
  if(shopflag || sizeof(item[0]))
    out += "#include <rooms.h>\n";
  if(sizeof(door))
    out += "#include <doors.h>\n";
  out += "inherit \"" + inheritfile + "\";\n";
  out += "\n";
  if(inheritfile == "/obj/room") out += "config() {\n";
  else out += "create()\n{\n  ::create();\n";
  out += rshort?(shopflag?"  SetShopName(\"":"  SetIntShort(\"") +
    lit_string(rshort) + "\");\n":
    shopflag?"":"  SetIntShort(\"<nameless room>\");\n";
  if (rlong2) {
    if (sizeof(rlong2) <= 64)
      out += (shopflag?"  SetAddDesc(":"  SetIntLong(") + rlong2[0..<2];
    else
      out += (shopflag?"  SetAddDesc(\n":"  SetIntLong(\n") + rlong2 + "    ";
    out += ");\n";
  } else
    out += shopflag?"":
      "  SetIntLong(\"<You are in a nameless room.>\\n\"));\n";
  for (i = 0; i < sizeof(detail2); i++) {
    out += "  AddDetail(" + detail2[i] + ",";
    if ((sizeof(detail2[i]) + sizeof(desc2[i])) <= 64)
      if(desc2[i][0] == '\"')
	out += desc2[i][0..<2];
      else
	out += desc2[i];
    else
      out += "\n" + desc2[i] + "    ";
    out += ");\n";
  }
  if(shopflag)
    for (i = 0; i < sizeof(article[0]); i++)
      out += "  AddArticle(\"" + article[0][i] + "\"," +
	refresh[article[1][i]] + ");\n";
  for (i = 0; i < sizeof(exit); i++)
    out += "  AddExit(\"" + exit[i] + "\",\"" + path[i] + "\");\n";
  for (i = 0; i < sizeof(item[0]); i++)
    out += "  AddItem(\"" + item[0][i] + "\"," + refresh[item[1][i]] + ");\n";
  for (i = 0; i < sizeof(door); i++) {
    out += "  AddDoor(\"" + door[i] + "\",\"" + dpath[i] + "\",\"" +
           dshort[i] + "\",\n";
    out += dlong2[i][0..<2] + ",\n    ";
    if(dkey[i] == "0") out += dkey[i];
    else out += "\"" + dkey[i] + "\"";
    out += "," + statlist[dstatus[i]] + ");\n";
  }
  for(i = 0; i < sizeof(specialprop[0]); i++)
    out += ((fun = call_other(this_object(),specialprop[1][i]))?fun:"");
  for(i = 0; i < sizeof(propies[0]); i++) {
    if(member(hardcoded, propies[0][i]) != -1) continue;
    if(assoc(propies[0][i],specialprop)) continue;
    if(member(builtin,propies[0][i]) != -1) {
      if(propies[1][i] && propies[1][i] != "0")
	out += "  Set" + assoc(propies[0][i],proplist) + "(" + propies[1][i] +
	  ");\n";
      continue;
    }
    out += "  Set(" + propies[0][i] + "," + propies[1][i] + ");\n";
  }
  out += "}\n";
  return out;
}

/* Some special functions for to handle special properties */

static string indoors()
{
  mixed indoors;
  if((indoors = assoc("P_INDOORS",propies)) && indoors != "0")
    return "";
  return "  SetIndoors(0);\n";
}

static void hints()
{
  smore(read_file("/obj/tool/roommaker/hints"), 0,
	this_object(),"gotomenu");
}

static void gotomenu()
{
  write(QueryMenuString());
  input_to("menu");
}

static void menu(string str)
{
  int i;
  string *strlist;

  if (str[0..1] == "~!") {
    if (str[2..<1] == "") {
      SetLabel("nextfun", "gotomenu");
      write("You interrupt to make " + Filename + "\nRestart with \"~m\".\n");
      say(this_player()->QueryName() + " interupts to write a file.\n");
      Set("making", 1);
      return;
    }
    this_player()->command_me(str[2..<1]);
    write("]");
    input_to("menu");
    return;
  }
  if(str[0..1] == "s " || str[0..3] == "set ") {
    if((i = sizeof(strlist = explode(str," "))) != 1) {
      if(i == 2) return insert_property(strlist[1]);
      SetLabel("prop",convert_prop(strlist[1]));
      SetLabel("nextfun","gotomenu");
      return insert_prop_value(implode(strlist[2..]," "));
    }
    str = "s";
  }
  if(str[0..1] == "i " || str[0..7] == "inherit ")
    return store_inherit((explode(str," ") - ({""}))[1]);
  switch (str) {
  case "~q":
    return abort();
  case "~h":
    return gotomenu();
  case "~u":
    input_to("menu");
    return;
  case "~f":
    write(file_print() + "]");
    input_to("menu");
    return;
  case "~s":
    SetLabel("nextfun", "gotomenu");
    SetLabel("nextwrite", "Continue...\n]");
    write(file_print() + "Do you want to save this file? (y/n) ");
    input_to("filewrite");
    return;
  case "~g":
    SetLabel("nextfun", "gotoroom");
    return filewrite("y");
  case "a ex":
  case "add ex":
  case "a exit":
  case "add exit":
    return enter_exit();

  case "a de":
  case "add de":
  case "a detail":
  case "add detail":
    return enter_detail();

  case "a it":
  case "add it":
  case "a item":
  case "add item":
    return enter_item();

  case "a do":
  case "add do":
  case "a door":
  case "add door":
    return enter_door();

  case "a ar":
  case "add ar":
  case "a article":
  case "add article": {
    if(shopflag) return enter_article();
    write("Exuse, but this is no shop.\n]");
    input_to("menu");
    return;
  }

  case "c":
  case "comment":
    return insert_comment();

  case "sh":
  case "short":
    return getshort2();

  case "l":
  case "long":
    return getlong();

  case "i":
  case "inherit":
    return get_inherit();

  case "f":
   case "filename":
    return getfilename();

  case "s":
  case "set":
    return getproperty();

  case "d ex":
  case "delete ex":
  case "d exit":
  case "delete exit":
    return goto_delete_exit();

  case "d it":
  case "delete it":
  case "d item":
  case "delete item":
    return goto_delete_item();

  case "d de":
  case "delete de":
  case "d detail":
  case "delete detail":
    return goto_delete_detail();

  case "d do":
  case "delete do":
  case "d door":
  case "delete door":
    return goto_delete_door();

  case "d ar":
  case "delete ar":
  case "d article":
  case "delete article": {
    if(shopflag) return goto_delete_article();
    write("Exuse, but this is no shop.\n]");
    input_to("menu");
    return;
  }

  case "h":
  case "hints":
    return hints();

  default:
    write("Excuse, but I don't understand. Please enter again:\n]");
    input_to("menu");
    /* end of switch */
  }
}

static void gotoroom()
{
  object          room;
  room = find_object(Filename[0..<3]);
  if (room)
    room->remove();
  if (room)
    destruct(room);
  this_player()->command_me("goto " + Filename[0..<3]);
  write("]");
  input_to("menu");
}
