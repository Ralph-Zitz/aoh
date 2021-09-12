/*
** A voting box
** Author: Thragor@Nightfall, 31oct94
** Update: Thragor@Nightfall, 27sep95
*/

inherit "/std/thing";
inherit "/lib/string";

#include <config.h>
#include <secure/wizlevels.h>
#include <properties.h>
#include <search.h>

#define STR "/lib/string"
#define TP this_player()
#define TI this_interactive()
#define ME this_object()
#define SF(x) #'x //'

#define V_NAME 0
#define V_DESC 1
#define V_ALTS 2

#define NO_LEVEL -255
#define ARCH_LEVEL -2
#define WIZ_LEVEL -1

mapping Pvotes;
// ([topic:<name>;<description>;([alternative:({names voted})])])

private nosave string Psavefile; // Where to save the results
private nosave int Pstarttime,
                   Pendtime,
                   Ptopiclvl, // which level you have to have to add a topic
                   Paltlvl,   // which level you have to have to add an
                              // alternative
                   Pvotelvl;  // from which level on you might vote

private nosave closure Pinformfunc;

public string SetSavefile(string f)
{
  if (f) f = resolve_file(f);
  return Psavefile = f;
}
public string QuerySavefile() { return Psavefile; }

public void LoadBox()
{
  string f;
  if (!(f = QuerySavefile())||Pvotes) return;
  if (file_size(f+".o")<0) return;
  seteuid(getuid());
  restore_object(f);
  seteuid(0);
}

public void SaveBox()
{
  string f;
  if (!f = QuerySavefile()) return;
  seteuid(getuid());
  save_object(f);
  seteuid(0);
}

public int SetTopicLevel(int l) { return Ptopiclvl = l; }
public int QueryTopicLevel() { return Ptopiclvl; }

public int SetAlternativeLevel(int l) { return Paltlvl = l; }
public int QueryAlternativeLevel() { return Paltlvl; }

public int SetVoteLevel(int l) { return Pvotelvl = l; }
public int QueryVoteLevel() { return Pvotelvl; }

public mapping QueryVotes()
// ([<topic>:([<alternative>:<nr votes>])])
{
  mapping res;
  string *Tidx,*Aidx;
  int Ti,Ai;
  Ti = sizeof(Tidx = m_indices(Pvotes||([])));
  res = ([]);
  while(Ti--)
    {
      Ai = sizeof(Aidx = m_indices(Pvotes[Tidx[Ti],V_ALTS]));
      res[Tidx[Ti]] = ([]);
      while(Ai--)
	res[Tidx[Ti]][Aidx[Ai]] = sizeof(Pvotes[Tidx[Ti],V_ALTS][Aidx[Ai]]);
    }
  return res;
}

public mapping SetVotes(mixed fasel) { return QueryVotes(); }

public int AddTopic(string topic,string title,string desc)
{
  if (TI) // if (!TI) AddTopic is e. g. called during reset
    {
      if (TI!=TP)
        return notify_fail("No valid call.\n",NOTIFY_NOT_VALID);
      switch(QueryTopicLevel())
        {
         case ARCH_LEVEL:
          if (!IS_WIZARD(TI))
            return notify_fail("You're not allowed to add a topic.\n",
			       NOTIFY_NOT_VALID);
         case WIZ_LEVEL:
          if (!IS_WIZARD(TI))
            return notify_fail("You're not allowed to add a topic.\n",
			       NOTIFY_NOT_VALID);
	 case NO_LEVEL:
	   return notify_fail("No topics might be added.\n",
                              NOTIFY_NOT_VALID);
         default:
          if (({int})TI->QueryLevel()<QueryTopicLevel())
            return notify_fail("You're not allowed to add a topic.\n",
			       NOTIFY_NOT_VALID);
	}
    }
  LoadBox();
  topic = lower_case(topic||"");
  if (   Pvotes
      && member(Pvotes,topic))
    return notify_fail("There's already another topic '"+topic+"'.\n",
		       NOTIFY_NOT_VALID);
  if (!Pvotes)
    Pvotes = m_allocate(0,3);
  Pvotes[topic,V_NAME] = title;
  Pvotes[topic,V_DESC] = desc;
  Pvotes[topic,V_ALTS] = ([]);
  SaveBox();
  return 1;
}

public int RemoveTopic(string topic)
{
  if (!TI||TI!=TP)
    return notify_fail("No valid call.\n",NOTIFY_NOT_VALID);
  switch(QueryTopicLevel())
    {
     case ARCH_LEVEL:
      if (!IS_WIZARD(TI))
        return notify_fail("You're not allowed to delete a topic.\n",
			   NOTIFY_NOT_VALID);
     case WIZ_LEVEL:
      if (!IS_WIZARD(TI))
        return notify_fail("You're not allowed to delete a topic.\n",
			   NOTIFY_NOT_VALID);
     case NO_LEVEL:
       return notify_fail("It's not allowed to remove topics.\n",
			  NOTIFY_NOT_VALID);
     default:
      if (({int})TI->QueryLevel()<QueryTopicLevel())
        return notify_fail("You're not allowed to delete a topic.\n",
			   NOTIFY_NOT_VALID);
    }
  topic = lower_case(topic||"");
  LoadBox();
  if (   !Pvotes
      || !member(Pvotes,topic)
     )
    return notify_fail("There's no topic '"+topic+"'.\n",
		       NOTIFY_NOT_VALID);
  m_delete(Pvotes,topic);
  SaveBox();
  return 1;
}

public string *QueryTopics()
{
  LoadBox();
  return sort_array(m_indices(Pvotes||([])),SF(>));
}

public int AddAlternative(string topic,string alt)
{
  if (!TI||TI!=TP)
    return notify_fail("No valid call.\n",NOTIFY_NOT_VALID);
  switch(QueryAlternativeLevel())
    {
     case ARCH_LEVEL:
      if (!IS_WIZARD(TI))
        return notify_fail("You're not allowed to add an alternative.\n",
			   NOTIFY_NOT_VALID);
     case WIZ_LEVEL:
      if (!IS_WIZARD(TI))
        return notify_fail("You're not allowed to add an alternative.\n",
			   NOTIFY_NOT_VALID);
     case NO_LEVEL:
       return notify_fail("It's not possible to add alternatives.",
			  NOTIFY_NOT_VALID);
     default:
      if (({int})TI->QueryLevel()<QueryAlternativeLevel())
        return notify_fail("You're not allowed to add an alternative.\n",
			   NOTIFY_NOT_VALID);
    }
  topic = lower_case(topic||"");
  LoadBox();
  if (   !Pvotes
      || !member(Pvotes,topic))
    return notify_fail("There's no topic '"+topic+"'.\n",
		       NOTIFY_NOT_VALID);
  if (member(Pvotes[topic,V_ALTS],alt))
    return notify_fail("There's already an alternative '"+alt+"' "
                       "for the topic '"+topic+"'.\n",
		       NOTIFY_NOT_VALID);
  Pvotes[topic,V_ALTS][alt] = ({});
  SaveBox();
  return 1;
}

public int RemoveAlternative(string topic,string alt)
{
  if (!TI||TI!=TP)
    return notify_fail("No valid call.\n",NOTIFY_NOT_VALID);
  switch(QueryAlternativeLevel())
    {
     case ARCH_LEVEL:
      if (!IS_WIZARD(TI))
        return notify_fail("You're not allowed to delete an alternative.\n",
			   NOTIFY_NOT_VALID);
     case WIZ_LEVEL:
      if (!IS_WIZARD(TI))
        return notify_fail("You're not allowed to delete an alternative.\n",
			   NOTIFY_NOT_VALID);
     case NO_LEVEL:
       return notify_fail("It's not possible to delete an alternative.\n",
			  NOTIFY_NOT_VALID);
     default:
      if (({int})TI->QueryLevel()<QueryAlternativeLevel())
        return notify_fail("You're not allowed to delete an alternative.\n",
			   NOTIFY_NOT_VALID);
    }
  topic = lower_case(topic||"");
  LoadBox();
  if (   !Pvotes
      || !member(Pvotes,topic)
     )
    return notify_fail("There's no topic '"+topic+"'.\n",
		       NOTIFY_NOT_VALID);
  if (!member(Pvotes[topic,V_ALTS],alt))
    return notify_fail("There's no alternative '"+alt+"' "
                       "for the topic '"+topic+"'.\n",
		       NOTIFY_NOT_VALID);
  m_delete(Pvotes[topic,V_ALTS],alt);
  SaveBox();
  return 1;
}

public string *QueryAlternatives(string topic)
{
  LoadBox();
  if (   !Pvotes
      || !member(Pvotes,topic))
    return 0;
  return sort_array(m_indices(Pvotes[topic,V_ALTS]),SF(>));
}

public nomask int AddVote(string topic,string alt)
{
  string voter,*voted,altvote,*alts;
  int Ai,Vi;
  
  if (!TI||TI!=TP)
    return notify_fail("No valid call.\n",NOTIFY_NOT_VALID);
  switch(QueryVoteLevel())
    {
     case ARCH_LEVEL:
      if (!IS_WIZARD(TI))
        return notify_fail("You're not allowed to vote.\n",
			   NOTIFY_NOT_VALID);
     case WIZ_LEVEL:
      if (!IS_WIZARD(TI))
        return notify_fail("You're not allowed to vote.\n",
			   NOTIFY_NOT_VALID);
     case NO_LEVEL:
       return notify_fail("It can't be voted here.\n",
			  NOTIFY_NOT_VALID);
     default:
      if (({int})TI->QueryLevel()<QueryVoteLevel())
        return notify_fail("You're not allowed to vote.\n",
			   NOTIFY_NOT_VALID);
      if (sscanf(getuid(TI),"guest%d",Ai))
        return notify_fail("You're not allowed to vote.\n",
			   NOTIFY_NOT_VALID);
    }
  topic = lower_case(topic||"");
  LoadBox();
  if (   !Pvotes
      || !member(Pvotes,topic))
    return notify_fail("There's no topic '"+topic+"'.\n",
		       NOTIFY_NOT_VALID);
  if (!member(Pvotes[topic,V_ALTS],alt))
    return notify_fail("There's no alternative '"+alt+"' "
                       "for the topic '"+topic+"'.\n",
		       NOTIFY_NOT_VALID);
  Ai = sizeof(alts = m_indices(Pvotes[topic,V_ALTS]));
  while(Ai-- && !altvote)
    {
      Vi = sizeof(voted = Pvotes[topic,V_ALTS][alts[Ai]]);
      while(Vi-- && !altvote)
	if ((voter = crypt(getuid(TI),voted[Vi][0..1]))==voted[Vi])
          {
	    write("Deleting old vote on: "+alts[Ai]+".\n");
	    altvote = alts[Ai];
	    break;
	  }
    }
  if (altvote)
    Pvotes[topic,V_ALTS][altvote]-=({voter});
  voter = crypt(getuid(TI));
  Pvotes[topic,V_ALTS][alt]+=({voter});
  SaveBox();
  return 1;
}

public string QueryVoted(string topic)
{
  int Ai,Vi;
  string altvote,*alts,*voted;

  topic = lower_case(topic||"");
  LoadBox();
  if (   !Pvotes
      || !member(Pvotes,topic)
      || !TI)
    return 0;
  Ai = sizeof(alts = m_indices(Pvotes[topic,V_ALTS]));
  while(Ai-- && !altvote)
    {
      Vi = sizeof(voted = Pvotes[topic,V_ALTS][alts[Ai]]);
      while(Vi-- && !altvote)
	if ((crypt(getuid(TI),voted[Vi][0..1]))==voted[Vi])
	  altvote = alts[Ai];
    }
  return altvote;
}
  
public closure SetInformFunc(closure f)
// This function is called at the end of voting with the
// votes as argument.
{
  return Pinformfunc = f;
}
public closure QueryInformFunc() { return Pinformfunc; }

public int SetStartTime(int t) { return Pstarttime = t; }
public int QueryStartTime() { return Pstarttime; }

public int SetEndTime(int t) { return Pendtime = t; }
public int QueryEndTime() { return Pendtime; }

public status CheckActive()
// A voting is active if the time is between starttime and
// endtime, or if no endtime is given, if the time is above
// the starttime.
{
  int t;
  t = time();
  return    t>QueryStartTime()
         && (t<QueryEndTime()||!QueryEndTime());
}

public int cmd_list(string str)
// list topics     list all topics
// list topic <nr> list info to topic
// list votes <nr> list votes to topic
{
  int nr,i;
  string *idx,*Aidx,txt;
  mapping alts;
  
  if (!str)
    return notify_fail("List what?\n",NOTIFY_NOT_OBJ);
  str = lower_case(str);
  if (str=="topics")
    {
      if (!CheckActive())
        return notify_fail("There's no voting right now.\n",
			   NOTIFY_NOT_VALID);
      idx = QueryTopics();
      if (!sizeof(idx))
        return notify_fail("No topics to vote for.\n",
			   NOTIFY_NOT_VALID);
      txt = "";
      for(i=0;i<sizeof(idx);i++)
	txt+=sprintf("%1s %2d. %s\n",(QueryVoted(idx[i])?"":"*"),i+1,
		     Pvotes[idx[i],V_NAME]);
      txt+="\n* marks topics you didn't vote for\n";
    }
  else if (sscanf(str,"topic %d",nr))
    {
      if (!CheckActive())
        return notify_fail("There's no voting right now.\n",
			   NOTIFY_NOT_VALID);
      idx = QueryTopics();
      if (!sizeof(idx))
        return notify_fail("No topics to list.\n",
			   NOTIFY_NOT_VALID);
      nr--;
      if (   nr<0
          || nr>=sizeof(idx))
        return notify_fail("No such topic available.\n",
			   NOTIFY_NOT_VALID);
      txt = Pvotes[idx[nr],V_DESC];
    }
  else if (sscanf(str,"votes %d",nr))
    {
      if (!CheckActive())
        return notify_fail("There's no voting right now.\n",
			   NOTIFY_NOT_VALID);
      idx = QueryTopics();
      if (!sizeof(idx))
        return notify_fail("No topics to vote for.\n",
			   NOTIFY_NOT_VALID);
      nr--;
      if (   nr<0
          || nr>=sizeof(idx))
        return notify_fail("No such topic available.\n",
			   NOTIFY_NOT_VALID);
      alts = QueryVotes()[idx[nr]];
      Aidx = QueryAlternatives(idx[nr]);
      txt = "Votes on topic '"+Pvotes[idx[nr],V_NAME]+"'\n";
      for(i=0;i<sizeof(Aidx);i++)
        txt+=sprintf("%1s %2d. %-30s %3d votes\n",
		     (QueryVoted(idx[nr])==Aidx[i]?"*":""),
                     i+1,Aidx[i],alts[Aidx[i]]);
      txt+="\n* marks the alternative you voted for\n";
    }
  else
    return notify_fail("List what?\n",NOTIFY_NOT_OBJ),0;
  smore(txt,({int})TP->QueryPageSize());
  return 1;
}

public int cmd_vote(string str)
// vote <topicnr> <altnr>
// vote <topicnr>,<altnr>
{
  string *Tidx,*Aidx;
  int Ntopic,Nalt;
  
  notify_fail(
    "Give the number of the topic and the number of the alternative\n"
    "you want to vote for, e. g. 'vote 2 3' (vote for topic 2,\n"
    "alternative 3).\n",NOTIFY_NOT_OBJ);
  if (!str) return 0;
  str = lower_case(str);
  if (   sscanf(str,"%d %d",Ntopic,Nalt)!=2
      && sscanf(implode(explode(str," "),""),"%d,%d",Ntopic,Nalt)!=2
     )
    return 0;

  if (!CheckActive())
    return notify_fail("There's no voting right now.\n",
		       NOTIFY_NOT_VALID);
  Tidx = QueryTopics();
  if (!sizeof(Tidx))
    return notify_fail("No topics to vote for.\n",
		       NOTIFY_NOT_VALID);
  Ntopic--;
  if (   Ntopic<0
      || Ntopic>=sizeof(Tidx))
    return notify_fail("No such topic available.\n",
		       NOTIFY_NOT_VALID);
  Aidx = QueryAlternatives(Tidx[Ntopic]);
  if (!sizeof(Aidx))
    return notify_fail("No alternatives to choose between.\n",
		       NOTIFY_NOT_VALID);
  Nalt--;
  if (   Nalt<0
      || Nalt>=sizeof(Aidx))
    return notify_fail("No such alternative available.\n",
		       NOTIFY_NOT_VALID);

  if (!AddVote(Tidx[Ntopic],Aidx[Nalt]))
    return 0;
  write("Voted.\n");
  return 1;
}
  
public void topic_description(string desc,string topic)
{
  AddTopic(lower_case(topic),topic,desc);
}

public int cmd_add(string str)
// add [topic] For what to vote
// -> Enter description
// add [alternative] 12 A new alternative
{
  string topic,alternative,*topics;
  int topicnr;
  
  if (!str)
    return notify_fail(
      "Add what? (add [topic] <title>, add [alternative] <topicnr> <title>\n",
      NOTIFY_NOT_OBJ),0;
  if (   !sscanf(str,"topic %s",topic)
      && sscanf(str,"%d %s",topicnr,alternative)!=2
      && sscanf(str,"alternative %d %s",topicnr,alternative)!=2)
    topic = str;
  if (!topic)
    {
      topicnr--;
      topics = QueryTopics();
      if (!sizeof(topics))
        return notify_fail("No topics add an alternative to.\n",
		           NOTIFY_NOT_VALID);
      if (   topicnr<0
          || topicnr>=sizeof(topics)
         )
        return notify_fail("No such topic available.\n",
		           NOTIFY_NOT_VALID);
      if (!AddAlternative(topics[topicnr],alternative))
	return 0;
      write("Alternative added.\n");
      return 1;
    }

  if (!TI||TI!=TP)
    return notify_fail("No valid call.\n",NOTIFY_NOT_VALID);
  switch(QueryTopicLevel())
    {
     case -2:
      if (!IS_WIZARD(TI))
        return notify_fail("You're not allowed to add a topic.\n",
			   NOTIFY_NOT_VALID);
     case -1:
      if (!IS_WIZARD(TI))
        return notify_fail("You're not allowed to add a topic.\n",
			   NOTIFY_NOT_VALID);
     default:
      if (({int})TI->QueryLevel()<QueryTopicLevel())
        return notify_fail("You're not allowed to add a topic.\n",
			   NOTIFY_NOT_VALID);
    }

  LoadBox();
  if (   Pvotes
      && member(Pvotes,lower_case(topic)))
    return notify_fail("There's already another topic '"+topic+"'.\n",
		       NOTIFY_NOT_VALID);

  write("Please give a description to the topic '"+topic+"':\n");
  input_text(SF(topic_description),({topic}));
  return 1;
}

public int cmd_remove(string str)
// remove [topic] <topicnr>
// remove [alternative] <topicnr> <altnr>
{
  string *topics,*alts;
  int topicnr,altnr;
  
  if (!str)
    return notify_fail(
      "Remove what? (remove [topic] <nr>, remove [alternative] <topicnr> "
      "<nr>)\n",NOTIFY_NOT_OBJ);
  if (   !sscanf(str,"topic %d",topicnr)
      && sscanf(str,"%d %d",topicnr,altnr)!=2
      && sscanf(str,"alternative %d %d",topicnr,altnr)!=2)
    topicnr = to_int(str);
  if (!altnr)
    {
      topics = QueryTopics();
      if (!sizeof(topics))
        return notify_fail("No topics to remove.\n",NOTIFY_NOT_VALID);
      topicnr--;
      if (   topicnr<0
          || topicnr>=sizeof(topics))
        return notify_fail("No such topic available.\n",
			   NOTIFY_NOT_VALID);
      if (!RemoveTopic(topics[topicnr]))
	return 0;
      write("Topic removed.\n");
      return 1;
    }

  topics = QueryTopics();
  if (!sizeof(topics))
    return notify_fail("No topics to remove alternatives from.\n",
		       NOTIFY_NOT_VALID);
  topicnr--;
  if (   topicnr<0
      || topicnr>=sizeof(topics))
    return notify_fail("No such topic available.\n",
		       NOTIFY_NOT_VALID);

  alts = QueryAlternatives(topics[topicnr]);
  if (!sizeof(alts))
    return notify_fail("No alternatives to remove.\n",
		       NOTIFY_NOT_VALID);
  altnr--;
  if (   altnr<0
      || altnr>=sizeof(alts))
    return notify_fail("No such alternative available.\n",
		       NOTIFY_NOT_VALID);

  if (!RemoveAlternative(topics[topicnr],alts[altnr]))
    return 0;
  write("Alternative removed.\n");
  return 1;
}

public int cmd_help(string str)
{
  if (!TP || !stringp(str) || this_object() != wsearch_inv(str))
    return 0;

  smore(
    "Available commands:\n"
    "  list topics\n"
    "    list all topics\n"
    "  list topic <no>\n"
    "    list info to topic no.\n"
    "  list votes <no>\n"
    "    list votes to topic no.\n"
    "\n"
    "  vote <topic no.> <alternative no.>\n"
    "    vote for an alternative of a topic\n"
    "\n"
    "  add [topic] <title>\n"
    "    add a new topic\n"
    "  add [alternative] <topic no.> <alternative>\n"
    "    add a new alternative to a topic\n"
    "\n"
    "  remove [topic] <no>\n"
    "    remove a topic\n"
    "  remove [alternative] <topic no> <alternative no>\n"
    "    remove an alternative\n"
    "\n"
    "  help voting\n"
    "    this page\n",({int})TP->QueryPageSize());
  return 1;
}

public void reset()
{
  mapping votes;
  
  if (!clonep()) return;
  if (   !CheckActive()
      && sizeof(votes = QueryVotes())
      && QueryInformFunc())
    {
      funcall(QueryInformFunc(),votes);
      Pvotes = 0;
      SaveBox();
    }
}

public varargs string QueryLong()
{
  return (::QueryLong())+
    (CheckActive()
     ? "You may give your votes here. Type 'help voting' to see the\n"
       "available commands.\n"
       "Voting ends on "+ctime(QueryEndTime())+".\n"
     : "Next voting starts on "+ctime(QueryStartTime())+".\n");
}

public void create()
{
  (::create());
  seteuid(getuid());
  SetShort("a voting box");
  SetLong("A simple voting box.\n");
  SetIds(({"box","votingbox"}));
  SetAds(({"voting","simple"}));
  SetNoGet("Don't carry the votes away!!!\n");
}

public void init()
{
  (::init());
  add_action("cmd_list","list");
  add_action("cmd_vote","vote");
  add_action("cmd_add","add");
  add_action("cmd_remove","remove");
  add_action("cmd_help","help");
}
