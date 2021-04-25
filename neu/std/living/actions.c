/*---------------------------------------------------------------------------
** std/living/actions
**
** Simple action handling of a living.
**
** 25-Oct-1993  [Mateese]
**---------------------------------------------------------------------------
*/

#include <living.h>
#include <messages.h>

#define THIS this_object()
#define TP   this_player()
#define ENV  environment

inherit "/std/living/moving";

public int QueryActions();                    // std/living/description
public int QueryName();                       // std/living/description
public int QueryPossessive();                 // std/living/description
public void ForgetEnemies();                  // std/living/enemies
public int CheckAutoAttack(object victim);    // std/living/enemies
public void Kill(object victim);              // std/living/enemies
public int DoAction(int actions, int round);  // std/living/enemies
public void RemoveHeart(mixed hook);          // std/living/heart
public int QueryHeart();                      // std/living/heart

//---------------------------------------------------------------------------

private static int pActionRound;
private static int pStunned;

//---------------------------------------------------------------------------

public int QueryActionRound () { return pActionRound; }
public int SetActionRound (int i) { return pActionRound; }

//---------------------------------------------------------------------------

public int QueryStunned () { return pStunned; }
public int SetStunned (int rounds) {
  if (rounds > pStunned) {
    tell_object(THIS, "You are knocked out.\n");
    if (THIS != TP)
      see("  "+QueryName()+" got knocked out.\n");
    show("  "+QueryName()+" got knocked out.\n", ({ THIS, TP }));
    say(({MSG_STUN, previous_object(), rounds}));
    pStunned = rounds;
  }
  else if (pStunned && rounds < 1) {
    tell_object(THIS, "You regain your consciousness.\n");
    if (THIS != TP)
      see("  "+QueryName()+" regains "+QueryPossessive()+" consciousness.\n");
    show("  "+QueryName()+" regains "+QueryPossessive()+" consciousness.\n", ({ THIS, TP }));
    say(({MSG_STUN, 0, 0}));
    pStunned = 0;
  }
  return pStunned;
}

//---------------------------------------------------------------------------
public void OneStep (int justSched)

// Don't go away during combat.

{
  if (QueryHeart() & HEART_COMBAT)
    moving::OneStep(1);
  else
    moving::OneStep(justSched);
}

//---------------------------------------------------------------------------
public void init()

// Check if new players are to be attacked.

{
  int delay;

  ForgetEnemies();
  if (CheckAutoAttack(TP))
  {
    /* We decided to kill this player. Now how fast can we react? */
    UseStat(A_INT, 0);
    UseStat(A_DEX, 0);
    delay = 6 - (QueryInt()*STAT_SCALE+QueryDex()*STAT_SCALE)/50;
    if (delay < 1)
      Kill(TP);
    else
      call_out("Kill", delay, TP);
  }
}

//---------------------------------------------------------------------------
public void heart_beat()

// Simple combat heartbeat. Just do physical combat.

{
  int actions, oldactions, stun;
  pActionRound++;
  if (pStunned) {
    if (pStunned > 1)
      pStunned--;
    else
      SetStunned(0); // for proper announcement
  }
  else {
    actions = oldactions = QueryActions();
    DoAction(&actions, pActionRound);
    if (actions == oldactions)
      RemoveHeart(HEART_COMBAT);
  }
}

/***************************************************************************/
