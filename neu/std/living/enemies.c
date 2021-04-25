/*---------------------------------------------------------------------------
** std/living/enemies
**
** Simple handling of a livings enemies.
**
** 25-Oct-1993  [Mateese]
**---------------------------------------------------------------------------
*/

#include "/neu/secure/wizlevels.h"
#include <attributes.h>
#include <search.h>
#include <living.h>
#include <messages.h>
#include <search.h>

#define THIS this_object()
#define TP   this_player()
#define ENV  environment

inherit "/neu/std/living/combat";

public varargs mixed Query(string pn, int sc);
                                 // std/base
public void DoAttackChat();      // std/living/chat
public int CanSeeLiving(object lib); // std/living/description
public varargs mixed Search(mixed what, int mode);
                                 // std/living/description
public int QueryIsPlayer();      // std/living/description
public int QueryIsNPC();         // std/living/description
public mixed AddHeart(mixed hook);  // std/living/heart
public varargs int UseStat(string name, int diff);
                                 // std/living/stats
public int QueryInt();           // std/living/stats

//---------------------------------------------------------------------------

private static mixed pFriendPred;
private static mapping pEnemies;
private int aAggressive, aAttChance;
private static int lastround;
private static object *leftWp;
private static string *leftHand;
private static int lastForgetDone;

//---------------------------------------------------------------------------

public mapping QueryEnemies ()        { return pEnemies; }
public mapping SetEnemies (mapping e) { return pEnemies = e; }

public int QueryAggressive ()         { return aAggressive; }
public int SetAggressive (int i)      { return aAggressive = i; }

public int QueryAttackChance ()       { return aAttChance; }
public int SetAttackChance (int i)    { return aAttChance = i; }

public mixed QueryFriendObj () { return pFriendPred; }
public mixed SetFriendObj (mixed p) { return pFriendPred = p; }

//---------------------------------------------------------------------------
public void AddEnemy (object enemy)
{
  pEnemies = m_delete(pEnemies, enemy);
  if (Search(enemy, SEARCH_ENV|SEARCH_OBJECT))
    pEnemies[enemy] = time();
  else
    pEnemies[enemy] = -time();
  say(({MSG_ENEMY, enemy, pEnemies[enemy]}));
}

public void RemoveEnemy (object enemy)
{
  pEnemies = m_delete(pEnemies, enemy);
  say(({MSG_ENEMY, enemy, 0}));
}

public void StartHunt (object enemy)
{
  pEnemies[enemy] = -time();
  say(({MSG_ENEMY, enemy, pEnemies[enemy]}));
}

public void StopAllCombat()
{
  int i;
  object *obj, o;

  obj = m_indices(pEnemies);
  for (i = sizeof(obj); i--;)
  {
    o = obj[i];
    RemoveEnemy(o);
    if (o) o->RemoveEnemy(THIS);
  }
}

//---------------------------------------------------------------------------
public int CheckForgetEnemy (object enemy, int diff)

// Return true if the enemy is to be forgot.

{
  return (pEnemies[enemy] < 0) && UseStat(A_INT, diff);
}


public void ForgetEnemies ()

// Forget enemies.

{
  int diff, timesteps;
  float faktor;
  object *enem;

  timesteps = (time() - lastForgetDone) / ENEMY_FORGET;
  if (timesteps < 1 || !sizeof(pEnemies))
    return;
  lastForgetDone = time();
  diff = 5;
  if (timesteps > 1) {
    faktor = to_float(QueryInt()) / to_float(100);
    faktor = exp(to_float(timesteps) * log(faktor));
    diff += 100 - to_int(faktor * 100);
  }
  enem = filter(m_indices(pEnemies), #'CheckForgetEnemy, diff);
  map(enem, #'RemoveEnemy);
}

//---------------------------------------------------------------------------
public varargs int IsFriend (object victim, int internal)

// Return true for 'Friend' livings.

{
  mixed friendp;
  if (!internal) {
    if (closurep(friendp = QueryFriendObj()))
      return funcall(friendp, victim);
    if (objectp(friendp))
      return friendp->IsFriend(victim);
  }
  return    (QueryIsNPC() && victim->QueryIsNPC())
         || (QueryIsPlayer() && victim->QueryIsPlayer());
}

//---------------------------------------------------------------------------
public int CheckAutoAttack (object victim)

// Return true if the victim may be attacked without provocation.

{
  return    victim != THIS
         && ENV(victim) == ENV()
         && victim->QueryLState() != LSTATE_GHOST
         && CanSeeLiving(this_player())
         && (!query_once_interactive(victim) 
             || !IS_IMMORTAL(victim) || victim->Query(P_NOWIZ)
	    )
         && Search(victim, SEARCH_ENV|SEARCH_OBJECT)
         && (   pEnemies[victim]
             || (aAggressive && random(1000) < aAttChance)
            )
         && (!IsFriend(victim) || pEnemies[victim])
        ;
}

//---------------------------------------------------------------------------
public void NotifyMiss(object weapon, int success)
{
  int oldchance;

  oldchance = aAttChance;
  if (aAggressive)
    aAttChance *= 2;
  if (TP && !pEnemies[TP] && CheckAutoAttack(TP)) {
    AddEnemy(TP);
    AddHeart(HEART_COMBAT);
  }
  aAttChance = oldchance;
}

//---------------------------------------------------------------------------
public void Kill (object victim)

// Start combat with victim

{
  if (victim && victim != THIS)
  {
    AddEnemy(victim);
    AddHeart(HEART_COMBAT);
  }
}

//---------------------------------------------------------------------------
public mixed SelectWeapon (object victim)

// Select a weapon for attack, which may be a true weapon (object) or
// a natural one (string).
// For multiple calls, return different weapons if possible.
// Return 0 if all weapons have been used.

{
  object obj;
  string hand;
  int i;

  if (sizeof(leftWp))
  {
    i = random(sizeof(leftWp));
    obj = leftWp[i];
    leftWp = leftWp[0..i-1] + leftWp[i+1..];
    return obj;
  }

  if (sizeof(leftHand))
  {
    i = random(sizeof(leftHand));
    hand = leftHand[i];
    leftHand = leftHand[0..i-1] + leftHand[i+1..];
    return hand;
  }

  return 0;
}

//---------------------------------------------------------------------------
public object SelectEnemy ()

// Select enemy to attack. This also marks some enemies as hunted if they
// are gone.

{
  object *obj, o;
  int i, left;

  if (!pEnemies)
    return 0;
  obj = m_indices(pEnemies);
  left = sizeof(obj);
  if (!left)
    return 0;

  i = random(left);
  while (left--)
  {
    o = obj[i];
    i = (i+1) % sizeof(obj);

    if (pEnemies[o] < 0 || (query_once_interactive(o) && !interactive(o)))
      continue;

    if (Search(o, SEARCH_ENV|SEARCH_INV|SEARCH_OBJECT))
      return o;

    if (environment() == o)
      return o;
      
    StartHunt(o);
  }
  return 0;
}

//---------------------------------------------------------------------------
public int DoAction (int actions, int round)

// Attack as many enemies as actions are provided.
// Return remaining actions.

{
  mixed weapon;
  object victim;
  int i;

  if (actions <= 0) return 0;

  if (!aHands && !pWeapons)
    return actions;

  if (round > lastround)
  {
    lastround = round;
    leftWp = mappingp(pWeapons) ? m_indices(pWeapons) : ({});
    leftHand = mappingp(aHands) ? m_indices(aHands) : ({});
  }

  if (Query(P_ACHAT_CHANCE))
    DoAttackChat();

  while (actions > 0)
  {
    victim = SelectEnemy();
    if (!victim)
      break;
    weapon = SelectWeapon(victim);
    if (!weapon)
    {
      leftWp = pWeapons ? m_indices(pWeapons) : ({});
      leftHand = aHands ? m_indices(aHands) : ({});
      weapon = SelectWeapon(victim);
      if (!weapon)
        break;
    }
    actions--;
    Hit(victim, weapon);
  }

  return 1;
}

//---------------------------------------------------------------------------
// Redefine the following functions from std/living/combat for proper
// enemy tracking.

public int Dodge(mixed weapon, int success)
{
  if (   TP && TP != THIS && !pEnemies[TP]
      && Search(TP, SEARCH_ENV|SEARCH_OBJECT))
    AddEnemy(TP);
  AddHeart(HEART_COMBAT);
  return combat::Dodge(weapon, success);
}

public int Defend(mapping damage, object weapon, int success)
{
  if (   TP && TP != THIS && !pEnemies[TP]
      && Search(TP, SEARCH_ENV|SEARCH_OBJECT))
    AddEnemy(TP);
  AddHeart(HEART_COMBAT);
  return combat::Defend(damage, weapon, success);
}

//---------------------------------------------------------------------------
public void create()
{
  "combat"::create();
  pEnemies = m_allocate(0,ENEMY_SIZE);
  aAttChance = 1000;
  aAggressive = 0;
  lastForgetDone = time();
}

/***************************************************************************/
