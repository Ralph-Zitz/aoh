//#define D
// Enable the define above to be able to get statistics via
// 'QueryResult()' from the guildobject.
//
// guildobj test

inherit "std/armour";
inherit "/obj/lib/guildobj";

#include <config.h>  // NOTIFY_
#include <combat.h>  // AT_, DT_, EQ_
#include <skills.h>  // SK_
#include <races.h>   // RD_

#define SF(x) #'x //'
#define NMB "/lib/number"
#define TP this_player()
#define STR "/lib/string"
#define ME this_object()
#define HOME "/d/woodland/common/guilds/"
#define FIGHTMASTER HOME "fight_master"
#define PALADINS HOME "paladins"

#ifdef D
#define TCHECK 0
#define TSUCCESS 1
#define TFAILED 2
#define ASUCC 3
#define AFAIL 4
#define MAXSUCC 5
#define MAXFAIL 6
#define TADVANCE 7

private static mapping test;
// (["skill":<times checked>;<times success>;<times failed>;<avrg success>;
//           <avrg fail>;<max success>;<max failure>;<times advaned>])

public mapping QueryTest() { return test||([]); }
public void QueryResult()
{
  string *idx;
  int i;
  
  i = sizeof(idx = sort_array(m_indices(QueryTest()),SF(<)));
  while(i--)
    printf("%12-s: %3d tries, %3d success,  %3d failed\n"
           "                         %3d avrg. s., %3d avrg. f.\n"
           "                         %3d max. s.,  %3d max. f.\n"
           "                         %3d s %%,      %3d f %%\n"
           "                         %3d advanced\n\n",idx[i],
           test[idx[i],TCHECK],test[idx[i],TSUCCESS],test[idx[i],TFAILED],
           test[idx[i],ASUCC],test[idx[i],AFAIL],
           test[idx[i],MAXSUCC],test[idx[i],MAXFAIL],
           test[idx[i],TSUCCESS]*100/test[idx[i],TCHECK],
           test[idx[i],TFAILED]*100/test[idx[i],TCHECK],
           test[idx[i],TADVANCE]);
}

public varargs void Raise(string name,int what,int percent)
{
  int avrg;
  if (!test)
    test = m_allocate(0,TADVANCE+1);
  if (what==TSUCCESS||what==TFAILED)
    {
      avrg = test[name,what+2];
      avrg = avrg*test[name,what];
      avrg = (avrg+percent)/(test[name,what]+1);
      test[name,what+2] = avrg;
      test[name,what+4] = NMB->max(({test[name,what+4],percent}));
    }
  test[name,what]=test[name,what]+1;
}

public void AdvanceSkill(object pl,string name)
{
  Raise(name,TADVANCE);
  (::AdvanceSkill(pl,name));
}

#endif

public varargs int UseSkill(object pl,string name,int chance,int percentage)
// Same as CheckSkill, but the skill might advance.
// As the fighting-skills otherwise raise to fast, it has been changed
// so it only advances on success.
{
  int p;
  if (member(({SK_FIGHT,SK_CRITICAL,SK_PARRY,SK_MULTIPLE}),name)==-1)
    return (::UseSkill(pl,name,chance,percentage));
  if (!pl) return 0;
  // for SK_FIGHT, SK_CRITICAL,SK_PARRY you only learn on success
  if (p = CheckSkill(pl,name,chance,&percentage))
    if (learn_skill(pl->QueryAttr(name,1)))
      AdvanceSkill(pl,name);
#ifdef D
  Raise(name,TCHECK);
  if (p)
    Raise(name,TSUCCESS,percentage);
  else
    Raise(name,TFAILED,percentage);
#endif
  return p;
}

private object owner()
{
  return find_player(QueryOwner());
}

public int QueryAC()
// Lvl  1.. 9: AC 0
// Lvl 10..19: AC 1
// Lvl 20..29: AC 2
// Lvl 30..50: AC 3
{
  if (!owner()) return 0;
  return NMB->min(({owner()->QueryLevel()/10,3}));
}
		   
public string QueryExtralook()
{
  object o;

  if (!o = owner()) return "";
  return
    capitalize(o->QueryName())+" has the tattoo of a broadsword on "+
    o->QueryPossessive()+" forehead.\n"+
    (PALADINS->is_leader(o)
     ?capitalize(o->QueryName())+" is a mighty guildcommander of "
      "the paladins!\n":"");
}

private void RaiseSkills()
// changed the settings of MaxDrink and MaxAlcohol, now only MaxAlcohol
// is upped for fighters. Until Level 20 they get level*5% more Alcohol
// from Level 20 to 30 100% plus level*3%, 21.09.94, Elfgar
{
  mixed racedata;
  int add_alc;
  object o;
  if (!o = owner()) return;
  racedata = RACEMASTER->RaceData(o->QueryRace());
  if (o->QueryLevel() > 20) 
    add_alc = NMB->min(({(o->QueryLevel()-20)*3+100,125}));
  else
    add_alc = o->QueryLevel()*5;
  o->SetMaxAlcohol(racedata[RD_ALC] * (100+add_alc)/100);
}

public string QueryShort()
{
  if (!owner()) return "a small shield";
  if (previous_object()==owner()) return "your small shield";
  return capitalize(add_gen(owner()->QueryName()))+" small shield";
}

private void ConvertSkills(object who)
{
  int i;
  if (i = who->QueryAttr("Cri",1))
    {
      who->SetAttr(SK_CRITICAL,i,1);
      who->RemoveAttr("Cri");
    }
  if (i = who->QueryAttr("Mul",1))
    {
      who->SetAttr(SK_MULTIPLE,i,1);
      who->RemoveAttr("Mul");
    }
  if (i = who->QueryAttr("Par",1))
    {
      who->SetAttr(SK_PARRY,i,1);
      who->RemoveAttr("Par");
    }
  if (i = who->QueryAttr("Aim",1))
    {
      who->SetAttr(SK_AIM,i,1);
      who->RemoveAttr("Aim");
    }
  if (i = who->QueryAttr("Fig",1))
    {
      who->SetAttr(SK_FIGHT,i,1);
      who->RemoveAttr("Fig");
    }
  if (i = who->QueryAttr("Hid",1))
    {
      who->SetAttr(SK_HIDE,i,1);
      who->RemoveAttr("Hid");
    }
  if (i = who->QueryAttr("Ban",1))
    {
      who->SetAttr(SK_BANDAGE,i,1);
      who->RemoveAttr("Ban");
    }
  if (i = who->QueryAttr("Cli",1))
    {
      who->SetAttr(SK_CLIMB,i,1);
      who->RemoveAttr("Cli");
    }
  if (i = who->QueryAttr("Luk",1))
    {
      who->SetAttr("luck",i,1);
      who->RemoveAttr("Luk");
    }
}

public varargs void SetAutoObject(mixed value)
{
  object pl;
  (::SetAutoObject(value));
  if (!pl = owner()) return;
  pl->SetAttr("luck",random(25),1);
  RaiseSkills();
  if (owner())
    {
      ConvertSkills(owner());
      owner()->AddEquipObj(ME);
    }
}

public void create()
{
  if (!is_clone()) return;
  (::create());
  seteuid(getuid());
  SetIds(({"shield","armour","membercard","paladincard","guildshield"}));
  SetAds(({"small","guild"}));
  SetLong("This shield shows the emblem of the fighters guild.\n"+
	  "Under the emblem you read: type 'sever' to leave this guild.\n");
  SetDefendObj(ME);
  SetValue(0);
  SetWeight(0);
  SetNoDrop("Don't throw away your guild's shield!\n");
  SetNoGive("Don't give your shield away!\n");
  SetNumberHands(1);
  SetArmourType(AT_SHIELD);
  SetGuild(PALADINS);
  SetAutoObject();
}

public void init()
{
  (::init());
  add_action("cmd_consider", "consider");
  add_action("cmd_attack", "attack");
  add_action("cmd_hide", "hide");
  add_action("cmd_aim", "aim");
  add_action("cmd_shoot", "shoot");
  add_action("cmd_sever", "sever");
  add_action("cmd_cut", "cut");
  add_action("cmd_gshout", "gshout");
  add_action("cmd_gwho", "gwho");
  add_action("cmd_gbug","gbug");
  add_action("cmd_gtypo","gtypo");
  add_action("cmd_gidea","gidea");
  add_action("cmd_skills", "skills");
  add_action("cmd_gtp", "gto");
  add_action("cmd_help", "help");
}

public int cmd_help(string str)
{
  mapping p;
  if (!str) return 0;
  p = ([SK_BANDAGE:"ban",
        SK_CRITICAL:"cri",
        SK_FIGHT:"fig",
        SK_HIDE:"hid",
        SK_PARRY:"par",
        SK_AIM:"aim",
        "luck":"luk",
        SK_MULTIPLE:"mul"]);
  str = lower_case(str);
  if (member(p,str)!=-1) str = p[str];
  if (member(m_values(p),str)==-1) return 0;
  STR->smore(read_file("/d/woodland/joern/doc/"+str),TP->QueryPageSize());
  return 1;
}

public int cmd_consider(string str)
{
  return FIGHTMASTER->consider(str);
}

public int cmd_cut(string str)
{
  return FIGHTMASTER->cut(str);
}

public int cmd_hide(string str)
{
  return FIGHTMASTER->hide(str);
}

public int cmd_attack(string str)
{
  return FIGHTMASTER->myattack(str);
}

public int cmd_aim(string str)
{
  return FIGHTMASTER->aim(str);
}

public int cmd_shoot(string str)
{
  return FIGHTMASTER->shoot(str);
}

public int cmd_sever(string str)
{
  if (lower_case(str||"")!="ties")
    return notify_fail("Type 'sever ties' to leave the guild.\n",
		       NOTIFY_NOT_OBJ),0;
  return QueryGuildObject()->cmd_leave("guild");
}

public int ChkWield(object liv, object ob, int is_weap)
{
//  write("Check Wield !!\n");
  if (!ob) return EQ_OK;
  if (!ob->QueryHitObj())
    ob->SetHitObj(ME);
  return EQ_OK;
}

public int ChkUnwield(object liv, object ob)
{
  if (!ob) return EQ_OK;
  if (ob->QueryHitObj()==ME) ob->SetHitObj(0);
  return EQ_OK;
}

public mixed DefendHit(object enemy, int damage, int damage_type)
{
  object o;
  int percent;
  if (!o = owner()) return 0;
  if (member(({DT_BLUDGEON,DT_SLASH,DT_PIERCE}),damage_type)!=-1)
    if (UseSkill(o,SK_PARRY,150+(o->QueryStr()/7+o->QueryDex()/5)*10,&percent))
      {
	percent = random(percent)+QueryAC();
        tell_object(o," (parried)");
        tell_room(environment(o)," (parried)",({o}));
	return NMB->min(({damage,percent}));
      }
  return QueryAC();
}

public int WeaponHit(object enemy)
{
  object o,weapon;
  int percent,dam;
  weapon = previous_object();
  if (!o = owner()) return weapon->CalcDamage(enemy);
  if (UseSkill(o,SK_CRITICAL,
               (o->QueryDex()+o->QueryStr())/8*10+
               NMB->min(({o->QueryAttr("luck",1),45}))/5*10,&percent))
    {
      write("You score a critical hit.\n");
      say("  "+capitalize(o->QueryName())+" scores a critical hit.\n");
      if (percent>70)
	dam = enemy->QueryHP()+15;
      else if (percent>40)
        dam = 1000;
      else if (percent>20)
        dam = 500;
      else
        dam = weapon->QueryWC()*10;
    }
  else if (UseSkill(o,SK_MULTIPLE,((o->QueryDex()+o->QueryStr())/7+15)*10))
    {
      write("You hit "+(enemy->QueryName()||"someone")+" twice.\n");
      say("  "+capitalize(o->QueryName())+" hits "+
          (enemy->QueryName()||"someone")+" twice.\n");
      dam = weapon->CalcDamage(enemy)+weapon->CalcDamage(enemy);
    }
  else if (UseSkill(o,SK_FIGHT,(o->QueryDex()*2/3+o->QueryStr()/10+20)*10,&percent))
    dam = percent/10+weapon->CalcDamage(enemy);
  else
    dam = weapon->CalcDamage(enemy);
  return dam;
}
