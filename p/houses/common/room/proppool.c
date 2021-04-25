/* The store where the certificates are stored */

#include <moving.h>
#include "/p/houses/common/sys/houses.h"
inherit "/std/store";

mixed *houses;
mixed *sold;
static mixed *present_players;

go_back();

create()
{
  object cert;
  int    i;

  ::create();
  seteuid(getuid());
  SetIntShort("makler's store");
  SetIntLong("You are standing in the store of the agency.\n\
Here you can find certificates for all houses the estate agent wants to sell.\n");
  if(file_size(PROPERTYFILE + ".o") != -1)
    restore_object(PROPERTYFILE);
  else {
    houses = ({});
    sold = ({});
  }
  for(i = sizeof(houses); i--; ) {
    if(houses[i][5]) cert = clone_object(houses[i][5]);
    else cert = clone_object(CERT_DUMMY);
    cert->SetHomedir(houses[i][0]);
    cert->SetExitverb(houses[i][1]);
    cert->SetExitpath(houses[i][2]);
    cert->SetValue(houses[i][3]);
    if(houses[i][4])
      cert->SetShort("property certificate for a house in " + houses[i][4]);
    else
      cert->SetShort("property certificate for a house somewhere");
    cert->move(this_object(),M_SILENT);
  }
  AddExit("shop",#'go_back);
}
  
notify_enter(object oldenv, mixed method, mixed extra)
{
  if(!interactive(previous_object())) return;
  if(!present_players)
    present_players = ({({getuid(previous_object())}),({object_name(oldenv)})});
  else
    present_players = insert_alist(getuid(previous_object()),
                                   object_name(oldenv),
                                   present_players);
}

go_back(string str)
{
  string dest;

  if(str) return 0;
  if(!interactive(this_player())) return 0;
  dest = assoc(getuid(this_player()),present_players);
  if(!dest) {
    notify_fail("You can't leave to any shop, because you didn't enter the\n\
store walking.\n");
    return 0;
  }
  this_player()->move(dest, M_GO, "into a shop");
  return 1;
}

AddProperty(string homedir, string exit, string path, int price,
            string location, string cert_file)
{
  object cert;

  if(!houses) houses = ({({homedir,exit,path,price,location,cert_file})});
  else {
    if(sizeof(filter(houses,"exist",this_object(),
			   ({homedir,exit,path}))))
      return 0;
    houses += ({({homedir,exit,path,price,location,cert_file})});
  }
  if(cert_file) cert = clone_object(cert_file);
  else cert = clone_object(CERT_DUMMY);
  if(!cert) {
    RemoveProperty(homedir, exit, path);
    return 0;
  }
  cert->SetHomedir(homedir);
  cert->SetExitverb(exit);
  cert->SetExitpath(path);
  cert->SetValue(price);
  if(location)
    cert->SetShort("property certificate for a house in " + location);
  else
    cert->SetShort("property certificate for a house somewhere");
  cert->move(this_object(),M_SILENT);
  save_object(PROPERTYFILE);
  return 1;
}

RemoveProperty(string homedir, string exit, string path)
{
  mixed *to_delete;

  if(!houses || !sizeof(houses)) return 0;
  if(!sizeof(to_delete =
	     filter(houses,"exist",this_object(),({homedir,exit,path}))))
    return 0;
  houses -= to_delete;
  save_object(PROPERTYFILE);
  return 1;
}

exist(mixed *house, string *key)
{ return house[0] == key[0] && house[1] == key[1] && house[2] == key[2]; }

notify_leave(mixed dest, mixed method, mixed extra)
{
  if(query_verb() != "buy") return;
  if(!sold) sold = ({});
  sold += ({({previous_object()->QueryHomedir(),
              previous_object()->QueryExitverb(),
              previous_object()->QueryExitpath(),
              capitalize(getuid(this_player())),
              ctime(time())})});
  RemoveProperty(previous_object()->QueryHomedir(),
		 previous_object()->QueryExitverb(),
		 previous_object()->QueryExitpath());
}

QueryAllProperties() { return houses; }

QuerySold() { return sold; }
