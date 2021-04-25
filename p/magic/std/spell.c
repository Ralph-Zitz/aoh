//---------------------------------------------------------------------------
// /p/magic/std/spellr.c
//
// The Std spell object in OSB
// (c) Softbyte@OSB 20jun98
// V1.0:  Basic spell handling and functions
//---------------------------------------------------------------------------

inherit "/std/base";

#include <magic.h>
#include <skills.h>

// ----------------- INTERNAL -------------------------------------------
static int sp_type,sp_backfire,sp_difficulty,sp_delay,sp_duration;
static string sp_short,sp_long;
static mixed sp_cost;
static string sp_skill;

/* The type of the spell form magic.h */
void SetSpellType(int type) {sp_type=type;}
int QuerySpellType() {return sp_type;}

/* The castname of the spell .. extract from filename */
string QueryCastName()
{
  return explode(object_name(this_object()),"/")[<1];
}

/* The classname of the spell .. extract from filename */
string QueryClassName()
{
  return explode(object_name(this_object()),"/")[<2];
}

/* The short name of the spell. Per default the castname */
void SetShort(string name) {sp_short=name;}
string QueryShort() { return sp_short||(add_a(QueryCastName())+" spell"); }

/* The long name of the spell. */
void SetLong(string name) {sp_long=name;}
string QueryLong() { return sp_long; }

/* The cost in SP for the spell. Int or closure returning int */
void SetCastCost(mixed cost)
{
  if (!closurep(cost) && !intp(cost) ) 
  {
    raise_error("Wrong argument to SetCastCost.\n");
    return;
  }
  sp_cost=cost;
}
int QueryCastCost()
{
  if (closurep(sp_cost)) return funcall(sp_cost);
  return sp_cost;
}

/* The backfire chance of the spell 0..1000 (0=none) */
void SetBackfireChance(int chance) {sp_backfire=chance;}
int QueryBackfireChance() {return sp_backfire;}

/* The difficulty of the spell 0..1000 (0=very easy) */
void SetDifficulty(int diff) {sp_difficulty=diff;}
int QueryDifficulty() {return sp_difficulty;}

/* The delay until the spell ends and CleanUp is called in seconds */
void SetDuration(int delay) {sp_duration=delay;}
int QueryDuration() {return sp_duration;}

/* The cast delay until the spell starts in seconds */
void SetCastDelay(int delay) {sp_delay=delay;}
int QueryCastDelay() {return sp_delay;}

/* On which skill depends the spell */
void SetSkill(string skill) { sp_skill=skill; }
string QuerySkill() {return sp_skill;}


/* Get the knowledge of a living to this spell (0..1000, 0=none) */
int GetKnowledge(object liv)
{
  if (!liv) return 0;
  return liv->QuerySpell(QueryCastName());
}

/*
   Does the spell succeed , +: yes, -: no
*/
int CheckSuccess(object liv)
{
  int wis,diff;
  int knowledge;
  int skill,magic;

  if (!liv) return 0;

  diff=QueryDifficulty();        // 0..1000

  wis=liv->QueryWis()*10;        // 0..100*10
  knowledge=GetKnowledge(liv);   // 0..1000

  if (sp_skill) // check the skill if given
  {
      skill=liv->UseSkill(sp_skill,diff);  // -1000..1000
  }
  else skill=1000;  // force succeed

  // combine Wisdom and spellkowledge to magic knowledge
  // TODO improve following formula
  magic=(knowledge*3+wis+2*skill)/6;
  if (magic<1) magic=1;
  if (magic>1000) magic=1000; // should never happen, but who knows

  if (random(magic)>diff/2) return magic;
  else return -magic;
}

create()
{

  ::create();
  if (is_clone())
  {
    destruct(this_object());
    return;
  }
  seteuid(getuid());

  // to std-spell
  sp_delay=0;
  sp_duration=0;
  sp_skill=0;
  sp_type=0;
  sp_backfire=500;
  sp_difficulty=500;
  sp_cost=0;
}

// ----------------- EXTERNAL -------------------------------------------
/* Will be called when a spell can be cast successfully  
   liv: The living which casts the spell
   arg: The arguments given to teh cast
   target: If possible the object array of the targets of the spell
*/
mixed OnSucceedCasting(object liv, string arg,object *target )
{
  write("Error: No cast function defined.\n");
  // TODO log_file("magic/...",...
  return 1;
}

/* Will be called when a spell fails 
   liv: The living which casts the spell
   arg: The arguments given to teh cast
   target: If possible the object array of the targets of the spell
   modus: BACKFIRE or FAIL
*/
mixed OnFailCasting(object liv,string arg,object *target,int modus)
{
  write("You fail in casting "+QueryShort()+".\n");
  return 1;
}

/*
   Check whether the living is allowed to cast the spell. 
   Return a string which will be notify_failed to the player or
   0 for ok 
*/
string CannotCast(object liv,string arg,object *target)
{
  return 0; // everybody can cast
}

/*
   Will be called if the spell is finished. Do all cleanup here 
   Mode=SP_SUCCESS or SP_FAILURE to indicate why cleanup is called
*/
int CleanUp(object liv,string arg,object *target,int mode)
{
  return 1; // all done
}
/*
   Will be called when a spell is begun to be cast but before the
   Success checks. This function is optinonal and actually only
   needed in the case of delayed spells
   liv: The living which casts the spell
   arg: The arguments given to teh cast
   target: If possible the object array of the targets of the spell
*/
mixed OnCastMessage(object liv, string arg,object *target )
{
  if (!liv) return 0;
  tell_object(liv,"You begin casting "+add_a(QueryShort())+".\n");
  if (liv->QueryName() && environment(liv))
    tell_room(environment(liv),capitalize(liv->QueryName())+
    "begins to cast "+add_a(QueryShort())+".\n",({liv})); 
  return 1;
}

/* Return the strength level of this spell if you have a proficienty
   of value in that spell
*/
int QueryStrengthLevel(int value)
{
  return 0;
}

