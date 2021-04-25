/* This file contains all functions needed for to read and change the
 * properties of an object.
 *
 * Part of RoomMaker V 0.99
 *
 * (c) Sique 2/1993
 *
 * Last Update 2-9-1993
 */

#include "/obj/tool/roommaker/sizes.h"

nosave mixed *proplist;      /* contains all aviable properties */
nosave mixed *propies;       /* contains the set properties and their values */
nosave mixed *builtin;       /* contains all builtin properties */
nosave mixed *hardcoded;     /* contains all hardcoded properties */

static void gotogetstring();
static void SetLabel(string label, mixed arg);
static void get_prop_value();
static mixed QueryLabel(string label);
string lit_string(string arg);
static void menu(string str);
string map_value(mixed key, mapping value);
varargs string mkstr(int lenght, string pattern);

/* Creates an array of all supported properties */

static void create_prop_list(string *exceptions)
{
  mixed *list;
  int    i;
  object std_room;
  string *trem = ({});

  if(is_clone()) return;
  list = ({});
  builtin = ({});
  call_other("/std/room","???");
  std_room = find_object("/std/room");
  proplist = (mixed *)"/lib/getproperties"->get_properties();
  for(i = 0; i < sizeof(proplist[1]); i++) {
    if(member(exceptions,proplist[0][i]) != -1) {
      trem += ({proplist[0][i]});
    } else {
      list += ({lower_case(proplist[1][i])});
      if(function_exists("Set" + proplist[1][i],std_room))
	     builtin += ({proplist[0][i]});
    }
  }
  proplist -= trem;
  proplist += ({list});
}

/* This function converts the given variable to a string */

string mixedtostring(mixed value)
{
  if (intp(value)||floatp(value)) return ""+value;
  else if (stringp(value)) return "\""+value+"\"";
  else if (objectp(value)) return object_name(value);
  else if (closurep(value)) {
    write("Warning: Can't convert a closure to a string!\n");
    return sprintf("%O",value);
  }
  else if (pointerp(value)) {
    if (!sizeof(value)) return "({})";
    return "({" + implode(map(value,#'mixedtostring),",") + "})";
  }
  else if (mappingp(value)) {
    if (!sizeof(value)) return "([])";
    return "(["+implode(map(m_indices(value),#'map_value,value),",")
      + "])";
  }
  write(sprintf("Warning: Can't convert unknown value: %O\n",value));
  return "???";
}

string map_value (mixed key, mapping value)
{
  int i, width;
  mixed *rc;

  width = get_type_info(value,1);
  rc = ({});
  for (i=0; i < width; i++)
    rc += ({ value[key,i] });
  return mixedtostring(key) + ":" + implode(map(rc,#'mixedtostring),";");
}


mixed QueryList() { return is_clone()?blueprint()->QueryList():proplist; }

/* This funktion reads all properties from ob and stores them in an alist */

static void dump_object(object ob, object std)
{
  int i;
  mixed propy;
  mixed list;

  list = QueryList();
  propies = ({({}),({})});
  if(!objectp(ob)) return;
  for(i = 0; i<sizeof(list[1]); i++)
    if((propy = mixedtostring(ob->Query(list[1][i]))) !=
       mixedtostring(std->Query(list[1][i])))
      propies = insert_alist(list[0][i],propy,propies);
}

/* returns a string containing lenght spaces */

static string blank(int length) { return mkstr(length); }

/* In a far future this function should check, if the given string is syntac-
 * tically correct */

static int syntax(string str)
{
  return 0;
}

/* Configures the getstring */

static void getproperty()
{
  SetLabel("prompt","Prop: ");
  SetLabel("insert_string","insert_property");
  SetLabel("ready","gotomenu");
  SetLabel("undo","gotomenu");
  SetLabel("nostring","gotomenu");
  SetLabel("redisplay","prop_redisplay");
  SetLabel("save_file","normal_save_string");
  SetLabel("delete","prop_delete");
  gotogetstring();
}

static string convert_prop(string str)
{
  int i;
  mixed list;

  list = QueryList();
  if((i = assoc(str,list[0])) == -1) {
    if((i = member(list[1], str)) == -1) {
      if((i = member(list[2],str)) == -1) {
	write("No predefined property!\n");
	str = implode(explode(str,"\\"),"\\\\");
	str = implode(explode(str,"\""),"\\\"");
	str = "\"" + str + "\"";
      } else str = list[0][i];
    } else str = list[0][i];
  }
  return str;
}

static void insert_property(string str)
{
  if(!str || str == "") {
    write("Please enter the name of a property!\n");
    return gotogetstring();
  }
  if(member(hardcoded,str) != -1) {
    write("Excuse, but for this property exists a special support.\n");
    return gotogetstring();
  }
  str = convert_prop(str);
  SetLabel("prop",str);
  return get_prop_value();
}

static void get_prop_value()
{
  SetLabel("prompt","Value: ");
  SetLabel("insert_string","insert_prop_value");
  SetLabel("ready","gotomenu");
  SetLabel("undo","gotomenu");
  SetLabel("nostring","gotomenu");
  SetLabel("redisplay","prop_redisplay");
  SetLabel("save_file","normal_save_string");
  SetLabel("delete","prop_delete");
  SetLabel("nextfun","gotogetstring");
  gotogetstring();
}

static void insert_prop_value(string str)
{
  int i;

  if(!str || str == "0") {
    if(assoc(QueryLabel("prop"),propies) != -1)
       propies = remove_alist(QueryLabel("prop"),propies);
    if(QueryLabel("nextfun") == "gotomenu") {
      write("]");
      input_to("menu");
      return;
    }
    return getproperty();
  }
  if(i = syntax(str)) {
    write("Syntax Error!\n" + str + "\n" + blank(i) + "^\n");
    return gotogetstring();
  }
  propies = insert_alist(QueryLabel("prop"),str,propies);
  if(QueryLabel("nextfun") == "gotomenu") {
    write("]");
    input_to("menu");
    return;
  }
  getproperty();
}

static void prop_redisplay()
{
  int i,j;

  if(!(i = sizeof(propies[0]))) {
    write("No properties are set.\n");
    return gotogetstring();
  }
  if(i == 1) write("This property is");
  else write("These properties are");
  write(" given:\n");
  for(j = 0; j < i; j++)
    write((propies[0][j] + " ................")[0..16] + " " +
	  to_string(propies[1][j])[0..SCREEN_WIDTH-17] + "\n");
  return gotogetstring();
}

static void prop_delete()
{
  write("Please set the property to 0 to delete it!\n");
  gotogetstring();
}
