// smith for the smithy
// made by magictcs - 25 apr 97

#include <properties.h>
#include <sykorath.h>

inherit "/std/npc";

#define SHOP INNER("smithy") // This is smith's armoury

private nosave int weldcount;
private nosave object item;

void create() 
{
  ::create();
  seteuid(getuid());
  SetName("Sylond");
  AddId("smith");
  AddId("weaponsmith");
  AddAdjective("old");
  SetShort("Sylond, the smith");
  SetLong(
    "This is an old weaponsmith. He nervously looks around in the store. It\n"
    "seems that he searches something. He likes to weld new weapons in\n"
    "front of his costumers. Perhaps you can see some of his skill if you\n"
    "are in the shop at the right time.\n");
  SetGender(GENDER_MALE);
  SetLevel(18);
  SetWeight(60000);
  //SetDefence(({10,0,0,0,5,-5,-5,-5,-5,-5}));
  call_out("weld",random(10));
  AddQuestion(({"sylond","smith"}),
    "You don't see that person here!\n");
  AddQuestion("sword",
    "Sylond, the old smithy says:\n"
    "I want to forge another masterpiece, but unfortunatly\n"
    "I have a shortage of raw material. If you can bring me\n"
    "iron, gold and coal I can start working on my next\n"
    "masterpiece that can be bought only by the person who\n"
    "brought me the raw materials.\n");
}

void weld() {
  object env,store;
  int i;
  if (!(env = environment())) { remove(); return; }
  if (blueprint(env) != SHOP)
  {
    tell_room(env,QueryName()+" goes into his shop.\n");
    move(SHOP);
  }  
  if (!(store = ({object})(env = environment())->GetStore())) return;
  if (sizeof(all_inventory(store))<10)
  {
    tell_room(env,"The smith starts to forge a new weapon.\n");
    weldcount = random(5);
    item = clone_object("/std/weapon");
    switch (random(10))
    {
      case 0..1 : item->SetStandard(WT_KNIFE,weldcount);
                  item->SetShort("a sharp knife");
                  item->SetLong("It looks really sharp. Surely it can do "
                        "some damage.\n");
                  item->AddAdjective("sharp");
                  break;
             
      case 2..3 : item->SetStandard(WT_CLUB,1+weldcount);
                  item->SetShort("an huge iron club");
                  item->SetLong("It is a very huge and heavy club.\n");
                  item->AddAdjective("huge");
                  item->AddAdjective("iron");
                  break;

      case 4..6 : item->SetStandard(WT_SWORD,6);
                  item->SetShort("a sharp and long sword");
                  item->SetLong("It looks sharp and is very long.\n");
                  item->AddAdjective("sharp");
                  break;

      case 7 :    item->SetStandard(WT_AXE,6);
                  item->SetShort("an iron axe");
                  item->SetLong("It is a heavy axe. If you are skilled, "
                        "you can surely kill nasty monsters with it.\n");
                  item->AddAdjective("iron");
                  break;

      default :   item->SetStandard(WT_SPEAR,6);
                  item->SetShort("a very sharp spear");
                  item->SetLong("It is a very sharp and good "
                        "balanced spear.\n");
                  item->AddAdjective("very");
                  item->AddAdjective("sharp");
                  
    }
    for (i=0; i<weldcount; i++)
      item->SetValue((({int})item->QueryValue() * 8) / 7);
    weldcount += 5;
    call_out("weld1",random(2)+5);
  }
  else
    call_out("weld",random(10));
}

void weld1() 
{
  object env;
  string *msgs;
  msgs = ({
     "Sylond heatens up a piece of iron in the fire.\n",
     "The smith fetches a new piece of iron and puts it on his anvil.\n",
     "Sylond takes his sledge-hammer and carefully hammers the "
         "iron on\nthe anvil.\n",
     "The smith puts the hot iron into cold water.\n",
     "Sylond takes his shovel and throws more coal onto the forge.\n"});

  if (!(env = environment())) { remove(); return; }
  if (blueprint(env) != SHOP)
  {
    tell_room(env,QueryName()+" enters his shop.\n");
    move(SHOP);
  }  
  if (!({object})(env = environment())->GetStore()) return;
  tell_room(environment(),msgs[random(sizeof(msgs))]);
  weldcount--;
  if (!weldcount)
  {
    tell_room(environment(),"Sylond just finished his weapon.\n");
    env->PutInStore(item);
    call_out("weld",random(50));
  }
  else
    call_out("weld1",random(2)+5);
}

