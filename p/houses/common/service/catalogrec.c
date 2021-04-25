/* The catalogue with all items the player can buy for their houses */

#include <moving.h>

#define SAVEFILE     "/p/houses/common/save/catalogue"
#define CATALOGSTORE (load_object("/p/houses/common/service/catstore"))

mixed *items;

create() { seteuid(getuid()); }

here(object ob, string path)
{ return explode(object_name(ob),"#")[0] == path; }

QueryReadMsg()
{
  string  msg;
  int     i,j;
  object  ob;
  object *pres;

  restore_object(SAVEFILE);
  if(!items || !(j = sizeof(items)))
    return "Sorry, but we haven't any item to sell.\n";
  msg = "We have the following items to sell (Prices in copper coins):\n\n\
<nr> <price>  <item>\n";
  for(i = 0; i < j; ++i) {
    if(!sizeof(pres = filter(all_inventory(CATALOGSTORE),"here",
				   this_object(),items[i]))) {
      ob = clone_object(items[i]);
      ob->move(CATALOGSTORE,M_SILENT);
    } else ob = pres[0];
    msg += ("    " + (i+1) + ". ")[<5..] +
           ("                 " + (ob->QueryValue()*3) + "   ")[<9..] +
           ob->QueryShort() + "\n"; 
  }
  return msg + "\nPlease 'order' something or 'inform' yourself about it!\n";
}

AddItem(string path)
{
  if(file_size(path + ".c") == -1 || catch(call_other(path,"???"))) return 0;
  if(!items) restore_object(SAVEFILE);
  if(!items) items = ({path});
  else {
    if(member(items, path) != -1) return 0;
    items += ({path});
  }
  save_object(SAVEFILE);
  return 1;
}

get_ob(string str)
{
  int    i;
  object ob;

  if(!items) restore_object(SAVEFILE);
  if(ob = present(str,CATALOGSTORE)) return ob;
  if(to_string(i = to_int(str)) != str) {
    notify_fail("Sorry, but currently we don't sell any " + str +
		".\nBut please return again, we hope we can sell you a " +
		str + " then.\n");
    return 0;
  }
  if(!i) {
    notify_fail("Excuse, but we don't trade with 'nothing'!\n");
    return 0;
  }
  if(i < 0) {
    notify_fail("Excuse, but the negative numbers are sold out.\n");
    return 0;
  }
  if(i > sizeof(items)) {
    notify_fail("Excuse, we have planned this item, but it isn't aviable yet.\n");
    return 0;
  }  
  return clone_object(items[i-1]);
}

QueryItems() { return items; }
