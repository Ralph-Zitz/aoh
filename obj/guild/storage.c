/*
** Storage-Room for warriors
** Author: Thragor@Nightfall, 08mar96
** Update:
*/

#define MIN_VALUE 50
#define TP this_player()
#define SF(x) #'x //'
#define COL_NAME 12
#define COL_OBJECT 20
#define COL_NR 4

#include <moving.h>
#include <secure/wizlevels.h>

inherit "/std/room";
inherit "/lib/string";

private nosave mapping Pobjects;
// ([<player>:([<short>:<nr>])])
// Only objects will be stored from a certain value on
private nosave string Pguild;
private nosave int Plevel;

public string SetGuild(string str)
{
  return Pguild = str;
}

public string QueryGuild()
{
  return Pguild;
}

public int SetLevel(int i)
{
  return Plevel = i;
}

public int QueryLevel()
{
  return Plevel;
}

public mapping SetObjects(mapping m)
{
  return Pobjects = m;
}

public mapping QueryObjects()
{
  return Pobjects||([]);
}

public void AddObject(object ob,object pl)
{
  object *inv;
  mapping obs;
  string short,uid;
  
  if (ob&&query_once_interactive(ob)) return;
  if (sizeof(inv = all_inventory(ob)))
    map(inv,SF(AddObject),pl);
  if (   !ob
      || !pl
      || ob->QueryValue()<MIN_VALUE
      || (   member(inherit_list(ob),"std/armour.c")==-1
          && member(inherit_list(ob),"std/weapon.c")==-1
         )
      || !ob->QueryShort()
     )
    return;
  uid = getuid(pl);
  short = (string)ob->QueryShort();
  obs = QueryObjects();
  if (!member(obs,uid))
    obs[uid] = ([short:1]);
  else
    if (!member(obs[uid],short))
      obs[uid][short] = 1;
    else
      obs[uid][short]++;
  if (!obs[uid][short])
    m_delete(obs[uid],short);
  if (!sizeof(obs[uid]))
    m_delete(obs,uid);
  SetObjects(obs);
}

public void RemoveObject(object ob,object pl)
{
  object *inv;
  mapping obs;
  string short,uid;
  
  if (ob&&query_once_interactive(ob)) return;
  if (sizeof(inv = all_inventory(ob)))
    map(inv,SF(RemoveObject),pl);
  if (   !ob
      || !pl
      || ob->QueryValue()<MIN_VALUE
      || (   member(inherit_list(ob),"std/armour.c")==-1
          && member(inherit_list(ob),"std/weapon.c")==-1
         )
      || !ob->QueryShort()
     )
    return;
  uid = getuid(pl);
  short = ob->QueryShort();
  obs = QueryObjects();
  if (!member(obs,uid))
    obs[uid] = ([short:-1]);
  else
    if (!member(obs[uid],short))
      obs[uid][short] = -1;
    else
      obs[uid][short]--;
  if (!obs[uid][short])
    m_delete(obs[uid],short);
  if (!sizeof(obs[uid]))
    m_delete(obs,uid);
  SetObjects(obs);
}

public string GetList()
{
  string *pls,res;
  mapping obs;
  int i;
  obs = QueryObjects();
  i = sizeof(pls = sort_array(m_indices(obs),SF(<)));
  res = "Objectlist\n"
        "==========\n\n"+
        cadjust("Player Name",COL_NAME)+" | "+
        cadjust("Object",COL_OBJECT)+" | "+
	cadjust("Acc",COL_NR)+"\n"+
        mkstr(COL_NAME,"-")+"-+-"+
        mkstr(COL_OBJECT,"-")+"-+-"+
        mkstr(COL_NR,"-")+"\n";
  while(i--)
    {
      string *shorts;
      int j;
      j = sizeof(shorts = sort_array(m_indices(obs[pls[i]]),SF(<)));
      res+=ladjust(capitalize(pls[i]),COL_NAME)+" | "+
	mkstr(COL_OBJECT)+" | "+mkstr(COL_NR)+"\n";
      while(j--)
	res+=mkstr(COL_NAME)+" | "+
        ladjust(shorts[j],COL_OBJECT)+" | "+
        ladjust(to_string(obs[pls[i]][shorts[j]]),COL_NR)+"\n";
    }
  return res;
}

public int allow_enter(int method,mixed extra)
{
  object po;
  po = previous_object();
  if (query_once_interactive(po)&&!IS_IMMORTAL(po))
    if (po->QueryGuild()!=QueryGuild())
      return write("Only members of "+QueryGuild()+" are allowed to enter.\n"),
                         ME_NO_ENTER;
    else if (po->QueryLevel()<QueryLevel())
      return write("You're not high enough in level to enter.\n"),
                        ME_NO_ENTER;
  return (::allow_enter(method,extra));
}

public void notify_leave(mixed to,int method,mixed extra)
{
  if (TP)
    RemoveObject(previous_object(),TP);
  return (::notify_leave(to,method,extra));
}

public void notify_enter(mixed from,int method,mixed extra)
{
  if (TP)
    AddObject(previous_object(),TP);
  return (::notify_enter(from,method,extra));
}

public int cmd_list(string str)
{
  if (str) return 0;
  smore(GetList(),TP?TP->QueryPageSize():25);
  return 1;
}

public varargs void create()
{
  (::create());
  SetIntShort("a storage room");
  SetIntLong("You're in a storage room.\n");
  AddRoomCmd("list",SF(cmd_list));
}

public int clean_up(int ref)
{
  if (ref<0) return (::clean_up(ref));
  if (sizeof(QueryObjects()))
    return 0;
  return (::clean_up(ref));
}
