/* This file was thought to sell the houses */

#include <moving.h>
#include <money.h>
#include "/p/houses/common/sys/houses.h"
#include "/p/houses/common/sys/prices.h"

#define FAIL() seteuid(0); return 0

mixed tmp;

create() { seteuid(0); }

static clean_room(object ob, object dest)
{
  if(ob->QueryCreator() == object_name(environment(ob))) return 0;
  if(living(ob)) return ob->move(dest,M_GO,tmp);
  return ob->move(dest,M_SILENT);
}

static remove_file(string filename, string dir)
{
  object tmpob;

  if(tmpob = find_object((dir + filename)[0..<3])) {
    filter(all_inventory(tmpob),#'clean_room,
                 environment(this_interactive()));
    tmpob->remove();
    if(tmpob) destruct(tmpob);
  }
  return rm(dir + filename);
}

sell()
{
  object dest;
  mixed  exits;
  int    price;
  string file, dir, *inhlist;

/*
   Experimental fix. No guarantee for nothing. May crash anything.
   [Loco&Suamor 190994]
  if(!geteuid()) return 0;
*/

  seteuid(getuid());
  if(!geteuid()) return 0;

  if(!this_interactive()) { FAIL(); }
  if(previous_object() != environment(this_interactive())) { FAIL(); }
  if(member(inhlist = inherit_list(previous_object()), "p/houses/common/room/home.c") == -1 &&
     member(inhlist, "p/houses/common/room/buildplace.c") == -1)
    { FAIL(); }
/* Commented out because otherwise the Domainmembers can't sell the house too.
  if((dir = explode(object_name(previous_object()),"/"))[<2] !=
     getuid(this_interactive()))
    { FAIL(); }
*/
  dir = explode(object_name(previous_object()),"/");
  dir = ({""}) + (dir[0..<2]-({""}));
  file = implode(dir,"/");
  exits = previous_object()->QueryExits()||([]);
  tmp = m_indices(exits)[member(m_values(exits), file + "/garden1")];
// changed to work with exit as mapping [Loco]
  filter(all_inventory(previous_object()),#'clean_room,
               load_object(file + "/garden1"));
  filter(all_inventory(environment(this_interactive())),#'clean_room,
load_object((environment(this_interactive())->QueryExits()||([]))[tmp]));
  price = 10000 * (sizeof(get_dir(file + "/*")-({".",".."}))-4) - 10;
  filter(get_dir(file + "/*")-({".",".."}),#'remove_file,file + "/");
  rmdir(file);
  environment(this_interactive())->DeleteHome(getuid(this_interactive()));
  file = implode(dir[0..<2],"/");
  POOLFILE->AddProperty(file,tmp,
                        object_name(environment(this_interactive())),
                        get_price(file,price),get_location(file),
                        get_cert_dummy(file));
   BANKOBJECT->AddAccount(getuid(this_interactive()),price/3);
  write("You got " + price/3 +
        " copper coins delivered at you bank account.\n");
  seteuid(0);
  return 1;
}

get_location(string str)
{
  switch(str) {
  case "/d/avalon/common/village/houses": return "Camelot Village, England";
  case "/d/avalon/common/viking": return "Vikings Village, England";
  case "/d/archwiz/common/houses": return "Nightfall City";
  case "/d/ireland/common/houses": return "Limerick, Ireland";
  case "/d/daydream/common/houses": return "Bing Tiddle Tiddle Bong City";
  case "/d/woodland/common/mandukam/houses": return "Mandukam";
  default: return capitalize((explode(str,"/")+({"","",""}))[2]);
  }
}

get_cert_dummy(string str)
{
  if(get_location(str) == "Mandukam")
    return "/d/woodland/obj/mandukam/certdummy";
  switch((explode(str,"/")+({"",""}))[2]) {
  case "avalon": return "/d/avalon/common/std/cert_dummy";
  case "archwiz": return "/d/archwiz/common/obj/cert_dummy";
  case "ireland": return "/d/ireland/common/street/cert_file";
  case "daydream": return "/d/daydream/xilef/obj/first_cert";
  default: return "/players/sique/obj/cert_dummy";
  }
}

get_price(string file, int price)
{
  string x, y;

  if(sscanf(file,"%s/archwiz%s",x,y) == 2) return 3*price/2;
  return price;
}
