/*:ts=2
 *-----------------------------------------------------------------
 * std/player/life.c
 *
 * player object life variables
 *
 * Defines some things which are different than in living.c
 *
 * This file is (c) Nightfall 1992 and (c) One Step Beyond 1997.
 * You may use of it whatever you like, but you should put a reference
 * in your file if you do it.
 *
 * Major authors: Deepthought, Marvin, Ergberg
 *
 * Version 4.0 first version of new living object
 * Version 4.2 Pepel fixed many things
 * Softbyte: Removed SetInt which was in comments anyways
 *
 *-----------------------------------------------------------------
 */

#include <config.h>
#include <properties.h>
#include <health.h>
#include <secure/wizlevels.h>
#include <driver/object_info.h>
#include <weather.h>
#include <moving.h>
#include <daemons.h>
#include <stdrooms.h>
#include <combat.h>
#include <msgclass.h>

#define ME this_object()
#define SF(x) #'x //' Emacs-Hack

inherit "/std/npc/body";

public int QueryInvis (); // std/living/description
public varargs mixed Query(string name);  // std/base
public string QueryPrompt(); // std/player
public void ValidizeHeart(); // std/living/heart
public object *QueryFollowers(); // std/living/moving
public string QueryRealName(); // std/living/description
public varargs string Short (string what); // std/living/description
public varargs string Long (string what); // std/living/description
public int QueryLight(); // std/thing/restrictions
public varargs int move(mixed dest, int method, mixed extra); // std/living/moving
public void StopAllHunting(); // std/living/combat
public void save_me(status value_items, status destruct_autos); // std/player
public varargs mixed print_prompt(); // std/player/message
static void replace_by_statue();

nosave int age_to_save;
nosave int netdead;       // time() of netdeath
nosave string ndroom;
nosave object ndstatue;
nosave int reallevel;
nosave int time_last_age;
nosave int add_xp_delay_time;
nosave int delayed_xp;

int age;

// -----------------------------------------------------------------------------
// Help-Functions (used by other functions inside here)
// -----------------------------------------------------------------------------

static varargs void update_age(int enforce)
  // Called by: QueryAge, CheckNetdead, autosave
{
  if (interactive(ME) || enforce)
    age += (time() - time_last_age)/2;
  time_last_age = time();
}

static void replace_by_statue()
  // Called by: CheckNetdead
{
  int rc;
  if (interactive(ME) || !netdead || ndroom)
    return;
  if (({int})ME->QueryGuest())
    {
      catch(filter_objects(deep_inventory(ME), "remove"));
      ME->remove();
      return;
    }
  if (!environment(ME) || ({int})environment(ME)->QueryIsVirtual())
    return;
  ndroom = to_string(environment(ME));
  disable_commands();
  if ((rc = move(NETDEATH,M_SILENT)) != ME_OK)
    {
      enable_commands();
      log_file("NETDEATH",
	       sprintf("%s: %O: Can't move from %O into nd-room, code %d\n"
		       , ctime(), ME, ndroom, rc));
      ndroom = 0;
      return;
    }
  if (catch(ndstatue = clone_object("/std/thing")) || !ndstatue)
    return;
  ndstatue->SetShort(Short());
  ndstatue->SetLong(Long());
  ndstatue->SetWeight(QueryWeight());
  ndstatue->SetLight(QueryLight());
  ndstatue->AddId("ndstatue-"+QueryRealName());
  ndstatue->AddId(QueryRealName());
  ndstatue->AddId("statue");
  ndstatue->AddAdjective("statue of");
  ndstatue->Set(P_NOGET, "That is too heavy for you.\n");
  ndstatue->Set(P_NOGIVE, "It won't budge.\n");
  if ((rc = ({int})ndstatue->move(ndroom,M_SILENT)) != ME_OK)
    {
      log_file("NETDEATH", sprintf("%s: %O: Can't move into %O, code %d\n"
				   , ctime(), ME, ndroom, rc));
      ndstatue->remove();
    }
}

private void CheckNetdead()
// Called by: QueryNetdead()
{
  if (!interactive(ME) && !netdead)
  {
    CHANNEL_D->UnregisterUser( this_object() );
    if (environment(ME))
      environment(ME)->PlayerNetdead(1);
    catch(map_objects(deep_inventory(ME), "PlayerNetdead", 1));
    catch(map_objects(QueryFollowers()||({}), "PlayerNetdead", 1));
    PARTYOBJECT->PlayerNetdead(1, this_object() );
    if (!IS_IMMORTAL(ME) || !QueryInvis())
      show(capitalize(QueryName())+" turns into a statue.\n");
    update_age(1);
    save_me(1, 0);
    netdead = time();
    ndroom = 0;
    ndstatue = 0;
    ValidizeHeart();
    while (remove_call_out(SF(replace_by_statue)) >= 0);
    while (remove_call_out("remove") >= 0);
    call_out(SF(replace_by_statue), 300);
    if (IS_IMMORTAL(ME))
      call_out("remove", 30*60, 2001);
  }
  else if (interactive(ME) && netdead)
  {
    int rc;
    netdead = 0;  // Prevents recursions
    while (remove_call_out("remove") >= 0);
    while (remove_call_out(SF(replace_by_statue)) >= 0);
    if (ndstatue)
      ndstatue->remove();
    if (!living(ME))
      enable_commands();
    if (ndroom)
    {
      if (catch(rc = move(ndroom,M_SILENT)) || rc != ME_OK)
        move(VOID,M_SILENT);
    }
    /* If this was an immortal, it might have been destructed since
     * the netdeath, meaning that  ndstatue lost its value.
     * We therefore explicitely look for any statue of us and destruct it.
     * TODO: If the wizard got destructed outside his home, that is where
     * TODO:: the statue will stand and this check won't find it. The solution
     * TODO:: would be to store the object_name() of either ndstatue or ndroom
     * TODO:: in the savefile and work from there.
     */
     while (0 != (ndstatue = present("ndstatue-"+QueryRealName(), environment(ME))))
      ndstatue->remove();
    ndstatue = 0;
    ndroom = 0;
    CHANNEL_D->RegisterUser( this_object() );
    if (environment(ME))
      environment(ME)->PlayerNetdead(0);
    catch(map_objects(deep_inventory(ME), "PlayerNetdead", 0));
    catch(map_objects(QueryFollowers()||({}), "PlayerNetdead", 0));
    PARTYOBJECT->PlayerNetdead(0, this_object() );
    if (!IS_IMMORTAL(ME) || !QueryInvis())
      show (capitalize(QueryName())+" becomes alive again.\n");
    time_last_age = time();
    ValidizeHeart();
  }
}

private status ValidUpdateXP()
  // Called by: SetXP and AddXP
{
  return
        IS_WIZARD(this_player())
     || previous_object() == ME
     || (   (ME == this_player() || ({int})previous_object()->QueryHP() == -31415)
         && (   !this_interactive()
             || this_interactive() == this_player()
             || getuid(this_interactive()) == ROOTID
            )
        );
}

static varargs void autosave(int initial)
  // Called by: create and in call_out-Loop
{
  update_age();
  if (age > age_to_save)
    {
      save_me(1,0);
      // if (!IS_WIZARD(this_object())) write("Autosave...\n");
      age_to_save = age + (initial ? 50 : 500);
    }
  if (interactive(ME))
    call_out(SF(autosave), 2*(age_to_save-age)+2);
}

// -----------------------------------------------------------------------------
// Add-, Query- and SetFunctions
// -----------------------------------------------------------------------------

public int QueryAge()
// This is used to set certain chars ages
{
  update_age();
  return (getuid() == "mopper") ? 99 : age;
}
public int SetAge(int i)
  // The age mustn't be set.
{
  return QueryAge();
}

public int QueryNetdead()
  // Returns the time since when a player is netdead (after doing checks
  // if the player is netdead, etc.
{
  if (!query_once_interactive(ME)) return 0;
  CheckNetdead();
  return netdead;
}

public int SetNetdead(int v) { return QueryNetdead(); }

public string QueryNetdeadRoom () { return QueryNetdead() && ndroom; }

public int SetXP(int xp)
{
  if (!intp(xp) || xp < 0) return QueryXP();
  if (ValidUpdateXP() ||  QueryHP() == -31415)
  {
    reallevel = 0;
    return ::SetXP(xp);
  }
  return QueryXP();
}

public int QueryXP()
{
  if (add_xp_delay_time<time())
    return (::QueryXP());
  return delayed_xp;
}

public varargs int AddXP(int amount,int delayed)
  // <delayed> may contain the time (in heartbeats) the XP
  // should wait before appearing in the score.
{
  if (ValidUpdateXP())
  {
    reallevel = 0;
    if (delayed)
    {
      delayed_xp = ::QueryXP();
      add_xp_delay_time = time()+delayed;
    }
    return ::AddXP(amount);
  }
  return 0;
}

public int QueryRealLevel ()
{
  int qp;
  if (reallevel < 1)
  {
    reallevel = ({int})RACEMASTER->XPtoLevel(QueryXP());
    qp = ({int})QUESTMASTER->QueryQuestPoints(ME);
    while (reallevel > 1 && ({int})QUESTMASTER->QPointLevel(reallevel) > qp)
      reallevel--;
    if (reallevel < 1)
      reallevel = 1;
  }
  return reallevel;
}

public int SetRealLevel (int i) { return QueryRealLevel(); }

// Hook for weather messages
public void SetStateData(mixed data)
{
  if (data)
    msg_object(ME, CMSG_ROOM, (data[WD_CDESC] || data[WD_WDESC]) );
}

// -----------------------------------------------------------------------------
// Misc. Functions
// -----------------------------------------------------------------------------

public varargs string AgeString(int age_time,int format)
  // <age_time>: by Default QueryAge() is used.
  // <format>:
  //  - 0 : D days H hours M minutes S seconds
  //  - 1 : Dd H:M:S
  //  - 2 : D:H  (xwho-format)
{
  int i,d,h,m,s;
  string res;

  i = age_time||QueryAge();

  if (d = i/43200) i -= d*43200;
  if (h = i/1800) i -= h*1800;
  if (m = i/30) i -= m*30;
  s = i*2;

  switch(format)
  {
    case 1:
      return sprintf("%dd %02d:%02d:%02d",d,h,m,s);
    case 2:
      return sprintf("%d:%02d",d,h);
    default:
      res = "";
      if (d) res += d+" days ";
      if (h) res += h+" hrs ";
      if (m) res += m+" min ";
      return res;
  }
  return res;
}

public varargs void Die(int silent)
{
  if (!ME) return;
  if (query_once_interactive(ME) && !interactive())
  {
    /* this player is linkdead. */
    write(QueryName()
      +" is not here. You cannot kill a player who is not logged in.\n");
    SetHP(20);
    StopAllHunting();
    return;
  }
  /* Don't damage wizards too much ! */
  if (   IS_IMMORTAL(QueryRealName())
      && !Query(P_NOWIZ)
     )
  {
    tell_object(ME, "Your wizardhood protects you from death.\n");
    return;
  }
  /* Do not kill ghosts! */
  if (({int})ME->QueryGhost())
  {
    tell_object(ME,"Your dead state protects you from being"+
      " killed again!\n");
    return;
  }
  (::Die(silent));
}

public void create()
{
  ::create();
  time_last_age = time();
  age_to_save = age+50;
  autosave();
}
