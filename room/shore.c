/*
** /room/underwater
** This room is for underwater-rooms.
** For the surface of a lake use SetUnderwater(0).
** It handles:
** - Usage of the exits, you can 'swim' into the directions or
**   dive up and down (the normal exit-commands still work)
** - Damage when player can't breath under water
** - The default underwater-room is outdoors!
** - The nd-server with special messages is set
** - Torches and other 'open fire' will get extinguished.
**
** 25jun95 [t] Creation
** 04dec96 [t] Added a bunch of comments and converted room to
**             /room/underwater
**
** [t] Thragor
*/

#include <rooms.h>
#include <properties.h>
#include <moving.h>
#include <nightday.h>

#define SF(x) #'x //'
#define SYM(x) 'x //'
#define TP this_player()
#define PO previous_object()
#define ME this_object()

#define P_UNDERWATER_TIME "UnderwaterTime"

private nosave int
  Punderwater, // is the room underwater?
  Pflats; // does tide take effect on underwater?

inherit "/std/room";

public int SetFlats(status i)
  // Function: SetFlats
  // Syntax:   status SetFlats(status i)
  // Description:
  //   If this room shall be under water 'from time to time', i. e.
  //   on high tide, and if the room shall not be under water if it
  //   is low tide, then this property must be set to true (=1).
  // Default:  0
{
  return Pflats = i;
}

public int QueryFlats()
  // Function: QueryFlats
  // Syntax:   status QueryFlats()
  // Description:
  //   If this room is under water 'from time to time', i. e.
  //   on high tide, and if the room isn't under water if it
  //   is low tide, then 1 is return else 0.
{
  return Pflats;
}

public int SetUnderwater(status i)
  // Function: SetUnderwater
  // Syntax:   status SetUnderwater(status i)
  // Description:
  //   If a room shall be under water all the time, then this property has
  //   to be set to 1. Otherwise, it must be set to 0.
  //   If QueryFlats() returns 1, then this setting is ignored and all
  //   the handling is done by the tides.
  // Default: 1
{
  return Punderwater = i;
}

public int QueryUnderwater()
  // Function: QueryUnderwater
  // Syntax:   status QueryUnderwater()
  // Description:
  //   The returned value is 1, if the room is always under water and 0
  //   if not. This doesn't check the actual tide. To see if a player is
  //   really under water, query the function QueryFlooded().
{
  return Punderwater;
}

public int QueryFlooded()
  // Function: QueryFlooded
  // Syntax:   status QueryFlooded()
  // Description:
  //   QueryFlooded() returns true (= 1) if the room is actually under
  //   water. This checks also the tides.
{
  if (QueryFlats()&&({int})QueryServer()->QueryTide()>-1)
    return 0;
  return Punderwater;
}

public int UseExit(string verb, string arg, int method)
  // Function: UseExit
  // Syntax:   int UseExit(string verb, string arg, int method)
  // Parent:   /std/room/exits:UseExit()
  // Description:
  //   This function has the same meaning as the original UseExit().
  //   In addition it handles 'swimming', i. e. the messages get
  //   modified if a player moves under water.
{
  mixed dest, obj;
  int   chk, hide;
  mapping exits,exits2;
  string movemsg;

  if (   method!=M_GO
      || !QueryFlooded())
    return (::UseExit(verb,arg,method));

  exits = QueryExitsDest();
  exits2 = QueryExitsData();

  if ( !exits || !(dest = exits[verb,EXIT_DEST]))
    return notify_fail(QueryNowayMsg(verb) || "No way.\n"),0;
  if (exits2&&sizeof(exits2))
    {
      if (obj = exits2[verb, EXIT_DOOR-EXIT_OBJ])
        hide = (({int})obj->QueryHideExit(verb)) & HIDE_OPEN;
      else
        hide = exits2[verb, EXIT_HIDDEN-EXIT_OBJ];
      chk = exits2[verb, EXIT_CHECK-EXIT_OBJ];
      if (chk & DACT_HIDE)
        chk = (chk & DACT_CHECK) ? !hide : hide;
      else
        chk = (chk & DACT_CHECK);
      if (chk && !CheckAction("exit", verb, arg, method))
        return 0;
    }
  if (closurep(dest))
    return ({int}) funcall(dest, arg);
  if (sizeof(exits2) && (obj = exits2[verb, EXIT_OBJ-EXIT_OBJ]))
    return closurep(dest) ? ({int}) funcall(dest, arg)
                          : ({int}) call_other (obj, dest, arg);
  if (arg || environment(TP||PO) != ME)
    return 0;
  if (verb=="up"||verb=="down")
    movemsg = "dives in";
  else
    movemsg = "swims in";
  switch(verb)
    {
     case "north":
      movemsg+=" from south";
      break;
     case "northeast":
      movemsg+=" from southwest";
      break;
     case "east":
      movemsg+=" from west";
      break;
     case "southeast":
      movemsg+=" from northwest";
      break;
     case "south":
      movemsg+=" from north";
      break;
     case "southwest":
      movemsg+=" from northeast";
      break;
     case "west":
      movemsg+=" from east";
      break;
     case "northwest":
      movemsg+=" from southeast";
      break;
     case "up":
      movemsg+=" from below";
      break;
     case "down":
      movemsg+=" from above";
      break;
    }
  if (verb=="up"||verb=="down")
    (TP||PO)->move(dest,M_SPECIAL,({"dives "+verb,movemsg,"dive "+verb}));
  else
    (TP||PO)->move(dest,M_SPECIAL,({"swims "+verb,movemsg,"swim "+verb}));
  return 1;
}

public int cmd_swim(string str)
  // Function: cmd_swim
  // Syntax:   int cmd_swim(string str)
  // Description:
  //   cmd_swim() is called, when a player enters the command 'swim'.
  //   As argument a player has either to give a direction or (s)he
  //   will just swim around.
  //   The direction will be directly translated into the command
  //   'north', 'south' or whatever and started as extra command. This
  //   way doors don't need to get checked as it will be automatically
  //   done.
{
  if (!TP) return 0;
  if (!QueryFlooded())
    return notify_fail("You can't "+query_verb()+" here.\n"),0;
  if (!str)
    {
      write("You swim around.\n");
      show(({string})TP->QueryName()+" swims around.\n");
      return 1;
    }
  return ({int})TP->command_me(str);
}

public string *QueryExitStrings()
{
  if (!QueryFlooded())
    return (::QueryExitStrings());
  return ({"You can swim into","direction",
           "You can swim into","directions"});
}

public varargs void create()
{
  (::create(1));
  SetIntShort("A Room Under Water");
  SetIntLong("You're underwater.\n");
  SetOutdoors(SHORENIGHTDAY);
  SetUnderwater(1);
  AddRoomCmd(({"swim","dive"}),SF(cmd_swim));
}

protected void log_homarid(object pl)
// Temporary log to check the strange disappearance of P_WATERBREATHING
{
  string inv;
  inv = implode(map(all_inventory(pl),
                          lambda(({SYM(pl)}),
                                 ({SF(+),"  ",({SF(object_name),SYM(pl)})})
                                )
                         ),
                "\n"
               )+"\n";
  log_file("WATERBREATHING",
             capitalize(getuid(pl))+
               " ("+({string})pl->QueryRace()+", "+object_name(({object})pl->QueryRaceObj())+")\n"+
             inv+"---\n"
            );
  pl->Set(P_WATERBREATHING,1);
}

public varargs void underwater_damage(object pl)
{
  int t,con;

  pl = pl||PO;
  // If a player is homarid, but has waterbreathing not set, this will be
  // logged and corrected.
  if (   query_once_interactive(pl)
      && (   ({string})pl->QueryRace()=="homarid"
          || ({string})pl->QueryRace()=="homarid-vampire"
         )
      && !({int})pl->Query(P_WATERBREATHING)
     )
    log_homarid(pl);
  // If a player (or a NPC) is under water, but can't breathe under water
  // (s)he will be checked if (s)he is too long under water.
  if (   ({int})pl->Query(P_WATERBREATHING)
      || !QueryFlooded()
      || ({int})pl->QueryGhost()
      || ({int})pl->QueryNetdead())
    return;
  // Did the player just dive? Then set the property to the time when
  // the player dived.
  if (!t = ({int})pl->Query(P_UNDERWATER_TIME))
    pl->Set(P_UNDERWATER_TIME,time());
  // Now let's see if the player is too long under water.
  t = time()-({int})pl->Query(P_UNDERWATER_TIME);
  // Depending on the constitution a player might stay longer
  // under water.
  con = ({int})pl->QueryCon();
  if (!t%(5*2)||pl==PO)
    {
      if (t<2*con)
        return;
      if (t<3*con)
        tell_object(pl,"Your lung doesn't have any oxygene left.\n");
      else if (t<5*con)
        tell_object(pl,"You feel as if a fire burns inside you.\n");
      else if (t<7*con)
        tell_object(pl,"Your lung seems to explode.\n");
      else if (t<10*con)
        tell_object(pl,"You're about to die because lack of air.\n");
      else
        tell_object(pl,"You feel that death is near.\n");
      if (t<10*con)
	pl->ReduceHP(t/con);
      else
        pl->DoDamage(t/con);
    }
}

protected void check_light(object pl)
  // Function: check_light
  // Syntax:   void check_light(object pl)
  // Description:
  //   If a player enters a flooded room (from a room which was
  //   not flooded) all items will be checked e. g. for torches.
  //   These torches will be worthless things afterwards.
{
  object *inv,ob;
  int i;
  i = sizeof(inv = all_inventory(pl));
  while(i--)
    if (member(inherit_list(inv[i]),"std/lamp.c")!=-1)
      {
	ob = clone_object("/std/thing");
	ob->SetProperties(({mapping})inv[i]->QueryProperties());
	ob->SetLight(0);
        tell_object(pl,"Your "+strip_article(({string})ob->QueryShort())+" becomes "
                       "wet and is worthless now.\n");
	ob->SetShort("a wet and worthless "+strip_article(({string})ob->QueryShort()));
	ob->SetValue(0);
        ob->move(pl,M_SILENT);
	inv[i]->remove();
	if (inv[i]) destruct(inv[i]);
      }
}

public void Enter(object pl,mixed oldenv,int method,mixed extra)
  // Function: Enter
  // Syntax:   void Enter(object pl,mixed oldenv,int method,mixed extra)
  // Description:
  //   This function will be called from notify_enter, if a living enters the
  //   room. It first checks for torches, then if the player receives damage
  //   because (s)he is under water for too long and finally a call_out is
  //   defined which will regulary damage a player just sitting in a room
  //   which is under water.
{
  if (!pl) return;
  if (!oldenv||!({int})oldenv->QueryFlooded())
    check_light(pl);
  underwater_damage(pl);
  if (!pl) return;
  if (!({object})pl->QueryHbObj())
    {
      pl->SetHbInterval(5);
      pl->SetHbFun(SF(underwater_damage));
    }
}

public void Exit(object pl,mixed dest,int method,mixed extra)
  // Function: Exit
  // Syntax:   void Exit(object pl,mixed dest,int method,mixed extra)
  // Description:
  //   If a player leaves an underwater-room and reaches fresh air
  //   again, the time, the player has been under water is set to
  //   0 again.
{
  if (!({int})dest->QueryFlooded())
    pl->Set(P_UNDERWATER_TIME,0);
}

public varargs void notify_enter(mixed oldenv,int method,mixed extra)
{
  (::notify_enter(oldenv,method,extra));
  if (living(PO))
    Enter(PO,oldenv,method,extra);
}

public varargs void notify_leave(mixed dest,int method,mixed extra)
{
  (::notify_leave(dest,method,extra));
  if (living(PO))
    Exit(PO,dest,method,extra);
}
