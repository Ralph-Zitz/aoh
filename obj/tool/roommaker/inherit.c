/* This file handles the inheritance
 *
 * Part of RoomMaker V 0.991
 *
 * Author: Sique 2-15-1993
 */
#include "/obj/tool/roommaker/roommaker.h"

nosave string inheritfile;
nosave int shopflag;

string build_from_instance(object room)
{
  int i;
  string *inhlist;

  if(!room) return 0;
  inhlist = inherit_list(room);
  return inheritfile = ("/" + inhlist[0][0..<3] == object_name(room)?
			"/" + inhlist[1][0..<3]:
			"/" + inhlist[0][0..<3]);
}

static void get_inherit()
{
  SetLabel("prompt","]");
  SetLabel("nostring","gotomenu");
  SetLabel("abort","abortx");
  SetLabel("undo","gotomenu");
  SetLabel("ready","gotomenu");
  SetLabel("redisplay","inherit_redisplay");
  SetLabel("delete","delete_inherit");
  SetLabel("save_file","normal_save_string");
  SetLabel("insert_string","store_inherit");
  write(
    "----- Actual inherited file: --------------------------------------------------\n" +
    inheritfile + "\n" +
    "-------------------------------------------------------------------------------\n" +
    "Please edit the inherited file of the room (help with \"~h\")!\n");
  return gotogetstring();
}

/* Shows the inherited file and returns to the getstring loop */

static void inherit_redisplay()
{
  write("Inherited file is: " + inheritfile + "\n");
  return gotogetstring();
}

/* Writes any delete message and returns to getstring */

static void delete_inherit()
{
  write("Inherited file deleted. Please enter a new file to inherit!\n");
  gotogetstring();
}

/* Store the new inheritfile after expanding it through TP->get_path() */

static void store_inherit(string str)
{
  if(str[<2..<1] == ".c") str = str[0..<3];
  str = (string)this_player()->get_path(str);
  if(str == inheritfile) {
    write("Inherited file hasn't changed.\n");
    write("]");
    input_to("menu");
    return;
  }
  inheritfile = str;
  write("Inherited file is now: " + inheritfile + "\n");
  shopflag = (inheritfile == "/std/shop");
  this_object()->gotomenu();
}
