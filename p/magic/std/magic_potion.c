// Standard magic potion
//
// (c) Softbyte 16apr97
//
//
//
// Use the standard potions as follows:
// What standard potions are defined you see in /sys/magic.h POTION_*
// At the moment the following standard potions are available, their 
// effect with 100% strength can be seen in the defines below !
// POTION_HEALING, POTION_MAGIC, POTION_POISON, POTION_HEALPOISON,
// POTION_VIGOR, POTION_SATIATE, POTION_INTOXICATE, POTION_SLEEPING
//
//    SetPotionName("garish green potion"); // SetShort ...
//    SetLong("This bottle contains some garish green potion.\n");
//    SetStandard(POTION_POISON);
//    SetStrength(100);  // default=100%: Raises or lowers the effect of
//                       //               the potion
// For own effects overwrite void NotifyDrink(object pl)
// it will be called when the player pl has drunk the potion
// You can there perform the necessary action and output some text
//  create()
//  {
//    ::create();
//    SetPotionName("purple potion");
//    SetLong("This bottle contains some purple potion.\n");
//    SetStrength(100);  // is optional but recommended for your own potion
//    // Other stuff from std/thing to make up your potion
//  }
//  void NotifyDrink(object pl)
//  {
//    tell_object(pl,"The purple potion raises your strength.\n");
//    pl->ModifyAttr("Str",1);
//  }
//   


#include <properties.h> 
#include <magic.h> 
inherit "/std/drink";

#define HEAL_HP      50   // default for standard healing HP 100%
#define HEAL_SP      50   // default for standard restoring SP 100%
#define HURT_POISON  50   // default for standard poisoning 100%
#define HEAL_POISON  50   // default for standard heal poison 100%
#define VIGOR        50   // default for standard restore EP 100%
#define SATIATE      25   // default for standard restore food 100%
#define SLEEP        120  // default for standard sleep time in seconds 100%


// Strength of the potion in percent (100=default)
static int Strength;
int SetStrength(int i) {Strength=i;return 1; }
int QueryStrength() {return Strength;}

// Check whether the sleeping mode is active
private int sleeping;
int SetSleeping(int i) {sleeping=i;return 1;}
int QuerySleeping() {return sleeping;}

// Set up some standard potions, using the POTION_XXX defines in magic.h
private mapping potions;
static int p_type;
int SetStandard(int i)
{
string s;
  if (!potions) return 0;
  p_type=i;
  s=potions[i];
  if (s) Set(P_MAGIC,s+".\n");
  else Set(P_MAGIC,"A potion.\n");
  return 1;
}
int QueryStandard() {return p_type;}

// Change the text output of the potions. The first argument is the
// P_MAGIC info and the second is the text the player gets when drinking
// the potion. type is one of the POTION_XXX which should be changed
int SetPotion(int type,string magicinfo,string outputstring)
{
  if (!potions) return 0;
  if (potions[type])
  {
    potions[type,0]=magicinfo;
    potions[type,1]=outputstring;
  }
  else
  {
    potions+=([type:magicinfo;outputstring]);
  }
  return 1;
}
string *QueryPotion(int type)
{
  if (!potions) return ({});
  if (potions[type]) return ({potions[type,0],potions[type,1]});
  else return ({});
}


// This is called when the potion has been drunk. 
// Overwrite this for own effects !!!
void NotifyDrink(object pl)
{
int amount;
string s;
  if (!potions) return;
  s=potions[p_type,1];
  if (!s) s="Nothing happens.\n";
  tell_object(pl,s);
  switch(p_type)
  {
    case POTION_HEALING:
        pl->HealHP(HEAL_HP*QueryStrength()/100);
	break;
    case POTION_MAGIC:
        pl->HealSP(HEAL_SP*QueryStrength()/100);
	break;
    case POTION_POISON:
        pl->AddPoison(HURT_POISON*QueryStrength()/100);
	break;
    case POTION_HEALPOISON:
        pl->AddPoison(-(HEAL_POISON*QueryStrength()/100));
	break;
    case POTION_VIGOR:
       amount=VIGOR*QueryStrength()/100;
       if (pl->QueryAttr("EP")+amount>pl->QueryAttr("MEP"))
	     amount=pl->QueryAttr("MEP")-pl->QueryAttr("EP");
	   pl->SetAttr("EP",pl->QueryAttr("EP")+amount);
	break;
    case POTION_SATIATE:
        pl->SetFood(pl->QueryFood()+(SATIATE*QueryStrength()/100));
	break;
    case POTION_SLEEPING:
	    SetSleeping(1);
        call_out("awake",(SLEEP*QueryStrength()/100));
	break;
    default:
  }

}
void awake()
{
  write("Slowly you awake...you yawn loudly...\n");
  say(capitalize(this_player()->QueryName())+" awakes and yawns loudly.\n",
  ({this_player()}));
  SetSleeping(0);
}
int cmd_sleep(string str)
{
  if (!QuerySleeping()) return 0;
  if (-1!=member( ({"quit","tell","score","xdes","xlook"}), query_verb())) return 0;
  write("You are too tired right now ... You will sleep for "+
  find_call_out("awake")+" more seconds.\n");
  return 1;
}
init()
{
 ::init();
 if (p_type==POTION_SLEEPING) add_action("cmd_sleep","",1);
}

create()
{
    ::create();
	if (!is_clone()) return;
	// Default potions
	potions=
	([
	   POTION_HEALING:"A healing potion";
	    "You feel the energy of the potion filling your body.\n",
       POTION_MAGIC:"A magic potion";
        "You feel the contains of the potion raising your magic energy.\n",
       POTION_POISON:"A poisonous potion";
	    "You feel the contains of the potion poisoning your blood.\n",
	   POTION_HEALPOISON:"A heal poison potion";
	    "You feel the contains of the potion cleaning your blood.\n",
	   POTION_VIGOR:"A vigor potion";
	    "You feel more vigorous now.\n",
	   POTION_SATIATE:"A satiating potion";
	    "You feel less hungry now.",
	   POTION_INTOXICATE:"A intoxicating potion";
	    "You feel less hungry now.\n",
	   POTION_SLEEPING:"A narcotic potion";
	    "You suddenly feel very sleepy.\nYou fall asleep.\n",
	]);
    Set_Vessel_Name("bottle");
    Set_Drink_Name("potion");
    SetLong("This bottle contains some useless potion.\n");
    // SetFoodMsgs(..) can be used to modify the drink message
    SetFoodAlc(0);
    SetFoodHeal(0);
	SetValue(1000);
    Set_Vessel_Value(0);
    Set_Vessel_Weight(300);
    Set_Vessel_Volume(100);     /* 250 is one sip */
    SetStrength(100);           /* 100 is default */
	Set(P_MAGIC,"A magic potion.\n");
	Set(P_INFO,"A potion.\n");
    Set(P_HELP_MSG,"You can drink the potion.\n");
	SetSleeping(0);
}
// ------------------ Compatibility Functions ---------------------
int SetPower(int i) {return SetStrength(i*2);}
string QueryPower() {
  switch (QueryStrength()/2)
  {
     case 0..20:     return "very weak";
     case 21..40:    return "weak";
     case 41..80:   return "normal";
     case 81..120:  return "strong";
     case 121..150: return "very strong";
     case 151..170: return "potent";
     case 171..200: return "very potent";
     default: return "extremely powerful";
  }
}
// Set the name of the potion
string SetPotionName(string str) {return Set_Drink_Name(str);}

// ---------------- Identify --------------------------------------
// This function should provide special idendification of an item
string Identify()
{
string s;
  s=potions[p_type];
  if (!s) "A unknown potion";
  s+=" ("+QueryPower()+").\n";
  return s;
}

