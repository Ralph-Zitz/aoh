// This file is part of UNItopia Mudlib.
// ----------------------------------------------------------------
// File:        /i/player/gmcp.c
// Description: Unterstuetzung fuer GMCP
//
// $Log: gmcp.c,v $
// Revision 1.5  2019/10/14 18:19:00  myonara
// Los of new stuff and bugfixes
//
// Revision 1.4  2019/09/22 14:10:37  myonara
// char um wizard ergaenzt. files und dir komplett neu.
//
// Revision 1.3  2019/03/29 15:49:48  gnomi
// Anpassungen fuer neuen Driver.
//
// Revision 1.2  2018/10/21 17:47:31  myonara
// grosser Quantensprung bei GMCP-Implementierung
//
// Revision 1.1  2018/02/09 21:44:26  gnomi
// GMCP-Unterstuetzung.
//
#pragma no_range_check

#include <config.h>
#include <driver/files.h>
#include <secure/wizlevels.h>
#include <msgclass.h>
#include <moving.h>
#include <classes.h>
#include <stats.h>
#include <driver/tls.h>
#include <driver/telnet.h>
#include <driver/time.h>
#include <player.h>
#include <macros.h>
#include <debug.h>

#define DEBUGGER "nostradamus"
#define DEBUG(x) dtell(DEBUGGER, x)
#define TPRN (({string})this_object()->Query(P_REALNAME))
#define GMCP_LOG        50
#define MSG_SOUND "message:sound"
#define MOVE_OBJECT "/i/move::move:object"

private void char_items_list();
private void char_items_add(string ctrl, mapping mv_infos) ;
private void char_items_remove(string ctrl, mapping mv_infos);
static int dump_gmcp_log();
static int update_points_display();
public mixed QueryGMCP();

nosave mapping packages = ([:1]);
nosave mapping editfiles = ([:3]);
nosave mapping editkeys = ([:1]);
#ifdef GMCP_LOG
nosave string* gmcp_log = ({});
#endif
private nosave string last_hpsp_str = "";
private nosave string lstats = "";

#define TIMESTR_ONLY_TIME 1
#define TIMESTR_ONLY_DATE 2

varargs string shorttimestr(int clock, int full_year, int only)
{
  int *t;
  string year;

  t = localtime(clock);

  if (only == TIMESTR_ONLY_TIME)
    return sprintf("%02d:%02d:%02d",t[TM_HOUR], t[TM_MIN], t[TM_SEC]);

  year = to_string(t[TM_YEAR]);
  if (only == TIMESTR_ONLY_DATE)
    return sprintf("%02d.%02d.%s", t[TM_MDAY], t[TM_MON]+1,
      full_year?year:year[<2..<1]);
  return sprintf("%02d.%02d.%s %02d:%02d:%02d", t[TM_MDAY], t[TM_MON]+1,
    full_year?year:year[<2..<1], t[TM_HOUR], t[TM_MIN], t[TM_SEC]);
}

private void log_gmcp(string msg)
{
#ifdef GMCP_LOG
  gmcp_log = gmcp_log[>-GMCP_LOG..<1] + ({ msg });
#endif
}

private void send_gmcp(string package, string message, varargs mixed* data)
{
#if __EFUN_DEFINED__(json_serialize)
  string pkg = lower_case(package);
  if (pkg != "core" && !member(packages, pkg))
    return;
  efun::binary_message( ({ IAC, SB, TELOPT_GMCP }), 1 );
  string msg = package + "." + message;
  if (sizeof(data))
    catch(msg += " " + json_serialize(data[0]));
#if __VERSION__ > "3.5.2"
  efun::binary_message(to_bytes(msg, "UTF-8"), 1);
#else
  efun::binary_message(msg, 1);
#endif
  log_gmcp("Sending: " + msg);
  efun::binary_message( ({ IAC, SE }), 1 );
#endif
}

private void init_gmcp_package(string package)
{
  switch (package)
  {
    case "sound":
      break;
    case "char":
      send_gmcp("Char","Name",
        ([
          "name":capitalize(({string})this_object()->Query(P_REALNAME)),
          "fullname":({string})this_object()->Short(),
          "gender":({string})this_object()->QueryGenderString(),
          "wizard":IS_WIZARD(this_object()),
        ])
      );
      break;
    case "char.items":
      char_items_list();
#if 0
      this_object()->add_controller(
        "notify_moved_in", #'char_items_add);
      this_object()->add_controller(
        "notify_moved_out", #'char_items_remove);
#endif
      break;
  }
}

static void receive_gmcp(string data)
{
  log_gmcp("Received: " + data);

#if __EFUN_DEFINED__(json_parse)
  string* words = explode(data, " ");
  mixed args, temp;

  if (sizeof(words) > 1)
    args = json_parse(implode(words[1..], " "));

  switch(lower_case(words[0]))
  {
    // Modul Core
    case "core.hello":
      break;
    case "core.supports.set":
      temp = packages;
      packages = ([:1]);
      // Fallthrough
    case "core.supports.add":
      foreach (string pkg: args)
      {
        mixed *values = explode(pkg, " ");
        string name;
        int ver, exists;

        if (sizeof(values) != 2)
          continue;

        ver = to_int(values[1]);
        if (!ver)
          continue;
        name = lower_case(values[0]);
        exists = (temp && member(temp, name)) || member(packages, name);
        m_add(packages, name, ver);
        if (!exists)
          init_gmcp_package(name);
      }
      break;
    case "core.supports.remove":
      foreach (string pkg: args)
        m_delete(packages, lower_case(pkg));
      break;
    case "core.ping":
      send_gmcp("Core", "Ping");
      break;
    case "char.items.inv":
      char_items_list();
      break;
    default:
      log_gmcp("Received (unhandled): " + data);
      break;
  }
#endif
}
/*
static int QueryGMCP()
{
  return ({int})this_object()->has_telnet_option(TELOPT_GMCP, 0);
}
*/
static void init_gmcp()
{
  if (!QueryGMCP())
    return;
  send_gmcp("Core", "Hello", ([ "name": MUDNAME ]));
  add_action(#'dump_gmcp_log /*'*/, "gmcp");
  add_action(#'update_points_display /*'*/, "updgmcp");
}

void transfer_gmcp(string *messages)
{
  if (!previous_object() || program_name(previous_object()) != LOGIN ".c")
    return 0;
  log_gmcp("Reset due to transfer from " + program_name(previous_object()));

  packages = ([:1]);
  foreach (string msg: messages)
    receive_gmcp(msg);
}

#ifdef GMCP_LOG
static int dump_gmcp_log()
{
  write(implode(gmcp_log, "\n") + "\n");
  return 1;
}
#endif

private mapping check_content(mapping attributes, string* keys)
{
  mapping result=([]);
  foreach (string key : keys)
  {
    if (!member(attributes||([]),key))
      return 0;
    result[key] = attributes[key];
  }
  return result;
}

#define MY_LIST_GMCP_CHAR_NAME ({"name","fullname","gender","wizard"})
#define MY_LIST_GMCP_STATUS ({"race","guild","level"})
#define MY_LIST_GMCP_VITALS ({"hp","maxhp","sp","maxsp","pep","maxpep",\
                              "mep","maxmep","string"})
#define MY_LIST_GMCP_STATS ({"str","int","con","dex","wis","agi","qui","cha"})
#define MY_LIST_GMCP_ITEMS_HEADER ({"location","items"})
#define MY_LIST_GMCP_ITEM_DETAILS ({"name","category"})
#define MY_LIST_GMCP_ITEM_HEADER ({"location","item"})
#define MY_LIST_GMCP_COMM_TELL ({"player","text"})
#define MY_LIST_GMCP_ROOM_HEADER ({"name", "fullname"})

static varargs void process_gmcp(mapping attributes,string package, string message)
{
  mapping data,header,detail;
  int ix;
  // Sound.Event implizt.
  string sound = attributes && attributes[MSG_SOUND];
  if (sound)
    send_gmcp("Sound", "Event", ([ "file": sound ]));
  if (!package || !message) {
    log_gmcp("Returning: "+package+"."+message);
    return;
  }
  log_gmcp("Process: "+package+"."+message);
  switch (lower_case(package+"."+message))
  {
    // Module core:------------------------------------------
    case "core.goodbye":
      data = check_content(attributes,({"msg"}));
      if (mappingp(data) && stringp(data["msg"]))
        send_gmcp("Core", "Goodbye", data["msg"]);
      return;
    // Module room:------------------------------------------
    case "room.removeplayer":
      data = check_content(attributes,({"msg"}));
      if (mappingp(data) && stringp(data["msg"]))
        send_gmcp("Room", "RemovePlayer", data["msg"]);
      return;
    case "room.addplayer":
      data = check_content(attributes,MY_LIST_GMCP_ROOM_HEADER);
      if (mappingp(data))
        send_gmcp("Room", "AddPlayer", data);
      return;
    case "room.players":
      data = check_content(attributes,({"content"}));
      if (mappingp(data) && pointerp(data["content"]))
        send_gmcp("Room", "Players", data["content"] );
      return;
    // Module char:------------------------------------------
    case "char.name":
      data = check_content(attributes,MY_LIST_GMCP_CHAR_NAME);
      if (mappingp(data))
        send_gmcp("Char", "Name", data);
      return;
    case "char.statusvars":
      data = ([
        "race": "Race",
        "guild": "Guild",
        "level": "Level"
      ]);
      send_gmcp("Char", "StatusVars", data);
      process_gmcp(
        ([
          "race": ({string})this_object()->Query(P_RACE),
          "guild": ({string})this_object()->Query(P_GUILD),
          "level": ({int})this_object()->Query(P_LEVEL),
        ]),
        "Char", "Status"
      );
      return;
    case "char.status": //
      data = check_content(attributes,MY_LIST_GMCP_STATUS);
      if (mappingp(data))
        send_gmcp("Char", "Status", data);
      return;
    case "char.vitals": //
      data = check_content(attributes,MY_LIST_GMCP_VITALS);
      if (mappingp(data))
        send_gmcp("Char", "Vitals", data);
      return;
    case "char.stats": //
      data = check_content(attributes,MY_LIST_GMCP_STATS);
      if (mappingp(data))
        send_gmcp("Char", "Stats", data);
      return;
    // Module char.items:------------------------------------------
    case "char.items.list":
      header = check_content(attributes,
                MY_LIST_GMCP_ITEMS_HEADER);
      if (!mappingp(header) || header["location"] != "inv"
        || !pointerp(header["items"]) )
        return;
      for (ix=0;ix<sizeof(header["items"]);ix++)
      {
        detail = check_content(header["items"][ix],
                  MY_LIST_GMCP_ITEM_DETAILS);
        header["items"][ix] = detail;
      }
      send_gmcp("Char.Items","List",header);
      return;
    case "char.items.add":
      header = check_content(attributes,
                MY_LIST_GMCP_ITEM_HEADER);
      if (!mappingp(header) || header["location"] != "inv"
        || !mappingp(header["item"]) )
        return;
      header["item"] = check_content(header["item"],
                        MY_LIST_GMCP_ITEM_DETAILS);
      send_gmcp("Char.Items","Add",header);
      return;
    case "char.items.remove":
      header = check_content(attributes,
                MY_LIST_GMCP_ITEM_HEADER);
      if (!mappingp(header) || header["location"] != "inv"
        || !mappingp(header["item"]) )
        return;
      header["item"] = check_content(header["item"],
                        MY_LIST_GMCP_ITEM_DETAILS);
      send_gmcp("Char.Items","Remove",header);
      return;
    // Module comm:------------------------------------------
    case "comm.say":
      data = check_content(attributes,MY_LIST_GMCP_COMM_TELL);
      if (mappingp(data))
        send_gmcp("Comm", "Say", data);
      return;
    case "comm.soul":
      data = check_content(attributes,MY_LIST_GMCP_COMM_TELL);
      if (mappingp(data))
        send_gmcp("Comm", "Soul", data);
      return;
    case "comm.tell":
      data = check_content(attributes,MY_LIST_GMCP_COMM_TELL);
      if (mappingp(data))
        send_gmcp("Comm", "Tell", data);
      return;
    default:
      return;
  }
}
#define PRINT_STAT(x) to_string(x)
static int update_points_display()
{
  if (!interactive() || !QueryGMCP() || !member(packages, "char"))
    return 0;
  int my_sp = ({int})this_object()->Query(P_SP);
  int my_hp = ({int})this_object()->Query(P_HP);
  int my_pep = ({int})this_object()->QueryPEP();
  int my_maxpep = ({int})this_object()->QueryMaxPEP();
  int my_mep = ({int})this_object()->QueryMEP();
  int my_maxmep = ({int})this_object()->QueryMaxMEP();
  int my_maxsp = ({int})this_object()->Query(P_MAX_SP);
  int my_maxhp = ({int})this_object()->Query(P_MAX_HP);
  string hpsp_str = my_hp+" HP("+my_maxhp+") and "+my_sp+" SP("+my_maxsp+
                    ") and "+my_pep+" PEP("+my_maxpep+") and "+
                    my_mep+" MEP("+my_maxmep+")";
  if (last_hpsp_str!=hpsp_str)
  {
    last_hpsp_str=hpsp_str;
    process_gmcp(
      ([
        "hp":       my_hp,
        "maxhp":    my_maxhp,
        "sp":       my_sp,
        "maxsp":    my_maxsp,
        "pep":      my_pep,
        "maxpep":   my_maxpep,
        "mep":      my_mep,
        "maxmep":   my_maxmep,
        "string":   hpsp_str,
      ]),
      "Char","Vitals");
  }
  string lstr = PRINT_STAT(({int})this_object()->Query(P_STR));
  string lint = PRINT_STAT(({int})this_object()->Query(P_INT));
  string lwis = PRINT_STAT(({int})this_object()->Query(P_WIS));
  string lcon = PRINT_STAT(({int})this_object()->Query(P_CON));
  string ldex = PRINT_STAT(({int})this_object()->Query(P_DEX));
  string lagi = PRINT_STAT(({int})this_object()->Query(P_AGI));
  string lqui = PRINT_STAT(({int})this_object()->Query(P_QUI));
  string lcha = PRINT_STAT(({int})this_object()->Query(P_CHA));
  string lneu = lstr+"/"+lint+"/"+lcon+"/"+ldex+"/"+lwis+"/"+
                lagi+"/"+lqui+"/"+lcha;
  if (lneu != lstats)
  {
    lstats = lneu;
    process_gmcp(
      ([
        "str": lstr,
        "int": lint,
        "wis": lwis,
        "con": lcon,
        "dex": ldex,
        "cha": lcha,
        "qui": lqui,
        "agi": lagi,
      ]),
      "Char","Stats");
  }
  return 1;
}

//---------------------------------------------------------------------
// Inventarfunktionen:
// redundant zu einem Teil von /i/player/inv.c
string object_category(object ob) {
  if (!pointerp(({string *})ob->QueryClassIds()))
    return "unknown";
  if (member(({string *})ob->QueryClassIds(), C_NPC) != -1) return C_NPC;
  if (member(({string *})ob->QueryClassIds(), C_LIVING) != -1) return C_LIVING;
  if (member(({string *})ob->QueryClassIds(), C_WEAPON) != -1) return C_WEAPON;
  if (member(({string *})ob->QueryClassIds(), C_ARMOUR) != -1) return C_ARMOUR;
  if (member(({string *})ob->QueryClassIds(), C_CONTAINER) != -1) return C_CONTAINER;
  if (member(({string *})ob->QueryClassIds(), C_FOOD) != -1) return C_FOOD;
  if (member(({string *})ob->QueryClassIds(), C_DRINK) != -1) return C_DRINK;
//  if (ob->query_cloth()) return C_CLOTHES;
//  if (ob->material("wasser") || ob->material("nahrung")) return IC_FOOD;
  if (member(({string *})ob->QueryClassIds(), C_MONEY) != -1) return C_MONEY;
//  if (ob->query_value()) return IC_VALUEABLES;
//  return IC_OTHER;
  return sizeof(({string *})ob->QueryClassIds()||({})) ? (({string *})ob->QueryClassIds())[0]: "unknown";
}

public void RoomPlayers() {
  mapping *res;
  object *obs = filter(all_inventory(ENV(this_object())), #'living /*'*/);
  filter(obs, (: ({object})this_object()->CanSeeLiving($1) :));
  res = map(obs,
    (:
      return ([
        "name": NAME($1),
        "fullname": ({string})$1->Short()
      ]);
    :)
  );
  process_gmcp(
    ([
      "content": res
    ]),
    "Room", "Players"
  );
}

public void RoomRemovePlayer(object pl) {
  if (!living(pl))
    return;
  if (!({int})this_object()->CanSeeLiving(pl))
    return;
  process_gmcp(
    ([
      "msg": ({string})pl->Query(P_NAME)
    ]),
    "Room", "RemovePlayer"
  );
}

public void RoomAddPlayer(object pl) {
  if (!living(pl))
    return;
  if (!({int})this_object()->CanSeeLiving(pl))
    return;
  process_gmcp(
    ([
      "name":     ({string})pl->Query(P_NAME),
      "fullname": ({string})pl->Short()
    ]),
    "Room", "AddPlayer"
  );
}

private void char_items_list() {
  object *obs;
  obs = all_inventory(this_object());
  if (!IS_WIZARD(this_object()))
    obs = filter(obs, (: !({int})$1->Query(P_INVIS) :));
  // TODO: Autoloaderanzeigeeinstellung abfragen und weiter filtern
  //ob->query_inventory_flags() & IF_HIDE_INVENTORY
  process_gmcp( ([
    "location": "inv",
    "items": map(obs, (:
      ([
         "name": ({string})$1->QueryShort(),
         "category": object_category($1)
      ])
    :)
  )]),"Char.Items", "List");
}

private void char_items_add(string ctrl, mapping mv_infos)
{
  object ob;

  ob = mv_infos[MOVE_OBJECT];
  if (!IS_WIZARD(this_object()) && ({int})ob->Query(P_INVIS))
    return;
  process_gmcp( ([
    "location": "inv",
    "item": ([
      "name": ({string})ob->QueryShort(),
      "category": object_category(ob)
    ])
  ]),"Char.Items", "Add");
}

private void char_items_remove(string ctrl, mapping mv_infos)
{
  object ob;

  ob = mv_infos[MOVE_OBJECT];
  if (!IS_WIZARD(this_object()) && ({int})ob->Query(P_INVIS))
    return;
  process_gmcp( ([
    "location": "inv",
    "item": ([
      "name": ({string})ob->QueryShort(),
      "category": object_category(ob)
    ])
  ]),"Char.Items", "Remove");
}

// tell,say,soul...
static void handle_gmcp_communication(int msg_type,int msg_action,
    string msg,string verursacher)
{
    string cmd;
    if (msg_action == CMSG_EMOTE)
    {
        cmd = "Soul";
    }
    else if (msg_action & (CMSG_SAY|CMSG_TELL))
    {
        if (msg_type & CMSG_TELL)
        {
            cmd = "Tell";
        }
        else
        {
            cmd = "Say";
        }
    }
    else
    {
        return;
    }
    process_gmcp( ([
        "player": verursacher || "-",
        "text":msg,
    ]),"Comm", cmd);
}
