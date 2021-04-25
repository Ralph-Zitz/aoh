/*--------------------------------------------------------------------------
 * /p/money/obj/designer/design.c -- part of the money designer
 *
 * Copyright (C) 1997 by One Step Beyond.
 * All Rights Reserved.
 * Permission granted for non-commercial use.
 *--------------------------------------------------------------------------
 */

#include <money.h>
#include <secure/wizlevels.h>
#include <secure/domaininfo.h>

static mapping SetCoinTypes(mixed s);
static mapping SetDomains(mixed s);

public mapping QueryMaterials();
public mapping QueryCoinTypes();
public mapping QueryDomains();
public mapping QueryFuns();
public varargs void input_text(string text, int chunk, object ob, string fun,
			       int end);

nosave string lastcoin;
nosave int    lastaction;

static string SetLastCoin(string s) { return lastcoin = s; }
public int    QueryLastAction()     { return lastaction; }

static int security()
{
  if(this_interactive() != this_player()) {
    write("Catching force... no reaction.\n");
    return 0;
  }
  if(this_player() != environment()) {
    notify_fail("You have to carry the designer.\n");
    return 0;
  }
  if(!IS_WIZARD(this_player())) {
    notify_fail("This tool is totally useless for you!\n");
    return 0;
  }
  lastaction = time();
  return 1;
}

static int is_member(string domain, string coin)
{
  return member(QueryDomains()[domain]||({}),coin) != -1;
}

static void insert_new_short(string str)
{
  if(str && str != "") {
    if(str == lastcoin[0..1]) QueryCoinTypes()[lastcoin,MONEY_SHORT] = 0;
    else QueryCoinTypes()[lastcoin,MONEY_SHORT] = str;
    write("Short now: " + 
	  (QueryCoinTypes()[lastcoin,MONEY_SHORT]||lastcoin[0..1]) +
	  "\n");
    return;
  }
  write("Short not changed.\n");
}

private void get_new_short(mixed arg)
{
  if(arg) return insert_new_short(arg);
  write("Current short: " +
	(QueryCoinTypes()[lastcoin,MONEY_SHORT]||lastcoin[0..1]) +
	"\nNew: ");
  input_to("insert_new_short");
}

static void insert_new_long(string str)
{
  if(str && str != "") {
    if(str == lastcoin[0..1]) QueryCoinTypes()[lastcoin,MONEY_LONG] = 0;
    else QueryCoinTypes()[lastcoin,MONEY_LONG] = str;
    write("Long now: " +
	  (QueryCoinTypes()[lastcoin,MONEY_LONG]||"- none -\n"));
    return;
  }
  write("Long not changed.\n");
}

private void get_new_long(mixed arg)
{
  if(arg) return insert_new_long(arg + "\n");
  write("Current long:\n" +
	(QueryCoinTypes()[lastcoin,MONEY_LONG]||lastcoin[0..1]) +
        "\nNew:\n");
  input_text("insert_new_long");
}

static void insert_new_plural(string str)
{
  if(str && str != "") {
    if(str == lastcoin) QueryCoinTypes()[lastcoin,MONEY_PLURAL] = 0;
    else if(str[0..sizeof(lastcoin)] == lastcoin + " ")
      if(str[sizeof(lastcoin)+1..] ==
	 QueryCoinTypes()[lastcoin,MONEY_COINTYPE])
	QueryCoinTypes()[lastcoin,MONEY_PLURAL] = 0;
      else  QueryCoinTypes()[lastcoin,MONEY_PLURAL] =
	(str[sizeof(lastcoin)+1..] != "")?str[sizeof(lastcoin)+1..]:0;
    else QueryCoinTypes()[lastcoin,MONEY_PLURAL] = str;
    write("Plural now: " +
	  (QueryCoinTypes()[lastcoin,MONEY_PLURAL]?
	   QueryCoinTypes()[lastcoin,MONEY_COINTYPE]?
	   lastcoin + " " +QueryCoinTypes()[lastcoin,MONEY_PLURAL]:
	   QueryCoinTypes()[lastcoin,MONEY_PLURAL]:
	   QueryCoinTypes()[lastcoin,MONEY_COINTYPE]?
	   lastcoin + " " +QueryCoinTypes()[lastcoin,MONEY_PLURAL]:
	   lastcoin) + "\n");
    return;
  }
  write("Plural not changed.\n");
}

private void get_new_plural(mixed arg)
{
  if(arg) return insert_new_plural(arg);
  write("Current plural: " +
	(QueryCoinTypes()[lastcoin,MONEY_PLURAL]?
	 QueryCoinTypes()[lastcoin,MONEY_COINTYPE]?
	 lastcoin + " " +QueryCoinTypes()[lastcoin,MONEY_PLURAL]:
	 QueryCoinTypes()[lastcoin,MONEY_PLURAL]:
	 QueryCoinTypes()[lastcoin,MONEY_COINTYPE]?
	 lastcoin + " " +QueryCoinTypes()[lastcoin,MONEY_PLURAL]:
	 lastcoin) +
	"\nNew: ");
  input_to("insert_new_plural");
}

static void insert_new_singular(string str)
{
  if(str && str != "") {
    if(str == lastcoin) QueryCoinTypes()[lastcoin,MONEY_COINTYPE] = 0;
    else if(str[0..sizeof(lastcoin)] == lastcoin + " ")
      QueryCoinTypes()[lastcoin,MONEY_COINTYPE] =
	(str[sizeof(lastcoin)+1..] != "")?str[sizeof(lastcoin)+1..]:0;
    else QueryCoinTypes()[lastcoin,MONEY_COINTYPE] = str;
    write("Singular now: " +
	  (QueryCoinTypes()[lastcoin,MONEY_COINTYPE]?
	   lastcoin + " " +QueryCoinTypes()[lastcoin,MONEY_COINTYPE]:
	   lastcoin) + "\n");
    return;
  }
  write("Singular not changed.\n");
}

private void get_new_singular(mixed arg)
{
  if(arg) return insert_new_singular(arg);
  write("Current singular: " +
	(QueryCoinTypes()[lastcoin,MONEY_COINTYPE]?
	 lastcoin + " " +QueryCoinTypes()[lastcoin,MONEY_COINTYPE]:
	 lastcoin) +
	"\nNew: ");
  input_to("insert_new_singular");
}

public int set_values(string str)
{
  mixed arg;

  if(!security()) return 0;
  if(!QueryCoinTypes()) {
    notify_fail("No database loaded yet.\n");
    return 0;
  }
  if(!lastcoin) {
    notify_fail("No coin type specified.\n");
    return 0;
  }
  if(!str) {
    notify_fail("Which value do you want to set?\n");
    return 0;
  }
  arg = explode(str," ");
  if(sizeof(arg) > 1) {
    str = arg[0];
    arg = implode(arg[1..]," ");
  } else arg = 0;
  switch(lower_case(str)) {

  case "short":
    get_new_short(arg);
    break;

  case "singular":
    get_new_singular(arg);
    break;

  case "plural":
    get_new_plural(arg);
    break;

  case "long":
    get_new_long(arg);
    break;

  case "value":
    write("You can't set the value directly. Manipulate it by changing the \
contents!\n");
    return 1;

  case "weight":
    write("You can't set the weight directly. Manipulate it by changing the \
contents!\n");
    return 1;

  case "content":
  case "contents":
    write("Use the 'add' command.\n");
    return 1;

  default: write("Don't know this setting.\n");
  }
  return 1;
}

public int add_material(string str)
{
  string mat;
  int val;

  if(!security()) return 0;
  if(!QueryCoinTypes()) {
    notify_fail("No database loaded yet.\n");
    return 0;
  }
  if(!lastcoin) {
    notify_fail("No coin type specified.\n");
    return 0;
  }
  if(!str) {
    notify_fail("Which material do you want to add?\n");
    return 0;
  }
  if(sscanf(str,"%s %d",mat,val) != 2) {
    notify_fail("** Usage: [design] " + QueryFuns()["add",1] + "\n");
    return 0;
  }
  if(member(m_indices(QueryMaterials()),mat) == -1) {
    notify_fail("\
This material has no value. Use 'show materials' to look at the possible ones.\
\n");
    return 0;
  }
  if(!val) {
    notify_fail("No change.\n");
    return 0;
  }
  QueryCoinTypes()[lastcoin,MONEY_CONTENTS][mat] += val;
  if(!QueryCoinTypes()[lastcoin,MONEY_CONTENTS][mat])
    QueryCoinTypes()[lastcoin,MONEY_CONTENTS] =
      m_delete(QueryCoinTypes()[lastcoin,MONEY_CONTENTS], mat);
  write("One " +
	(QueryCoinTypes()[lastcoin,MONEY_COINTYPE]?
	 lastcoin + " " +QueryCoinTypes()[lastcoin,MONEY_COINTYPE]:
	 lastcoin) +
	" now contains " +   QueryCoinTypes()[lastcoin,MONEY_CONTENTS][mat] +
	" grams of " + mat + ".\n");
  return 1;
}

public int new_cointype(string str)
{
  if(!security()) return 0;
  if(!QueryCoinTypes()) {
    notify_fail("No database loaded yet.\n");
    return 0;
  }
  if(!str) {
    notify_fail("** Usage: [design] " + QueryFuns()["new",1] + "\n");
    return 0;
  }
  if(QueryCoinTypes()[str]) {
    notify_fail("The " + 
		(QueryCoinTypes()[str,MONEY_COINTYPE]?
		 str + " " +QueryCoinTypes()[str,MONEY_COINTYPE]:
		 str) + " does already exist!\n");
    return 0;
  }
  QueryCoinTypes()[str,MONEY_CONTENTS] = ([]);
  QueryCoinTypes()[str,MONEY_LONG] = "A " +
    (QueryCoinTypes()[str,MONEY_COINTYPE]?
     str + " " +QueryCoinTypes()[str,MONEY_COINTYPE]:
     str) + ".\n";
  write("Adding new cointype: " + str + ".\n");
  lastcoin = str;
  return 1;
}

private void remove_coin_from_domain(string domain, string *coins, string coin)
{
  QueryDomains()[domain] = coins - ({coin});
}

public int delete_cointype(string str)
{
  mapping tmp;

  if(!security()) return 0;
  if(!QueryCoinTypes()) {
    notify_fail("No database loaded yet.\n");
    return 0;
  }
  if(!str) {
    notify_fail("** Usage: [design] " + QueryFuns()["delete",1] + "\n");
    return 0;
  }
  if(!QueryCoinTypes()[str]) {
    notify_fail("The type " + str + " doesn't exist!\n");
    return 0;
  }
  write("Deleting " +
	(QueryCoinTypes()[str,MONEY_COINTYPE]?
	 str + " " +QueryCoinTypes()[str,MONEY_COINTYPE]:
	 str) + ".\n");
  SetCoinTypes(m_delete(QueryCoinTypes(),str));
  walk_mapping(QueryDomains(),#'remove_coin_from_domain,str);
  return 1;
}

public int domain_set_money(string str)
{
  int sub;

  if(!security()) return 0;
  if(!QueryCoinTypes()) {
    notify_fail("No database loaded yet.\n");
    return 0;
  }
  if(!lastcoin) {
    notify_fail("No coin type specified.\n");
    return 0;
  }
  if(!str) {
    write("The coin is available in: " +
	  implode(filter(DOMAININFO->QueryAllDomains(),
                               #'is_member,lastcoin),", ") + "\n");
    return 1;
  }
  if(str[0] == '-') sub = 1, str = str[1..];
  else if(str[0] == '+') str = str[1..];
  if(member(DOMAININFO->QueryAllDomains(),str) == -1) {
    notify_fail("No domain \"" + str + "\" existing.\n");
    return 0;
  }
  if(!IS_WIZARD(this_player()) &&
     DOMAININFO->QueryWizard(getuid(this_player()),str) != D_LORD) {
    notify_fail("You don't have the permission for this domain.\n\
Ask the Lord!\n");
    return 0;
  }
  if(!QueryDomains())
    SetDomains(([str:sub?0:({lastcoin})]));
  else
    if(!QueryDomains()[str])
      QueryDomains()[str] = sub?0:({lastcoin});
    else {
      QueryDomains()[str] -= ({lastcoin});
      if(!sub) QueryDomains()[str] += ({lastcoin});
    }
  write("The " + (QueryCoinTypes()[lastcoin,MONEY_COINTYPE]?
		  lastcoin + " " +QueryCoinTypes()[lastcoin,MONEY_COINTYPE]:
		  lastcoin) +
	" is " + (sub?"no longer":"now") +
	" available in " + capitalize(str) + ".\n");
  return 1;
}
