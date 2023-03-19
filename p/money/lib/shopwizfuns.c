/*--------------------------------------------------------------------------
 * /p/money/lib/showwizfuns
 *
 * Copyright (C) 1997 by One Step Beyond.
 * All Rights Reserved.
 * Permission granted for non-commercial use.
 *--------------------------------------------------------------------------
 * This file is called by the shops in the whole mud, 
 * if you want to do a special wizardcommand.
 */

inherit "/lib/string";

#include <driver/functionlist.h>
#include <money.h>
#include <config.h>
#include <moving.h>
#include <combat.h>
#include <properties.h>

#define REGISTER "/obj/lib/register"
#define LONGLIST "/open/finance/SHOPCHECK"
#define SHORTLIST "/open/finance/SHOPLIST"
#define SHOPITEMS "/open/finance/SHOPITEMS"
#define SHOPPROFILE "/open/finance/SHOPPROFILE"

#define TP this_player()
#define SF(x) #'x //'
#define SYM(x) 'x //'
#define CL_BRACK #'[ //'

#define I_ARMOUR "std/armour.c"
#define I_WEAPON "std/weapon.c"
#define ITEMS ({I_ARMOUR,I_WEAPON})
#define POS_ARMOUR 0
#define POS_WEAPON 1
#define I_ENTRIES 4
#define I_COUNT 0
#define I_MIN 1
#define I_MAX 2
#define I_AVG 3

private nosave mapping items;
// ([<this_player>:([<what>:([<awtype>:([<ac/wc>:<count>;<minvalue>;<maxvalue>;<avgvalue>])])])

protected object check_shop(string shop,string error)
// Returns the objectpointer of the shop or 0, if any
// error occured. <error> will contain the error-message
// if called by reference.
{
  object result;
  if (!error = catch(result = load_object(shop)))
    if (result&&member(inherit_list(result),"std/shop.c")==-1)
      error = "This file is no shop.\n";
  return result;
}

public void create()
{
  // don't clone this item
  if (is_clone())
    destruct(this_object());
  seteuid(getuid());
}

public varargs string analyze_shop(string ob,status err)
// Called in the wiz_funcs to analyze a special shop, given in the shop-
// register. It returns 1 if all is fine and 0 on error, i. e. we should
// remove this shop from the list.
{
  string error,result;
  object shop,store;
  int min,dynamic;

  err = 0;
  if (   file_size(ob)<1
      && file_size(ob+".c")<1
     )
    {
      err = 1;
      return "The shop doesn't exist anymore!\n";
    }
  if (!shop = check_shop(ob,&error))
    return "Error: "+error+"\n";
  else
    result =
      (({string})shop->QueryIntShort()||"<nameless room>")+".\n"+
      "Cash: "+radjust(to_string(({int})shop->QueryCash()),8)+
        " CU, Minimal cash: "+
        radjust(to_string(min = ({int})shop->QueryMinCash()),8)+" CU.\n"+
      "Buy factor: "+radjust(to_string(({int})shop->QueryBuyFactor()),4)+"  "+
        "Dynamic factor: "+radjust(to_string(dynamic = ({int})shop->QueryDynamic()),3)
        +"\n"
      "Coins per Reset (if < MinCash): "+
        radjust(to_string(min*dynamic/100),8)+" CU\n"+
      "List: "+(({string})shop->QueryList()?"yes":"no")+"  "+
        "Ident.: "+(({string})shop->QueryIdentify()?"yes":"no")+"  "+
        "Appr.: "+(({string})shop->QueryAppraise()?"yes":"no")+"  "+
        "Sell: "+(({string})shop->QuerySell()?"yes":"no")+"  "+
        "Buy: "+(({string})shop->QueryBuy()?"yes":"no")+"  "+
        "Liv.: "+(({int})shop->QueryLivingTrade()?"yes":"no")+"\n";

  if (shop&&(store = ({object})shop->GetStore())
      &&!sizeof(map(functionlist(store,RETURN_FUNCTION_FLAGS),
                          lambda(({SYM(x)}),
                                 ({SF(!),
                                   ({SF(&),SYM(x),NAME_INHERITED})
                                 })
                                ) )-({0}))
     )
    result+="Obsolete store, because no functions defined in it.\n";

  return result;
}

public string int_short(string file)
{
  object shop;
  string error;
  
  if (shop = check_shop(file,&error))
    return (({string})shop->QueryIntShort()||"<no intshort>");
  return error;
}

protected string size2string(int size)
{
  switch(size)
    {
     case PSIZE_SMALL:
      return "small";
     case PSIZE_NORMAL:
      return "normal";
     case PSIZE_LARGE:
      return "large";
     case PSIZE_GENERIC:
      return "generic";
     default:
      return "unknown";
    }
}

protected string wtype2string(int wtype)
{
  switch(wtype)
    {
     case WT_SWORD:
      return "WT_SWORD";
     case WT_AXE:
      return "WT_AXE";
     case WT_CLUB:
      return "WT_CLUB";
     case WT_SPEAR:
      return "WT_SPEAR";
     case WT_KNIFE:
      return "WT_KNIFE";
     case WT_STAFF:
      return "WT_STAFF";
     case WT_WHIP:
      return "WT_WHIP";
     case WT_INFANTRY:
      return "WT_INFANTRY";
     case WT_CHAIN:
      return "WT_CHAIN";
     default:
      return "<unknown>";
    }
}

protected string atype2string(int atype)
{
  switch(atype)
    {
     case AT_SKIN:
      return "AT_SKIN";
     case AT_RING:
      return "AT_RING";
     case AT_AMULET:
      return "AT_AMULET";
     case AT_BOOTS:
      return "AT_BOOTS";
     case AT_CLOAK:
      return "AT_CLOAK";
     case AT_MAIL:
      return "AT_MAIL";
     case AT_GLOVES:
      return "AT_GLOVES";
     case AT_HELMET:
      return "AT_HELMET";
     case AT_SHIELD:
      return "AT_SHIELD";
     case AT_TROUSERS:
      return "AT_TROUSERS";
     default:
      return "<unknown>";
    }
}

protected string itemcheck(object ob)
// Checks an item in the shop especially some which inherit certain
// /std-files. Called by shop_itemlist().
{
  string result,*inh,cr;
  object cob;
  if (!ob) return "<destructed>\n";
  result = strip_article(({string})ob->Short())+"\n"
           "  File: "+blueprint(ob)+".c\n";
  inh = inherit_list(ob);
  if (member(inh,"std/weapon.c")!=-1)
    {
      result +=
        "  WType: "+wtype2string(({int})ob->QueryWeaponType())+
        ", WC: "+({int})ob->QueryWC()+
        ", Hands: "+({int})ob->QueryNumberHands()+"\n"
	"  Size: "+size2string(({int})ob->QuerySize())+"\n";
    }
  if (member(inh,"std/armour.c")!=-1)
    {
      result +=
        "  AType: "+atype2string(({int})ob->QueryArmourType())+
        ", AC: "+({int})ob->QueryAC()+
        ", Hands: "+({int})ob->QueryNumberHands()+"\n"
        "  Size: "+size2string(({int})ob->QuerySize())+"\n";
    }
  if ((member(inh,"std/food.c") !=-1)
      ||(member(inh,"std/drink.c")!=-1))
    {
      result +=
        "  Foodkind: "+({string})ob->QueryFoodKind()+
        "  Stuff: "+({int})ob->QueryFoodStuff()+
        "  Alc: "+({int})ob->QueryFoodAlc()+
        "  Soak: "+({int})ob->QueryFoodSoak()+
        "  Heal: "+({int})ob->QueryFoodHeal()+"\n";
    }
  cr = ({string})ob->QueryCreator();
  if (  (cob = find_object(cr))
      &&({int})cob->id_store())
    if (cob==environment(ob))
      cr = "Shop";
    else
      cr = ({string})cob->QueryCreator();
  result+=
    "  Creator: "+cr+"\n"
    "  Weight: "+({int})ob->QueryWeight()+", Value: "+({int})ob->QueryValue()+"\n";
  return result; 
}

public string itemlist(string file)
{
  string error;
  object shop,store;

  if (!shop = check_shop(file,&error))
    return "Shop-Error: "+error+"\n";
  if (error = catch(store = ({object})shop->GetStore()))
    return "Store-Error: "+error+"\n";
  if (!store)
    return "Shop has no store defined.\n";
  return implode(map(map(m_values(({mapping})store->QueryInventory()||([])),
                                     lambda(({SYM(s)}),
                                            ({CL_BRACK,SYM(s),0})
                                           )
                                    ),SF(itemcheck)
                          ),"\n");
}

protected void write_list(mapping res)
// res = ([<type>:([<awtype>:([<ac/wc>:<count>;<minvalue>;<maxvalue>;<avgvalue>])])])
{
  string *types;
  mixed *awtypes;
  int *acwcs,t_count,awt_count,acwc_count;
  t_count = sizeof(types = m_indices(res));
  seteuid(getuid());
  rm(SHOPPROFILE);
  while(t_count--)
    {
      write_file(SHOPPROFILE,types[t_count]+"\n");
      awt_count = sizeof(awtypes = m_indices(res[types[t_count]]));
      while(awt_count--)
	{
          write_file(SHOPPROFILE,"  "+awtypes[awt_count]+"\n");
	  acwc_count = sizeof(acwcs = sort_array(
            m_indices(res[types[t_count]][awtypes[awt_count]]),SF(<)));
          while(acwc_count--)
	    {
	      write_file(SHOPPROFILE,
                sprintf("    AC/WC: %2d (%3d pieces)\n"
                        "      Min: %4d CU, Max: %4d CU, Avg: %4d CU\n",
                        acwcs[acwc_count],
                        res[types[t_count]][awtypes[awt_count]]
                           [acwcs[acwc_count],I_COUNT],
                        res[types[t_count]][awtypes[awt_count]]
                           [acwcs[acwc_count],I_MIN],
                        res[types[t_count]][awtypes[awt_count]]
                           [acwcs[acwc_count],I_MAX],
                        res[types[t_count]][awtypes[awt_count]]
                           [acwcs[acwc_count],I_AVG]));
	    }
	}
    }
  write("Profile-data stored in " SHOPPROFILE ".\n");
  seteuid(0);
}

public string profile(string file,int last)
{
  string error;
  object store,*inv;
  int i,j,pos,oldq,val,acwc,
      vmin,vmax,vavg,
      old,count;
  mapping plres,awtypes;
  mixed type;
  
  if (!TP) return itemlist(file);
  if (!store = check_shop(file,&error))
    return "Shop-Error: "+error+"\n";
  if (error = catch(store = ({object})store->GetStore()))
    return "Store-Error: "+error+"\n";
  if (!store)
    return "Shop has no store defined.\n";
  inv = map(m_values(({mapping})store->QueryInventory()||([])),
                  lambda(({SYM(s)}),({CL_BRACK,SYM(s),0}))
                 )-({0});
  i = sizeof(inv);
  if (!items) items = ([]);
  if (!member(items,getuid(TP)))
    plres = ([]);
  else
    plres = items[getuid(TP)];
  while(i--)
    {
      j = sizeof(ITEMS);
      while(j--)
        if ((pos = member(inherit_list(inv[i]),ITEMS[j]))!=-1)
          switch(member(ITEMS,inherit_list(inv[i])[pos]))
	    {
	     case POS_ARMOUR:
	      oldq = ({int})inv[i]->QueryQuality();
	      inv[i]->SetQuality(100);
	      val = ({int})inv[i]->QueryValue();
              acwc = ({int})inv[i]->QueryAC();
              type = atype2string(({int})inv[i]->QueryArmourType());
	      inv[i]->SetQuality(oldq);
	      if (!member(plres,I_ARMOUR))
                plres[I_ARMOUR] = ([]);
              if (!member(awtypes = plres[I_ARMOUR],type))
                awtypes[type] = m_allocate(0,I_ENTRIES);
              count = awtypes[type][acwc,I_COUNT];
              old = awtypes[type][acwc,I_MIN];
	      vmin = old?val<old?val:old:val;
	      old = awtypes[type][acwc,I_MAX];
	      vmax = val>old?val:old;
	      old = awtypes[type][acwc,I_AVG];
	      vavg = (old*count+val)/(++count);
	      awtypes[type][acwc,I_COUNT] = count;
	      awtypes[type][acwc,I_MIN] = vmin;
	      awtypes[type][acwc,I_MAX] = vmax;
	      awtypes[type][acwc,I_AVG] = vavg;
	      plres[I_ARMOUR][type] = awtypes[type];
              items[getuid(TP)] = plres;
	      break;
	     case POS_WEAPON:
	      oldq = ({int})inv[i]->QueryQuality();
	      inv[i]->SetQuality(100);
	      val = ({int})inv[i]->QueryValue();
              acwc = ({int})inv[i]->QueryWC();
              type = wtype2string(({int})inv[i]->QueryWeaponType());
	      inv[i]->SetQuality(oldq);
	      if (!member(plres,I_WEAPON))
                plres[I_WEAPON] = ([]);
              if (!member(awtypes = plres[I_WEAPON],type))
                awtypes[type] = m_allocate(0,I_ENTRIES);
              count = awtypes[type][acwc,I_COUNT];
              old = awtypes[type][acwc,I_MIN];
	      vmin = old?val<old?val:old:val;
	      old = awtypes[type][acwc,I_MAX];
	      vmax = val>old?val:old;
	      old = awtypes[type][acwc,I_AVG];
	      vavg = (old*count+val)/(++count);
	      awtypes[type][acwc,I_COUNT] = count;
	      awtypes[type][acwc,I_MIN] = vmin;
	      awtypes[type][acwc,I_MAX] = vmax;
	      awtypes[type][acwc,I_AVG] = vavg;
	      plres[I_WEAPON][type] = awtypes[type];
              items[getuid(TP)] = plres;
	      break;
	    }
    }
  if (last)
    write_list(plres);
  return itemlist(file);
}
  
protected string deep_update(string file)
{
  object ob;
  string *ilist,res;
  int i;
  
  if (ob = find_object(file))
    {
      // Get all inherited files of the current file and destruct all of
      // them to ensure, that the file really gets the new version.
      i = sizeof(ilist = inherit_list(ob));
      while(i--)
        if (ob = find_object(ilist[i]))
          destruct(ob);
    }
  else
    {
      // The shop wasn't loaded up to now, so try to do it.
      // If no error occurs call deep_update again to ensure,
      // that even all inherited files get updated.
      // This is no chance for a 'too deep recursion' unless
      // the object doesn't destruct itself again and again.
      if (res = catch(load_object(file)))
        return res;
      return deep_update(file);
    }
  return (res = catch(load_object(file)))?res:0;
}

public string update(string file)
{
  string error;
  object shop,*pls;
  int i;

  if (error = deep_update("/std/store"))
    return "/std/store: "+error;

  if (shop = find_object(file))
    {
      i = sizeof(pls = filter(deep_inventory(shop),
                 SF(query_once_interactive)));
      while(i--)
        {
          tell_object(pls[i],"The shop gets updated.\n");
          pls[i]->move("/std/void",M_SILENT);
        }
      shop->clean_up(-1);
      if (shop) shop->remove();
      if (shop) destruct(shop);
      i = sizeof(pls);
      if (!shop = check_shop(file,&error))
        {
          while(i--)
            tell_object(pls[i],"An error occured in the shop.\n");
          return "Error: "+error;
	}
      if (!i)
	{
          // Don't keep the (now loaded) shop in memory, if it isn't
          // used by a player right now anyway.
	  shop->clean_up(-1);
          if (shop) shop->remove();
          if (shop) destruct(shop);
          return "Shop destructed as no players present.\n";
	}
      while(i--)
	{
	  pls[i]->move(shop,M_SILENT);
          tell_object(pls[i],"Shop is updated.\n");
	}
    }
  else
    {
      // The shop doesn't exist, so just load it to check for errors and
      // destruct it again.
      if (!shop = check_shop(file,&error))
        return error;
      shop->clean_up(-1);
      if (shop) shop->remove();
      if (shop) destruct(shop);
    }
  return "Update ok.\n";
}

public int startfun(object player,string str,string regfile)
// Called with action 'shop'. These are special commands for wizards to check
// this and other shops.
{
  int i,number;
  string file,*files;
  if (!TP) return 0;
  if (str=="analyze")
    {
      write(analyze_shop(blueprint(previous_object()),&i));
      if (i)
        REGISTER->RemoveEntry(blueprint(previous_object()),regfile);
      return 1;
    }
  if (sscanf(str,"analyze %d",number)==1)
    {
      files = m_indices(({mapping})REGISTER->QueryRegister(regfile));
      if (!number||number>sizeof(files))
        return notify_fail("Range of shops from 1 to "+to_string(sizeof(files))
                           +".\n",NOTIFY_ILL_ARG),0;
      file = files[--number];
      write(analyze_shop(file,&i));
      if (i)
        REGISTER->RemoveEntry(file,regfile);
      return 1;
    }
  if (sscanf(str,"analyze %s",file)==1)
    {
      files = m_indices(({mapping})REGISTER->QueryRegister(regfile));
      file = ({string})TP->get_path(file);
      if (file[<2..]==".c")
        file = file[0..<3];
      if (member(files,file)==-1)
        return notify_fail("No shop "+file+" known to the shopregister.\n",
			   NOTIFY_ILL_ARG),0;
      write(analyze_shop(file,&i));
      if (i)
        REGISTER->RemoveEntry(file,regfile);
      return 1;
    }
  switch(str)
    {
     case "shortlist":
      write("Short list of shops will be evaluated.\n");
      REGISTER->PrintList(regfile,SHORTLIST,SF(int_short));
      break;
     case "list":
      write("Long list of shops will be evaluated.\n");
      REGISTER->PrintList(regfile,LONGLIST,SF(analyze_shop));
      break;
     case "itemlist":
      write("Long list (including articles) of shops will be evaluated.\n");
      REGISTER->PrintList(regfile,SHOPITEMS,SF(itemlist));
      break;
     case "update":
      write("All loaded shops will be updated.\n");
      REGISTER->PrintList(regfile,0,SF(update));
      break;
     case "profile":
      write("Evaluating a profile of all items.\n");
      REGISTER->PrintList(regfile,SHOPITEMS,SF(profile));
      break;
    case "stat":
    case "statistics":
      write("Finance-Statistic-Report will be generated.\n");
      STATISTIC->GenerateReport();
      write("Done. (Report in /log/finance/statistics)\n");
      break;
     default:
      write(
       "Here are some useful 'shop' commands:\n"
       "  help                   This text\n"
       "  shortlist              Short list of shops in file " SHORTLIST "\n"
       "  list                   List of shops in file " LONGLIST "\n"
       "  itemlist               List items of each shop in file "SHOPITEMS"\n"
       "  analyze [<nr>|<file>]  Analyze a special (or current) shop\n"
       "  profile                Creates a profile of all items in the shops\n"
       "                         to get an overview about common values in\n"
       "                         the mud, results in " SHOPPROFILE "\n"
       "  stat[istics]           Evaluate the finance-statistics; they can be\n"
       "                         read in /log/finance/statistics\n"
       "  update                 Update all shops\n");
    }
 return 1; 
}
