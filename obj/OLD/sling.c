/*
** A Sling
** Author: Thragor@Nightfall, 15may95
** Update: Thragor@Nightfall, 15may95
**
** The sling allows to attack a monster from another room.
*/

inherit "/std/weapon";
inherit "/std/container";

#include <config.h>
#include <combat.h>
#include <moving.h>
#include <properties.h>
#include <secure/wizlevels.h>
#include <search.h>
#include <lock.h>

#define TP this_player()
#define ME this_object()
#define MAX_SPEED 3

private static int speed,hp;

public int QueryWC()
// range from 0 to 10
// (perfect conditions to acchieve WC9:
//  wielder = halfling
//  ammunition = bullets (special made for slings)
//  speed = maximum
// )
{
  int modify;
  object stone;
  
  if (!sizeof(all_inventory()))
    return 0;

  stone = all_inventory()[0];
  
  if (!stone)
    return 0;

  if (stone->id("stone")
      ||stone->id("bullet")
      ||stone->id("ball")
      ||stone->id("rock"))
    modify=5;
  if (TP&&TP->QueryRace()=="halfling"&&modify)
    modify+=2;
  if (stone->id("bullet"))
    modify+=2;
  if (speed>=MAX_SPEED)
    modify+=1;
  return modify;
}

protected int check_hit(object aim)
{
  int res,chance;
  int alevel,level,adex,dex;
  alevel = aim->QueryLevel()||1;
  level = TP->QueryLevel()||1;
  adex = aim->QueryDex()||1;
  dex = TP->QueryDex()||1;
  
  res = 100;
  if (adex>dex)
    {
      chance = adex*100/dex;
      res-=chance/2;
    }
  else
    if (adex!=dex)
      {
        chance = dex*100/adex;
        res+=chance/2;
    }
  if (alevel>level)
    {
      chance = alevel*100/level;
      res-=chance/2;
    }
  else
    if (alevel!=level)
      {
        chance = level*100/alevel;
        res+=chance/2;
      }
  if (res>100) res = 100;
  if (environment(aim)!=environment(TP))
    res-=30;
  return random(101)<res;
}

protected int calc_damage(object aim)
{
  if (!TP||!speed||!check_hit(aim)) return 0;
  return  ((random(QueryWC())+random(TP->QueryDex())+1)
           *(QueryQuality()+5))/105;
}

public int SetSpeed(int i) { return speed = i; }
public int QuerySpeed() { return speed; }
public int RaiseSpeed() { return speed++; }

public int allow_enter(int method, mixed extra)
// Stones are not checked, because you even might throw potatoes at
// someone this way. But it doesn't cause as much damage.
{
  if (method==M_PUT)
    {
      if (QueryWC())
        return ME_TOO_HEAVY;
    }
  return (::allow_enter(method,extra));
}

public int cmd_load(string str)
{
  if (IS_IMMORTAL(TP))
    return write(
      "Well, you just used the command load. Perhaps you want really\n"
      "to load a file, but it leads to conflicts with the sling. So\n"
      "here's the error-message you would get from the sling:\n"
      "Just put the stone into the sling.\n")||1;
  return (int)notify_fail(
    "Just put the stone into the sling.\n",
    NOTIFY_NOT_VALID)&&0;
}

protected void increase_speed()
{
  object *inv,ob;
  int i;

  RaiseSpeed();
  if (speed<MAX_SPEED)
    {
      write("You twirl the sling around your head.\n");
      show(({TP->QueryName()+" twirls the sling around "+
             TP->QueryPossessive()+" head.\n",
             "You hear some noises in the air.\n"}));
    }
  else
    if (speed==MAX_SPEED)
      {
        write("You twirl the sling around your head.\n"
              "Now the sling has the right speed to shoot.\n");
        show(({TP->QueryName()+" twirls the sling around "+
               TP->QueryPossessive()+" head.\n",
               "You hear some noises in the air.\n"}));
      }
    else
      if (speed>MAX_SPEED&&speed<MAX_SPEED*5)
	{
	  write("You twirl the sling around your head.\n"
                "The speed is very high. Better shoot now.\n");
          show(({TP->QueryName()+" twirls the sling around "+
                 TP->QueryPossessive()+" head.\n",
                 "You hear some noises in the air.\n"}));
	}
      else
	{
	  write("The sling breaks into pieces.\n");
          show(({TP->QueryName()+" twirls the sling around "+
                 TP->QueryPossessive()+" head and suddenly\n"
                 "the sling breaks into pieces.\n",
                 "You hear some noises in the air and a loud curse.\n"}));
	  i = sizeof(inv = all_inventory());
          if (environment()&&ob = environment(environment()))
	    while(i--)
	      inv[i]->move(ob,M_DROP);
	  remove();
	  if (ME) destruct(ME);
	  return;
	}
  TP->SetCombatDelay(4);
  TP->SetCombatDelayMsg("You twirl the sling...\n");
  call_out(#'increase_speed,4);
}

public int cmd_stop(string str)
{
  if (!TP)
    return 0;
  if (search(TP,str,SEARCH_INV|SM_IGNLIGHT|SM_OBJECT)!=ME)
    return (int)notify_fail(
      "Stop what?\n",
      NOTIFY_NOT_OBJ)&&0;
  if (find_call_out(#'increase_speed)==-1)
    return (int)notify_fail(
      "The don't twirl the sling.\n",
      NOTIFY_NOT_VALID)&&0;
  write("You stop twirling the sling.\n");
  show(({TP->QueryName()+" stops to twirl the sling.\n",
         "The noises stop.\n"}));
  remove_call_out(#'increase_speed);
  SetSpeed(0);
  TP->SetCombatDelay(0);
  return 1;
}

public int cmd_twirl(string str)
{
  if (!TP)
    return 0;
  if (search(TP,str,SEARCH_INV|SM_IGNLIGHT|SM_OBJECT)!=ME)
    return (int)notify_fail(
      "Twirl what?\n",
      NOTIFY_NOT_OBJ)&&0;
  if (!QueryWielded())
    return (int)notify_fail(
      "You first have to wield the sling.\n",
      NOTIFY_NOT_VALID)&&0;
  if (!sizeof(all_inventory()))
    return (int)notify_fail(
      "There's nothing in the sling.\n",
      NOTIFY_NOT_VALID)&&0;
  if (find_call_out(#'increase_speed)!=-1)
    return (int)notify_fail(
      "You already twirl the sling around your head.\n",
      NOTIFY_NOT_VALID)&&0;
  write("You start to twirl the sling around your head.\n"
        "To stop twirling, type 'stop sling'.\n");
  show(({TP->QueryName()+" starts to twirl the sling around "+
         TP->QueryPossessive()+" head.\n",
         "You hear some noises in the air.\n"}));
  SetSpeed(0);
  TP->SetCombatDelay(2);
  TP->SetCombatDelayMsg("You twirl the sling...\n");
  call_out(#'increase_speed,2);
  return 1;
}

public void PlayerNetdead(int i)
{
  if (i)
    {
      remove_call_out(#'increase_speed);
      SetSpeed(0);
    }
}

protected object find_aim(string str)
{
  object res,room,*door_obs;
  string exit,*exits_arr,*door_arr;
  mapping exits,doors;
  int i,pos;

  if (!environment(TP)) return 0;
  if (res = search(environment(TP),str,SEARCH_INV|SM_OBJECT))
    return res;
  i = sizeof(exits_arr = m_indices(exits = environment(TP)->QueryExitsDest()));
  door_obs = m_indices(environment(TP)->QueryDoors()||([]));
  door_arr = m_values(environment(TP)->QueryDoors()||([]));
  res = 0;
  while(i--&&!res)
    {
      if (stringp(exits[exits_arr[i]]))
        if ((pos = member(door_arr,exits_arr[i]))==-1
            ||(door_obs[pos]->QueryLockState()==LOCK_OPEN))
        res =
          search(load_object(exits[exits_arr[i]]),str,
                 SEARCH_INV|SM_OBJECT|SM_IGNLIGHT);
      else
        if (closurep(exits[exits_arr[i]]))
          {
            res = clone_object("/std/npc");
            res->SetName("");
            res->SetShort(0);
            res->SetHP(100000);
            res->command_me(exits_arr[i]);
            room = environment(res);
            res->remove();
            if (res) destruct(res);
            res =
              search(room,str,SEARCH_INV|SM_OBJECT|SM_IGNLIGHT);
          }
    }
  return res;
}
          
public int cmd_shoot(string str)
{
  object aim,stone,env;
  string what,atwhat;
  int dam;
  
  if (!TP)
    return 0;
  if (!sizeof(all_inventory()))
    return (int)notify_fail(
      "The sling is not loaded yet.\n",
      NOTIFY_NOT_VALID)&&0;
  if (speed) TP->SetCombatDelay(0);
  if (TP->QueryCombatDelay())
    return (int)notify_fail(
      TP->QueryCombatDelayMsg()||"You are unconscious.\n",
      NOTIFY_NOT_VALID)&&0;
  if (!str||TP->CantSee())
    {
      aim = TP;
      stone = all_inventory()[0];
    }
  else
    {
      if (sscanf(lower_case(str),"%s at %s",what,atwhat)!=2)
        return (int)notify_fail("Shoot what at whom?\n",
          NOTIFY_NOT_OBJ)&&0;
      if (!stone = search(ME,what,SEARCH_INV|SM_OBJECT|SM_IGNLIGHT))
        return (int)notify_fail(
          "Your sling is not loaded with "+what+".\n",
          NOTIFY_NOT_VALID)&&0;
      if (!aim = find_aim(atwhat))
        return (int)notify_fail(
          "You don't see "+atwhat+" anywhere.\n",
	     NOTIFY_NOT_VALID)&&0;
      if (environment(aim)!=environment(TP))
        {
          if (environment(aim)->QueryIntLight()>TP->QueryUVision())
            return (int)notify_fail("It's too bright there.\n",
              NOTIFY_NOT_VALID)&&0;
          if (environment(aim)->QueryIntLight()<TP->QueryIVision())
            return (int)notify_fail("It's too dark there.\n",
              NOTIFY_NOT_VALID)&&0;
        }
      if (!living(aim)||aim->QueryNetdead())
        {
          remove_call_out(#'increase_speed);
          SetSpeed(0);
          write("You shoot "+stone->QueryShort()+" at "+
                aim->QueryShort()+" and hit it.\n");
          show(({TP->QueryName()+" shoots "+stone->QueryShort()+" at "+
                 aim->QueryShort()+" and hit it.\n",
                 "You hear a hollow 'Dong'.\n"}));
          if (environment(aim)!=environment(TP))
            show_room(environment(aim),
              ({capitalize(stone->QueryShort())+" hits "+
                aim->QueryShort()+".\n","You hear a hollow 'Dong'.\n"}));
          stone->move(environment(aim),M_SILENT);
          return 1;
        }
      if (aim->QueryGhost())
        {
          remove_call_out(#'increase_speed);
          SetSpeed(0);
          write("You shoot "+stone->QueryShort()+" at "+
                aim->QueryShort()+" and "+stone->QueryShort()+
                " lands behind "+aim->QueryObjective()+".\n");
          show(({TP->QueryName()+" shoots "+stone->QueryShort()+" at "+
                 aim->QueryShort()+" and "+stone->QueryShort()+
                " lands behind "+aim->QueryObjective()+".\n",
                "You hear a hollow 'Dong'.\n"}));
          if (environment(aim)!=environment(TP))
            show_room(environment(aim),
              capitalize(stone->QueryShort())+" flies through "+
              aim->QueryShort()+".\n");
          stone->move(environment(aim),M_SILENT);
          return 1;
        }
    }
  remove_call_out(#'increase_speed);
  tell_object(aim,TP->QueryName()+" shoots "+stone->QueryShort()+"at you");
  if (environment(aim)!=environment(TP))
    {
      tell_room(environment(aim),
        capitalize(stone->QueryShort())+" flies towards "+aim->QueryShort(),
        ({aim}));
      say(TP->QueryName()+" shoots "+stone->QueryShort()+" at "+
          aim->QueryShort()+".\n");
    }
  else
    say(TP->QueryName()+" shoots "+stone->QueryShort()+" at "+
        aim->QueryName(),aim);
 
  if (TP==aim)
    if (random(TP->QueryDex())>15)
      {
        write(
          "You nearly shoot "+stone->QueryShort()+
          " at yourself but instinctivly you\n"
          "step aside.\n");
        show(({TP->QueryName()+" makes a step aside.\n",
               "Someone moves inside the room.\n"}),TP);
        stone->move(environment(aim),M_SILENT);
      }
    else
      {
        write("Ouch! You shoot the stone at yourself ");
        say(TP->QueryName()+" shoots a stone at "+TP->QueryObjective()+
            "self");
        TP->Defend(calc_damage(TP),QueryDamType(),ME);
        TP->StopHunting(TP);
        stone->move(environment(aim),M_SILENT);
      }
  else
    {
      write("You shoot a stone at "+aim->QueryName());
      if (!dam = calc_damage(aim))
        {
          tell_object(aim," but misses.\n");
          write(" but you miss.\n");
          if (environment(aim)!=environment(TP))
            tell_room(environment(aim)," but misses.\n",({aim}));
          else
            say(" but misses.\n",aim);
          stone->move(environment(aim));
        }
      else
        {
          aim->Defend(dam,QueryDamType(),ME);
          if (aim->QueryGoChance()
              &&environment(aim)!=environment(TP)
              &&random(aim->QueryInt())>5
              &&!aim->QueryCombatDelay())
            call_out(
              lambda(0,({#'call_other,aim,"move",environment(TP),M_GO})),5);
          stone->move(environment(aim));
        }
    }
  SetSpeed(0);
  return 1;
}

public void create()
{
  (weapon::create());
  (container::create());
  
  SetShort("a sling");
  SetLong(({
    "The sling consists of a leather strap with a pouch for holding\n"
    "the stone. The weapon is held by both ends of the strap and\n"
    "twirled around the wielder's head. When top speed is attained,\n"
    "the stone is launched by releasing one of the strap's ends.\n"
    "You may even misuse it when attacking a monster directly. But\n"
    "you should first load it with a stone, otherwise the sling has\n"
    "no effect.\n"
    "To attack from another room, you first have to load the sling, then\n"
    "you have to twirl the sling and then you have to release one of the\n"
    "strap's ends. You may even attack someone from the distance (but not\n"
    "too far).\n",
    "Example commands to attack: (Harry is in the pub north of you):\n"
    "> wield sling\n"
    "> put stone into sling\n"
    "> twirl sling\n"
    "> shoot stone at harry\n"
    "Done.\n"}));
  SetIds(({"sling","weapon"}));
  SetAds(({"leather"}));
  SetWeaponType(WT_CHAIN);
  SetWeaponHands(1);
  SetDamType(DT_BLUDGEON);
  SetMaxWeight(200);
  SetWeight(50);
  SetValue(5);
  SetUnwieldObj(ME); // if the player unwields the sling, the stone
                     // shall be unloaded
}

public int CheckUnwield(object wielder)
{
  int i;
  object *inv;
  
  remove_call_out(#'increase_speed);
  i = sizeof(inv = all_inventory());
  if (QuerySpeed()) wielder->SetCombatDelay(0);
  SetSpeed(0);
  if (environment())
    while(i--)
      inv[i]->move(environment(),M_DROP);
  return 1;
}

public void init()
{
  (::init());
  add_action("cmd_load","load");
  add_action("cmd_twirl","twirl");
  add_action("cmd_shoot","shoot");
  add_action("cmd_stop","stop");
}
