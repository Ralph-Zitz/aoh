/*
** Staff of the Sourcerer
** Item of the Culture Center Quest
** Author: Thragor@Nightfall, 03apr97
** Update:
** Cloned by: /d/archwiz/common/room/culture/culture
*/

#include <combat.h>
#include <properties.h>
#include <search.h>
#include <config.h>

#define P_CODE "MachineCode"

#define ME this_object()
#define TP this_player()

// visible part
inherit "/std/weapon";

public void create()
{
  (::create());
  SetStandard(WT_STAFF,2,PSIZE_GENERIC);
  SetShort("Staff of the Sourcerer");
  SetInfo("This is the staff of our mighty God Deepthought.\n");
  SetLong("The staff is made of marble and carries some strange "
	  "inscriptions.\n");
  AddId(({"staff","staff of sourcerer","staff of the sourcerer",
	  "staff of deepthought"}));
  AddSubReadMsg(({"inscriptions","inscription","strange inscriptions",
		  "strange inscription"}),
		explode(read_file(blueprint()+".c"),"// visible part\n")[1]+
		"}\nThe rest isn't readable anymore.\n");
  SetWPperQP(0); // hardly nothing can break this staff
// visible part
  SetInfo("This staff is a good thing to wave with.\n");
  SetMagic("This staff can change the reality of the world.\n"
           "If you wave it it might dissolve and change the "
	   "reality.\n");
  SetHelpMsg("You may wield the staff to fight.\n"
	     "And you may identify this staff at a shop, for example.\n");
}

public string CCQuest()
{
  return "staff";
}

public void init()
{
  (::init());
  add_action("cmd_wave","wave");
}

#define SF(x) #'x //'

public int cmd_wave(string what)
{
  object env,ob,*obs1,*obs2;
  int i1,i2;
  if (   !what
      || !TP
      || !(env = environment(TP))
      || search(TP,what,SEARCH_INV|SM_IGNLIGHT|SM_OBJECT)!=ME
     )
    return notify_fail("Wave what?\n",NOTIFY_NOT_OBJ),0;
  write("You wave the staff through the air.\n");
  if (!TP->QueryInvis())
    show(capitalize(TP->QueryName())+" waves a staff through the air.\n");
  if ((i1 = member(map_objects(obs1 = deep_inventory(env),"CCQuest"),
		   "machine"))==-1)
    return write("Nothing happens.\n"),1;
  if ((i2 = member(map_objects(obs2 = all_inventory(env),"CCQuest"),
		   "machine"))!=-1)
    obs2[i2]->Set(P_CODE,1);
  else
    obs1[i1]->Set(P_CODE,1);
  write("The staff starts to glow and vanished into the cube.\n");
  env->CheckQuest(TP);
  if (ME) remove();
  if (ME) destruct(ME);
  return 1;
}

private void stop_fight(object pl,object enemy)
{
  object env;
  if (!pl)
    return;
  env = enemy?environment(enemy):environment(pl);
  map_objects(all_inventory(env),"StopAllHunting");
}

public int CalcDamage(object enemy)
{
  int dam;
  object pl;

  if (   !enemy
      || !(dam = (::CalcDamage(enemy)))
      || (!pl = QueryWielded())
     )
    return 0;

  switch(random(10))
    {
    case 0:
      tell_object(pl,"The staff dances in your hand and heals "+
		  enemy->QueryName()+".\n");
      enemy->SetHP(enemy->QueryMaxHP());
      return 0;
    case 1:
      tell_object(pl,"The staff goes deep into "+enemy->QueryName()+
		  " and suddenly "+enemy->QueryPronoun()+" looks very "
		  "stupid.\n");
      enemy->SetInt(1);
      return 0;
    case 2:
      tell_object(pl,"The staff glows in a green light and suddenly "
		  "all fighting stops.\n");
      call_out(SF(stop_fight),1,pl,enemy);
      return 0;
    default:
      return dam;
    }
}
