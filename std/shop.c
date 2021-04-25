/*
** /std/shop
**
** The standard shop
** (related file: SHOP_DIRECTORY "store")
**
**         92 [t] Creation
**  01-Oct-92 [m] adapted and cleaned
**  15-Oct-92 [o]
**  15-Oct-92 [m] adapted and cleaned (just a bit)
**  24-Nov-93 [p] changed to utilise combine_strings
**                changed present(SK_SHORT, ) into present(shopkeeper, )
**  31-Mar-94 [c] added AddNArticle and AddNSpecialArticle
**  12-Apr-94 [th] Rebuild old shop.
**  04-May-94 [th] Save-Files removed
**  10-May-94 [th] changed the handling of 'forbidden' actions
**  30-May-94 [th] sell_items now sells the items in a sold container, too
**                 and you may set the description of the store now.
**  01-Jun-94 [th] selling done via call_out to fight against monster-sellers
**                 and too long evaluations
**  22-Sep-94 [th] Adapted to new lib,
**                 Shutshop and Archwiz-Shop now have money without end
**  20-Apr-95 [th] Adapted to new lib (sizes, ...)
**  08-Jun-95 [th] 'list' checks get_eval_cost()
**  21-Aug-95 [th] 'buy', 'sell'-verbs handled more flexible and
**                 player_market from Suamor included into the shop
**  12-Aug-96 [m]  Add P_SELL_MUTEX to fight the 'do 100#sell treasure' bug.
**  23-Oct-96 [th] Advanced dustbin-handling
**
** [m] Mateese
** [o] Orcgrimm
** [p] Pepel
** [c] Cadra
** [t] Tares
** [th] Thragor
*/

#define SHOP_DIRECTORY  "/std/"
#define REGISTER "SHOP"

inherit "/std/room";
inherit SHOP_DIRECTORY "shop/articles";
inherit SHOP_DIRECTORY "shop/comm";
inherit SHOP_DIRECTORY "shop/trading";

#include <config.h>
#include <properties.h>		// P_NOBUY, P_NOSELL
#include <moving.h>		// move-methods and errors
#include <secure/wizlevels.h>		// IS_IMMORTAL and such
#include <rooms.h>		// REFRESH
#include <money.h>

// general defines
#define TP	this_player()
#define ME	this_object()
#define WIZFUNS "/p/money/lib/shopwizfuns"
#define MYFILE	SHOP_DIRECTORY "shop"	// this is me, we need this for tests
#define P_LASTLIST "shoplist"
#define P_SELL_MUTEX (to_string(this_object())+":is selling")
#define CUT_PRICE       2000
#define MAX_PRICE	5000 	// Is used in 'GetMaxPrice', this func
				// can be overloaded by an inherit shop.
//#define D(x,y) "/players/thragor/lib/debug"->debug("thragor",x,y)
#define SF(x) #'x //'
#define SYM(x) 'x //'
#define CL_PROGN #', //'
#define STR "/lib/string"
#define DYNAMIC_DEFAULT   20	// Speed refers to dynamic.
#define DEFAULT_MIN_CASH 10000	// Minimal cash of a shop. When a shop is lower
				// at cash it will be restored at reset().

/*----------------------------------------------------------------------
* Variables
*/

private nosave string Psigndescr,Pverb_list,Pverb_appraise,
                      Pverb_identify,Pverb_sell,Pverb_buy,Pverb_show,Pmarket;
private nosave status Pliving_trade, Pno_shopkeeper;
private nosave int Psecure;

private void log_compat(string old,string new)
{
  log_file("COMPAT.shop",
           object_name()+" "+old+" -> "+new+"\n");
}

/*----------------------------------------------------------------------
* The following function may be overloaded by a customized shop for your
* own purpose.
*/

public status CheckTrade(object ob)
  // Returns 1 if the shop trades with objects like this, else 0.
{
  return 1;
}

public int ModifyPrice(object ob, int price, string verb)
  // Here you may modify the price of an object depending on its original price.
  // The modified value will be returned.
{
  return price;
}

public varargs int GetMaxPrice(object ob)
// Return the maximum possible price.
{
  return MAX_PRICE;
}

public varargs int GetCutPrice(object ob)
  // Return the point from where the prices will be
  // cut asymptotically (up to MaxPrice)
{
  int val,overval,maxpr,modpr;
  val = ({int})ob->QueryValue();
  if (val<=CUT_PRICE) return val;
  maxpr = GetMaxPrice(ob);
  if ((modpr = maxpr-CUT_PRICE)<0) modpr = 0;
  if (val>maxpr+modpr) return maxpr;
  modpr = modpr*2;
  overval = val-CUT_PRICE;
  overval = to_int(overval*(1./(2.*(modpr/(overval+1.)))));
  return val-overval;
}

protected status CheckLivingTrade(object ob)
// Returns 1 if it is allowed to trade with livings, or the object is no living
// and otherwise it returns 0.
{
  return Pliving_trade || !living(ob);
}

public mixed SetMarket(mixed i)
// If a player wants to sell items in here for a certain amount of money
// which he will get when someone buys his item, then this flag
// should be set to 1. The flag might also be a string which will specify
// the filename where to store the sold items.
{
  if (!i)
    return Pmarket = 0;
  if (intp(i))
    return Pmarket = blueprint();
  return Pmarket = resolve_file(i);
}

public string QueryMarket() { return Pmarket; }

public status SetSecurity(status i)
// If set to 1, no player will be able to enter the store, even if the shop-
// keeper is gone.
{
  return Psecure = i;
}

public status QuerySecurity()
{
  return Psecure;
}

public string SetSignDesc(string str)
// You may give an alternetive description to the sign. Otherwise the
// standard-description is taken, telling the player how to use the
// commands in here.
{
  return Psigndescr = str;
}

public string QuerySignDesc()
{
  return Psigndescr;
}

public string SetList(string verb) { return Pverb_list = verb; }
public string QueryList() { return Pverb_list; }

public string SetIdentify(string verb) { return Pverb_identify = verb; }
public string QueryIdentify() { return Pverb_identify; }

public string SetAppraise(string verb) { return Pverb_appraise = verb; }
public string QueryAppraise() { return Pverb_appraise; }

public string SetSell(string verb) { return Pverb_sell = verb; }
public string QuerySell() { return Pverb_sell; }

public string SetBuy(string verb) { return Pverb_buy = verb; }
public string QueryBuy() { return Pverb_buy; }

public string SetShow(string verb) { return Pverb_show = verb; }
public string QueryShow() { return Pverb_show; }

public int SetLivingTrade(int i) { return Pliving_trade = i; }
public int QueryLivingTrade() { return Pliving_trade; }

public int QueryNoShopkeeper() { return Pno_shopkeeper; }
public void ForbidShopkeeper()
{
  Pno_shopkeeper = 1;
  RemoveShopkeeper();
}

public void AllowShopkeeper()
{
  Pno_shopkeeper = 0;
  SetShopkeeper();
}

/*-------------------------------------------------------------------
 *   Functions of this room
 */

public string detail_shopkeeper()
// If the shopkeeper is just an 'idea' and no thing ...
{
  if (!QueryNoShopkeeper())
    return QueryShopkeeper()?0:"None here.\n";
  return "This is the shopkeeper.\n";
}

protected varargs string format_sign(string cmd,string desc,string help)
{
  return
    ({string})STR->wrap(({string})STR->ladjust(cmd+" ",27,".")+" "+desc,0,28)+"\n"+
    (help?({string})STR->wrap(({string})STR->mkstr(28)+"Example: "+help,0,28)+"\n":"");
}

public string detail_sign()
// Give a help on what functions you may use in here.
{
  string buy,sell,identify,appraise,list,vshow,result;
  if (buy = QuerySignDesc()) return buy;
  buy = QueryBuy();
  sell = QuerySell();
  identify = QueryIdentify();
  appraise = QueryAppraise();
  list = QueryList();
  vshow = QueryShow();
  if (!(buy||sell||identify||appraise||list||vshow))
    return "The shopkeeper is stupid and understands no command at all.\n";

  result = "";
  if (buy||sell)
    result+="You can "+(buy?buy+(sell?" or ":""):"")
                      +(sell||"")+" things here.\n";
  result+="The shopkeeper understands the following commands:\n";
  result+=list?format_sign(list+" [<what>]",
                           "List (all/matching) items in the store.",
                           list+", "+list+" weapon"):"";
  result+=identify?format_sign(identify+" <what>",
                               "Ask for special information on an item "
                               "you have. This will cost you 10% of the "
                               "items value.",identify+" sword"):"";
  result+=vshow?format_sign(vshow+" <what>",
                            "Ask the shopkeeper about information for "
                            "an item in the store.",
                            vshow+" torch"):"";
  result+=appraise?format_sign(appraise+" <what>",
                               "Ask the shopkeeper for the value of one of "
                               "your items.",
                               appraise+" amulet"):"";
  if (!QueryMarket())
    result+=sell?format_sign(sell+" <what>",
                             "Sell items you have.",
                             sell+" all, "+sell+" torches from bag"):"";
  else
    result+=sell?format_sign(sell+" <what> [for <price>]",
                             "Sell items you have. You'll get paid (on your "
                             "bankaccount) as soon as someone buys this item. "
                             "If you want to sell the item for a certain "
                             "price, give the price in copper coins.",
                             sell+" torch for 20, "+sell+
                             " torches from bag"):"";
  result+=buy?format_sign(buy+" <what>",
                          "Buy items from the store. 'buy really <what>' "
                          "will try to pay the price in any currency but "
                          "you'll have to pay a small fee.",
                          buy+" sunglasses, "+buy+" 3, "+buy+" all torch"):"";
  result+=(TP&&IS_IMMORTAL(TP))?format_sign("shop <command>",
                                           "Special commands for wizards. "
                                           "See 'shop help' for more."):"";
  return result;
}

public int go_store()
// A special exit leading into the store. Wizards can enter the store anytime.
{
  if (!IS_IMMORTAL(TP))
    {
      if((QueryShopkeeper()&&environment(QueryShopkeeper())==ME)
         ||QueryNoShopkeeper())
        return notify_fail(
          lambda(0,
            ({
              SF(SkEmote),
              quote(
                ({
                   "holds you back.\n",
                   "holds "+CustomerName()+" back as "+
                   ({string})TP->QueryPronoun()+" tries to enter the store.\n"
                }) )
            })),NOTIFY_NOT_VALID),0;
      // Ah, now we may prevent players from entering the store even if the
      // shopkeeper is not there anymore. (s. SetSecurity())
      if (QuerySecurity())
        return notify_fail(
          lambda(0,
            ({CL_PROGN,
              ({SF(Write),
                "A complicated security-system closes the door right in front "
                "of you.\n"
              }),
              ({SF(Say),
                CustomerName(1)+" tries to enter the store.\n"
                "Suddenly a complicated security-system closes the door "
                "right in front of "+({string})TP->QueryObjective()+".\n",
                quote(({TP}))
              })
            })),NOTIFY_NOT_VALID),0;
    }
  return PutInStore(TP);
}

/*-------------------------------------------------------------------
** Actions in this room
*/

public object *do_buy(string str)
// This is called when the customer types 'buy'.
{
  int number, force, count, count2;
  string h;
  string *spl, *spl2, *plids;
  object *ret;
  object ob;

  if (str&&sscanf(str,"really %s",h))
    {
      str = h;
      force = 1;
    }
  if (!to_int(str))
    Say(CustomerName(1)+" wants to "+(QueryBuy()||"buy")+" "+
        (str||"something")+".\n",({TP}),1);
  else
    Say(CustomerName(1)+" wants to "+(QueryBuy()||"buy")+
        " article nr. "+str+".\n",({TP}),1);
  if (!sk_in_shop())
    return 0;	// Check if the shopkeeper is in the shop and
		// if not, give a message to the player.


  if (!QueryBuy())		// you can't buy anything in here
    return notify_fail(
      lambda(0,
        ({SF(SkSay),
          "I won't sell any items.\n"
        })),
      NOTIFY_NOT_OBJ),0;

  if (!GetStore())
  return notify_fail(
    lambda(0,
      ({SF(SkSay),
        "I'm sorry but my store is ruined. It needs to be rebuilt first.\n"
      })),
    NOTIFY_NOT_VALID),0;

  if (!str || str == "")
    return notify_fail(
      lambda(0,
        ({SF(SkSay),
          "What do you want to "+QueryBuy()+"?\n"
        })),
      NOTIFY_NOT_VALID),0;

  str = norm_id( str ); // norm first
  str += " "; // add blank if someone used ... and
  spl = explode( str, " and ") - ({ "" });
  ret = ({});

  for ( count = 0; count < sizeof( spl ); count++ )
  {
    str = norm_id( spl[count] ); // norm it again
    if ((number = to_int(str)) > 0 && to_string(number) == str)
      ret += buy_number(number,force) || ({});
    else
    {
      spl2 = explode( str, " " );
      if ( ( ( number = to_int( spl2[0] ) ) > 0 ) && to_string(number) == spl2[0])
      {
        if ( ob = present( str, GetStore() ) )
        {
          ob->SetLastAmount(0); // fix for unit-problem
          number = 1;
        }
        else
        {
          str = implode( spl2[1..], " " );
          if ( ob = present( str, GetStore() ) )
          {
            // if it is an unit and not the plural id was given...
            // sort of a hack, may cause buying the wrong item, but as
            // a loop for buying 200 unit-item causes a too long eval
            // due to joining units, better than nothing.
            if ( sizeof(plids = ({string *})ob->QueryPluralIds()) )
            {
              str = to_string(number)+" "+plids[0];
              number = 1;
              ob->SetLastAmount(0);
            }
          }
        }
      }
      else
      {
        number = 1;
        if ( ( ob = present( str, GetStore() ) ) && ( ({int})ob->QueryAmount() ) )
        {
          if ( sizeof(plids = ({string *})ob->QueryIds()) )
          {
            str = to_string(number)+" "+plids[0];
            ob->SetLastAmount(0);
          }
        }
      }

      for (count2 = 1; count2 <= number; count2++ )
        ret += buy_string(str,force) || ({});
    }
  }

  if ( sizeof ( ret ) )
    return ret;
  else
    return 0;
}

public int do_sell(string str)
// Called when the customer types 'sell'.
{
  mixed found, tmp;
  int i,price;
  string msg, msg2, emsg;
  if (!TP)
    return 0;

  Say(CustomerName(1)+" wants to "+(QuerySell()||"sell")+" "+
      (str||"something")+".\n",({TP}),1);
  if (!sk_in_shop())
    return 0;	// Check if the shopkeeper is in the shop and
		// if not, give a message to the player.
  if (!QuerySell())	// you can't sell anything in here
    return notify_fail(
      lambda(0,
        ({SF(SkSay),
          "I don't buy any items.\n"
        })),
      NOTIFY_NOT_OBJ),0;
  if (!str)
    return notify_fail(
      lambda(0,
        ({SF(SkSay),
          "What do you want to "+QuerySell()+"?\n"
        })),NOTIFY_NOT_VALID),0;

  if (sscanf(str,"%s for %d",str,price)==2
      &&!QueryMarket())
    return notify_fail(
      lambda(0,
        ({SF(SkSay),
          "I don't deal. Sell it to me for the price I want or go to "
          "another shop.\n"
        })),NOTIFY_NOT_VALID),0;

  found = searchm(TP,str, SEARCH_INV|SM_CMULTI|SM_REALLY|WizSearch());
  found = sort_findings(found, sort_f_prop(P_NODROP));
  msg = found[-FOUND_SINGLE]
        ? "You didn't find as many things as you wanted to "+QuerySell()+".\n"
        : "";
  emsg = found[-FOUND_SINGLE]
         ? CustomerName(1)+
           " didn't find as many things as "+({string})TP->QueryPronoun()+
           " wanted to "+QuerySell()+".\n"
         : "";

  if (i = sizeof(tmp = found[FOUND_IDS]))
    {
      if (i == 1)
        msg2 = tmp[0];
      else
        msg2 = implode(tmp[0..<2], ", ")+" or "+tmp[<1];
      emsg += CustomerName(1)+" doesn't find "+msg2+" to "+QuerySell()+".\n";
      msg+= "You don't find "+msg2+" to "+QuerySell()+".\n";
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
    return notify_fail(
      msg
      ?lambda(0,
        ({CL_PROGN,
          ({SF(Say),
            emsg,
            quote(({TP}))
          }),
          ({SF(Write),
            msg,
            1
          })
        }))
      :"You don't have any "+str+" to "+QuerySell()+".\n",
      NOTIFY_NOT_VALID),0;

  if (msg != "")
    {
      Write(msg,1);
      Say(emsg,({TP}));
    }
  if (!sell_items(found[FOUND_SINGLE],price))
  // sell all items in the array, give messages
  // and errors like 'I don't buy such items.'
    return 0;
  return 1;
}

public int do_appraise(string str)
// Action called when a player types "appraise".
{
  object ob;
  int price,maxprice;
  Say(CustomerName(1)+" asks "+(SkShort()||"someone")+" to "+
      (QueryAppraise()||"appraise")+" "+(str||"something")+".\n",({TP}));
  if (!sk_in_shop())
    return 0;	// Check if the shopkeeper is in the shop and
		// if not, give a message to the player.
  if (!QueryAppraise())		// you can't appraise anything in here
    return notify_fail(
      lambda(0,
        ({SF(SkSay),
          "You can't appraise things here.\n"
        })),
      NOTIFY_NOT_OBJ),0;
  if(!str)
    return notify_fail(
      lambda(0,
        ({SF(SkSay),
          "What should I "+QueryAppraise()+"?\n"
        })),
      NOTIFY_NOT_VALID),0;
  if(!ob = present(str, TP))
    return notify_fail(
       lambda(0,
         ({SF(SkSay),
         "Sorry "+CustomerName()+", you have no item called '"+str+"'.\n"})),
         NOTIFY_NOT_VALID),0;

  if(blueprint(ob) == "/obj/money")
    return notify_fail(
      lambda(0,
        ({SF(SkSay),
          "Sorry "+CustomerName()+", if you want to deal with money\n"
          "you must go to the bank.\n"
        })),NOTIFY_NOT_VALID),0;

  // check if we trade with this item
  if(!CheckTrade(ob) || !CheckLivingTrade(ob))
    return notify_fail(
      lambda(0,
        ({SF(SkSay),
          "I don't deal with "+({string})ob->Short()+".\n"
        })),NOTIFY_NOT_VALID),0;
  price = (price = GetPrice(ob))>(maxprice = GetMaxPrice(ob))
          ?maxprice:price;
  SkSay("I'll give you "+({string})MONEY_DEMON->QueryValueString(price,QueryValuta())+
        " for "+({string})ob->Short()+".\n");
  return 1;
}

public int do_identify(string str)
// Called when the customer types 'identify'.
{
  object ob;
  string s,msg;
  Say(CustomerName(1)+" wants "+(SkShort()||"someone")+" to "+
      (QueryIdentify()||"identify")+" "+(str||"something")+".\n",({TP}),1);
  if (!sk_in_shop())
    return 0;	// Check if the shopkeeper is in the shop and
		// if not, give a message to the player.
  if (!QueryIdentify())		// you can't identify anything in here
    return notify_fail(
      lambda(0,
        ({SF(SkSay),
          "I don't identify items.\n"
        })),
      NOTIFY_NOT_OBJ),0;
  if(!str)
    return notify_fail(
      lambda(0,
        ({SF(SkSay),"What should I "+QueryIdentify()+"?\n"})),
      NOTIFY_NOT_VALID),0;

  if(!ob = search(TP,str,SEARCH_INV|SM_OBJECT|SM_IGNLIGHT))
    return notify_fail(
      lambda(0,
        ({SF(SkSay),
          "Sorry "+CustomerName()+", you have no item called '"+str+"'.\n"
        })),NOTIFY_NOT_VALID),0;

  if(!check_money(round_value(GetPrice(ob) / 10)))
    return notify_fail(
      lambda(0,
        ({SF(SkSay),
          "Ha, I may say you this: This item is too cheap to be even worth "
          "to take a closer look at it.\n"
        })),NOTIFY_NOT_VALID),0;

  if (({mixed})ob->QueryMagic()&&intp(({mixed})ob->QueryMagic()))
    msg = "This is a magic item. I can't say anything more, that is "
          "if there's something special with it or not.\n";
  else
    if (s = ({string})ob->QueryInfo())
      msg = "The following is special about the item:\n"+
            s+(s[<1..]=="\n"?"":"\n");
    else
      msg = "There's nothing special about this item.\n";
  if (member(inherit_list(ob),"std/equipment.c")!=-1)
    switch(({int})ob->QuerySize())
      {
       case PSIZE_SMALL:
	msg+="This item fits for small beings.\n";
	break;
       case PSIZE_NORMAL:
	msg+="This item fits for medium beings.\n";
	break;
       case PSIZE_LARGE:
	msg+="This item fits for large beings.\n";
	break;
       case PSIZE_GENERIC:
	msg+="This item fits for all beings.\n";
	break;
       default:
	msg+="This item has a strange size. I can't say for whom "
             "it was created.\n";
      }
  SkSay(msg);
  return 1;
}

public int do_show(string str)
// Called when the customer types 'show'.
{
  object ob,*obs;
  mixed msg;
  int number;
  mapping list;

  Say(CustomerName(1)+" wants "+(SkShort()||"someone")+" to "+
      (QueryShow()||"show")+" "+(str||"something")+".\n",({TP}),1);
  if (!sk_in_shop())
    return 0;	// Check if the shopkeeper is in the shop and
		// if not, give a message to the player.
  if (!QueryShow())		// you can't show anything in here
    return notify_fail(
      lambda(0,
        ({SF(SkSay),
          "I don't show my items.\n"
        })),
      NOTIFY_NOT_OBJ),0;
  if(!str)
    return notify_fail(
      lambda(0,
        ({SF(SkSay),"What should I "+QueryShow()+"?\n"})),
      NOTIFY_NOT_VALID),0;

  if (!GetStore())
  return notify_fail(
    lambda(0,
      ({SF(SkSay),
        "I'm sorry but my store is ruined. It needs to be rebuilt first.\n"
      })),
    NOTIFY_NOT_VALID),0;

  if ((number = to_int(str)) > 0 && to_string(number) == str)
    {
      if (!list = ({mapping})TP->Query(P_LASTLIST))
        return notify_fail(
          lambda(0,
            ({SF(SkSay),"I haven't shown my list to you.\n"})),
            NOTIFY_NOT_VALID),0;
      if (!member(list,number))
        return notify_fail(
          lambda(0,({SF(SkSay),"I don't have that number in my store.\n"})),
          NOTIFY_NOT_VALID),0;
      if (!sizeof(obs = filter(list[number]-({0}),
                  lambda(({SYM(o)}),({SF(==),({SF(environment),SYM(o)}),
                                             GetStore()})))))
        return notify_fail(
          lambda(0,({SF(SkSay),"I don't have that item anymore.\n"})),
          NOTIFY_NOT_VALID),0;
      ob = obs[0];
    }
  else if(!ob = search(GetStore(),str,SEARCH_INV|SM_OBJECT|SM_IGNLIGHT))
    return notify_fail(
      lambda(0,
        ({SF(SkSay),
          "I have no item called '"+str+"'.\n"
        })),NOTIFY_NOT_VALID),0;

  msg = ({string})ob->QueryLong()||"This item looks like nothing.\n";
  if (pointerp(msg)) msg = msg[0];
  if (member(inherit_list(ob),"std/equipment.c")!=-1)
    switch(({int})ob->QuerySize())
      {
       case PSIZE_SMALL:
	msg+="This item fits for small beings.\n";
	break;
       case PSIZE_NORMAL:
	msg+="This item fits for medium beings.\n";
	break;
       case PSIZE_LARGE:
	msg+="This item fits for large beings.\n";
	break;
       case PSIZE_GENERIC:
	msg+="This item fits for all beings.\n";
	break;
       default:
	msg+="This item has a strange size. I can't say for whom "
             "it was created.\n";
      }
  Write(SkShort(1)+" shows "+({string})ob->Short()+" to you:\n"+({string})ob->Long());
  Say(SkShort(1)+" shows "+({string})ob->QueryShort()+" to "+CustomerName()+".\n",({TP}));
  return 1;
}

protected void list_done(string str)
{
  mapping list;
  int *idx,i,diff;
  string *linelist,*res,short;
  object ob;
  if (!TP)
    return 0;
  list = ({mapping})TP->Query(P_LASTLIST);
  if (!list||!sizeof(list))
    {
      SkSay(str?"Sorry "+CustomerName()+
            ", I have no such items in the store.\n"
            :"Sorry "+CustomerName()+", I have no item in the store.\n");
      return;
    }

  Say(SkShort(1)+" shows a list to "+CustomerName()+".\n",({TP}));
  // devide the list in two parts and if the number of the lines is odd,
  // then show more lines on the left side.
  res = ({});
  if (({int})TP->CanSee())
    {
      linelist = ({});
      idx = sort_array(m_indices(list),SF(>));
      //first create all lines (including wrapping)
      for (i=0;i<sizeof(idx);i++)
	{
          ob = list[idx[i]][0];
          if (!ob || !objectp(ob))
            continue;
          short = strip_article(({string})ob->QueryShort());
	  switch(({int})ob->QuerySize())
	    {
	     case PSIZE_SMALL:
	      short+=" (s)";
	      break;
	     case PSIZE_NORMAL:
	      short+=" (m)";
	      break;
	     case PSIZE_LARGE:
	      short+=" (l)";
	      break;
	    }
	  if (({int})ob->QueryAmount()>1) short+=sprintf(" [%3d pieces]",
						  ({int})ob->QueryAmount());
          linelist+=
            explode(({string})STR->wrap(" "+({string})STR->radjcut(to_string(idx[i]), 3)+". "+
              ({string})STR->radjcut(to_string(convert_value(
                           GetPrice(ob)/(({int})ob->QueryAmount()||1))),6)
	      +"  "+strip_article(short),39,14),"\n");
	}

      diff = (sizeof(linelist)/2)+(sizeof(linelist)%2);
      for (i = 0; i < (sizeof(linelist)/2)+(sizeof(linelist)%2); i++)
        res+=({({string})STR->ladjust(linelist[i],39)+
               ((i+diff<sizeof(linelist))
                ?" |"+linelist[i+diff]
                :"")
             });
      res = ({SkShort(1) +
           " shows a list to you (all prices in "+get_min_coin()+"):","",
           "<nr>  <cost> of <item>                  |<nr>  <cost> of <item>"})
	+res+
        ({"","I trade with:",
          "  "+print_list(QueryValuta())});
    }
  else
    res = ({SkShort(1)+" does something."});
  STR->smore(res,TP->QueryPageSize());
}

public int do_list(string str)
// Called whenever a customer types 'list' in here.
{
  Say(CustomerName(1)+" asks "+(SkShort()||"someone")+
      " for the list.\n",({TP}),1);

  if (!sk_in_shop())
    return 0;	// Check if the shopkeeper is in the shop and
		// if not, give a message to the player.

  if (!QueryList())	// you can't call for a list in here
    return notify_fail(
      lambda(0,
        ({SF(SkSay),
          "I have no list for you.\n"
        })),
          NOTIFY_NOT_VALID),0;

  if (!GetStore())
  return notify_fail(
    lambda(0,
      ({SF(SkSay),
        "I'm sorry but my store is ruined. It needs to be rebuild first.\n"
      })),
    NOTIFY_NOT_VALID),0;

  if (str=="all")
    str = 0;
  SkSay("One moment. I'll take a look into my store.\n");
  get_list(str);
  return 1;
}

public int do_toss(string str)
// This function is called when a player 'toss'es something into the
// dustbin.
{
  object *obs, trash;
  mixed error,m;
  string *s,what,intowhat;
  int i;

  //Say(CustomerName(1)+" wants to toss "+(str||"something")+" into the "
  //    "dustbin.\n",({TP}));

  trash = QueryDustbin();

  if (!trash)
    // damn, this is ugly! Only Thagor could write such $#@%
    return notify_fail(
		       lambda(0,
			      ({
				CL_PROGN,
				({
				  SF(Write),
				  "There isn't anything you can toss items "
				  "into here.\n"
				}),
				({
				  SF(Say),
				  CustomerName(1)+" looks puzzled as "+
				  ({string})TP->QueryPronoun()+
				  " can't find a dustbin.\n",
				  quote(({TP}))
				})
			      })
			     ),
		       NOTIFY_NOT_VALID
		       ),0;
  if (!str)
    return notify_fail(
		       lambda(0,
			      ({
				CL_PROGN,
				({
				  SF(Write),
				  "What do you want to toss?\n"
				}),
				({
				  SF(Say),
				  CustomerName(1)+" looks puzzled as "+
				  ({string})TP->QueryPronoun()+
				  " recognizes that "+({string})TP->QueryPronoun()+
				  " doesn't really know "
				  "what to toss.\n",
				  quote(({TP}))
				})
			      })
			     ),
		       NOTIFY_NOT_VALID
		       ),0;
  if (sscanf(str,"%s into %s",what,intowhat)!=2)
    what = str;
  else
    if (search(TP,intowhat,SEARCH_ENV|SM_IGNLIGHT|SM_OBJECT) != trash)
      return notify_fail("You can't toss anything into "+intowhat+".\n",
			 NOTIFY_NOT_OBJ),0;

  m = searchm(TP, what, SEARCH_INV|SM_MULTI) || ({});
  obs = m[FOUND_SINGLE] || ({});
  if (sizeof(m[FOUND_CLASS]))
  {
    for (i = 0; i < sizeof(m[FOUND_CLASS]); i++)
      if (sizeof(m[FOUND_CLASS][i]))
      {
        obs -= m[FOUND_CLASS][i][0..0];
        obs += m[FOUND_CLASS][i][0..0];
      }
  }
  if (sizeof(m[FOUND_MULTI]))
  {
    for (i = 0; i < sizeof(m[FOUND_MULTI]); i++)
    {
      if (sizeof(m[FOUND_MULTI][i]) < 2)
	continue;
      error = m[FOUND_MULTI][i][0];
      if (!error)
        error = sizeof(m[FOUND_MULTI][i])-1;
      obs -= m[FOUND_MULTI][i][1..error];
      obs += m[FOUND_MULTI][i][1..error];
    }
  }

  if(!sizeof(obs))
    {
      if (({int})TP->CantSee())
        return notify_fail(
          lambda(0,
            ({CL_PROGN,
              ({SF(Write),""
              }),
              ({SF(Say),
                ({CustomerName(1)+" seems to have problems with "+
                  ({string})TP->QueryPossessive()+" eyes.\n",""}),
                quote(({TP}))
              })
            })),NOTIFY_NOT_VALID),0;
      return notify_fail(
        lambda(0,
          ({CL_PROGN,
            ({SF(Write),
              "You can't find that in your backpack.\n"
            }),
            ({SF(Say),
              CustomerName(1)+" can't find "+what+" in "+({string})TP->QueryPossessive()+
              " backpack.\n",
               quote(({TP}))
            })
          })),NOTIFY_NOT_VALID),0;
    }

  if(sizeof(filter(obs,SF(query_once_interactive))))
    return notify_fail(
      lambda(0,
        ({SF(Say),
          "You hear Wessex giggling in the distance: Don't toss players!!\n"
        })),NOTIFY_NOT_VALID),0;

  s = ({ });
  for (i = 0; i < sizeof(obs); i++)
  {
    error = ({mixed})obs[i]->Query(P_NODROP);
    if (!error)
      error = (ME_OK != ({int})obs[i]->move(trash, M_SILENT));
    if (error)
    {
      Write(stringp(error) ? error
                           : ("You fail to drop "+({string})obs[i]->Short()+".\n"));
      obs[i] = 0;
    }
    else
      s += ({ ({string})obs[i]->Short() });
  }

  obs -= ({ 0 });
  s -= ({ 0, "" });
  if (!sizeof(obs))
    return notify_fail("There is nothing to toss.\n", NOTIFY_NOT_VALID),0;

  if (sizeof(s))
  {
     Write(implode_list(s,
       "You toss ", " into the " + ({string})trash->QueryShort() + "."));
     Say(implode_list(s,
       CustomerName(1)+" tosses "," into " +
       ({string})trash->QueryShort() + ".\n"),({TP}));
  }
  map_objects(obs, "remove");
  map(obs - ({ 0 }), SF(destruct));
  Say(
    "A small grey creature arrives from the bottom of the "
    + ({string})trash->QueryShort() + ", smiles happily, dances around "
    "and vanishes again.\n");
  return 1;
}

public int wiz_funcs (string str)
// Called with action 'shop'. These are special commands for wizards to check
// this and other shops.
{
  if (!str) str = "help";
  str = lower_case(str);

  return ({int})WIZFUNS->startfun(TP,str,REGISTER);
}

protected void clean_room()
// Check for items, which don't belong to the shop and remove them
// through an exit (best to the outside) or if they are of any
// value, put them into the store.
{
  object *inv;
  int i,j;
  status moved;
  string *idx,exit;
  mapping exits;

  j = sizeof(idx = m_indices(exits = QueryExitsDest()));
  // search an exit where to throw out useless items
  while(j--)
    {
      if (!objectp(exits[idx[j]]) && !stringp(exits[idx[j]]))
	continue;
      if (stringp(exits[idx[j]]))
	if (!exit
	    ||!({int})exits[idx[j]]->QueryIndoors())
	  exit = exits[idx[j]];
      if (exit&&!({int})exit->QueryIndoors())
	break;
      else
	exit = 0;
    }
  // Don't do anything, if a player is in here. It might be one of
  // her/his items.
  i = sizeof(inv = all_inventory());
  if (sizeof(filter(inv,SF(interactive))))
    return;
  while(i--)
    if (({string})inv[i]->QueryCreator()!=object_name(ME)
        &&(!query_once_interactive(inv[i])&&!({int})inv[i]->QueryIsPlayer()))
      {
	if (living(inv[i])
            ||!({int})inv[i]->QueryValue()
            ||!CheckTrade(inv[i]))
	  {
	    if (exit && living(inv[i]) && query_once_interactive(inv[i]))
	      {
                moved = 1;
		inv[i]->move(exit,M_SILENT);
	      }
	    else
	      {
		inv[i]->remove();
		if (inv[i]) destruct(inv[i]);
	      }
	  }
	else
	  PutInStore(inv[i]);
      }
  if (moved&&exit)
    show_room(exit,({string})STR->wrap(SkShort(1)+
              " arrives with a broom and throws some rubbish "
              "out of the shop.\n",75,0));
}

/*
* Overriding functions of /std/room
*/

public varargs string QueryIntLong (string arg)
{
  if (arg) return (::QueryIntLong(arg));
  return ((::QueryIntLong())||"You are in a shop.\n")+
         "A sign is placed on the counter.\n";
}

/*
* reset, create, init, notify_enter
*/

public void reset()
{
  object st;
  if (object_name(ME)==MYFILE) return;
  room::reset();
  reset_cash();
  clean_room();
  st = GetStore();
  if (st)
    st->reset();
}

public varargs void create()
{
  (::create());

  if (object_name(ME)==MYFILE) return;

  AddDetail("counter","There's a sign on it.\n");
  AddDetail("sign",SF(detail_sign));
  AddDetail("shopkeeper",SF(detail_shopkeeper));

  Pno_shopkeeper = 0;

  SetIndoors(1);
  SetBright(MAX_SUNBRIGHT/5);
  AddExit ("store",SF(go_store));
  AddReadMsg("sign",SF(detail_sign));

  SetBuyFactor(300);
  SetDynamic(DYNAMIC_DEFAULT);
  SetMinCash(DEFAULT_MIN_CASH);
  SetCash(DEFAULT_MIN_CASH);
  SetStore();
  SetShopkeeper();
  SetDustbin();
  SetIdentify("identify");
  SetList("list");
  SetSell("sell");
  SetAppraise("appraise");
  SetBuy("buy");
  SetShow("show");

  Psigndescr = 0;

  call_out(SF(PayInsurances),1);
  // As we may be defined as market later on, try to pay the
  // insurances if we are.
  "/obj/lib/register"->Register(ME,REGISTER);
}

public void init()
{
  (::init());
  TP->Set(P_SELL_MUTEX, 0);
  add_action("do_identify",QueryIdentify()||"identify");
  add_action("do_toss","toss");
  add_action("do_list",QueryList()||"list");
  add_action("do_sell",QuerySell()||"sell");
  add_action("do_appraise",QueryAppraise()||"appraise");
  add_action("do_buy",QueryBuy()||"buy");
  add_action("do_show",QueryShow()||"show");
  if(IS_IMMORTAL(TP)) add_action("wiz_funcs", "shop");
}

// prevent that someone enters the /std/shop

public int allow_enter(int method,mixed extra)
{
  if (object_name(ME) == MYFILE) return ME_NO_ENTER;
  return (::allow_enter(method,extra));
}

public void notify_leave(mixed to, int method, mixed extra)
{
  if (query_once_interactive(previous_object()))
    previous_object()->Set(P_LASTLIST,0);
  return (::notify_leave(to,method,extra));
}
