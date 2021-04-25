/*
** The Spellmonster
**
** 20mar94 [r] Created
** 05apr95 [t] The spells might possibly effect the whole room
** 30aug95 [s] Repulse handling                               
**
** [r] Raist
** [t] Thragor
*/

#include <combat.h>
#include <magic.h>

#define SP_INFO 0 // the info of the spell (either a function or an array)
#define SP_SP   1 // spellpoints

#define SP_PLMSG   0 // message to player in the array
#define SP_ENVMSG  1 // message to environment in array
#define SP_MINDAM  2 // minimal damage
#define SP_MAXDAM  3 // maximum damage
#define SP_DAMTYPE 4 // damagetype (see combat.h)
#define SP_ROOM    5 // room-attack yes/no

#define TO this_object()
#define LSF(x) symbol_function(x,TO)

inherit "/std/npc";

private static mapping spells;
private static int spellchance;

public int create() 
{
  if (::create()) return 1;
  spellchance = 50;
  return 0;
}

public varargs mapping
  AddSpell(
    string name, // the name of the spell used as key (has to be unique)
    mixed msg1,  // the message to the player who got hit
                 // or the function to be called
    mixed msg2,  // the message to the environment
                 // or the spellpoints, if a function is called instead
    int mindam,  // the minimal damage
    int maxdam,  // the maximum damage
    int damtype, // how do we damage our enemies
    int sp,      // how many spellpoints does it cost to cast this spell
    status room) // if !=0 all players in the room will be effected
{  
   if (!name) return spells;
   if (!spells) spells = m_allocate(0,2);
   if (!damtype) damtype=ST_ALL;
   
   if (closurep(msg1))
     {
       spells[name,SP_INFO] = msg1;
       spells[name,SP_SP]   = msg2;
       // if the spellpoints for this are 0, the function has to handle
       // them; so maybe AddSpell("name",#func); is enough
     }
   else
     {
       if (damtype >= ST_OTHER || damtype < ST_ALL) damtype = ST_PSYCHO;
       if (!msg1) msg1 = "get hit by a lightning bolt.";
       if (mindam > maxdam) mindam = maxdam;
       spells[name,SP_INFO]             = allocate(6);
       spells[name,SP_INFO][SP_PLMSG]   = msg1;
       spells[name,SP_INFO][SP_ENVMSG]  = msg2;
       spells[name,SP_INFO][SP_MINDAM]  = mindam;
       spells[name,SP_INFO][SP_MAXDAM]  = maxdam;
       spells[name,SP_INFO][SP_DAMTYPE] = damtype;
       spells[name,SP_INFO][SP_ROOM]    = room;
       spells[name,SP_SP] = sp;
     }
  return spells;
}

public mapping SetSpells(mapping sp)
{
  return spells = sp;
}

public mapping QuerySpells()
{
  return spells||([]);
}

public int SetSpellChance(int ch)
{
  return spellchance = ch;
}
public int QuerySpellChance()
{
  return spellchance;
}


protected void RestoreSP(int val)
{
  int sp;
  
  sp = (int)QuerySP();
  if (sp = (sp+val)>QueryMaxSP())
    sp = (int)QueryMaxSP();
  SetSP(sp);
}

protected int valid_victim(object pl)
{
  return
        pl
      &&!pl->QueryGhost()
      &&interactive(pl)
      &&pl!=TO
      &&living(pl);
}

protected string add_dot(string str)
{
  return str+(str[<1]=='.'
              ||str[<1]=='!'
              ||str[<1]=='?'
              ?"":".");
}

/* Dummy repulse function */
varargs int Repulse(object target,string spname,mixed damtype)
{
mixed info;
  if (!target || ! spname) return 0;
  if (!damtype)
  {
    if (!pointerp(info = spells[spname,SP_INFO])) return 0;
    damtype=info[SP_DAMTYPE];
  }
  return CASTER->Repulse(target,TO,damtype,spname,TO);
}
void CastSpell() 
{
  string *keys,key;
  object who,*en,*targets;
  int i;
  mixed info;
  
  if (!environment(TO)   // No environment
      ||!spells
      ||!sizeof(spells)             // No spells there
      ||!(en = QueryEnemies()||({}))
      ||!sizeof(en))                // No enemies there
    return;

  key = (keys = m_indices(spells))[random(sizeof(keys))];
  if (!ReduceSP(spells[key,SP_SP])) return; // not enough spellpoints
  if (closurep(spells[key,SP_INFO]))
    {
      tell_room(environment(),QueryName()+" casts a spell.\n");
      if (!funcall(spells[key,SP_INFO],environment(),en))
        RestoreSP(spells[key,SP_SP]/2);
      // if the spell failed
      return;
    }
  if (!pointerp(info = spells[key,SP_INFO]))
    return;
  if (info[SP_ROOM])
    targets = filter(all_inventory(environment()),LSF("valid_victim"));
  else
    targets = ({en[random(sizeof(en))]});
  i = sizeof(targets);
  tell_room(environment(),QueryName()+" casts a spell.\n");
  while(i--)
    {
      if (!targets[i] || !present(targets[i],environment()))
        continue;
      if (Repulse(targets[i],key,info[SP_DAMTYPE]) )
	{
	  RestoreSP(spells[key,SP_SP]/2); 
	  continue;
	}
      tell_object(targets[i],"You "+add_dot(info[SP_PLMSG])+"\n");
      if (info[SP_ENVMSG])
        tell_room(environment(),targets[i]->QueryName()+" "+
          add_dot(info[SP_ENVMSG])+"\n",({targets[i]}));
      targets[i]->Defend(
        info[SP_MINDAM]+random(info[SP_MAXDAM]-info[SP_MINDAM]+1),
        info[SP_DAMTYPE],0);
    }
}
// called by repulse, but only default action yet
varargs mixed NotifySpellFail(object enemy,object caster,string spname)
{
 return 0;
}

public void Attack() 
{
  (::Attack());
  if (random(100) < spellchance) CastSpell();
}
