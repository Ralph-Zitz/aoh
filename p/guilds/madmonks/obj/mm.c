/* The main guild object of the MADMONKS */
// made and updated by Magictcs

// armour is needed??
inherit "/std/armour";
//inherit "/obj/equipment";
inherit "/obj/lib/guildobj";

#include <combat.h>
#include <config.h>
#include <properties.h>
#include <moving.h>
#include "/p/guilds/madmonks/madmonks.h"

// to do: remove the new hands from the player if he is logged in and has no
// membership of the guild!

// initialize the guildobject (autoloading, guildname and and and)
void create() {
  if (!is_clone()) return;
  (::create());
  seteuid(getuid());
  SetStandard(AT_AMULET,0,PSIZE_GENERIC);
  SetShort("a tattoo of an dragon");
  SetIds(({"tattoo","mm_tattoo"}));
  SetLong("The tattoo shows a big golden dragon.\n"
    "This is the sign of the madmonks.\n"
    "Type <mm> for a list of commands.\n\n");
  SetDefendObj(TO);
  SetValue(0);
  SetWeight(0);
  SetNoDrop("Don't throw away your guild's tattoo!\n");
  SetNoGive("How do you want to give your tattoo away?\n");
  SetNumberHands(0);
  SetGuild(MAINGUILD);
  SetAutoObject();
}

private object owner()
{
  return find_player(QueryOwner());
}

public varargs void SetAutoObject(mixed value)
{
  object pl;
  (::SetAutoObject(value));
  if (!pl = owner()) return;
  if (owner())
    {
      owner()->AddEquipObj(TO);
    }
}

// we added a death object - so we have to remove it!
remove(arg) {
  if (this_interactive())
    this_interactive()->SetDeadOb(0);
// remove the flying fists!  
  ::remove(arg);         // call the normal remove
}

// installs the death object, add the actions
init() {
  ::init();
  if (TP !=environment()) return;
  if(present("tattoo 2",TP))
  {
    destruct(this_object());
  }
  TP->SetDeadOb(GUILDOBJ("death_mark.c"));
  TP->SetGuild(GC_MADMONKS);
  add_action("mm","mm");
  add_action("cmd_sever","leave");     // leave guild on the fly
  add_action("cmd_touch","touch");     // cast a light per magic
  add_action("cmd_rubb","rubb");       // heal a little bit (SP->QP)

  add_action("cmd_gemote","demote");    // guild Emote
  add_action("cmd_gshout","dshout");    // guild shout
  add_action("cmd_gwho","dwho");        // guild who
//  add_action("","");
//  add_action("","");
//  add_action("","");
//  add_action("","");
//  add_action("","");
//  add_action("","");
//  add_action("","");
//  add_action("","");
//  add_action("","");
//  add_action("","");
//  add_action("","");
//  add_action("","");
//  add_action("","");
//  add_action("","");
//  add_action("","");
//  add_action("","");


  SetFlyingHands();
}

// command list
int mm () {
  write(
    "You can use folowing features of the madmonks:\n"
    "  leave guild\n"
    "  touch tattoo   - will fetch a torch\n"
    "  rubb tattoo    - heals you for some spellpoints\n"
  );
  return 1;
}

// fetch a torch
int cmd_touch(string str) {
  object ob;
  if (!str) return 0;
  if (str=="tattoo") {
    if (TP->QuerySP()>5) {
      TP->ReduceSP(3);   // sub 3 SPs
      ob=clone_object("/obj/torch");
      if (ob) { 
        ob->move(TP,M_SILENT);
        write(
        "You touch the tattoo on your head. Your guildmaster Balgarion appears\n"
        "in a black cloud. He fetches a torch from another dimension and give\n"
        "the torch to you.\n"
        "Then Balagarion vanished in another big black cloud.\n");
      }
      return 1;
    }
    write("You don't have enough spellpoints to call you guildmaster.\n");
    return 1;
  }
  notify_fail("What do you want to touch?\n");
  return 0;
}

// changes SP to HP
int cmd_rubb(string str) {
  object ob;
  if (!str) return 0;
  if (str=="tattoo") {
    if (TP->QuerySP()>25) {
      if (TP->QueryMaxHP()>TP->QueryHP()) {
      TP->ReduceSP(20);   // sub 20 SPs
      TP->Heal(20);
      write(
      "You rubb the tattoo on your head. Your guildmaster Balgarion appears\n"
      "in a black cloud. He looks at you and laughs demonically.\n"
      "But after some seconds he makes some mystical gestures and heals you\n"
      "a little bit.\n"
      "Then Balagarion vanished in another big black cloud.\n");
      return 1;
      }
      else
      {
      notify_fail(
        "You are perfectly healed! - Why do you want to rubb your tattoo?\n");
      return 0;
      }
    }
    write("You don't have enough spellpoints to call you guildmaster.\n");
    return 1;
  }
  notify_fail("What do you want to rubb?\n");
  return 0;
}

// leave the guild stuff, check and ....
int cmd_sever(string str) {
  object ob;
  if (lower_case(str||"")!="ties") {
    notify_fail("Type 'sever ties' to leave the guild.\n",
        NOTIFY_NOT_OBJ),0;
  }
  return QueryGuildObject()->cmd_leave("guild");
}

// Wear,wield objects: --> only for this object :((
public int ChkWield(object liv, object ob, int is_weap)
{
  // check for ritual weapons (Quest weapons!)
  // they have to be wielded!
  write("It is not allowed for 'Madmonks' to wield weapons\n"
    "But you can use your flying fists instead!\n");
  return EQ_FORBID;
  if (!ob) return EQ_OK;
  if (!ob->QueryHitObj())
    ob->SetHitObj(this_object());
    return EQ_OK;
}

public int SetFlyingHands()
{
  // I believe - a better way is to setup two cloned weapons with short=
  // "flying fists"
  // - a player can get problems if the system is crashed or something like 
  // that
  // level dependend???
  int level,handwc;
  level=this_player()->QueryLevel();
  switch (level) {
    case 1..3:   handwc=1;break;
    case 4..6:   handwc=2;break;
    case 7..9:   handwc=3;break;
    case 10..12: handwc=4;break;
    case 13..15: handwc=5;break;
    case 16..18: handwc=6;break;
    case 19..21: handwc=7;break;
    case 22..24: handwc=8;break;
    case 25..27: handwc=9;break;
    case 28..30: handwc=10;break;
    case 31..33: handwc=11;break;
    case 34..36: handwc=12;break;
    case 37..39: handwc=13;break;
    case 40..42: handwc=14;break;
    case 43..45: handwc=15;break;
    case 46..50: handwc=16;break;
    default: handwc=1;
  }
  handwc=handwc*2;
  // Check for players with other hands (like demons!)
  this_player()->SetHands(
    ({({"flying right fist",0,handwc+1}),({"flying left fist",0,handwc+1})}));
  return 1;
}
