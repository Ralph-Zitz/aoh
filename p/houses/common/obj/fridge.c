/* A fridge to put food and drink in it */

#include <properties.h>
#include <moving.h>
inherit "/std/container";

static int     opened;
static string *props;
static object *contains;
mixed         *items;

create()
{
  (::create());
  if(member(object_name(), '#') == -1) return;
  seteuid(getuid());
  SetShort("a fridge");
  SetLong("This is a fridge. You can put food and drink in it.\n\
It will stay here until you get it back from the fridge.\n");
  AddId("fridge");
  SetWeight(250000);
  SetMaxWeight(260000);
  SetValue(1333);
  call_out("fill",0);
}

fill()
{
  int    i,j;
  object ob;

  props = "/lib/getproperties"->get_properties()[1];
  contains = ({});
  if(!items) return;
  for(i = sizeof(items[0]); i--; ) {
    ob = clone_object(items[0][i]);
    for(j = sizeof(items[1][i][0]); j--; )
      ob->Set(items[1][i][0][j],items[1][i][1][j]);
    ob->move(this_object(),M_PUT,"fill");
    contains += ({ob});
  }
}

init()
{
  (::init());
  add_action("close","close");
  add_action("open","open");
}

allow_enter(int method, mixed extra)
{
  string *inhlist;
  mixed  *proplist;
  object  ob;
  string  filename;
  mixed   prop;
  int     i;

  if(method != M_PUT) return ME_NO_ENTER;
  if(extra == "fill") return ME_OK;
  if (!this_player()) return ME_OK;
  if(!opened) {
    write("The fridge is closed!\n");
    return ME_NO_ENTER;
  }

  if(member((inhlist = inherit_list(previous_object())), "std/food.c") == -1 &&
     member(inhlist, "std/drink.c") == -1) {
    notify_fail("You can't put anything except food and drink into a fridge");
    return ME_NO_ENTER;
  }
  ob = clone_object(filename = explode(object_name(previous_object()),"#")[0]);
  proplist = ({({}),({})});
  for(i = sizeof(props); i--; ) {
    if(props[i] == P_CREATOR) continue;
    if((props[i] == P_FOOD_MSGS) &&
       member( (inhlist = inherit_list(previous_object())), "std/food.c") == -1)
      continue;
/* This passage is too slow - often a "too long evaluation" is the result.
   as a quick and dirty fix I replaced it by the "unreliable" standard '=='.
   [Loco 13Sep1996]
    if(!equal(prop = previous_object()->Query(props[i]),
	      ob->Query(props[i]))) {
*/
   if ((prop = previous_object()->Query(props[i]))!=ob->Query(props[i])) {
      proplist[0] += ({props[i]});
      proplist[1] += ({prop});
    }
  }
  if(!items) items = ({({filename}),({proplist})});
  else {
    items[0] += ({filename});
    items[1] += ({proplist});
  }
  if(!contains) contains = ({previous_object()});
  else contains += ({previous_object()});
  save_object(file());
  return ME_OK;
}

notify_leave(mixed dest, int method, mixed extra)
{
  int i;

  if(method != M_GET) return;
  if((i = member(contains, previous_object())) == -1)
    return ::notify_leave(dest,method,extra);
  if(sizeof(contains) == 1) {
    contains = ({});
    items =({({}),({})});
  } else if(!i) {
    contains = contains[1..];
    items[0] = items[0][1..];
    items[1] = items[1][1..];
  }
  else if(i == sizeof(contains)-1) {
    contains = contains[0..<2];
    items[0] = items[0][0..<2];
    items[1] = items[1][0..<2];
  } else {
    contains = contains[1..i-1] + contains[i+1..];
    items[0] = items[0][1..i-1] + items[0][i+1..];
    items[1] = items[1][1..i-1] + items[1][i+1..];
  }
  save_object(file());
  return ::notify_leave(dest,method,extra);
}

prevent_leave(mixed dest, int method, mixed extra)
{
  if(method != M_GET || opened) return 0;
  write("The fridge is closed.\n");
  return ME_NO_GET;
}

move(mixed dest, int method, mixed extra)
{
  int to_return;

  if(environment()) rm(file() + ".o");
  to_return = ::move(dest,method,extra);
  if(file_size(file() + ".o") != -1) {
    restore_object(file());
  }
  else if (environment())
    save_object(file());
  return to_return;
}

file()
{
  if(!environment()) return "fridge";
  return "/p/houses/common/save/fridge/" +
    implode(explode(object_name(environment()),"/")-({""}),".");
}

equal(mixed a, mixed b)
{
  return "/lib/string"->mixed_to_string(a) ==
    "/lib/string"->mixed_to_string(b);
}

QueryItems() { return items; }

open(string str)
{
  int to_see;

  if( !str || present(str) != this_object() ) {
    notify_fail("What do you want to open?\n");
    return 0;
  }
  if((to_see = this_player()->CantSee()) > 0) {
    write("It is too bright!\n");
    return 1;
  }
  if(to_see < 0) {
    write("It is too dark!\n");
    return 1;
  }
  if(opened) {
    notify_fail("The fridge is already opened.\n");
    return 0;
  }
  opened = 1;
  write("You opened the fridge.\n");
  say(capitalize(this_player()->QueryName()) + " opened the fridge.\n");
  return 1;
}

QueryLong()
{
  if(opened) return ::QueryLong();
  return ::QueryLong()+"The fridge is closed.\n";
}

close(string str)
{
  int to_see;

  if( !str || present(str) !=this_object()) {
    notify_fail("What do you want to close?\n");
    return 0;
  }
  if((to_see = this_player()->CantSee()) > 0) {
    write("It is too bright!\n");
    return 1;
  }
  if(to_see < 0) {
    write("It is too dark!\n");
    return 1;
  }
  if(!opened) {
    notify_fail("The fridge is already closed.\n");
    return 0;
  }
  opened = 0;
  write("You closed the fridge.\n");
  say(capitalize(this_player()->QueryName()) + " closed the fridge.\n");
  return 1;
}

deinstall() { rm(file() + ".o"); }
