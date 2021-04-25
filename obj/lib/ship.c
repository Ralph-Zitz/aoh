/*
** Ship-Functions
** To be able to use the ship even as NPC, this lib offers all necessary
** functions to communicate with /room/port.
** File:   /obj/lib/ship.c
** Author: Thragor@Nightfall, 17sep95
** Update:
*/

#include <config.h>
#include <moving.h>
#include <search.h>
#include <stdrooms.h>
#include <properties.h>

#define OCEAN "/room/ocean"

#define TP this_player()
#define ME this_object()
#define SF(x) #'x //'

#define PORT_FILE 0
#define PORT_NAME 1
#define PORT_ENTRIES 2
#define SCENE_TEXT 0
#define SCENE_DELAY 1
#define SCENE_ENTRIES 2

#define WAIT_TIME (60/4)/2 // Wait 1/4 minute(s) (/2 because heart_beat)
#define DBG(x)  if(find_player("nostradamus")) tell_object(find_player("nostradamus"), x)


inherit "/lib/number";

// public int calc_next_port();
public object *QueryCustomers();
public object *QueryPassengers();

private nosave object Pship;
// Holds the clone of the ship (stored in the blueprint)
private nosave string *Pdecks,
// first room is the room where someone will arrive entering the ship
// you can leave the room from any deckroom and the messages
// will be displayed to every deck-room.
                      *Prooms;
// The ship has to know all rooms belonging to the ship to be able
// to know when the ship may get cleaned up:
// - no heart_beat, no players on board
// and to get to know when a ship might switch off the heart_beat
// waiting at a harbour
private nosave mixed Penter_msgs,Pleave_msgs,
// messages of a player leaving/entering a ship
                     Pship_enter_msgs,Pship_leave_msgs,
// the message of a ship leaving/entering the harbour
                     Pmove_msgs;
// Random messages while the ship is moving
// Pmove_msgs: ({({<msg1>,<msg2>}),({<probability1>,<probability2>})})
private nosave mapping Pmove_scenes,
// Scenes which will be displayed in the given order. If all scenes
// got displayed the journey comes to an end.
// move_scenes:
//   (["fromport":(["toport":([<nr>:<scene>;<delay>])])])
                       Pports;
// The ports a ship will sail to (in the given order)
// Pports:
//   ([<nr>:<file>;<name>])
private nosave int Pto_port,
// nr. holding the index of the port we're sailing to or just arrived at
                   Pfrom_port,
// nr. holding the index of the port we just came from
                   Pcurrent_port,
// Current port
                   Phbcount;
// Variable to count the heart_beats until a ship leaves a port.

public object RegisterMe()
{
  if (previous_object()!=ME
      &&blueprint(previous_object())==object_name())
    Pship = previous_object();
  return Pship;
}

public object QueryShip()
{
  return Pship;
}

protected object valid_port(string port)
{
  object res;
  string error;
  seteuid(getuid());
  if (file_size(port+".c")<0)
  {
    MASTER->runtime_error("Port not existant!\n",
                            object_name(),port,-1);
    return 0;
  }
  if (error = catch(res = load_object(port)))
  {
    MASTER->runtime_error("Port error: "+error,
                            port,object_name(),-1);
    return 0;
  }
  if (member(({mixed *})res->QueryShips()||({}),blueprint())==-1)
  {
    MASTER->runtime_error("Port doesn't support ship.\n",
                            port,object_name(),-1);
    return 0;
  }
  seteuid(0);
  return res;
}

public mapping SetPorts(mapping p) { return Pports = p; }
public mapping QueryPorts() { return Pports||([]); }

protected int max_port() {
  return max(m_indices(QueryPorts()));
}

protected int next_port() {
  object *cust;
  string *ports_to_support;

  if (!sizeof(m_values(QueryPorts())))
    return -1;
  if (clonep() && environment())
  {
    if (!sizeof(QueryPassengers())
      && sizeof(cust = QueryCustomers()))
    {
      // We have no passengers, i. e. we can directly sail to our customers.
      ports_to_support = map(map(cust, SF(environment)), SF(object_name));
      foreach (int key, string val : QueryPorts()) {
        if (member(ports_to_support, val) != -1)
          return key;
      }
    }
  }
  return (Pcurrent_port + 1) % sizeof(m_indices(QueryPorts()));
}

protected int current_port() {
  foreach (int key, string val : QueryPorts()) {
    if (object_name(environment()) == val)
      return key;
  }
  return -1;
}

public mapping AddPort(string portname,string portfile)
// The ports are inserted in the given order in the create.
{
  int i;
  portfile = resolve_file(portfile);
  if (!valid_port(portfile))
    return Pports||([]);
  if (!Pports)
    Pports = m_allocate(0,PORT_ENTRIES);
  i = sizeof(Pports);
  Pports[i,PORT_FILE] = portfile;
  Pports[i,PORT_NAME] = portname;
  return Pports;
}

public string QueryFromPort()
// Return the filename of the port the ship comes from
{
  return QueryPorts()[Pfrom_port,PORT_FILE];
}

public string QueryToPort()
// Return the filename of the port the ship is sailing to or just arrived at.
{
  return QueryPorts()[Pto_port,PORT_FILE];
}
public string Harbour()
// Return the name of the port the ship is sailing to or just arrived at.
{
  return QueryPorts()[Pto_port,PORT_NAME];
}
public string FromHarbour()
// Return the name of the port the ship comes from.
{
  return QueryPorts()[Pfrom_port,PORT_NAME];
}

public mixed SetShipEnterMsgs(mixed msg)
// ({<msg_ship>,<msg_harbour>})
// or closure: If returning string see above,
//             else no extra message
// @@Harbour@@ in the string will be replaced by the harbour-name
// name the ship docks on.
{
  if (msg)
    if (stringp(msg))
      msg = ({msg,capitalize(({string})ME->QueryShort())+" docks at the quay.\n"});
    else if (   pointerp(msg)
             && sizeof(msg)<2)
      msg+=({"The captain announces: We arrive at @@Harbour@@!\n",
             capitalize(({string})ME->QueryShort())+" docks at the quay.\n"})
           [sizeof(msg)..];
  return Pship_enter_msgs = msg;
}

public mixed QueryShipEnterMsgs()
{
  return Pship_enter_msgs||
    ({"The captain announces: We arrive at @@Harbour@@!\n",
      capitalize(({string})ME->QueryShort())+" docks at the quay.\n"});
}

public mixed SetShipLeaveMsgs(mixed msg)
// ({<msg_ship>,<msg_harbour>})
// or closure: If returning array see above,
//             else no extra message
{
  if (msg)
    if (stringp(msg))
      msg = ({msg,capitalize(({string})ME->QueryShort())+" leaves the quay.\n"});
    else if (   pointerp(msg)
             && sizeof(msg)<2)
      msg+=({"The captain announces: We leave @@FromHarbour@@ and head\n"
             "  towards @@Harbour@@!\n",
             capitalize(({string})ME->QueryShort())+" leaves the quay.\n"})
           [sizeof(msg)..];
  return Pship_leave_msgs = msg;
}

public mixed QueryShipLeaveMsgs()
{
  return Pship_leave_msgs||
    ({"The captain announces: We leave @@FromHarbour@@ and head\n"
      "  towards @@Harbour@@!\n",
      capitalize(({string})ME->QueryShort())+" leaves the quay.\n"});

}

public string *SetRooms(string *f) { return Prooms = f; }
public string *QueryRooms() { return (Prooms||({}))+({}); }
public string *AddRoom(string file)
// For clean_up and heart_beat-handling a ship needs to know
// which rooms belong to it.
{
  file = resolve_file(file);
  return Prooms = (Prooms||({}))-({file})+({file});
}

public object *QueryPassengers()
// Returns all passengers on the ship.
// Needed for certain calculations.
{
  string *rooms;
  int i;
  object *res,room;
  i = sizeof(rooms = QueryRooms());
  res = ({});
  while(i--)
    if (room = find_object(rooms[i]))
      res+=filter(deep_inventory(room),SF(query_once_interactive));
  return res;
}

public object *QueryCustomers()
// Return all possible passengers waiting at the ports.
// Needed for certain calculations.
{
  string *ports;
  int i;
  object *res,port;
  i = sizeof(ports = m_values(QueryPorts()));
  res = ({});
  while(i--)
    if (port = find_object(ports[i]))
      res+=filter(deep_inventory(port),SF(query_once_interactive));
  return res;
}

#if 0
public int calc_next_port()
{
  return 0;
  DBG(sprintf("calc_next_port(%O)\n", next_port()));
  return 0;
  object *cust;
  string *ports_to_support;
  mapping ports;
  int res,i,max_port;
  ports = QueryPorts();
  max_port = max(m_indices(ports));
  if (   !sizeof(QueryPassengers())
      && sizeof(cust = QueryCustomers())
     )
  {
      // We have no passengers, i. e. we can directly drive to our customers.
    ports_to_support = map(map(cust,SF(environment)),
                               SF(object_name));
    DBG(sprintf("ports_to_support: %O\n", ports_to_support));
    for(i=0;i<max_port+1;i++) {
      if (member(ports_to_support,ports[i])!=-1&&i!=Pto_port) {
        DBG(sprintf("short cut: %O (Pto_port: %O, Pfrom_port: %O)\n", i, Pto_port, Pfrom_port));
        return i;
      }
    }
    DBG(sprintf("Pto_port: %O (Pfrom_port: %O)\n", Pto_port, Pfrom_port));
    return Pto_port;
  }
  res = Pto_port+1;
  if (res>max_port) {
    res = 0;
  }
  DBG(sprintf("res: %O (Pto_port: %O, Pfrom_port: %O)\n", res, Pto_port, Pfrom_port));
  return res;
}
#endif

protected int can_sleep()
{
  if (   !environment()
      || object_name(environment())==OCEAN
      || sizeof(QueryPassengers())
      || sizeof(QueryCustomers())
     ) {
    // DBG(sprintf("can_sleep(%O) = 0\n", this_object()));
    return 0;
  }
  // DBG(sprintf("can_sleep(%O) = 1\n", this_object()));
  return 1;
}

protected int can_clean_up()
{
  if (!clonep()) {
//    DBG(sprintf("can_clean_up(1 - %O) = %O\n", this_object(), Pship != 0));
    return Pship!=0;
  }
  // The blueprint may clean_up if there's no clone of the
  // ship left.
//  DBG(sprintf("can_clean_up(2 - %O) = %O\n", this_object(), !can_sleep()));
  return !can_sleep();
}

public string *SetDecks(string *f) { return Pdecks = f; }
public string *QueryDecks() { return (Pdecks||({}))+({}); }
public string *AddDeck(string file)
// Add the room-files which shall get messages about movement
// arrival, etc. and where a player might leave the ship.
// the first defined deck is the one where a player will start
// entering the ship.
// If a room shall get this message, but a player shouldn't
// be able to leave the ship there, add an action 'leave'
// in the init of the room (after ::init()).
// A deck is automatically stored as room belonging to the
// ship.
{
  file = resolve_file(file);
  AddRoom(file);
  return Pdecks = (Pdecks||({}))-({file})+({file});
}

public varargs void DeckDisplay(string msg,int shore)
// Display a message on all decks
// <shore> is used for AnnounceEnter and Leave.
// If <shore> is 1, all decks get the property P_HARBOUR
// set to 0 else if <shore> is -1 the property P_HARBOUR
// gets set to the name the ship is currently at.
{
  int i;
  string *decks;
  decks = QueryDecks();
  i = sizeof(decks);
  seteuid(getuid());
  while(i--) {
    if (shore)
      decks[i]->Set(P_HARBOUR,shore<0?Harbour():0);
      tell_room(decks[i],msg);
  }
  seteuid(0);
}

public void AnnounceEnter()
// Tell the harbour that the ship arrived and tell it
// to all decks of the ship
{
  mixed msg;
  msg = QueryShipEnterMsgs();
  if (closurep(msg))
    if (!pointerp(msg = funcall(msg,QueryToPort())))
      return DeckDisplay("",-1);
  DeckDisplay(process_string(msg[0]),-1);
  DeckDisplay("You can leave "+(({string})ME->Query(P_NAME)||
                                strip_article(({string})ME->QueryShort()))+" now.\n");
  tell_room(environment(),process_string(msg[1]));
}

public void AnnounceLeave()
// Tell the harbour that the ship leaves and tell it
// to all decks of the ship
{
  mixed msg;
  msg = QueryShipLeaveMsgs();
  if (closurep(msg))
    if (!pointerp(msg = funcall(msg,QueryToPort())))
      return DeckDisplay("",1);
  DeckDisplay(process_string(msg[0]),1);
  tell_room(environment(),process_string(msg[1]));
}

public mapping SetMoveScenes(mapping sc) { return Pmove_scenes = sc; }
public mapping QueryMoveScenes() { return Pmove_scenes||([]); }
public mapping AddMoveScene(string from,string to,mixed scene,int delay)
// from: Where the ship just left (filename)
// to: Where the ship is sailing to (filename)
// scene: string: String will be displayed to all decks
//        closure: Will be called, if string printed else ignored
// delay: Time before calling the next scene
// The ship will arrive at the next harbour when all move-scenes have
// been displayed.
{
  int i;
  if (!Pmove_scenes)
    Pmove_scenes = ([]);
  if (!member(Pmove_scenes,from))
    Pmove_scenes[from] = ([]);
  if (!member(Pmove_scenes[from],to))
    Pmove_scenes[from][to] = m_allocate(0,SCENE_ENTRIES);
  i = sizeof(Pmove_scenes[from][to]);
  Pmove_scenes[from][to][i,SCENE_TEXT] = scene;
  Pmove_scenes[from][to][i,SCENE_DELAY] = delay;
  return Pmove_scenes;
}

public mixed SetMoveMsgs(mixed mm) { return Pmove_msgs = mm; }
public mixed QueryMoveMsgs() { return Pmove_msgs||({({}),({})}); }
public mixed AddMoveMsg(mixed msg,int probability)
{
  if (!Pmove_msgs) {
    Pmove_msgs = allocate(2);
    Pmove_msgs[0] = ({});
    Pmove_msgs[1] = ({});
  }
  Pmove_msgs[0]+=({msg});
  Pmove_msgs[1]+=({probability});
  return Pmove_msgs;
}

protected void PrintMoveMsg()
// If defined print a random move-message (depending on its
// probability) and start a call_out with a random time
// for the new move-message.
{
  int maxp,actmsg;
  int i;
  mixed msgs;

  while(remove_call_out(SF(PrintMoveMsg)) != -1);
  if (!Pmove_msgs) return;
  maxp = sum(Pmove_msgs[1]);
  actmsg = random(maxp+1);
  for (i=0;i<sizeof(Pmove_msgs[1]);i++)
    if (actmsg<sum(Pmove_msgs[1][0..i]))
      break;
  if (i>=sizeof(Pmove_msgs[1])) i = sizeof(Pmove_msgs[1])-1;
  msgs = Pmove_msgs[0][i];
  if (closurep(msgs))
    if (!stringp(msgs =
      funcall(msgs,QueryFromPort(),QueryToPort(),QueryDecks())))
      return;
  DeckDisplay(msgs);
  call_out(SF(PrintMoveMsg),2+random(10));
}

public void StartJourney()
{
  set_heart_beat(0);
  // heart_beat only needed when waiting in the harbour
  AnnounceLeave();
  seteuid(getuid());
  ME->move(OCEAN,M_SILENT);
  seteuid(0);
  //temporary file to wait there
  call_out(SF(PrintMoveMsg),2+random(10));
}

public void EndJourney()
{
  seteuid(getuid());
  ME->move(QueryToPort(),M_SILENT);
  seteuid(0);
  // move from /obj/ocean or (void) if the ship just got cloned
  AnnounceEnter();
  set_heart_beat(1);
  // Wait at the harbour
  while(remove_call_out(SF(PrintMoveMsg)) != -1);
}

protected void PrintScene(int scene)
// If no further scenes are available end the movement
// else print the scene and start a call_out for
// the next scene
{
  mixed text;
  string from,to;
  int delay;
  while(remove_call_out(SF(PrintScene)) != -1);
  if (!Pmove_scenes
      ||!sizeof(QueryPassengers())
      ||!member(Pmove_scenes,from = QueryFromPort())
      ||!member(Pmove_scenes[from],to = QueryToPort())
      ||!member(Pmove_scenes[from][to],scene))
    return EndJourney();

  text = Pmove_scenes[from][to][scene,SCENE_TEXT];
  delay = Pmove_scenes[from][to][scene,SCENE_DELAY];
  if (closurep(text))
    text = funcall(text,from,to,QueryDecks());
  if (stringp(text))
    DeckDisplay(text);
  call_out(SF(PrintScene),delay,scene+1);
}

public int QueryPFromPort() { return Pfrom_port; }
public int QueryPToPort() { return Pto_port; }

public void NextPort()
// called from heart_beat when being at a harbour
{
  string *ports;
  /* No ports available - simply return */
  if (!sizeof(ports = m_values(QueryPorts()))) return;
  /* Ship does not have an environment or isn't in a port */
  if (!environment()
  || (member(ports,object_name(environment()))) == -1) {
      Pto_port = 0;
      Pcurrent_port = 0;
      Pfrom_port = max_port();
      // DBG(sprintf("NextPort(1): Pcurrent_port=%O, Pto_port=%O, Pfrom_port=%O\n", Pcurrent_port, Pto_port, Pfrom_port));
      return EndJourney();
  }
  Pcurrent_port = current_port();
  Pto_port = next_port();
  // DBG(sprintf("NextPort(2): Pcurrent_port=%O, Pto_port=%O, Pfrom_port=%O\n", Pcurrent_port, Pto_port, Pfrom_port));
  if (Pto_port != Pcurrent_port) {
    Pfrom_port = Pcurrent_port;
    StartJourney();
    call_out(SF(PrintScene),5,0);
  }
#if 0
  DBG(sprintf("NextPort(1): Pto_port=%O, Pfrom_port=%O\n", Pto_port, Pfrom_port));
  if (  !environment()
      ||(pos = member(ports,object_name(environment())))==-1) {
    pos = 0;
    DBG(sprintf("NextPort(2): Pto_port=%O, Pfrom_port=%O\n", Pto_port, Pfrom_port));
    if (!Pto_port = calc_next_port()) {
      Pfrom_port = max(m_indices(QueryPorts()));
      DBG(sprintf("NextPort(3): Pto_port=%O, Pfrom_port=%O\n", Pto_port, Pfrom_port));
    }
    DBG(sprintf("NextPort(4): Pto_port=%O, Pfrom_port=%O\n", Pto_port, Pfrom_port));
    return EndJourney();
  }
  // DBG(sprintf("NextPort(4a): Pto_port=%O, Pfrom_port=%O\n", Pto_port, Pfrom_port));
  // if (!Pto_port = calc_next_port())
  //   Pfrom_port = max(m_indices(QueryPorts()));
  // DBG(sprintf("NextPort(4b): Pto_port=%O, Pfrom_port=%O\n", Pto_port, Pfrom_port));
  Pfrom_port = pos;
  DBG(sprintf("NextPort(5): Pto_port=%O, Pfrom_port=%O\n", Pto_port, Pfrom_port));
  if ((pos = calc_next_port())==Pto_port) {
  // if the only useful destination to move to is the same harbour
  // as we are, just stay here.
    DBG(sprintf("NextPort(6): Pto_port=%O, Pfrom_port=%O\n", Pto_port, Pfrom_port));
    return;
  }
  StartJourney();
  Pto_port = pos;
  DBG(sprintf("NextPort(7): Pto_port=%O, Pfrom_port=%O\n", Pto_port, Pfrom_port));
  call_out(SF(PrintScene),5,0);
#endif
}

public void heart_beat()
// In NPCs the npc::heart_beat() and the
// ship::heart_beat() needs to be called.
{
  if (can_sleep()) {
    set_heart_beat(0);
    return;
  }
  if (   !environment()
      && !Phbcount)
    Phbcount = WAIT_TIME-random(5);
  // Don't let all ships in one harbour arrive at the same time
  Phbcount++;
  if (Phbcount > WAIT_TIME) {
    Phbcount = 0;
    NextPort();
  }
}

public void CallShip()
{
  object ob;
  if (clonep()&&object_name(previous_object())==blueprint()) {
    if (   !environment()
        || object_name(environment())!=OCEAN
      )
      set_heart_beat(1);
    return;
  }
  if (!ob = QueryShip()) {
    seteuid(getuid());
    ob = clone_object(object_name());
    seteuid(0);
  }
  ob->CallShip();
}

public mixed SetEnterMsgs(mixed msg)
// ({<port>,<ship>,<player>}) (either string or closure)
// or closure: If returning array see above,
//             else no extra message
{
  return Penter_msgs = msg;
}

public mixed QueryEnterMsgs()
{
  return Penter_msgs ||
    ({"enters "+({string})ME->QueryShort(),
      "arrives on board",
      "enter "+({string})ME->QueryShort()});
}

public mixed SetLeaveMsgs(mixed msg)
// ({<ship>,<port>,<player>}) (either string or closure)
// or closure: If returning array see above,
//             else no extra message
{
  return Pleave_msgs = msg;
}

public mixed QueryLeaveMsgs()
{
   return Pleave_msgs ||
      ({"leaves the ship",
        "leaves "+({string})ME->QueryShort(),
        "leave "+({string})ME->QueryShort()});
}

public int cmd_leave(string str)
{
  if (   str
      && member(({"ship","boat"}),lower_case(str))==-1
      && locate(environment(),str,SM_OBJECT|SM_IGNLIGHT)!=ME
     )
    return notify_fail("You can't leave "+str+".\n",NOTIFY_NOT_OBJ),0;
  if (object_name(environment())==OCEAN)
    return notify_fail("Sorry, you're not at a harbour.\n",
                       NOTIFY_NOT_VALID),0;
  if (!TP)
    return 0;
  TP->move(environment(),M_SPECIAL,funcall(QueryLeaveMsgs()));
  return 1;
}

public void InitDecks()
{
  string *decks;
  int i;
  i = sizeof(decks = QueryDecks());
  seteuid(getuid());
  while(i--) {
    decks[i]->SetCleanUp(0);
    decks[i]->AddExit("leave",SF(cmd_leave));
  }
  seteuid(0);
}

public int cmd_enter(string str)
{
  if (!TP)
    return 0;
  if (   str
      && member(({"ship","boat"}),lower_case(str))==-1
      && search(TP,str,SM_OBJECT|SM_IGNLIGHT|SEARCH_ENV)!=ME
     )
    return notify_fail("You can't enter "+str+".\n",NOTIFY_NOT_OBJ),0;
  if (!sizeof(QueryDecks()))
    return notify_fail(
      capitalize(({string})ME->QueryShort())+" isn't built to transport passengers.\n",
      NOTIFY_NOT_VALID),0;
  InitDecks();
  TP->move(QueryDecks()[0],M_SPECIAL,funcall(QueryEnterMsgs()));
  return 1;
}

public int remove()
{
  mixed obs;
  int i;
  if (!clonep()&&Pship) {
    write("Warning! You mustn't destroy this blueprint!\n"
          "There's still a ship stored: "+object_name(Pship)+".\n"
          "To help passengers which may be trapped, destroy this\n"
          "ship (passengers will move to void).\n");
    return 0;
  }
  i = sizeof(obs = QueryPassengers());
  // SAVE OUR SOULES!!!
  while(i--)
    obs[i]->move(VOID,M_SPECIAL,
      ({"is secured from a ship which gets destructed",
        "arrives from a sunken ship",
        "escaped from a sinking ship"}));
  // The ship sinks
  i = sizeof(obs = QueryRooms());
  while(i--)
     if (find_object(obs[i]))
       obs[i]->remove();
  destruct(ME);
  return 1;
}
