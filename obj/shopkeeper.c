// obj/shopkeeper.c
// Originally written by Suamor 8/96

// Implemented to allow easy extension of shop commands for services
// f.e. to buy a thing and add some inscriptions or engravements to it
// as the player whishes it or to work a natural gem into a real ornament :-)
// Key function is Service() which is not implemented in the shopkeeper
// itself, but it is called from here (!). It implements the services
// itself (i.e. manipulating the objects) but without the need to care
// for prices and command handling. It may even use objects from the
// shop where the shopkeeper is using GetShopObject() function (which
// adds the price of the object to the price list).
//
// TODO: appraise service: price for the service including all shop items

#include <money.h>
#include <living.h>
#include <search.h>
#include <config.h>
#include <moving.h>
#include <properties.h>
#include <stdrooms.h>
#include "shopkeeper.h"

#define SHOP_DIRECTORY "/std/"

inherit "/std/npc";
inherit SHOP_DIRECTORY "shop/comm";
inherit SHOP_DIRECTORY "shop/trading";

#define ME this_object()
#define TP this_player()
#define P_PRICE "shopprice" // the price a player wants for an item
#define P_OWNER "shopowner" // the player who sold this item
#define SF(x) #'x //'
#define CL_PROGN #', //'
#define STR "/lib/string"
#define DYNAMIC_DEFAULT   20    // Speed refers to dynamic.
#define DEFAULT_MIN_CASH 10000  // Minimal cash of a shop. When a shop is lower
                                // at cash it will be restored at reset().
#define MAX_PRICE   1000    // Is used in 'GetMaxPrice', this func
#define MYSHOP environment(ME) // Needed for checking the store and money of the shop
                               // else the shopkeeper would keep the money for himself

private nosave object *shopobjects;
private nosave string Pverb_list, Pverb_show;
private nosave mapping serv_list;
// consists of (["Pserv_command":"serv_descr",price])

object Service(string s1,string s2,int force);

// Usually you should not use these functions directly
mapping SetServices(mapping serv) { return serv_list = serv; }
mapping QueryServices() { return serv_list; }

// These are the best to be used
// Add Service
void AddService(string scomm,string sdescr,mixed price)
{
  if (!serv_list) serv_list=([]);
  serv_list += ([scomm:sdescr;price]);
}

// Remove Service
void RemoveService(string scomm)
{
  if (member(serv_list,scomm))
    serv_list -= ([scomm]);
}

// Check if command exists
string GetServCommand(string scomm)
{
  if (member(serv_list,scomm)) return scomm;
}

// Set/Query a description for service command scomm
void SetServDesc(string scomm,string sdesc)
{
  if (member(serv_list,scomm)) serv_list[scomm,0]=sdesc;
}

string QueryServDesc(string scomm) { return serv_list[scomm,0]; }

// Set/Query price for service command scomm
mixed SetServPrice(string scomm,mixed price)
{
  if (member(serv_list,scomm)) return serv_list[scomm,1]=price;
}

mixed QueryServPrice(string scomm,string cobj)
{
  if (cobj&&stringp(cobj))
    if(mappingp(serv_list[scomm,1]))
      return serv_list[scomm,1][cobj];
  return serv_list[scomm,1]; 
}

// For several services needing multiple items like
// 'make ladder', 'make gem' you may add/remove the prices
// for every item which should be built
void AddServPrice(string scomm,string cobj,int price)
{
  mixed tpr;

  if(tpr=QueryServPrice(scomm,cobj))
    if(mappingp(tpr)) // old price must not be an integer
      {
        tpr+=([cobj:price]);
        SetServPrice(scomm,tpr);
      }
  else 
    {
      tpr=([cobj,price]);
      SetServPrice(scomm,tpr);
    }
}

void RemoveServPrice(string scomm,string cobj)
{
  mixed tpr;
  if(tpr=QueryServPrice(scomm,cobj))
  if(mappingp(tpr)) // old price must not be an integer
    {
      if (member(tpr,cobj))
        tpr-=([cobj]);
      SetServPrice(scomm,tpr);
    }
}

// public string SetSell(string verb) { return Pverb_sell = verb; }
// public string QuerySell() { return Pverb_sell; }

// xprops-Problembereiniger wegen Query-Funktionen..
public string QueryMarket() { return 0; }

public string SetList(string verb) { return Pverb_list = verb; }
public string QueryList() { return Pverb_list; }

public string SetShow(string verb) { return Pverb_show = verb; }
public string QueryShow() { return Pverb_show; }

public varargs string SkShort(int cap)
{
  if (cap) {
    if (({int})ME->QueryNoShopkeeper()) return "The shopkeeper";
    return capitalize(({string})ME->QueryShort());
  }
  if (({int})ME->QueryNoShopkeeper()) return "the shopkeeper";
  return ({string})ME->QueryShort();
}

protected object|int GetShopObject(string shopobj)
// Gets the object shopobj from the shop
// This must be the shop where the shopkeeper resides
{
  object shop;
  object found;
  string msg;

  shop=MYSHOP;
  if (!shop || member(inherit_list(ME),"std/shop.c")!=-1)
    {
      return notify_fail(
        lambda(0,
          ({SF(SkSay),
            "Oh sorry, I'm not in my shop and can't help you further.\n"
          })),
        NOTIFY_NOT_VALID);
    }
  found = search(({object})shop->GetStore(), shopobj,
                  SM_LOCATE|SM_OBJECT,
                  symbol_function("CheckList",({object})shop->GetStore()));
  if (!found)
    {
      msg = "I don't have "+shopobj+" to sell.\n";
      return notify_fail(
        lambda(0,
          ({SF(SkSay),
            msg
          })),NOTIFY_NOT_VALID);
    }

  if (!shopobjects) shopobjects=({});
  shopobjects+=({found});
  return found;
}

protected varargs int buy_routine(object ob,int force)
// std/shop: function gives object to player
// here: object will be destructed if P_NORMV is not set
{
  object shop;

  shop=MYSHOP;
  if (!shop || member(inherit_list(ME),"std/shop.c")!=-1)
    {
      return notify_fail(
        lambda(0,
          ({SF(SkSay),
            "Oh sorry, I'm not in my shop and can't help you further.\n"
          })),
        NOTIFY_NOT_VALID);
    }

  // Test coins of customer.
  if (!({int})shop->check_money(({int})shop->GetPrice(ob), 1,force))
    return 0;

  // Get coins from customer.
  if(!({int})shop->check_money(({int})shop->GetPrice(ob),0,force))
    return 0;

  return 1;
}

protected int pay_service(string scomm,string cobj,int force)
// This is called to pay the service and the objects (if any) from the shop
// it is called after the service has been performed
{
  int i;
  object shop;

  // Try to "buy" objects from shop
  if(shopobjects)
    for (i=0; i<sizeof(shopobjects); i++)
      if (!buy_routine(shopobjects[i])) return 0;

  // We could "buy" all objects, now lets pay the service

  shop=MYSHOP;
  if (!shop || member(inherit_list(ME),"std/shop.c")!=-1)
    {
      return notify_fail(
        lambda(0,
          ({SF(SkSay),
            "Oh sorry, I'm not in my shop and can't help you further.\n"
          })),
        NOTIFY_NOT_VALID);
    }

  // Test coins of customer.
  if (!({int})shop->check_money(QueryServPrice(scomm,cobj), 1,force))
    return 0;

  // Get coins from customer.
  if(!({int})shop->check_money(QueryServPrice(scomm,cobj),0,force))
    return 0;

   // Finally clean up
   for (i=0; i<sizeof(shopobjects); i++)
     if (!({int})shopobjects[i]->Query(P_NORMV))
       shopobjects[i]->remove();  // destruct object, it is used to form another object

   return 1;
}

public object|int do_service(string str2)
// This is called when the customer calls a service
{
  string h,str1;
  int force;
  object ob;

  str1=query_verb();
  if (str2&&sscanf(str2,"really %s",h))
    {
      str2 = h;
      force = 1;
    }
  Say(CustomerName(1)+" wants to "+(GetServCommand(str1))+" "+
      (str2||"something")+".\n",({TP}),1);

  if (!str2 || str2 == "")
    return notify_fail(
      lambda(0,
        ({SF(SkSay),
          "What do you want to "+(GetServCommand(str1))+"?\n"
        })),
      NOTIFY_NOT_VALID),0;
  str2 = norm_id( str2 ); // norm first

  if (GetServCommand(str1)) // find out if the service exists
    {
      ob=Service(str1,str2,force); // Perform service
      if (!ob)
        return notify_fail(
          lambda(0,
            ({SF(SkSay),
               "I can't perform the service "+GetServCommand(str1)+" for you\n"
            })),
          NOTIFY_ILL_ARG),0;

       // Check if player can carry the item.
       if(!({int})TP->MayAddWeight(({int})ob->QueryWeight())) {
         if (!member(shopobjects,ob)) ob->remove(); // no shopobject
         return notify_fail(
           lambda(0,
             ({SF(SkSay),
               CustomerName()+", you can't carry that much.\n"
             })),NOTIFY_NOT_VALID);
       }
       if (!pay_service(str1,str2,force)) // Try to pay service (and objects)
       {
         ob->remove(); // clean up
         return 0;
       }

       if (!ob) // If someone forgets to set P_NORMV (happened to me :-( )
         return notify_fail(
          lambda(0,
            ({SF(SkSay),
              CustomerName()+", something went wrong. Please inform a Wizard.\n"
            })),NOTIFY_NOT_VALID);

       ob->move(TP,M_SILENT);
       return ob;
     }
  else
    {
      return notify_fail(
          lambda(0,
            ({SF(SkSay),
               "I don't offer the service "+str1+" for you\n"
            })),
          NOTIFY_NOT_VALID);
    }
}

public int do_show(string scomm)
{
  string h;

  if(scomm=="service")
    return SkSay("What service should I "+QueryShow()+" you?\n"),1;

  if (scomm&&sscanf(scomm,"service %s",h))
    scomm = h;
  else return 0;

  if (!GetServCommand(scomm))
    return SkSay("Which service do you want to be shown ?.\n"),1;

  Say(CustomerName(1)+" wants "+(SkShort()||"someone")+" to "+
      (QueryShow()||"show service")+" "+(scomm||"something")+".\n",({TP}),1);

  if (!QueryShow())             // you can't show anything in here
    return SkSay("I don't show my services.\n"),1;


  Write(SkShort(1)+" shows service "+scomm+" to you:\n"+QueryServDesc(scomm));
  Say(SkShort(1)+" shows service"+scomm+" to "+CustomerName()+".\n",({TP}));
  return 1;
}

public int do_list(string str)
// Called whenever a customer types 'list service(s)' in here.
{
  int j,ct;
  string short,linelist,*idx,*idy,*res;

  if (!str || (str!="service" && str!="services")) return  0;
  dtell("suamor",str+"\n");
  Say(CustomerName(1)+" asks "+(SkShort()||"someone")+
      " for the service list.\n",({TP}),1);

  if (!QueryList())	// you can't call for a list in here
    return notify_fail(
      lambda(0,
        ({SF(SkSay),
          "I have no list for you.\n"
        })),
          NOTIFY_NOT_VALID),0;

  if (!QueryServices()||!sizeof(QueryServices()))
    {
      SkSay(str?"Sorry "+CustomerName()+
            ", I have no services to offer.\n"
            :"Sorry "+CustomerName()+", I have no item in the store.\n");
      return 0;
    }
  Say(SkShort(1)+" shows a service list to "+CustomerName()+".\n",({TP}));
  if (({int})TP->CanSee())
    {
      linelist="";
      idx = sort_array(m_indices(QueryServices()),SF(>));
      ct=1;
      for (i=0;i<sizeof(idx);i++)
      {
        if (mappingp(QueryServPrice(idx[i],0)))
          {
            idy=sort_array(m_indices(QueryServPrice(idx[i],0)),SF(>));
            for (j=0;j<sizeof(idy);j++)
            {
              short=idx[i]+" "+idy[j];
              linelist+=({string})STR->wrap(" "+({string})STR->radjcut(to_string(ct), 3)+". "+
               "  "+({string})STR->ladjcut(strip_article(short),20)
                   +({string})STR->radjcut(to_string(convert_value(
                             QueryServPrice(idx[i],idy[j]))),6)+"\n",39,14);
              ct++;
            }
          }
        else
          {
            short=idx[i];
            linelist+=({string})STR->wrap(" "+({string})STR->radjcut(to_string(ct), 3)+". "+
                  "  "+({string})STR->ladjcut(strip_article(short),20)+
                  ({string})STR->radjcut(to_string(convert_value(
                             QueryServPrice(idx[i],0))),6)+"\n",39,14);
            ct++;
          }
      }
        res=explode(linelist,"\n");
     //  dtell("suamor",linelist);
      res = ({capitalize(SkShort(1)) +
           " shows a list to you (all prices in "+get_min_coin()+"):","",
           "  <nr> <service>             <cost>"})
	+res+
        ({"","I trade with:",
          "  "+print_list(QueryValuta())});
    }
  else
    res = ({SkShort(1)+" does something."});
  STR->smore(res,({int})TP->QueryPageSize());
  return 1;
}

string do_shrug()
{
  if (member(inherit_list(environment(this_player())), "std/shop.c")==-1)
    return 0; // Shopkeeper is not in the shop
  return "The joiner says: Sorry, "+({string})environment(ME)->CustomerName()+", I don't offer such an service.\n";
}

void add_comm(string comm)
// Add the service commands
{
  add_action("do_service",comm,1); // Same action for every service, Service()
                                 // executes the real service functions.
}

void create()
{
  ::create();
  SetName("shopkeeper");
  AddId("shop keeper");
  SetShort("the shopkeeper");
  SetLong(
"This is the shopkeeper. He trades with you and may offer you\n\
some additional services, just try to ask for them. He also can\n\
'list' you all his services and 'show' you a description to them\n"
  );
  SetShrugMsg(#'do_shrug);
  SetRace("human"); /* Should be halfling, shouldn't it ? :-) */
  SetGender(GENDER_MALE);
  AddQuestion(({"buy","buying","sell","selling","appraise","appraising","identify","identifying"}),
"Read the sign at the counter for more information.\n");

  AddQuestion(({"list","listing"}),
"'list service' shows you a list of services which I offer to you.");
  AddQuestion(({"show","showing"}),
"'show service <service>' shows you a description of the service <service>");
  AddQuestion(({"service","services"}),
"The shopkeeper says: Sorry, these lazy wizards did not implement any services for you :-("
  );
  SetBuyFactor(300);
  SetDynamic(DYNAMIC_DEFAULT);
  SetMinCash(DEFAULT_MIN_CASH);
  SetCash(DEFAULT_MIN_CASH);
  SetList("list");
  SetShow("show");
}

void init()
{
  int i;

  ::init();
  if (serv_list)  walk_mapping(serv_list, "add_comm"); // "Install" services
  add_action("do_list",QueryList()||"list");
  add_action("do_show",QueryShow()||"show");
}
