/* Plasma Bolt  (Mateese, 19-Feb-92)
*/

#include <config.h>
#include <properties.h>
#include <rooms.h>
#include <combat.h>
#include <moving.h>

#define ME this_object()
#define PL this_player()

inherit "/std/thing";

/*-------------------------------------------------------------------------*/

#define STEPS_PER_CALLOUT 30
#define MAX_DISTANCE 10
#define MIN_ENERGY 5
#define ERG_PER_STEP 5
#define BASE_BRIGHT 100
#define BRIGHT_PER_ERG 1

static object pl;  /* The true this_player() */
static object target;
static object *aim_data;
static string dir;
static int distance, energy;

/*-------------------------------------------------------------------------
** Initialize us.
*/

create () {
  ::create();
  SetShort (0);
  SetLong (0);
  SetValue (0);
  SetWeight (0);
  SetInfo ("Experimental Plasma Bolt, by Mateese");
  Set(P_MSGIN, "enters");
  Set(P_MSGOUT, "leaves");
  Set(P_MMSGIN, "enters");
  Set(P_MMSGOUT, "leaves");
}

true_bolt () {
  SetShort ("a plasma bolt");
  SetLong ("A bolt of pure hot plasma.\n");
  SetIds (({ "bolt", "plasmabolt" }));
  SetBright (BASE_BRIGHT+energy * BRIGHT_PER_ERG);
  AddAdjective ("plasma");
}

int move (mixed a, int b, mixed c) {
  if (b != M_NOCHECK) b = M_SILENT;
  return ::move(a, b, c);
}

/*-------------------------------------------------------------------------
** Move one room into the desired direction. Return success.
*/

static int _move_on (string dir, int ack_doors) {
  mixed *exits, *data;
  mixed nextroom;
  int rc, w;

  if (aim_data) {
    nextroom = aim_data[0];
    aim_data = aim_data[1..];
    if (!sizeof(aim_data)) aim_data = 0;
  }
  else {
    exits = (mixed *)environment()->QueryExits();
    if (!(nextroom = assoc (dir, exits))) {
      exits = (mixed *)environment()->QueryDoors();
      if (    (data = assoc (dir, exits)) 
           && (!ack_doors || !data[DOOR_STATUS]))
        nextroom = data[DOOR_DEST];
    }
  }
  rc = 1;
  if (!nextroom || catch(w = (int) move(nextroom, M_SILENT, 0))
      || w != ME_OK) {
    enable_commands();
    rc = !catch(w = command(dir)) && w;
  }
  return rc;
}

/*-------------------------------------------------------------------------
** Start an aim at target into given direction.
** Return a list of the found object (or 0 if not found) and the
** rooms trespassed.
*/

object *aim (mixed _target, string _dir, int _energy) {
  object *rooms, obj;
  int stp, dist, trans;
  
  pl = PL;
  aim_data = 0;
  enable_commands();
  rooms = ({ 0 }); trans = 1000;
  do {
    rooms += ({ environment() });
    if (_energy < MIN_ENERGY || trans < 500) break;
    if (    (int) pl->CanSee (environment()) 
         && (obj = present (_target, environment()))) {
      rooms[0] = obj;
      break;
    }
    trans = (trans * (int) environment()->QueryTransparent()) / 1000;
    if (!_move_on (_dir, 0)) break;
    _energy -= ERG_PER_STEP;
  } while (stp++ < STEPS_PER_CALLOUT && dist++ < MAX_DISTANCE);
  return rooms;
}

/*-------------------------------------------------------------------------
** Hit a room.
*/

void hit_room (object room) {
  int damage, hp;
  string long;

  SetBright (0);
  show_room (environment(),
             ({ "An energy bolt "+(distance ? "howls in and ":"")+
            "impacts into a wall.\n",
            "An energy bolt "+(distance ? "howls in and ":"")+
            "dies in a heavy impact.\n" }));

  hp = (int) room->Query("_PlasmaHits");
  room->Set("_PlasmaHits", hp+1);
  if (!hp) {
    long = (string)room->Query(P_LONG);
    room->Set("_PlasmaLong", long);
    long += "There is a dark stain on the wall.\n";
    room->Set(P_LONG, long);
    room->AddDetail ("stain", 
"You see a dark crater in the wall. Something powerful had its impact here.\n"
                    );
  }
  else if (hp == 1) {
    long = (string)room->Query("_PlasmaLong");
    long += "The black stained walls have suffered serious damage.\n";
    room->Set(P_LONG, long);
    room->AddDetail ("stains",
"There are numerous dark craters in the walls. You wonder what has\n"+
"happened here.\n"
                    );
    if (!(long = (string)room->QueryDetail("walls"))) long = "";
    room->AddDetail ("walls", long + 
"The walls are carved and battered from numerous impacts of pure energy.\n"
                    );
  }
  remove();
}

/*-------------------------------------------------------------------------
** Let an object explode shortly after hit.
*/

void _explode () {
  show_room (environment(),
             ({ capitalize((string)target->QueryShort())+
                " explodes into small rubbles.\n",
                "Something explodes besides you.\n"}) );
  target->remove();
  remove();
}

/*-------------------------------------------------------------------------
** Hit an object.
*/

int hit_object (object obj) {
  int damage, hp;
  string long;

  SetBright (0);
  if (living(obj) && function_exists("Defend", obj)) {
    damage = energy;
    hp = (int) obj->QueryHP();
    if ((int) obj->QueryPlayer()) {
      if (!interactive(obj)) return 0;
      if (damage > hp / 2) damage = hp / 2;
    }
    tell_object (obj, "A bright bolt of energy "+
                      (distance ? "howls in and ": "")+
                      "hits you.\n");
    show_room (environment(),
               ({ "A plasma bolt "+(distance ? "howls in and ":"")+
              "hits "+(string)obj->QueryName()+".\n",
              "A plasma bolt "+(distance ? "howls in and ":"")+
              "dies in a heavy impact.\n" }));
    obj->Defend (damage, DT_FIRE); remove();
    return 1;
  }

  hp = (int) obj->QueryWeight() - (int) obj->QueryWeightContents();
  damage = energy * 1000;
  if (hp <= damage) {
    show_room (environment(),
               ({ "A plasma bolt "+(distance ? "howls in and ":"")+
              "strikes "+(string)obj->QueryShort()+
              ",\nwhich explodes into nothingness.\n",
              "An energy bolt "+(distance ? "howls in and ":"")+
              "dies in a heavy impact.\n" }));
    obj->remove(); remove(); 
    return 1;
  }  
  
  show_room (environment(),
             ({ "A plasma bolt "+(distance ? "howls in and ":"")+
            "impacts in "+(string)obj->QueryShort()+
            ",\ncausing intrastructural damage.\n",
            "A plasma bolt "+(distance ? "howls in and ":"")+
            "dies in a heavy impact.\n" }));
  obj->SetWeight((int) obj->QueryWeight() - damage);
  
  hp = (int) obj->Query("_PlasmaHits");
  obj->Set("_PlasmaHits", hp+1);
  if (!hp) {
    long = (string)obj->Query(P_LONG);
    obj->Set("_PlasmaLong", long);
    long += "There is a dark stain where something powerful has hit it.\n";
    obj->Set(P_LONG, long);
  }
  else if (hp == 1) {
    long = (string)obj->Query("_PlasmaLong");
    long += "Several plasma hits have caused serious damage to it.\n";
    obj->Set(P_LONG, long);
  }

  if (hp >= damage/2) {
    target = obj;
    call_out ("_explode", 0);
    SetShort(0);
  }
  else remove();
  return 1;
}

/*-------------------------------------------------------------------------
** Fly through the rooms.
*/

void _fire () {
  int step;
  object lastroom;

  do {

      /* Are we too low on energy ? */
    if (energy <= MIN_ENERGY || distance > MAX_DISTANCE) {
      say ("A bright bolt "+(distance?"whines in and ":"")+
           "sizzles away.\n");
      remove(); return;
    }
    lastroom = environment();

      /* Is our target here? 
      ** If yes, try to hit it. 
      ** The bolt has a minimal accuracy of 10%, and a maximal of 99%.
      */
    if (target && present (target, environment())) {
      int accuracy;
      accuracy = 990 - ((900 * distance) / MAX_DISTANCE);
      if (random(1000) <= accuracy && hit_object (target)) return;
    }

    if (!dir || !_move_on (dir, 0)) {
      hit_room (lastroom);
      return;
    }
    if (distance)
      tell_room (lastroom, 
                 "A bright bolt howls through heading "+dir+".\n");
    energy -= ERG_PER_STEP;
    SetBright (BASE_BRIGHT+energy * BRIGHT_PER_ERG);
    distance++;
  } while (step++ < STEPS_PER_CALLOUT);
  step = 0;
  call_out ("_fire", 0);
}

/*-------------------------------------------------------------------------
** Start a bolt-flight.
*/

void fire (string _dir, object *_aim_data, int _energy, object _pl) {
  object *rooms, obj;
  int stp, dist, trans;
  
  pl = _pl;
  if (_aim_data) {
    target = _aim_data[0];
    aim_data = _aim_data[2..];
    if (!sizeof(aim_data)) aim_data = 0;
  }
  dir = _dir;
  energy = _energy;
  distance = 0;
  true_bolt();
  _fire();
}

/*************************************************************************/



