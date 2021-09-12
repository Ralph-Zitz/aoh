/*
** A board for the guilds based on /std/board.
** It allows to restrict groups to players of certain levels
** or to the guild the board is placed in.
**
** Author: Thragor@Nightfall, 08may96
** Update:
*/

#define TP this_player()
#define SF(x) #'x //'
#define D_GROUP   0      /* Last read group */
#define D(x,y) "/players/thragor/lib/debug"->debug("thragor",x,y)

#include <config.h>
#include <news.h>

inherit "/std/board";

private nosave string *guildgroups;
private nosave mapping levelgroups;
// 'group':fromlevel

public string *QueryGroups()
{
  return ((::QueryGroups())||({}))+({});
}

public string *QueryGuildGroups()
{
  return (guildgroups||({}))+({});
}

public string *SetGuildGroups(mixed names)
{
  string *g;
  if (!names) return QueryGuildGroups();
  if (!pointerp(names)) names = ({names});
  g = QueryGroups();
  g = (g-names)+names;
  SetGroups(g);
  return guildgroups = ({})+names;
}

public mapping QueryLevelGroups()
{
  return (levelgroups||([]))+([]);
}

public mapping SetLevelGroups(mixed names)
{
  string *g;
  if (!names) return QueryLevelGroups();
  if (pointerp(names))
    names = mkmapping(names,allocate(sizeof(names)));
  if (!mappingp(names)) names = ([names:0]);
  g = QueryGroups();
  g = (g-m_indices(names))+m_indices(names);
  SetGroups(g);
  return levelgroups = ([])+names;
}

public string *SetGroups(mixed names)
{
  if (!names) return QueryGroups();
  if (!pointerp(names)) names = ({names});
  names = (names-m_indices(QueryLevelGroups())-QueryGuildGroups())+
    m_indices(QueryLevelGroups())+QueryGuildGroups();
  return (::SetGroups(names));
}

private status query_allowed(string group,object pl,object env)
{
  string *g;
  mapping l;
  string *regsearch;
  g = QueryGuildGroups();
  l = QueryLevelGroups();
  if (sizeof(regsearch = regexp(QueryGroups(),group))>1)
    if (sizeof(regexp(QueryGroups(),"^"+group+"$"))==1)
      group = "^"+group+"$";
    else
      return notify_fail("Your selection fits to more than one group.\n"
			 "Please be a bit more specific.\n"
			 "Matching groups: "+implode(regsearch,", ")+".\n",
			 NOTIFY_NOT_VALID),0;
  if (sizeof(regsearch = regexp(g,group)))
    if (!({int})env->is_member(pl))
      return notify_fail("You must be a member of this guild to read "
			 "this group.\n",NOTIFY_NOT_VALID),0;
  g = m_indices(l);
  if (!sizeof(regsearch = regexp(g,group)))
    return 1;
  if (sizeof(regsearch)>1)
    return notify_fail("Your selection fits to more than one group.\n"
		       "Please be a bit more specific.\n",
		       NOTIFY_NOT_VALID),0;
  if (l[regsearch[0]]>({int})pl->QueryLevel())
    return notify_fail("Your level is not high enough.\n",
		       NOTIFY_NOT_VALID),0;
  return 1;
}

public varargs int _chg_act(string newgroup)
{
  object env,news;

  if (   !TP
      || !(env = environment())
      || !(news = _newsserver())
     )
    return 0;
  if (   !newgroup
      && !newgroup = _get_data(TP)[D_GROUP]
     )
    return notify_fail("No group specified.\n",NOTIFY_NOT_VALID),0;
  if (member(_match_groups(),newgroup)==-1)
    return notify_fail("No group '"+newgroup+"' on this board.\n",
		       NOTIFY_NOT_VALID),0;
  if ((({mixed})news->GetStatus(newgroup)) == NEWS_ILLPARM)
    return notify_fail("No such group '"+newgroup+"'.\n",NOTIFY_NOT_VALID),0;
  if (!query_allowed(newgroup,TP,env))
    return 0;
  _set_pgroup(TP, newgroup);
  return 1;
}
