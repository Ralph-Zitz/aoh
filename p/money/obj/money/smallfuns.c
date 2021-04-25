/*--------------------------------------------------------------------------
 * /p/daemons/money/obj/money/smallfuns.c -- part of the money object
 *
 * Copyright (C) 1997 by One Step Beyond.
 * All Rights Reserved.
 * Permission granted for non-commercial use.
 *--------------------------------------------------------------------------
 * This file contains some small functions which are used in implicite
 * loops in the money object.
 */

#include <moving.h>
#include <money.h>

varargs int AdjustMoney(mixed amnt, string type);
varargs int move(mixed dest, int method, mixed extra);
int QueryValue();

nosave object remaining;
nosave object move_to;

static object SetRemaining(object ob) { return remaining = ob; }
public object QueryRemaining() { return remaining; }

public object SetMoveTo(object ob) { return move_to = ob; }
public object QueryMoveTo() { return move_to; }

static void subtract_value(string type, int amount, mixed other)
{
  if(other[0][type] < amount) other[1] = 1;
  else other[0][type] -= amount;
}

/* This function is used by money_rearrange() */

static void insert_other_money(object mon)
{
  mixed tmp;

  if(!mon) return;
  if(tmp = ({mixed})mon->QueryRemaining())
    insert_other_money(tmp);
  if(mon) tmp = ({mixed})mon->QueryMoney();
  if(tmp && (!({int})mon->remove() || mon)) destruct(mon);
  AdjustMoney(tmp);
}

/* This is used to do a move with some time delation and to rearrange the
 * temporarily used moneyobjects */

public varargs void money_rearrange(int flag)
{
  object *stuff, *prev;
  int i;

  remove_call_out("money_rearrange");

  if(!QueryMoveTo()) {
    if(QueryRemaining()) {
      if(!environment()) {
	log_file(MONEY_ERRORLOG,
		 sprintf("--\n%s: %O destructed without environment.\n",
			 ctime(time()),QueryRemaining()));
	QueryRemaining()->remove();
	if(QueryRemaining()) destruct(QueryRemaining());
	return;
      }
      QueryRemaining()->move(environment(),M_SILENT);
    }
  } else {
    if(!environment()) move(QueryMoveTo(),M_SILENT);
    if(QueryRemaining()) {
      if(({mixed})QueryRemaining()->QueryMoney() &&
	 sizeof(({mixed})QueryRemaining()->QueryMoney())) {
	QueryRemaining()->move(QueryMoveTo(),M_SILENT);
      } else {
	QueryRemaining()->remove();
	if(QueryRemaining()) destruct(QueryRemaining());
      }
    }
  }
  SetMoveTo(0);
  SetRemaining(0);
  if(flag) return;
  /* If we get the flag, we don't check for further money objects */

  if(environment()) {
    prev = ({this_object()});
    while(previous_object(i)) {
      if(({object})previous_object(i)->QueryMoneyObject())
       prev += ({previous_object(i)});
      ++i;
    }
    stuff = filter(all_inventory(environment()),
                         #'call_other /*'*/,"QueryMoneyObject") - prev;
    /* If we are the only moneyobject here, we are ready.
     * stuff now contains all objects that are named "money". */
    filter(stuff||({}),#'insert_other_money /*'*/);
  /* We insert the money of the other objects into ourself and destruct them */
  }

  return;
  /* And we are ready */
}

/* These functions are used by id() */

static void insert_value(string type, int value, mapping map)
{
  map[type] += value;
}

static void merge_mappings(mapping map1, mapping map2)
{
  walk_mapping(map1,#'insert_value /*'*/,&map2);
}

static void subtract_from_value(string type, int amount, mixed other)
{
  other[type] = amount - other[type];
}

static void subtract_from_remaining(string type, int amount)
{
  if (remaining)
    remaining->AdjustMoney2(type, -amount);
}

/* This function searches an environment where we can put the object in */

static object find_environment(object oldenv, int wei)
{
  if(!(oldenv = environment(oldenv))) return 0;
  return ({object})oldenv->MayAddWeight(wei)?oldenv:find_environment(oldenv, wei);
}

/* Compares the value of two money types */

int compare_value(string t1, string t2)
{
  return ({int})MONEY_D->QueryCoinValue(t1) < ({int})MONEY_D->QueryCoinValue(t2);
}

private void replace_object(object *obs)
{
  object ob;

  if(!pointerp(obs)) {
    printf("Error: No arguments to replace_object()!\n");
    return;
  }
  if(!objectp(obs[0])) {
    printf("No money object to replace%s!\n",
	   obs[1]?sprintf(" in environment: %O",obs[1]):"");
    return;
  }
  if(!objectp(obs[1])) {
    printf("Environment dissappeared for %O, removing it.\n",obs[0]);
    obs[0]->remove();
    if(obs[0]) destruct(obs[0]);
    return;
  }
  if(!({int})obs[0]->QueryValue()) {
    printf("Object %O with no value destructed...\n",obs[0]);
    obs[0]->remove();
    if(obs[0]) destruct(obs[0]);
    return;
  }
  printf("Replacing %O...\n",obs[0]);
  (ob = clone_object(MONEY))->move(obs[1],M_NOCHECK);
  ob->money_rearrange();
}

/* This function replaces the old money object by a new one */

private void get_new_money(object ob)
{
  object env;

  if(!(env = environment(ob))) {
    printf("-> Removing %O...\n",ob);
    ob->remove();
    if(ob) destruct(ob);
  } else call_out(#'replace_object /*'*/,0,({ob,env}));
}

/* This function tries to find the object for the given filename and
 * removes it. */

private void remove_object(string file)
{
  object ob;

  file = "/" + file[0..<3];

  if(ob = find_object(file)) {
    printf("Removing %O...\n",ob);
    ob->remove();
    if(ob) destruct(ob);
  }
}

private void overwriting_ourself()
{
  object ob;

  if(find_call_out(#'replace_object /*'*/) != -1)
    return call_out(#'overwriting_ourself /*'*/,1);
  printf("Overwriting ourself (%O)...\n",this_object());
  (ob = clone_object(MONEY))->move(environment(),M_NOCHECK);
  ob->money_rearrange();
}

/* This function replaces all instances of the money with new ones */

static int update_money()
{
  write("Updating all money objects...\n");

  filter(inherit_list(this_object()),#'remove_object /*'*/);
  /* We destruct the blueprint and all inherited files */

  printf("Blueprint updated, replacing the money objects...\n");

  filter(find_clones(MONEY)-({this_object()}),#'get_new_money /*'*/);
  /* We destruct all other clones we find except ourself */

  call_out(#'overwriting_ourself /*'*/,1);
  return 1;
}

/* Gets the maximal number of coins of the weight wei fitting into
 * environment() */

public int get_max_number(int wei)
{
  int maxwei;

  maxwei = ({int})environment()->QueryMaxWeight() -
           ({int})environment()->QueryWeight();
  /* We got the difference between the maximal weight and the current weight
   * of the environment */

  if(!maxwei) return 0;

  maxwei = (maxwei > 1000000 || maxwei < -1000000) ?
           (maxwei / (({int})environment()->QueryWeightChg()||1000)) * 1000 :
	   (maxwei * 1000) / (({int})environment()->QueryWeightChg()||1000);

  /* We adapt the weight according to the WeightCharge of the environment */

 return maxwei/wei;
}

/* This function moves the money we got to much into the environment that
 * has announced the ability to carry it. */

static void move_too_much(object too_much, object new_env, object oldenv)
{
  if(new_env == oldenv) return;
  if(!({int})too_much->QueryValue()) {
    too_much->remove();
    if(too_much) destruct(too_much);
  } else {
    printf("The money is too heavy!\n");
    if(!pointerp(({mixed *})environment()->Drop(this_object(),0)))
      move(new_env, M_DROP, 0);
    /* We drop the new (and too heavy) money */
    too_much->move(oldenv,M_SILENT);
    too_much->money_rearrange();
    too_much = 0;
  }
}
