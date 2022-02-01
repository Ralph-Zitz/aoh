/*--------------------------------------------------------------------------
 * DAEMONPATH "moneylog_d" -- The money logger daemon
 *
 * Copyright (C) 1997 by One Step Beyond.
 * All Rights Reserved.
 * Permission granted for non-commercial use.
 *--------------------------------------------------------------------------
 */

#include <money.h>
#include <secure/wizlevels.h>

#define MONEYLOG_DATA "/save/daemons/moneylog"

mapping cash_flow;

public void create()
{
  seteuid(getuid());
  restore_object( MONEYLOG_DATA );
}

/* Returns a list of all previous objects */

public object *list_previous_objects(int step)
{
  return previous_object(step)?
    ({previous_object(step)})+list_previous_objects(++step):({});
}
/* Checks, if ob is a money object */

private int is_money(object ob)
{
  return ob?blueprint(ob) == MONEY:0;
}

/* Looks, if there is a shop involved in the action. If yes, it returns the
 * shop object */

private object get_action_shop()
{
  int    i;
  object ob;

  while(ob = previous_object(++i))
    if(member(inherit_list(ob), "std/shop.c") != -1) return ob;
  return 0;
}

/* Looks if there is a cbteller involved */

private object get_cbteller()
{
  int    i;
  object ob;

  while(ob = previous_object(++i))
    if(member(inherit_list(ob), "d/archwiz/common/finance/cbteller"
) != -1)
      return ob;
  return 0;
}

private void store_cash(string uid1, string uid2, int value)
{
  if(uid1 == uid2) return;
  /* Cash going back to the same uid isn't to be stored */

  if(!uid1) uid1 = uid2, uid2= "-none-", value = -value;
  /* Cash coming from nowhere or going to nowhere is stored by the other uid */

  if(!cash_flow) cash_flow = ([uid1:([uid2:value])]);
  else if(!cash_flow[uid1]) cash_flow[uid1] = ([uid2:value]);
  else cash_flow[uid1][uid2] += value;
}

/* Called, when the value of a money object changes */

public void new_money(string fun, int value)
{
  object shop, involved;
  string uid1, uid2, *path, crea;

  if(!is_money(previous_object())) return;
  /* We don't care calls from no-money-objects */

  if(get_cbteller()) return;
  /* The money is cloned by a cbteller, that's ok */

  if(!(shop = get_action_shop())) {
    /* The money isn't cloned by a shop! */

    if(is_money(involved = previous_object(1)))
      if(!previous_object(2)) involved = this_interactive()||this_player();
      else return;
    /* The money is cloned by an other money object */
    if(involved && IS_IMMORTAL(involved))
      log_file(MONEY_WATCHLOG,
	       sprintf("--\n%s: %d CU created by %O,\n\
                          using %s(), command was \"%s\",\n\
                          given by %O\n",
		       ctime(time()),value,involved,fun,
		       query_verb()||0,this_interactive()||this_player()));
    /* The money is cloned by a wizard */
  } else {
//    return; /* Wait for the new QueryCurrentItems() */
    if(!(involved = ({object})shop->QueryCurrentObject())) {
      log_file(MONEY_ERRORLOG,
	       sprintf("--\n%s: Can't find selled object in %O\n",
		       ctime(time()),shop));
      return;
    }
  }
  if(!(crea = ({string})involved->QueryCreator()) ||
     !sizeof(path = explode(crea,"/")-({""}))) {
    log_file(MONEY_ERRORLOG,
	     sprintf("--\n%s: Can't resolve creator of %O\n",
		     ctime(time()),involved));
    return;
  }
  if(sizeof(path)==1) uid1 = lower_case(path[0]);
  else uid1 = path[1];
  uid2 = this_player()?getuid(this_player()):0;
  if(uid1 != uid2) store_cash(uid1, uid2, value);
}

/* Called, if a money object is moved or if the value is transferred */

public void move_money(string fun, int value, object oldenv, object newenv)
{
  string uid1,uid2;

  if(!is_money(previous_object())) return;
  /* We don't care calls from no-money-objects */

  if((uid1 = (oldenv?getuid(oldenv):0)) != (uid2 = (newenv?getuid(newenv):0)))
  store_cash(uid1, uid2, value);
}

/* Called, if a money object is removed */

public void remove_money(string fun, int value)
{
  object shop, involved;

  if(!is_money(previous_object())) return;
  /* We don't care calls from no-money-objects */

  if(get_cbteller()) return;
  /* CB tellers just move the money to the account without really removing it
   */

  if(!(shop = get_action_shop())) {
    /* Shops give out other objects for the money! */

    if(is_money(previous_object(1))) return;
    /* money can destruct other money without notify them */

    if(!previous_object(1)) return;
    /* The money somehow destructed itself */

    store_cash(getuid(previous_object()),0,value);
  } else
    if(!(involved = ({object})shop->QueryCurrentObject()))
      /* There is no object sold?! */

      store_cash(this_player()?getuid(this_player()):0,
		 getuid(shop),value*2/3);
    /* We guess that the thing is sold for 3 times the price */

    else
      store_cash(this_player()?getuid(this_player()):0,
		 getuid(shop),value - ({int})involved->QueryValue());
}

void reset(int arg)
{
  save_object( MONEYLOG_DATA );
}

int remove()
{
  reset(1);
  destruct(this_object());
  return 1;
}

int query_prevent_shadow() { return 1; }

mapping QueryCashflow() { return cash_flow; }
