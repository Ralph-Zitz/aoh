/*
* Player-Market
*
* The player market uses commands similary to a shop but the prices
* are set by the players.
* All money a player gets for something will be payed onto his bank
* account. If the item won't get sold, the player gets the half of
* the price he would normally get in a shop as insurance-payment.
* For more please read ~suamor/doc/playermarket.doc
*
* 01apr93 [s] Creation
* 26sep94 [t] Adapted to new /std/shop and moved to /obj
*
* [s] Suamor
* [t] Thragor
*/

#include <properties.h>
#include <attributes.h>
#include <moving.h>
#include <secure/wizlevels.h>
#include <money.h>
#include <search.h>
#include <config.h>
#include <daemons.h>

#define PL this_player()
#define TP PL
#define ME this_object()

#define LSF(x) symbol_function(x,ME)
#define ESF(x) symbol_function(x)
#define MYFILE "/obj/player_market"
#define STR "/lib/string"
#define MAX_POLICE 1000 /* max. insurance-payment */
#define POLICE(x) x/2 /* insurance amount of an object */
#define NO_TRADE_VALUE 5000
#define P_SELLER "pm-seller"
#define P_ORIG_VALUE "pm-value"
#define PROFIT_SELL 4/5
#define OBJ_PRICE 0
#define OBJ_POLICE 1

inherit "/std/shop";

mapping sold_objects;
// (["player1":([obj1:price1;police1,obj2:price2;police2]),
//   "player2":([obj1:price3;police1])
// ])

void SaveObjList(object ob,int pr) // former Save_objlist
{
 string player;
 mapping objlist;

 if (!ob) return;
 player = getuid(PL);
 objlist = sold_objects[player]||([]);
 objlist+=([object_name(ob):pr;POLICE(pr)?POLICE(pr):1]);
 sold_objects+=([player:objlist]);
 save_object(object_name());
}

/*
* ClearObjList()
* Load what the players sold in here and pay the
* sum they wanted to have as insurance to their account
*/

void ClearObjList() // former Del_objlist
{
 int i,j,objpolice,police;
 string *players,*objects;
 object ob;

 restore_object(object_name());
 sold_objects=sold_objects||([]);
 for (i=sizeof(players=m_indices(sold_objects));i--;)
 {
  police = 0;
  for (j=sizeof(objects=m_indices(sold_objects[players[i]]));j--;)
  {
   objpolice=sold_objects[players[i]][objects[j],OBJ_POLICE];
   if (ob=find_object(objects[j])) destruct(ob);
   if (objpolice>MAX_POLICE) objpolice=MAX_POLICE;
   police+=objpolice;
  }
  BANK_D->add_account(players[i],police);
 }
 sold_objects=([]);  /* insurance has been paid */
 save_object(object_name());
}

void create()
{
 if(object_name(this_object()) == MYFILE) return;
 ::create();
 ClearObjList();  // Delete saved list
 SetAppraise(0); // No Appraise in the player market
 SetSecurity(1);
 SetShopkeeper(0,([P_NAME:"marketkeeper",
                   P_IDS:({"marketkeeper"}),
                   P_SHORT:"the marketkeeper",
                   P_LONG:"This is the marketkeeper.\n",
                   P_NOGET:"The marketkeeper has own feet to stand on.\n"
                 ]));
}

void reset()
{
 if(object_name(this_object()) == MYFILE) return;
 ::reset();
}

/*
* Modified functions for the Player-Market
*
*/

public status CheckTrade(object ob)
{
 if (function_exists("QueryValue",ob)==blueprint(ob)) return 0;
 if (member(inherit_list(ob),"std/lamp.c")!=-1) return 0;
 if (member(inherit_list(ob),"std/food.c")!=-1) return 0;
 if (member(inherit_list(ob),"std/drink.c")!=-1) return 0;
 // we can't handle items here, which override QueryValue
 if (ob->Query(P_NOBUY)) return 0;
 if (ob->Query(P_NOSELL)) return 0;
 // we don't take items which may not be sold again
 return 1;
}

int modify_on_amount(object ob,int price)
{
 return price;
}

int buy_routine(object ob)
{
 if (shop::buy_routine(ob))
 {
  m_delete(sold_objects[ob->Query(P_SELLER)],object_name(ob));
  BANK_D->AddAccount(ob->Query(P_SELLER),
                         ob->QueryValue()*3*PROFIT_SELL);
  ob->SetValue(ob->Query(P_ORIG_VALUE));
  save_object(object_name());
  return 1;
 }
 return 0;
}

protected void pm_sell_items(object *items,object env,int price)
{
 int i,limit;
 object ob,st,*cinv,*further_items;
 string str;

 if (!items || (sizeof(items) < 1))
 {
  SkWrite ("Sorry "+CustomerName()+", you don't have such item(s)!\n");
  return;
 }

 if (!st = GetStore())
 {
  SkWrite("Sorry, "+CustomerName()+" our store needs to be rebuild.\n");
  return;
 }

 if (BANK_D->QueryAccount(getuid(PL))==-1)
 {
  SkWrite("Sorry "+CustomerName()+", you have no bank account.\n");
  return;
 }

 limit = (sizeof(items)>20)?21:sizeof(items); // don't sell more than 20 items
                                              // per round
 further_items = items;

 for (i = 0; i < limit; i++)
 {
  ob = present(items[i], env);
  if (!ob) continue;
  str = (string)ob->QueryShort();
  // Items with no value are not accepted from the shop.
  if (!(int)ob->QueryValue())
  {
   SkWrite("Sorry "+CustomerName()+", this item ("+str+") has no value.\n");
   SkWrite("Toss it in a dust-bin...\n");
   further_items-=({ob});
   continue;
  }
  if (ob->Query(P_NOBUY))
  {
   SkWrite ("Sorry "+CustomerName()+", I don't buy such items!\n");
   if (env!=PL)
   {
    SkWrite("Hm. I think you might want to have this back.\n");
    write(SkShort(1)+" gives "+(ob->QueryShort()||"something")+" to you.\n");
    if (ob->move(PL,M_SILENT)!=ME_OK)
    {
      SkWrite("You can't take it. I'll put in on the floor.\n");
      ob->move(ME,M_SILENT);
    }
   }
   further_items-=({ob});
   continue;
  }
  if (!CheckTrade(ob) || !CheckLivingTrade(ob))
  {
   SkWrite(CustomerName()+", I don't buy such items!\n");
   further_items-=({ob});
   continue;
  }
  // Check if item is droppable.
  if (ob->QueryNoDrop()||ob->QueryNoGive())
  {
   SkWrite("Oh " + CustomerName() + ", I can't take '" +str+ "' from you!\n");
   further_items-=({ob});
   continue;
  }
  if (!st->MayAddWeight(ob->QueryWeight()))
  {
   SkWrite("Oh, " + CustomerName() + ", "+str+" doesn't fit into my store!\n");
   further_items-=({ob});
   continue;
  }
  if (sizeof(cinv = (all_inventory(ob)||({}))))
   pm_sell_items(cinv,ob,price);

  further_items-=({ob});	// o.k. we sold this item

  // Now handle the item.
  show( ({ SkShort(1)+" takes an item from " + CustomerName()+".\n",
           "Something happens.\n"
        }) );
  show_room(ME, SkShort(1) + " puts the item into the store.\n");
  PutInStore(ob);
  SaveObjList(ob,ob->QueryValue());
  ob->Set(P_ORIG_VALUE,ob->QueryValue());
  ob->Set(P_SELLER,getuid(PL));
  ob->SetValue(price);
 }
 if (sizeof(further_items)) call_out(#'pm_sell_items,2,further_items,env,price);
}

/*
* do_sell()
* Called when the customer types 'sell'.
*/

public int do_sell(string str)
{
  mixed found, tmp;
  object env;
  int i,price;
  string msg, msg2, emsg,hstr,valstr;
//  object ob,env;
//  object *items;
//  string all_id,hstr,fromwhat,valstr;
//  int price;

  if (!env = TP)
    return 0;

  Say(CustomerName(1)+" wants to sell "+(str||"something")+
      ".\n",({TP}),1);
  if (QueryNoSell())	// you can't sell anything in here
    return (int)notify_fail(
      lambda(0,
        ({LSF("SkSay"),
          "I don't buy any items.\n"
        })),
      NOTIFY_NOT_OBJ)&&0;
  if (!sk_in_shop())
    return 0;	// Check if the shopkeeper is in the shop and
		// if not, give a message to the player.
  if (!str)
    return (int)notify_fail(
      lambda(0,
        ({LSF("SkSay"),
          "What would you sell?\n"
        })),NOTIFY_NOT_VALID)&&0;

  if (sscanf(str,"%s for %s",hstr,valstr)==2)
    {
      str = hstr;
      if (sscanf(valstr,"%d %s",price,hstr)==2)
        return (int)notify_fail(SkShort(1)+
         " says: Just give the amount of copper coins as amount.\n")&&0;
      sscanf(valstr,"%d",price);
      if (!price = price / 3) price = 1;
    }
  else
    return (int)notify_fail(
      SkShort(1)+" says: Please tell me the price you want to sell your "
      "items for.\n")&&0;

  found = TP->SearchM(str, SEARCH_INV|SM_CMULTI|SM_REALLY|WizSearch());
  found = sort_findings(found, sort_f_prop(P_NODROP));
  msg = found[-FOUND_SINGLE]
        ? "You didn't find as much things as you wanted to sell.\n"
        : "";
  emsg = found[-FOUND_SINGLE]
         ? CustomerName(1)+
           " didn't find as much things as "+TP->QueryPronoun()+
           " wanted to sell.\n"
         : "";

  if (i = sizeof(tmp = found[FOUND_IDS]))
    {
      if (i == 1)
        msg2 = tmp[0];
      else
        msg2 = implode(tmp[0..<2], ", ")+" or "+tmp[<1];
      emsg += CustomerName(1)+" doesn't find "+msg2+" to sell.\n";
      msg+= "You don't find "+msg2+" to sell.\n";
    }
  if (i = sizeof(tmp = found[FOUND_NOTHANDLE]))
    {
      tmp = fold_objlist(tmp);
      if (i == 1)
        msg2 = tmp[0];
      else
        msg2 = implode(tmp[0..<2], ", ")+" or "+tmp[<1];
      emsg += CustomerName(1)+" can't drop "+msg2+".\n";
      msg += "You can't drop "+msg2+".\n";
    }
  if (!(sizeof(found[FOUND_SINGLE])))
    return (int)notify_fail(
      msg
      ?lambda(0,
        ({ESF(","),
          ({LSF("Say"),
            emsg,
            quote(({TP}))
          }),
          ({LSF("Write"),
            msg,
            1
          })
        }))
      :"You don't have any "+str+" to sell.\n",
      NOTIFY_NOT_VALID)&&0;

  pm_sell_items(found[FOUND_SINGLE],env,price);
  // sell all items in the array, give messages and
  // errors like 'I don't buy such items.'
  if (msg != "")
    {
      Write(msg,1);
      Say(emsg,({TP}));
    }
  return 1;
}

is_own(object ob, string uid)
{
 if(ob->Query(P_SELLER) == uid)
 {
  write(STR->radjust(to_string(get_price(ob)),6)+"    "+ob->QueryShort()+"\n");
  return 1;
 }
 return 0;
}

list_own(string str)
{
 if (PL->CanSee())
 {
  write(SkShort(1) + " shows a list to you (all prices in copper coins):\n\n\
<cost> of <item>\n");
  show(({SkShort(1) + " shows a list to " + CustomerName() + ".\n",
       "Something happens.\n"}));
  if(!sizeof(filter(all_inventory(GetStore()),#'is_own,getuid(PL))))
   write("(empty)\n");
 }
 else write("The shopkeeper does something.\n");
 return 1;
}

do_list(string str)
{
 if (lower_case(str||"")!="own")
  return ::do_list(str);
 return list_own(str);
}

/*
* detail_sign()
* Give a help on what functions you may use in here.
*/

public string detail_sign()
{
 return QuerySignDesc()?QuerySignDesc():
(
 (QueryNoBuy()&&QueryNoSell())?"":("You can "+(!QueryNoBuy()?"buy"+(!QueryNoSell()?" or ":""):"")+
                                  (!QueryNoSell()?"sell":"")+
                       " things here.\n")
)+
((QueryNoList()&&QueryNoIdentify()&&QueryNoAppraise()
  &&QueryNoSell()&&QueryNoBuy())
 ?"The shop-assistant is stupid and understands no command at all.\n"
 :"The shop-assistant understands the following commands:\n"+
(!QueryNoList()?
 "'list [<pattern>]'   e.g. list, list weapon, list armour, list bag, ...\n"
 "                     pattern might be a regular expression\n":"")+
(!QueryNoIdentify()?
 "'identify <id>'      Give a special info and the creator of the item.\n"
 "                     That will cost you 10% of the item value.\n":"")+
(!QueryNoAppraise()?
 "'appraise <id>'      Let the assistant appraise the value of one of your items.\n":"")+
(!QueryNoSell()?
 "'sell <id> for <pr>' Sell the item with the name <id> for price <pr>.\n"
 :"")+
(!QueryNoBuy()?
"'buy <id>            Buy the first item with name <id> from the store.\n"
"'buy <nr>            Buy the item with number <nr> from the last list that\n"
"                     the shop-assistant has showed to you.\n":"")+
(PL?
 (IS_IMMORTAL(PL)?
"'shop <command>'     Get infos about shops. Type 'shop help' for more.\n":"")
:"") );
}
