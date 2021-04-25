// Standard herb for OSB
//
// (c) Softbyte 21jun97
//
//
//
// TODO: Functions need to be implemented

#include <properties.h>
#include <herbs.h>
#include <classes.h>

inherit "/std/unit";
inherit "/std/food";

mapping herbs;   // the herb output texts


// ------------------ EXTERNAL FUNCTIONS -------------------------
// Initialises a standard herb:
// - Sets P_MAGIC,P_INFO,Strength, Amount, Value, etc...
// by choosing a type and its strength. Giving an optional text will
// output this text to the player when eating the herb
varargs int SetStandard(int type,int strength,string text)
{
  if (!type || !strength) return 0;
  Set(P_HERB,type);
  Set(P_HERB_STRENGTH,strength);

  if (text)
  {
    if (member(herbs,type)) herbs[type]=text;
	else herbs+=([type:text]);
  }
  return 1;
}
// Types can be:
//               HERB_HEAL_HP
//               HERB_HEAL_SP
//               HERB_HEAL_EP
//               HERB_HEAL_POISON
//               HERB_HURT_HP
//               HERB_HURT_SP
//               HERB_HURT_EP
//               HERB_SATIATE
//               HERB_INTOXICATE
//               HERB_SLEEP
// and strength can be any value you choose. It will be the effect in
// HP or SP, .. the herb does when eaten


// Optional you can change the look of the herb when a skilled player
// looks at it which has a skill between minskill and maxskill
// The text is then simply added to the long or plural long description
// depending on the amount of herbs the player has
static int *herb_min;
static string *herb_long,*herb_plural;
varargs int	AddHerbInfo(int minskill,string long,string plurallong)
{
  if (!plurallong) plurallong=long;
  herb_min+=({minskill});
  herb_long+=({long});
  herb_plural+=({plurallong});
  return 1;
}

// ------------------ INTERNAL FUNCTIONS -------------------------
// returns the modified long descripton if 'skill' is high enough to
// get it. if pluralflag is set the modified plural long is returned
string GetHerbInfo(int skill,int pluralflag)
{
int i;
int found_skill;
string long;

  found_skill=-999999;
  long="";
  for (i=0;i<sizeof(herb_min);i++)
  {
    if (skill>=herb_min[i] && herb_min[i]>found_skill)
	{
	  found_skill=herb_min[i];
	  if (pluralflag) long=herb_plural[i];
	  else long=herb_long[i];
	}
  }
  return long;
}

// If you want completely own herb handling overwrite the NotifyEat
// function but use the standard herb anyways. NotifyEat will be called
// when the player has eaten the herb sucessfully
void NotifyEat(object player)
{
string s;
int amount;
   amount=Query(P_HERB_STRENGTH);    // strength
   s=herbs[Query(P_HERB)];           // text output
   switch(Query(P_HERB))
   {
     case HERB_HEAL_HP:
	         player->HealHP(amount);
	 break;
     case HERB_HEAL_SP:
	         player->HealSP(amount);
	 break;
     case HERB_HEAL_POISON:
	         player->AddPoison(-amount);
	 break;
     case HERB_HURT_HP:
	         player->ReduceHP(amount);
	 break;
     case HERB_HURT_SP:
	         player->ReduceSP(amount);
	 break;
     case HERB_HURT_POISON:
           player->AddPoison(amount);
     break;
     case HERB_SATIATE:
	         player->AddFood(amount);
	 break;
	 default:
           s="Nothing special happens.\n";
   }
   return tell_object(player,process_string(s||""));
}

// Modify the long description
varargs string QueryLong()
{
int skill;
   skill=({int})this_player()->QueryAttr(SK_HERB);
   return ::QueryLong()+GetHerbInfo(skill,0);
}
// Modify the plural long description
varargs string QueryPluralLong(string what)
{
int skill;
   skill=({int})this_player()->QueryAttr(SK_HERB);
   return ::QueryPluralLong(what)+GetHerbInfo(skill,1);
}

varargs void create()
{
  ::create();
  food::create();

  // Some food initialisations
  SetFood();        // herbs are food in principle
  SetFoodStuff(1);  // They hardly stuff
  SetFoodHeal(0);   // Possible heal handling is done be ourselves

  // our own initialisations
  herb_min=({});
  herb_long=({});
  herb_plural=({});

  herbs=([
    HERB_HEAL_HP:
	   "The refreshing taste of the @@QueryShort@@ restores your health.\n",
    HERB_HEAL_SP:
	   "You feel strange but the @@QueryShort@@ restores your magic energy.\n",
    HERB_HEAL_POISON:
	   "The power of the @@QueryShort@@ cleans your blood from poison.\n",
    HERB_HURT_HP:
	   "The bitter taste of the @@QueryShort@@ hurts you.\n",
    HERB_HURT_SP:
	   "You gasp at the bitter taste of the @@QueryShort@@ and loose magic energy.\n",
    HERB_HURT_POISON:
	   "The @@QueryShort@@ taste poisonous. You don't feel very well now.\n",
    HERB_SATIATE:
	   "The @@QueryShort@@ let you feel less hungry.\n",
    ]);

  Set(P_MAGIC     "A magic herb which can be eaten.\n");
  Set(P_INFO,     "A herb which you can eat.\n");
  Set(P_HELP_MSG, "You can 'eat' this.\n");
}

void init()
{
  ::init();
  food::init();
}
