/*--------------------------------------------------------------------------
 * /p/money/obj/designer/viewfuns.c -- part of the money designer
 *
 * Copyright (C) 1997 by One Step Beyond.
 * All Rights Reserved.
 * Permission granted for non-commercial use.
 *--------------------------------------------------------------------------
 */

#include <money.h>

inherit "/lib/string";

static int     security();
static void    restore_database();

static string  SetLastCoin(string s);
static mapping SetCoinTypes(mixed s);
static mapping SetDomains(mixed s);
static mapping SetMaterials(mixed s);
static string *SetStandard(mixed s);

public mapping QueryMaterials();
public mapping QueryCoinTypes();
public mapping QueryDomains();

private int mode;

static int get_val(string type)
{
  mixed coin;
  int   val, i;
  
  coin = QueryCoinTypes()[type,MONEY_CONTENTS];
  if(!type) val = 0;
  else for(i = sizeof(m_indices(coin));i--;)
    val += coin[m_indices(coin)[i]] * QueryMaterials()[m_indices(coin)[i]] / 32;
  return val;
}

static int get_weight(string type)
{
  mixed coin;
  int   val, i;
  
  if(!type) return 0;
  coin = QueryCoinTypes()[type,MONEY_CONTENTS];
  for(i = sizeof(m_indices(coin));i--;)
    val += coin[m_indices(coin)[i]];
  return val;
}

static string format_material(string material, string type)
{
  return sprintf("  %10s: %d grams",
		 material,QueryCoinTypes()[type,MONEY_CONTENTS][material]);
}

static string single_coin(string type)
{
  return sprintf("%-14s %-19s %-24s %-5s %13d\n",
		 type,
		 "1 " + (QueryCoinTypes()[type,MONEY_COINTYPE]?
			 type + " " + QueryCoinTypes()[type,MONEY_COINTYPE]:
			 type),
		 "10 " + (QueryCoinTypes()[type,MONEY_PLURAL]?
			  (QueryCoinTypes()[type,MONEY_COINTYPE]?
			   type + " " + QueryCoinTypes()[type,MONEY_PLURAL]:
			   QueryCoinTypes()[type,MONEY_PLURAL]):
			  type),
		 "1" + (QueryCoinTypes()[type,MONEY_SHORT]||type[0..1]),
                 get_val(type));
}

static string get_material_config(string mat)
{
  return sprintf("%-19s %10d             %8s    %8s",
		 mat, QueryMaterials()[mat],
		 sprintf("%.5f",to_float(QueryMaterials()[mat])/32.0),
		 QueryMaterials()[mat]?
		 sprintf("%.5f",32.0/QueryMaterials()[mat]):" - ");
}

static string get_domain_list(string domain)
{
  string *coins;

  return (coins = QueryDomains()[domain])?
          sprintf("%-14s %s",capitalize(domain),implode(coins,", ")):0;
}

private int sort_up(mixed a, mixed b)
{
  switch(mode) {
  case 0: return a > b;
  case 1: return QueryMaterials()[a] > QueryMaterials()[b];
  default: return get_val(a) > get_val(b);
  }
  return get_val(a) > get_val(b);
}


private int sort_down(mixed a, mixed b)
{
  switch(mode) {
  case 0: return a < b;
  case 1: return QueryMaterials()[a] < QueryMaterials()[b];
  default: return get_val(a) < get_val(b);
  }
  return get_val(a) < get_val(b);
}

public int show_money(string str)
{
  int      justload;
  string  *args;
  closure  sortfun;

  if(!security()) return 0;
  if(!QueryCoinTypes()) {
    restore_database();
    justload = 3;
  }
  if(!str) args = ({"money"});
  else args = explode(str," ");
  switch(args[<1]) {
  case "up":
    sortfun = #'sort_up;
    args[<1] = 0;
    break;
  case "down":
    sortfun = #'sort_down;
    args[<1] = 0;
    break;
  case "val":
  case "value":
    sortfun = #'sort_down;
    break;
  default:
    sortfun = #'sort_up;
  } /* end switch */
  args += ({0});
  switch(args[1]) {
  case "val":
  case "value":
    if(args[0][0..5] == "domain") {
      notify_fail("Domains have no value to sort them.\n");
      return 0;
    }
    mode = (args[0] == "materials"||args[0] == "material")?1:2;
    break;
  case "name":
    mode = 0;
    break;
  } /* end switch */
  switch(args[0]) {
  case "val":
  case "value":
    mode = 2;
  case "money":
    smore("\
Cointype       Singular            Plural                   Short         \
Value\n" + implode(map(sort_array(m_indices(QueryCoinTypes()),sortfun),
                             #'single_coin),""));
    break;
  case "material":
  case "materials":
    mode|=1;
    smore("\
Name                Value                  Value      Weight for\n\
                    per once (32 grams)    per gram   a value of 1\n" +
          implode(map(sort_array(m_indices(QueryMaterials()),sortfun),
                            #'get_material_config),"\n") + "\n");
    break;

  case "domain":
  case "domains":
    if(!QueryDomains() || !sizeof(QueryDomains()))
      write("No coins are associated to any domain.\n");
    else smore("\
Domain         Cointypes\n" +
	       implode(map(sort_array(m_indices(QueryDomains()),sortfun),
                                 #'get_domain_list)-({0}),"\n") + "\n");
    break;

  default:
    if(QueryCoinTypes()[str]) {
      smore(sprintf("Type:     %s\nSingular: %s\nPlural:   %s\nShort:    %s\n\
Value:    %d\nWeigth:   %d\nLong:\n%s\nContents:\n%s\n",
		    str,QueryCoinTypes()[str,MONEY_COINTYPE]?
		    str + " " + QueryCoinTypes()[str,MONEY_COINTYPE]:str,
		    QueryCoinTypes()[str,MONEY_PLURAL]?
		    (QueryCoinTypes()[str,MONEY_COINTYPE]?
		     str + " " + QueryCoinTypes()[str,MONEY_PLURAL]:
		     QueryCoinTypes()[str,MONEY_PLURAL]):str,
		    QueryCoinTypes()[str,MONEY_SHORT]||str[0..1],
		    get_val(str),
		    get_weight(str),
		    QueryCoinTypes()[str,MONEY_LONG]||"- none -\n",
		    implode(map(m_indices(QueryCoinTypes()[str,
								 MONEY_CONTENTS])
				      ,#'format_material,
				      str),"\n")));
      SetLastCoin(str);
    } else {
      notify_fail("Unrecognized argument: " + str + ".\n");
      justload &= 1;
    }
  }
  if(justload&1) {
    SetCoinTypes(0);
    SetMaterials(0);
    SetDomains(0);
    SetStandard(0);
  }
  return justload&2;
}
