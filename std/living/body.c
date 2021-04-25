/* std/living/body
**
**  This are the life functions of the living.
**
**  09-Aug-94 [Mateese]
**  19-Nov-97 [Softbyte] Stat/Skill handling removed, new stats added
*/

#include <attributes.h>
#include <combat.h>
#include <combattune.h>
#include <living.h>
#include <messages.h>
#include <moving.h>
#include <secure/wizlevels.h>
#include <driver/object_info.h>

#define THIS this_object()
#define TP   this_player()
#define ENV  environment
#define PREV previous_object()

#define NETDEAD  (!interactive() && query_once_interactive(THIS))

public varargs mixed Query(string pn, int sc);
                                 // std/base
public varargs mixed Set(string pn, mixed arg, int sc);
                                 // std/base
public varargs mixed QueryAttr(string aname, int sc);
                                 // std/living/attributes
public varargs mixed SetAttr(string aname, mixed value, int sc);
                                 // std/living/attributes
public void StopAllHunting();    // std/living/combat
public object *QueryEnemies();   // std/living/combat
public int QueryCombatDelay ();  // std/living/combat
public mixed QueryStunned();     // std/living/combat
public int QueryAlign();         // std/living/description
public int QueryLState();        // std/living/description
public int SetLState(int i);     // std/living/description
public int QueryLevel();         // std/living/description
public string QueryName();       // std/living/description
public string QueryRace();       // std/living/description
public varargs int remove();     // std/living/moving
public void Wimpy();             // std/living/moving
public void AddHeart(mixed hook);     // std/living/heart
public void RemoveHeart(mixed hook);  // std/living/heart
public int CheckHeart(mixed hook);    // std/living/heart
public int QueryCon();           // std/living/stats
public int QueryInt();           // std/living/stats
#ifdef NF
public int ApplyStat(string name, int diff);
                                 // std/living/stats
public int UseStat(string name, int diff);
                                 // std/living/stats
#endif
public varargs void DropOverload(int howmuch);
                                 // std/living/restrictions
public int QueryLoad();          // std/living/restrictions
public int QueryWeight();        // std/living/restrictions
public int QueryWeightContent(); // std/living/restrictions

/*--------------------------------------------------------------------------*/

private int aHP, aMaxHP, pRegHP;
private int aSP, aMaxSP, pRegSP;
private int aPEP, aMaxPEP, pRegPEP;
private int aMEP, aMaxMEP, pRegMEP;
private int aXP, pKillXP;
private int aMaxPoison, aPoison, pDegPoison;
private int aRateHP, aRateSP, aRatePEP, aRateMEP, aRatePoison;
private mixed aWimpy, aCorpse;

// These variables accumulate the 1/10-regeneration points
// over the heart_beat()s.

private int accHP, accSP, accPEP, accMEP, accP;

/*--------------------------------------------------------------------------*/

public varargs void Die(int silent);            // forward
public varargs mixed QueryWimpy (int absolut);  // forward
public int QueryPoison ();                      // forward
public int SetPoison (int i);                   // forward
static int update_points_display();             // forward
/*--------- Hitpoints ------------------------------------------------------*/


public int QueryMaxHP()
{
  return aMaxHP;
}

public int SetMaxHP (int i) {
  int _upd = 0;
  if (aMaxHP != i)
    _upd = 1;
  aMaxHP = i;
  if (_upd)
    THIS->update_points_display();
  return aMaxHP;
}

public int QueryHP ()        { return aHP; }
public int SetHP (int i)
{
  int _upd = 0;
  if (NETDEAD || QueryLState() == LSTATE_GHOST)
    return aHP;
#if 1
  if (query_once_interactive(THIS) && i < 0)
  {
    object ob;
    string msg;
    int j;

    msg = sprintf("%s %O->SetHP(%O)\n", ctime(), this_object(), i);
    for (j = 0; ob = previous_object(j); j++)
      msg += sprintf("     %d: %O\n", j, ob);
    log_file("KILLPROBLEM", msg);
  }
#endif
  AddHeart(HEART_BODY);
  if (aHP != i)
    _upd = 1;
  aHP = i;
  if (_upd)
    THIS->update_points_display();
  return aHP;
}

public int AddHP (int hps) { return SetHP(hps + QueryHP()); }

public int QueryHealth() { return (QueryHP() * 100) / (QueryMaxHP()||1); }

public string HealthMsg(int health, int stunned) {
  string str;

  switch(health) {
  case  0..10:
    str = "in very bad shape"; break;
  case 11..20:
    str = "in bad shape"; break;
  case 21..50:
    str = "severely wounded"; break;
  case 51..80:
    str = "wounded"; break;
  case 81..90:
    str = "slightly hurt"; break;
  case 91..99:
    str = "in good shape"; break;
  default:
    if (health < 0)
      str = "living dead";
    else
      str = "perfectly healthy";
  }
  if (stunned)
  {
    str +=  (health > 90 ? " but " : " and ");
    if (stunned > 10)
      str += "deeply unconscious";
    else if (stunned > 5)
      str += "unconscious";
    else
      str += "stunned";
  }
  return str;
}

public string QueryHealthMsg() {
  return HealthMsg(QueryHealth(), QueryCombatDelay());
}

/*--------------------------------------------------------------------------*/

public varargs int ReduceHP (int amount, int min, int check_ko)
{
  /* Note that the function is also used to heal the living. */

  int oldh, hp, newh, reldam;

  reldam = (amount * 100) / ((hp = QueryHP())||1);
  if (reldam > 100) reldam = 100;
  oldh = QueryHealth();

  if (min >= hp)
    return hp;

  if (hp - min < amount)
    amount = hp-min;

  SetHP(hp -= amount);

#ifdef NF
  if (check_ko)
    if (reldam < 5)
      ApplyStat(A_CON, 0);
    else
      ApplyStat(A_CON,80-(reldam/2));
#endif

  if (hp >= 0 && oldh / 10 != (newh = QueryHealth())/10 && THIS)
    tell_object(THIS, "You are "+((amount < 0 && newh < 95) ? "still " : "")
                                +HealthMsg(newh, QueryCombatDelay())+".\n"
               );
  AddHeart(HEART_BODY);
  return hp;
}

public int HealHP (int amount)
{
  int maxhp;

  maxhp = QueryMaxHP();
  if (amount < 0 || (QueryHP() >= maxhp && amount > 0))
    return 0;
#ifdef NF
  UseStat(A_CON, 0);
#endif
  amount = -amount;
  if (ReduceHP(&amount, 0) >= maxhp)
    SetHP(maxhp);
  return -amount;
}

public varargs int DoDamage (int damage, int gentle)
{
  int hp;
  if (NETDEAD || QueryLState() == LSTATE_GHOST || damage < 0)
    return 0;
  if (gentle < 0)
    gentle = (-gentle)*QueryMaxHP() / 100;
#ifdef NF
  UseStat(A_CON, 0);
#endif
  hp = ReduceHP(&damage, gentle || -1, 1);
  say(({MSG_DAMAGE, PREV, damage }));
  AddHeart(HEART_BODY);
  if (hp < 0)
  {
    SetHP(0);
    Die();
  }
  else if (damage > 0 && hp <= QueryWimpy(1))
    Wimpy();

  return damage;
}

public int QueryRegHP ()     { return pRegHP; }
public int SetRegHP (int i)  { return pRegHP = i; }
public int AddRegHP (int i)  { return pRegHP += i; }

public int QueryRateHP ()     { return aRateHP; }
public int SetRateHP (int i)  {
  return SetRegHP(aRateHP = (i > 0 ? i : aRateHP));
}

//-------- Spellpoints --------------------------------------------------

// Softbyte 19nov97
public int QueryMaxSP () {
  return aMaxSP;
//  return aMaxSP || (42+(5/STAT_SCALE)*QueryInt()*8);
}

public int SetMaxSP (int i) {
  int _upd = 0;
  if (aMaxSP != i)
    _upd = 1;
  aMaxSP = i;
  if (_upd)
    THIS->update_points_display();
  return aMaxSP;
}

public int QuerySP ()        { return aSP; }
public int SetSP (int i) {
  int _upd = 0;
  if (NETDEAD || QueryLState() == LSTATE_GHOST || i < 0)
    return aSP;
  AddHeart(HEART_BODY);
  if (aSP != i)
    _upd = 1;
  aSP = i;
  if (_upd)
    THIS->update_points_display();
  return aSP;
}

public int AddSP (int sps)        { return SetSP(sps + QuerySP()); }

public int QueryRegSP ()     { return pRegSP; }
public int SetRegSP (int i)  { return pRegSP = i; }
public int AddRegSP (int i)  { return pRegSP += i; }

public int QueryRateSP ()     { return aRateSP; }
public int SetRateSP (int i)  {
  return SetRegSP(aRateSP = (i > 0 ? i : aRateSP));
}

public varargs int ChangeSP (int amount, int min)
{
  int sp;

  if ((sp = QuerySP()) - min < amount)
    amount = sp-min;
  SetSP(sp -= amount);
  AddHeart(HEART_BODY);
  return sp;
}

public int ReduceSP (int amount, int min) {
  int sp;

  if ((sp = QuerySP()) - min < amount)
    return 0;
  SetSP(sp -= amount);
  AddHeart(HEART_BODY);
  return amount;
}

public int HealSP (int amount)
{
  int maxsp;

  maxsp = QueryMaxSP();
  if (amount < 0 || (QuerySP() >= maxsp && amount > 0))
    return 0;
#ifdef NF
  UseStat(A_INT, 0);
#endif
  amount = -amount;
  if (ChangeSP(&amount, 0) >= maxsp)
    SetSP(maxsp);
  return -amount;
}

//-------- Physical endurance points ------------------------------------

// Softbyte 19nov97
public int QueryMaxPEP () { return aMaxPEP; }

public int SetMaxPEP (int i) {
  int _upd = 0;
  if (aMaxPEP != i)
    _upd = 1;
  aMaxPEP = i;
  if (_upd)
    THIS->update_points_display();
  return aMaxPEP;
}

public int QueryPEP ()        { return aPEP; }
public int SetPEP (int i) {
  int _upd = 0;
  if (NETDEAD || QueryLState() == LSTATE_GHOST || i < 0)
    return aPEP;
  AddHeart(HEART_BODY);
  if (aPEP != i)
    _upd = 1;
  aPEP = i;
  if (_upd)
    THIS->update_points_display();
  return aPEP;
}

public int AddPEP (int sps)        { return SetPEP(sps + QueryPEP()); }

public int QueryRegPEP ()     { return pRegPEP; }
public int SetRegPEP (int i)  { return pRegPEP = i; }
public int AddRegPEP (int i)  { return pRegPEP += i; }

public int QueryRatePEP ()     { return aRatePEP; }
public int SetRatePEP (int i)  {
  return SetRegPEP(aRatePEP = (i > 0 ? i : aRatePEP));
}


varargs public int Tire(int amount, int min) {

  int pep;
  if (!amount) amount=STAT_PEP_DEFAULT;   // defult argument
  pep=QueryPEP();
  if (pep-amount < min) return 0;

  SetPEP(pep -= amount);
  AddHeart(HEART_BODY);
  return amount;
}

public int HealPEP (int amount)
{
  int maxpep,pep;

  if (amount<=0) return 0;

  maxpep = QueryMaxPEP();
  pep = QueryPEP();

  if (pep+amount>maxpep) SetPEP(maxpep);
  else SetPEP(pep+amount);

  return amount;
}

//-------- Mental endurance points --------------------------------------

// Softbyte 19nov97
public int QueryMaxMEP () { return aMaxMEP; }

public int SetMaxMEP (int i) {
  int _upd = 0;
  if (aMaxMEP != i)
    _upd = 1;
  aMaxMEP = i;
  if (_upd)
    THIS->update_points_display();
  return aMaxMEP;
}

public int QueryMEP ()        { return aMEP; }
public int SetMEP (int i) {
  int _upd = 0;
  if (NETDEAD || QueryLState() == LSTATE_GHOST || i < 0)
    return aMEP;
  AddHeart(HEART_BODY);
  if (aMEP != i)
    _upd = 1;
  aMEP = i;
  if (_upd)
    THIS->update_points_display();
  return aMEP;
}

public int AddMEP (int sps)        { return SetMEP(sps + QueryMEP()); }

public int QueryRegMEP ()     { return pRegMEP; }
public int SetRegMEP (int i)  { return pRegMEP = i; }
public int AddRegMEP (int i)  { return pRegMEP += i; }

public int QueryRateMEP ()     { return aRateMEP; }
public int SetRateMEP (int i)  {
  return SetRegMEP(aRateMEP = (i > 0 ? i : aRateMEP));
}

varargs public int Drain(int amount, int min) {

  int mep;
  if (!amount) amount=STAT_MEP_DEFAULT;   // default argument
  mep=QueryMEP();
  if (mep-amount < min) return 0;

  SetMEP(mep -= amount);
  AddHeart(HEART_BODY);
  return amount;
}

public int HealMEP (int amount)
{
  int maxmep,mep;

  if (amount<=0) return 0;

  maxmep = QueryMaxMEP();
  mep = QueryMEP();

  if (mep+amount>maxmep) SetMEP(maxmep);
  else SetMEP(mep+amount);

  return amount;
}

//-----------------------------------------------------------------------

public int QueryMaxPoison () {
  return aMaxPoison || (42+(5/STAT_SCALE)*QueryCon()*8);
}
public int SetMaxPoison (int i) {
  if (!query_once_interactive(THIS))
    aMaxPoison = i;
  return QueryMaxPoison();
}

public int QueryPoison ()    { return aPoison; }
public int SetPoison (int i) {
  int hp, maxp;
  if (NETDEAD || i < 0)
    return aPoison;
  AddHeart(HEART_BODY);
  hp = QueryHP();
  maxp = QueryMaxPoison();
  if (i > maxp && i > aPoison) {
    hp -= (i - maxp);
    i -= (i - maxp) >> 1;
    SetHP(hp);
  }
  return aPoison = i;
}

public int AddPoison (int p)        { return SetPoison(p + QueryPoison()); }

public int QueryDegPoison ()    { return pDegPoison; }
public int SetDegPoison (int i) { return pDegPoison = i; }
public int AddDegPoison (int i) { return pDegPoison += i; }

public int QueryRatePoison ()        { return aRatePoison; }
public int SetRatePoison (int i)
{
  return SetDegPoison(aRatePoison = (i > 0 ? i : aRatePoison));
}

//-----------------------------------------------------------------------

public mixed SetWimpy (mixed v) {
  return aWimpy = v;
}

public varargs mixed QueryWimpy (int absolut) {
  return (absolut && stringp(aWimpy))
          ? (QueryMaxHP() * to_int(aWimpy)) / 100
          : aWimpy;
}

public mixed SetCorpse(mixed c) { return aCorpse = c; }
public mixed QueryCorpse()      { return aCorpse; }

//-----------------------------------------------------------------------

public int QueryXP ()             { return aXP; }
public int SetXP (int i)          { return aXP = i >= 0 ? i : 0; }
public varargs int AddXP (int xp)         { return SetXP(xp+QueryXP()); }

public int SetKillXP (int i)      { return pKillXP = i; }
public varargs int QueryKillXP (int count) {
  int xp;

  if (pKillXP)
    return pKillXP / (count || 1);

  xp = QueryXP();
#ifdef CT_KILL_XP
  CT_KILL_XP
#else
  xp = XP_GAIN(xp);
  if (xp > MAX_XP_ADD/2)
    xp = MAX_XP_ADD/2 + (xp-MAX_XP_ADD/2)/10;
  if (xp > MAX_XP_ADD)
    xp = MAX_XP_ADD + (xp-MAX_XP_ADD)/100;
#endif
  xp = xp/(count || 1);
  return xp;
}

//-----------------------------------------------------------------------

public void Heal(int amount)
{
  HealHP(amount);
  HealSP(amount);
  HealPEP(amount);
  HealMEP(amount);
}

/*--------------------------------------------------------------------------*/
/* Regenerate/poisen the living. */

public int body_heartbeat() {
  int rate, val, max;
  int hp, reghp, poison;
  int flag, load, stun, health;

  if (NETDEAD || QueryLState() == LSTATE_GHOST)
    return 1;

  health = QueryHealth();
  load = QueryLoad();
  hp = QueryHP();
  reghp = QueryRegHP();
  stun = QueryStunned();
  flag = CheckHeart(HEART_COMBAT);

  //  80% <  load <= 100%: ratehp reduces to 0
  // 100% <  load <= 200%: ratehp reduces to -QueryRegHP();
  if (load > LOAD_THRESH) {
    max = val = QueryRateHP();
    if (load <= 100)
      val -= (100-load) * max / (100-LOAD_THRESH);
    else
      val = (load-200) * max / 100;
    if (val < 0) { // Direct damage by muscle strain
      hp -= val;
      if (hp < LOAD_MINHP(QueryMaxHP())) {  // Collapse
        hp = LOAD_MINHP(QueryMaxHP());
        DropOverload(1);
        val = 0;
      }
      else
        flag = 1;
    }
    reghp -= (max - val);
  }

  poison = QueryPoison();
  max = QueryMaxPoison();
  if (poison > 0) {
    if (poison <= max)
      reghp -= (poison * QueryRateHP()) / (max || 1);
    else if (reghp > 0)
      reghp = 0;

    accP += QueryDegPoison();
    rate = poison - accP/10 - max;
    if (rate > 0) {  // Decrease poison over max level extremely fast.
      accHP -= rate;
      accP += rate >> 1;
    }
    if (accP / 10) {
      poison -= accP/10;
      accP %= 10;
      if (poison <= 0) poison = accP = 0;
      else flag = 1;
      SetPoison(poison);
    }
    else flag = 1;
  }

  val = QuerySP();
  max = QueryMaxSP();
  rate = QueryRegSP();
  // was: if (rate < 0 || (val <= max && !load)) {
  // but this kept the heart beat unnecessarily on.
  if (rate < 0 || (val < max && !load)) {
    accSP += rate;
    if (accSP/10) {
      val += accSP/10;
      accSP %= 10;
      if (accSP >= 0 && val >= max) val = max, accSP = 0;
      else if (val != max) flag = 1;
      SetSP(val);
    }
    else flag = 1;
  }

  // physical endurance
  val = QueryPEP();
  max = QueryMaxPEP();
  rate = QueryRegPEP();
  // was: if (rate < 0 || (val <= max && !load)) {
  // but this kept the heart beat unnecessarily on.
  if (rate < 0 || (val < max && !load)) {
    accPEP += rate;
    if (accPEP/10) {
      val += accPEP/10;
      accPEP %= 10;
      if (accPEP >= 0 && val >= max) val = max, accPEP = 0;
      else if (val != max) flag = 1;
      SetPEP(val);
    }
    else flag = 1;
  }

  // mental endurance
  val = QueryMEP();
  max = QueryMaxMEP();
  rate = QueryRegMEP();
  // was: if (rate < 0 || (val <= max && !load)) {
  // but this kept the heart beat unnecessarily on.
  if (rate < 0 || (val < max && !load)) {
    accMEP += rate;
    if (accMEP/10) {
      val += accMEP/10;
      accMEP %= 10;
      if (accMEP >= 0 && val >= max) val = max, accMEP = 0;
      else if (val != max) flag = 1;
      SetMEP(val);
    }
    else flag = 1;
  }

  max = QueryMaxHP();
  // was: if (reghp < 0 || (val < max && !load)) {
  // but this kept the heart beat unnecessarily on.
  if (reghp < 0 || (hp < max && !load)) {
    accHP += reghp;
    if (accHP/10) {
      hp += accHP/10;
      accHP %= 10;
      if (accHP >= 0 && hp >= max) hp = max, accHP = 0;
      else if (hp != max) flag = 1;
      SetHP(hp);
    }
    else flag = 1;
  }

  if (hp < 0)
    Die();

  else
  {
    rate = QueryHealth();
    if (health >= rate+20 || health <= rate-10 || !(stun+1 % 4))
      tell_object(THIS, "You are "+((accHP > 0 && rate < 95) ? "still " : "")
                                  +HealthMsg(rate, stun)+".\n"
                 );
  }

  return (flag || load);
}

/*--------------------------------------------------------------------------*/

  // May be overloaded to something more useful.
public varargs void Death (object * killers) { remove(); }

public object LeaveCorpse() {
  mixed fcp;
  object cp, *obs;
  int i;

  seteuid(getuid());
  SetLState(LSTATE_NORMAL); // So P_WEIGHT will be right.
  fcp = QueryAttr(A_CORPSE);
  if(!stringp(fcp) || catch(cp = clone_object(fcp)))
    return 0;

  cp->SetMaxWeight(QueryWeight());
  cp->SetWeight(QueryWeight()-QueryWeightContent());
  cp->SetRace(QueryRace());
  cp->SetName(QueryName());
  cp->SetLevel(QueryLevel());
  cp->move(ENV());
  obs = all_inventory(THIS);
  for (i = sizeof(obs); i--; )
  {
    if (    !({object})obs[i]->QueryMoneyObject()
         &&  ({mixed})obs[i]->Query(P_AUTOOBJECT))
      continue;
    obs[i]->move(cp, M_NOCHECK);
  }
  cp->DoDecay(1);
  return cp;
}

public varargs void Die (int silent) {
  object cp, *enemies,leader;
  int xp, i,txp;

  if (!silent)
    say (capitalize(QueryName()) + " died.\n");
  enemies = QueryEnemies();
  if (member(enemies, TP) == -1)
      enemies += ({ TP });
  enemies -= ({ 0, THIS });

  StopAllHunting();

  xp = QueryKillXP(sizeof(enemies)+1);

  SetHP(-31415); // Security override for /std/player/life::AddXP() and ::SetXP()

  for (i = sizeof(enemies); i--; )
  {
    if (leader=({object})enemies[i]->Query("Party"))
    {
      txp=({int})PARTYOBJECT->PartyXP(xp,leader);
      if (txp>0) enemies[i]->AddXP(txp);  // left over xp
    }
    else enemies[i]->AddXP(xp);

    if (enemies[i] == TP) {
      if (!silent) tell_object(enemies[i], "You killed "+QueryName()+".\n");
      if (leader)
      {
        txp=({int})PARTYOBJECT->PartyXP(xp,leader);
        PARTYOBJECT->PartyKills(leader);
        if (txp>0) enemies[i]->AddXP(txp);  // left over xp
      }
      else enemies[i]->AddXP(xp);
    }
    else if (!silent)
      tell_object(enemies[i], "You helped killing "+QueryName()+".\n");
    enemies[i]->UpdateAlign(QueryAlign()/sizeof(enemies));
  }

  if (ENV() && ({int})ENV()->QueryKillZone())
    xp = (QueryXP() * 5) / 6;
  else
    xp = XP_DEATH(QueryXP());
  i = QueryXP() - (interactive(THIS) ? ({int})THIS->QueryRealLevel() : QueryLevel())
                  * 200000;
  if (THIS && query_once_interactive(THIS))
    // Save the old stats for players
    THIS->SaveStats();
  if (i > xp)
    SetXP(i);
  else
    SetXP(xp);

  SetHP(10);

  cp = LeaveCorpse();

  SetLState(LSTATE_GHOST);
  say(({ MSG_DIE, cp }));
  Death(enemies);
}

//---------------------------------------------------------------------------
// Auxiliary functions to handle P_RESISTANCE

public int Resistance(mixed type)
{
  return (Query(P_RESISTANCE) || ([]))[type];
}

public int AddResistance(mixed type, int delta)
{
  mapping res;
  res = Query(P_RESISTANCE) || ([]);
  res[type] += delta;
  Set(P_RESISTANCE, res);
  return res[type];
}

//---------------------------------------------------------------------------
public void create()
{
  seteuid(getuid());
  SetRateHP(HP_RATE);
  SetRateSP(SP_RATE);
  SetRatePEP(PEP_RATE);
  SetRateMEP(MEP_RATE);
  SetRatePoison(POISON_RATE);
  SetCorpse("/std/corpse");
}

/****************************************************************************/
