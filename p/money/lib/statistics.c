/*--------------------------------------------------------------------------
 * /p/money/lib/statistics -- some statistic funs
 *
 * Copyright (C) 1997 by One Step Beyond.
 * All Rights Reserved.
 * Permission granted for non-commercial use.
 *--------------------------------------------------------------------------
 */

#include "/p/money/sys/statistics.h"

#define SF(x) #'x //'

inherit "/lib/string";

public mapping Pstatistics;
// (["armour":number_of_tests,
//   ([COST_PER_AC:float,
//     MAX_COST_PER_AC:float,
//     MAX_COST_PER_AC_FILE:string,
//     MIN_COST_PER_AC:float,
//     MIN_COST_PER_AC_FILE:string,
//     AC_AVARAGE_PRICE:([ac:([AV_NUMBER:,AV_PRICE,AV_MAX,AV_MAX_FILE,
//                             AV_MIN,AV_MIN_FILE])])
//   ]);

protected float get_delta(float avarage,int old_nr_tests,float new_value)
  // Returns the change of the current calculated avarage with the
  // new value. 'old_nr_tests' is the number that have been made so
  // far (without the test counted, which resulted in the new value).
  // Formula:
  //
  // delta = 1/n+1 * (a_n+1 - Sum(1,n,a_i)/n)
{
  return (new_value-avarage)/(old_nr_tests+1.0);
}

public void SaveData()
{
  seteuid(getuid());
  save_object(LOG_FILE);
  seteuid(0);
}

public void LoadData()
{
  if (   Pstatistics
      || file_size(LOG_FILE+".o")<=0
     )
    return;
  seteuid(getuid());
  restore_object(LOG_FILE);
  seteuid(0);
}

public mapping QueryStatistics()
{
  LoadData();
  return (Pstatistics||EMPTY)+EMPTY;
}

public void AddArmour(string file,int value,int ac)
{
  int nrtests;
  float cost_per_ac;
  mapping results,prices;
  // AC=0 isn't of any interest and it would lead to 'mathmatical problems'.
  if (!ac) return;
  LoadData();
  if (!Pstatistics) Pstatistics = EMPTY;
  nrtests = Pstatistics[K_ARMOUR,S_NUMBER_TESTS];
  results = Pstatistics[K_ARMOUR,S_RESULTS]||([]);
  prices  = results[AC_AVARAGE_PRICE]||AV_EMPTY;
  cost_per_ac = value*1.0/ac;
  // Change the total avarage:
  results[COST_PER_AC]=results[COST_PER_AC]+
    get_delta(results[COST_PER_AC],nrtests,cost_per_ac);
  // Check maximum cost_per_ac:
  if (results[MAX_COST_PER_AC]*1.0<cost_per_ac)
    {
      results[MAX_COST_PER_AC] = cost_per_ac;
      results[MAX_COST_PER_AC_FILE] = file;
    }
  // Check minimum cost_per_ac:
  if (   !results[MIN_COST_PER_AC]
      || results[MIN_COST_PER_AC]*1.0>cost_per_ac
     )
    {
      results[MIN_COST_PER_AC] = cost_per_ac;
      results[MIN_COST_PER_AC_FILE] = file;
    }

  prices[ac,AV_PRICE]=prices[ac,AV_PRICE]+
    get_delta(prices[ac,AV_PRICE],
	      prices[ac,AV_NUMBER],
	      value*1.0);
  prices[ac,AV_NUMBER]++;
  if (value>prices[ac,AV_MAX])
    {
      prices[ac,AV_MAX] = value;
      prices[ac,AV_MAX_FILE] = file;
    }
  if (value<prices[ac,AV_MIN])
    {
      prices[ac,AV_MIN] = value;
      prices[ac,AV_MIN_FILE] = file;
    }
  results[AC_AVARAGE_PRICE] = prices;
  Pstatistics[K_ARMOUR,S_NUMBER_TESTS]++;
  Pstatistics[K_ARMOUR,S_RESULTS] = results;
  SaveData();
}

public void AddWeapon(string file,int value,int wc)
{
  int nrtests;
  float cost_per_wc;
  mapping results,prices;
  // WC=0 isn't of any interest and it would lead to 'mathmatical problems'.
  if (!wc) return;
  LoadData();
  if (!Pstatistics) Pstatistics = EMPTY;
  nrtests = Pstatistics[K_WEAPON,S_NUMBER_TESTS];
  results = Pstatistics[K_WEAPON,S_RESULTS]||([]);
  prices  = results[WC_AVARAGE_PRICE]||AV_EMPTY;
  cost_per_wc = value*1.0/wc;
  // Change the total avarage:
  results[COST_PER_WC]=results[COST_PER_WC]+
    get_delta(results[COST_PER_WC],nrtests,cost_per_wc);
  // Check maximum cost_per_ac:
  if (results[MAX_COST_PER_WC]*1.0<cost_per_wc)
    {
      results[MAX_COST_PER_WC] = cost_per_wc;
      results[MAX_COST_PER_WC_FILE] = file;
    }
  // Check minimum cost_per_ac:
  if (   !results[MIN_COST_PER_WC]
      || results[MIN_COST_PER_WC]*1.0>cost_per_wc
     )
    {
      results[MIN_COST_PER_WC] = cost_per_wc;
      results[MIN_COST_PER_WC_FILE] = file;
    }
  prices[wc,AV_PRICE]=prices[wc,AV_PRICE]+
    get_delta(prices[wc,AV_PRICE],
	      prices[wc,AV_NUMBER],
	      value*1.0);
  prices[wc,AV_NUMBER]++;
  if (value>prices[wc,AV_MAX])
    {
      prices[wc,AV_MAX] = value;
      prices[wc,AV_MAX_FILE] = file;
    }
  if (value<prices[wc,AV_MIN])
    {
      prices[wc,AV_MIN] = value;
      prices[wc,AV_MIN_FILE] = file;
    }
  results[WC_AVARAGE_PRICE] = prices;
  Pstatistics[K_WEAPON,S_NUMBER_TESTS]++;
  Pstatistics[K_WEAPON,S_RESULTS] = results;
  SaveData();
}

public string ArmourReport()
{
  string msg;
  mapping results,prices;
  int nrtests,*acs,i;

  LoadData();
  if (!Pstatistics) Pstatistics = EMPTY;
  nrtests = Pstatistics[K_ARMOUR,S_NUMBER_TESTS];
  results = Pstatistics[K_ARMOUR,S_RESULTS]||([]);
  prices  = results[AC_AVARAGE_PRICE]||EMPTY;

  msg =
    "Armours\n-------\n\n"
    "  Entries:      "+sprintf("%10d\n",nrtests)+
    "  Cost per AC:  "+sprintf("%10.2f\n",results[COST_PER_AC]*1.0)+
    "  Maximum CpAC: "+sprintf("%10.2f\n",results[MAX_COST_PER_AC]*1.0)+
    "  ("+(results[MAX_COST_PER_AC_FILE]||"<none>")+")\n"
    "  Minimum CpAC: "+sprintf("%10.2f\n",results[MIN_COST_PER_AC]*1.0)+
    "  ("+(results[MIN_COST_PER_AC_FILE]||"<none>")+")\n\n"
    "    +------+---------+--------------+\n"
    "    |  AC  | Entries | Avarage Cost | Maximum amount (file)\n"
    "    +------+---------+--------------+\n";
  i = sizeof(acs = sort_array(m_indices(prices),SF(<)));
  while(i--)
    msg+=sprintf("    | %4d | %7d | %12.2f | %s\n",
		 acs[i],prices[acs[i],S_NUMBER_TESTS],
		 prices[acs[i],AV_PRICE],prices[acs[i],AV_MAX_FILE]||"");
  msg+=
    "    +------+---------+--------------+\n";
  return msg;
}

public string WeaponReport()
{
  string msg;
  mapping results,prices;
  int nrtests,*wcs,i;

  LoadData();
  if (!Pstatistics) Pstatistics = EMPTY;
  nrtests = Pstatistics[K_WEAPON,S_NUMBER_TESTS];
  results = Pstatistics[K_WEAPON,S_RESULTS]||([]);
  prices  = results[WC_AVARAGE_PRICE]||EMPTY;

  msg =
    "Weapons\n-------\n\n"
    "  Entries:      "+sprintf("%10d\n",nrtests)+
    "  Cost per WC:  "+sprintf("%10.2f\n",results[COST_PER_WC]*1.0)+
    "  Maximum CpWC: "+sprintf("%10.2f\n",results[MAX_COST_PER_WC]*1.0)+
    "  ("+(results[MAX_COST_PER_WC_FILE]||"<none>")+")\n"
    "  Minimum CpWC: "+sprintf("%10.2f\n",results[MIN_COST_PER_WC]*1.0)+
    "  ("+(results[MIN_COST_PER_WC_FILE]||"<none>")+")\n\n"
    "    +------+---------+--------------+\n"
    "    |  WC  | Entries | Avarage Cost | Maximum amount (file)\n"
    "    +------+---------+--------------+\n";
  i = sizeof(wcs = sort_array(m_indices(prices),SF(<)));
  while(i--)
    msg+=sprintf("    | %4d | %7d | %12.2f | %s\n",
		 wcs[i],prices[wcs[i],S_NUMBER_TESTS],
		 prices[wcs[i],AV_PRICE],prices[wcs[i],AV_MAX_FILE]||"");
  msg+=
    "    +------+---------+--------------+\n";
  return msg;
}

public void AddItem(object ob)
{
  if (!ob) return;
  if (member(inherit_list(ob),"std/armour.c"))
    AddArmour(blueprint(ob),({int})ob->QueryValue(),({int})ob->QueryAC());
  if (member(inherit_list(ob),"std/weapon.c"))
    AddWeapon(blueprint(ob),({int})ob->QueryValue(),({int})ob->QueryWC());
}

public void GenerateReport()
{
  string msg;
  LoadData();
  seteuid(getuid());
  if (file_size(REPORT_FILE)>0)
    rename(REPORT_FILE,REPORT_FILE+".old");

  msg = "Report about finance statistics, "+ctime();
  msg+= "\n"+mkstr(sizeof(msg),"=")+"\n\n";
  msg+= ArmourReport()+"\n";
  msg+= WeaponReport()+"\n";

  write_file(REPORT_FILE,msg);
  seteuid(0);
}
