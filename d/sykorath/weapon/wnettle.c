// a dangerous stinging nettle
// made by magictcs - 14 jul 97

// TODO: change the nettle that it hurts more players without a body armour
// overload CalcDamage()

#include <properties.h>
#include <living.h>
#include <moving.h>
#include <sykorath.h>

inherit "/std/weapon";

void create() {
  ::create();
  SetStandard(WT_CLUB,4,PSIZE_GENERIC);
  SetShort("a stinging nettle");
  SetLong("You see a stinging nettle. A dangerous weapon!\n"
    "Be careful while touching the nettle!\n");
  AddId("nettle");
  SetAds(({"stinging"}));
}

// return 1 for gloves
int IsGloves(object armour) {
  if (({int})armour->QueryArmourType()==AT_GLOVES) return 1;
  return 0;
}

// if not wearing gloves - burn the skin while wielding
varargs void NotifyWield(object living,object weapon,int flags) {
  object *armours;
  ::NotifyWield(living,weapon,flags);
  if (!living) return;
  if (!interactive(living)) return;
  if (!weapon) return;
  if (weapon!=TO) return;
  armours=({object *})TP->QueryArmours();
  armours=filter(armours,"IsGloves");
  if (!sizeof(armours)) {
    write(
      "You burn your skin while wielding the nettle. Better use some "
      "gloves the next time!\n");
    TP->Defend(2+random(5),DT_PIERCE,DEFEND_F_NOMSG);
  }
}

// if move nettle into player checkk if player wears gloves
// if not - burn skin of player
varargs int move (object dest,int method,mixed extra) {
  object *armours;
  if (!dest) return ::move(dest,method,extra);
  if (!({int})dest->QueryIsLiving()) return ::move(dest,method,extra);
  armours=({object *})dest->QueryArmours()||({});
  armours=filter(armours,"IsGloves");
  if (!sizeof(armours)) {
    write(
      "You burn your skin while touching the nettle. Better use some "
      "gloves the next time!\n");
    TP->Defend(2+random(5),DT_PIERCE,DEFEND_F_NOMSG);
  }
  return ::move(dest,method,extra);
}

// hurt livings more which have no body armour (burn their skin)
int CalcDamage (object enemy) {
  int dam;
  object *armours;
  if (!enemy) return 0;
  dam=::CalcDamage(enemy);
  if (random(10)<6) return dam;
  armours=({object *})enemy->QueryArmours()||({});
  armours=filter(armours,"IsBodyArmour");
  if (!sizeof(armours)) {
    write("You burn the skin of "+(({string})enemy->QueryShort()||"Someone")+
          " while touching it with the nettle.\n");
    show(({string})NAME+" burns the skin of "+(({string})enemy->QueryShort()||"Someone")+
         " with the nettle.\n",enemy);
    tell_object(enemy,"Your skin is burned by a stinging nettle, wielded by "+
         (({string})NAME||"Someone")+".\n");
    dam*=2;
  }
  return dam;
}
