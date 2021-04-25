//---------------------------------------------------------------------------
// /p/daemons/voteserver.c
//
// The OSB voting server
// (c) Softbyte@OSB 8dec97
// [Softbyte 8dec97]  V1.0:  All standard functions for the voting system
// [Softbyte 7feb98]  V1.1:  Minor improvemnts, coloring
// [Softbyte 10feb98] V1.1:  Proposals can be prolonged and shortened
// [Softbyte 28feb98] V1.1:  Mail will be sent to wizards which did not vote
// [Softbyte 11mar98] V1.1:  Mail text changed (help from Tune)
//---------------------------------------------------------------------------

#include <config.h>
#include <news.h>
#include <properties.h>
#include <moving.h>
#include <daemons.h>
#include <mail.h>
#include <msgclass.h>
#include <skills.h>
#include <string.h>

inherit "/std/base";
inherit "/lib/string";

#define TO this_object()
#define TP this_player()
#define TI this_interactive()


#define VOTE_MAX_OFFLINE_DAYS 40     // more than this days offline will
                                     // omit you in a vote
#define VOTE_MIN_VOTES         0     // so many votes needs an alternative
                                     // at least to be valid
#define VOTE_FINAL_PERCENTAGE 90     // more than this percent participation
                                     // will end a vote early
#define ACTIVE_VOTER "/p/daemons/vote_control"

#define SAVEFILE DAEMONSAVEPATH "voter"
#define VOTE_GRP  "vote"             // newsread group for note news

#define VOTE_DONT_KNOW "Don't know"

#define SEC_PER_DAY  86400

#define VOTE_CREATOR      0   // uid of the crator of the vote
#define VOTE_GROUP        1   // group of the votes, i.e. 'votes.XXX'
#define VOTE_LEVEL        2   // level for the newsgroups
#define VOTE_SHORT        3   // short description of the vote
#define VOTE_LONG         4   // long description of the vote
#define VOTE_ALT          5   // array of alternatives of the vote
#define VOTE_PART         6   // participation to make the vote valid
#define VOTE_PERCENTAGE   7   // percent to accept an alternative
#define VOTE_DURATION     8   // days of vote duration
#define VOTE_PROPOSAL     9   // days of proposal duration
#define VOTE_START       10   // time of vote start
#define VOTE_STATUS      11   // status of the vote
#define VOTE_VOTER       12   // crypted array of who has voted
#define VOTE_VOTES       13   // array of votes for each alternative

#define VOTE_SIZE        14  // last element +1

#define VOTE_STATUS_CREATION  0  // vote is in the creation process
#define VOTE_STATUS_PROPOSED  1  // vote got proposed
#define VOTE_STATUS_ACTIVE    2  // vote is out for vote
#define VOTE_STATUS_CLOSED    3  // vote is over
#define VOTE_STATUS_PROLONGED 4  // vote is prolonged

#define VOTE_IS_PROPOSAL      0  // argument flag means a proposal
#define VOTE_IS_VOTE          1  // argument flag means a vote
#define VOTE_DONT_CARE        2  // argument is vote or proposal

// private mixed *proposed;    // votes which are out for proposal
private mixed *votes;       // votes which are out for vote
private nosave string *act_wizards; // active wizards


// ------------------------------------------------------------------------
//                    internal auxilary functions
// ------------------------------------------------------------------------

// Save the internal data
void Save()
{
  save_object(SAVEFILE);
}

// Checks whether the creation of a vote shall be aborted
private int IsAborted(string str)
{
  if (!str || str=="" || str=="~q")
  {
    msg_write( CMSG_GENERIC|MMSG_DIRECT,
      "You abort the creation of a new vote.\n");
    return 1;
  }
  else return 0;
}
// Checks whether TI has already voted in a vote or proposal
private varargs int HasVoted(mixed *data,mixed who)
{
  string *voter;
  string name;
  int i;
  if (!who) who=TI;
  if (!who || !data ) return 0;

  if (objectp(who)) who=getuid(who);

  if (!who || !stringp(who)) return 0;

  voter=data[VOTE_VOTER];
  name=lower_case(who);

  for (i=0;i<sizeof(voter);i++)
  {
    if (crypt(name,voter[i])==voter[i])
    {
      return 1;
    }
  }
  return 0;
}

// Shows the data of a new vote
private string GetVoteData(mixed *data,int showonly)
{
  string s,pre;
  string what,name;
  string alts;
  int i,j,votes;
  switch(data[VOTE_STATUS])
  {
    case VOTE_STATUS_CREATION: what="vote (in creation)";
    break;
    case VOTE_STATUS_PROPOSED: what="vote proposal";
    break;
    case VOTE_STATUS_ACTIVE: what="vote (active)";
    break;
    case VOTE_STATUS_CLOSED: what="vote (closed)";
    break;
    case VOTE_STATUS_PROLONGED: what="vote (prolonged)";
    break;
    default: what="ERROR";
  }
  alts="";
  if (TI && getuid(TI) )
    name=lower_case(getuid(TI));
  else
    name="dummy";
  for (i=0;i<sizeof(data[VOTE_ALT]);i++)
  {
    // mark the alternative you have voted for
    pre=" ";
    if (HasVoted(data) && showonly)
    {
      for (j=1;j<sizeof(data[VOTE_VOTES][i]);j++)
      {
        if (crypt(name,data[VOTE_VOTES][i][j])==data[VOTE_VOTES][i][j])
        {
          pre="*";
          break;   // found and jump out (performance)
        }
      }
    }
    votes=data[VOTE_VOTES][i][0];
    if (data[VOTE_STATUS]==VOTE_STATUS_CLOSED)  // only show final result
      alts+=sprintf(" %s %2d. %-65.65s (%d)\n",
                pre,i+1,data[VOTE_ALT][i],votes);
    else
      alts+=sprintf(" %s %2d. %-65.65s\n",
                pre,i+1,data[VOTE_ALT][i]);
  }
  s="%^BOLD%^%^GREEN%^"+"The "+what+" '"+data[VOTE_SHORT]+"' ha"+
    (data[VOTE_STATUS]==VOTE_STATUS_CLOSED?"d":"s")+" been created by "+
  capitalize( data[VOTE_CREATOR])+" on "+VOTE_GRP+"."+data[VOTE_GROUP]+
  " on "+ctime(data[VOTE_START])+
  "%^RESET%^\n\n"
  "%^BOLD%^%^GREEN%^Description:%^RESET%^\n"+
  wrap("    "+data[VOTE_LONG],74,4)+"\n"+
  "%^BOLD%^%^GREEN%^Alternatives are:%^RESET%^\n"+
  alts+
  "\n";
  if (data[VOTE_STATUS]!=VOTE_STATUS_CLOSED)
  {
    s+="The vote will last for "+data[VOTE_DURATION]+" days";
    if (data[VOTE_PROPOSAL]>0)
      s+=" and will be out for suggestions for "+data[VOTE_PROPOSAL]+" days.";
    else
      s+=".";
    if (data[VOTE_PART]>0)
     s+=" It needs "+data[VOTE_PART]+"% participation of all active wizards ("+
       (sizeof(act_wizards)*data[VOTE_PART]/100)+"/"+sizeof(act_wizards)+ ").";
    if (data[VOTE_PERCENTAGE]>0)
      s+=" An alternative will be accepted with "+data[VOTE_PERCENTAGE]+
      "% votes.\n";
    else
      s+=" An alternative will be accepted with relative majority.\n";
  }
  return s;
}

// does the vote belong to the right group?
private int is_vote(mixed *data,string vote)
{
  if (data[VOTE_GROUP]!=vote) return 0;
  return 1;
}
// find a vote in the proposed|vote data arrays whose number is 'no'
// if flag==0 the proposed votes are checked if flag==1 the real votes
// are checked. Returned is the vote data array
private mixed *FindVote(string vote,int no,int flag)
{
  mixed *list;
  mixed *data;
  if (!votes) votes=({});
  no--;
  list=filter(votes,#'is_vote,vote /*'*/);
  if (!list || no<0 || no>=sizeof(list) )
  {
    msg_write(CMSG_ERROR,
      "There is no vote or proposal "+(no+1)+" on '"+VOTE_GRP+"."+vote+"'.\n");
    return 0;
  }
  data=list[no];
  if (flag==VOTE_IS_PROPOSAL)
  {
    if (data[VOTE_STATUS]!=VOTE_STATUS_PROPOSED)
    {
      msg_write(CMSG_ERROR,
        "There is no proposal "+(no+1)+" open on '"+VOTE_GRP+"."+vote+"'.\n");
      return 0;
    }
  }
  else if (flag==VOTE_IS_VOTE)
  {
    if (data[VOTE_STATUS]!=VOTE_STATUS_ACTIVE &&
        data[VOTE_STATUS]!=VOTE_STATUS_PROLONGED )
    {
      msg_write(CMSG_ERROR,
        "There is no vote "+(no+1)+" open on '"+VOTE_GRP+"."+vote+"'.\n");
      return 0;
    }
  }
  return data;
}
// makes the group 'group_name' if it does not exists and sets
// read/write access to 'level'
private string MakeNewGroup(string group_name,int level)
{
  mixed erg;
  mixed newdsc;
  string fname;

  fname="@"+explode(object_name(this_object()),"#")[0];
  erg=({mixed})NEWSSERVER->GetStatus(group_name);
  if (!erg)
  {
    // group_owner, read, write, delete, subgroup
    // maxmsg, logmode, 0,0,0
    newdsc=({ group_name,({40}),({level,fname}),({level,fname}),({100,fname}),
             ({100}), -40,M_LOG_EXP,0,0,0 });
    if (NEWS_OK!=(erg=({mixed})NEWSSERVER->AddGroup(group_name,0,0)))
    {
      return ("Error #"+erg+" - cannot create group.\n");
    }
    if (NEWS_OK!=(erg=({mixed})NEWSSERVER->SetGroup(newdsc)))
    {
      return ("Error #"+erg+" - cannot set group.\n");
    }
  }
  return 0;
}
//---------------------------------------------------------------------------
// The newsserver calls the following functions
//---------------------------------------------------------------------------
int QueryXP()
{
  object ob;
  catch(call_other(RACEMASTER, "???"));
  ob = find_object(RACEMASTER);
  if (ob) return ({int})ob->XPfromLevel(42);
  else return 999999;
}
int QueryLevel()
{
  return 42;
}
int CheckRights(string uid)
{
  if (uid==getuid()) return 1;
  log_file("vote_server.log",sprintf("Checkrights failed uid=%O myuid=%O\n",
        uid,getuid()));
  return 0;
}
//---------------------------------------------------------------------------


// Post a note to the group with title and text. If the group does
// not exist it will be created with (level) rights
int PostNote(string group,string title,string text, int level)
{
  mixed erg;
  string res;
  object ipl;

  ipl=this_object()||TI;
  // log_file("vote_server.log",sprintf("TI=%O ipl=%O\n",TI,ipl));
  res=MakeNewGroup(group,level);
  if (res) // error
  {
    if (TP) msg_write(CMSG_ERROR,res);
    log_file("vote_server.log",sprintf("PostNote: %O\n",res));
    return 0;
  }
  erg=({mixed})NEWSSERVER->StoreNote(title,text,group,ipl);
  if (erg==NEWS_ILLPARM || ( pointerp(erg) && sizeof(erg)!=1 ) )
  {
    if (TP) msg_write(CMSG_ERROR,erg);
    else log_file("vote_server.log",erg);
    return 0;
  }
  return 1;
}

// Mail a mail to someone
int MailTo(string to, string subject, string text)
{
  mixed *msg;
  string *deliver;

  msg=({"Voteserver",   // from
        0,
        to,             // to
        ({}),           // cc
        ({}),           // bcc
        subject,        // subject
        0,
        0,
        text});         // body

  deliver=({string *})MAILER->DeliverMail(msg);
  if (!deliver)
  {
    log_file("vote_server.log","Cannot mail to "+to+" on "+ctime()+".\n");
    return 0;
  }
 return 1;
}
// ------------------------------------------------------------------------
//                    internal input_to functions
// ------------------------------------------------------------------------
static void _duration(string str,mixed data)
{
  int duration;
  int proposal;
  int i;
  if (IsAborted(str)) return;
  // Either got two numbers or one
  if (2!=sscanf(str,"%d %d",duration,proposal))
  {
    duration=to_int(str);
    proposal=duration/2;
  }
  if (duration<7) duration=7;
  if (duration>100) duration=100;
  if (proposal<2) proposal=2;
  if (proposal>30) proposal=30;

  data+=({duration,proposal});
  // fill the array
  data+=({time(),VOTE_STATUS_CREATION,({}),
         allocate(sizeof(data[VOTE_ALT])) });
  for (i=0;i<sizeof(data[VOTE_VOTES]);i++)
  {
    data[VOTE_VOTES][i]=({0});  // 0 votes for each alternative
  }

  msg_write( CMSG_GENERIC|MMSG_DIRECT,
    GetVoteData(data,1)+mkstr(70,"-")+"\n"+
    "%^BOLD%^%^GREEN%^To propose this vote type 'propose vote' now.%^RESET%^\n");
  TI->Set("_create_vote",data);
}
static void _choose(string str,mixed data)
{
  int participation;
  int percentage;
  if (IsAborted(str)) return;
  switch(to_int(str))
  {
    case 1:  // new wizard
      participation=50;  // percent of all active wizards must take part
      percentage=66;     // percent of all votes need to be yes
    break;
    case 2:  // new concept
      participation=50;
      percentage=50;
    break;
    case 3:  // change of concept
      participation=66;
      percentage=66;
    break;
    case 4:  // opinion poll
      participation=10;
      percentage=0;
    break;
    default:
      participation=50;
      percentage=50;
  }
  data+=({participation,percentage});
  msg_write( CMSG_GENERIC|MMSG_DIRECT,
    "%^BOLD%^%^GREEN%^Please enter how many days the vote should last (e.g. 14):"
    "%^RESET%^\n[Giving two numbers will specify the duration for the "
    "vote and for the proposal separately (e.g. 14 7).]\n]");
  input_to("_duration",0,data);
}
static void _alternatives(string str,mixed data)
{
  string *alts;
  if (IsAborted(str)) return;
  alts=explode(str,"\n")-({})-({0})-({""});
  alts+=({VOTE_DONT_KNOW});
  if (!alts) alts=({});
  // printf("str=[%s] alts=%O\n",str,alts);
  data+=({alts});
  msg_write( CMSG_GENERIC|MMSG_DIRECT,
    "%^BOLD%^%^GREEN%^Please choose which of the following fits "
    "best to your vote (participation% / majority %):\n"
    "1. Promotion of a wizard (66% / 66%)\n"
    "2. Proposal of a new concept, rule or idea (50% / 50%)\n"
    "3. Change of an existing concept, rule or idea (66% / 66%) \n"
    "4. Opinion poll (10% / relative)\n"
    "%^RESET%^\n]");
  input_to("_choose",0,data);
}
static void _long_title(string str,mixed data)
{
  if (IsAborted(str)) return;
  data+=({str});
  msg_write( CMSG_GENERIC|MMSG_DIRECT,
    "%^BOLD%^%^GREEN%^Please add the alternatives for the vote "
    "(one in each line, 'Don't know' is automatically added):%^RESET%^\n");
  input_text("_alternatives",this_object(),TI,0,({data}));
}
static void _short_title(string str,mixed data)
{
  if (IsAborted(str)) return;
  data+=({str});
  msg_write( CMSG_GENERIC|MMSG_DIRECT,
    "%^BOLD%^%^GREEN%^Please input the long title of the vote "
    "(i.e. a 2-3 line description:%^RESET%^\n");
  input_text("_long_title",this_object(),TI,0,({data}));
}


// ------------------------------------------------------------------------
//                    External commands
// ------------------------------------------------------------------------

// create a new vote...save its data in TI
int CreateVote(string vote,int level)
{
  mixed *data;
  if (!TI || !vote) return 0;
  if (({int})TI->QueryIsNoVoter())
  {
    msg_write(CMSG_GENERIC,
     "%^BOLD%^%^RED%^You are no active voter!"
     " Change it with the voting command.%^RESET%^\n");
    return 1;
  }
  msg_write(CMSG_GENERIC,
  "%^BOLD%^%^RED%^Creating a new vote for wizards:%^RESET%^\n"
  "  The creation of the vote will be aborted if you type '~q' or an empty\n"
  "  line in any of the following questions.\n");
  msg_write(CMSG_GENERIC|MMSG_DIRECT,
  "%^BOLD%^%^GREEN%^Please input the short title of the vote "
  "(e.g. XYZ for Wizard):%^RESET%^\n]");
  data=({getuid(TI),vote,level});
  input_to("_short_title",0,data);
  return 1;
}

// propose a new vote...save the data in TO
int ProposeVote(mixed *data)
{
  string group_name;
  string title,text;
  int i;

  if (!TI || !data || sizeof(data)!=VOTE_SIZE) return 0;

  group_name=VOTE_GRP+"."+data[VOTE_GROUP];

  data[VOTE_START]=time();
  data[VOTE_STATUS]=VOTE_STATUS_PROPOSED;
  data[VOTE_VOTER]=({});
  data[VOTE_VOTES]= allocate(sizeof(data[VOTE_ALT]));
  for (i=0;i<sizeof(data[VOTE_VOTES]);i++)
  {
    data[VOTE_VOTES][i]=({0});  // 0 votes for each alternative
  }

  title="New vote proposal '"+data[VOTE_SHORT]+"'";
  text=GetVoteData(data,0);
  text+=mkstr(70,"-")+"\n";
  text+="The proposal for this vote starts on "+ctime()+" and ends\non "
       +ctime(time()+data[VOTE_PROPOSAL]*SEC_PER_DAY)+".\n";
  text+="You can add further alternatives to this vote (see the help\n"
        "of your voting tool for details)\n";
  text+=mkstr(70,"-")+"\n";

  if (!votes) votes=({});
  votes+=({data});

  Save();

  if (!PostNote(group_name,title,text,data[VOTE_LEVEL]))
    msg_write(CMSG_ERROR,"Cannot post note.\n");
  else
    msg_write(CMSG_GENERIC,"Proposal posted on "+group_name+".\n");

  return 1;
}

// Shall the list of votes been shown to 'who'
varargs int ShallShowVotes(string vote,object who)
{
  int i;
  mixed *list;
  mixed *data;
  if (!who) who=TI||TP;
  if (!who) return 0;
  if (!votes) votes=({});
  list=filter(votes,#'is_vote,vote /*'*/);
  for (i=0;i<sizeof(list);i++)
  {
    data=list[i];
    // Only show for open votes
    if (data[VOTE_STATUS]==VOTE_STATUS_ACTIVE ||
             data[VOTE_STATUS]==VOTE_STATUS_PROLONGED )
    {
      if (!HasVoted(data,who)) return 1;
    }
  }
  return 0;
}

// list the proposals or the votes
// flag has no meaning anymore
// mode 0:default: 1:short list
varargs int ListVotes(string vote,int flag,object who,int mode)
{
  mixed *list;
  mixed *data;
  string s,sta;
  int i,no;
  int d,t;
  if (!who) who=(TP||TI);
  if (!who) return 0;
  if (!votes) votes=({});

  list=filter(votes,#'is_vote,vote /*'*/);
  s="%^BOLD%^%^GREEN%^The following votes are for election on '"+VOTE_GRP+"."+
        vote+"':%^RESET%^\n";
  s+=sprintf(" %-2.2s  %-20.20s  %-10.10s  %-5.5s  %-8.8s %-7.7s %-6.6s %-7.7s\n",
     "","","","","","Partic-","Perce-","Sum of");
  s+=sprintf(" %-2.2s: %-20.20s  %-10.10s  %-5.5s  %-8.8s %-7.7s %-6.6s %-7.7s\n",
     "No","Title","Creator","Time","Status","ipation", "ntage","Votes");

  for (i=0;i<sizeof(list);i++)
  {
    data=list[i];
    t=(time()-data[VOTE_START])/SEC_PER_DAY;  // days running
    d=data[VOTE_DURATION];  // duration of the vote
    no=sizeof(data[VOTE_VOTER]);
    switch(data[VOTE_STATUS])
    {
      case VOTE_STATUS_PROPOSED: sta="proposed";
                                 no=-no;
                                 d=data[VOTE_PROPOSAL]; // overwrite time
      break;
      case VOTE_STATUS_ACTIVE:   sta="open";
      break;
      case VOTE_STATUS_CLOSED:   sta="closed";
      break;
      default:                   sta="unknown";
    }
    s+=sprintf("%c%2.2d: %-20.20s  %-10.10s  %2d/%2d  %-8.8s  %4d%%  %4s  %4s\n",
      HasVoted(data,who)?'*':' ',i+1,data[VOTE_SHORT],
      capitalize(data[VOTE_CREATOR]),
      t,d,sta,data[VOTE_PART],
      data[VOTE_PERCENTAGE]>0?to_string(data[VOTE_PERCENTAGE])+"%":"rel",
      no!=0?to_string(no):"--" );
  } // end for
  if ( ! (mode & 1) ) // no short form
  {
    s+="\n- Currently "+sizeof(act_wizards)+" people are listed as voter.\n";
    s+="- In the case of a proposal a negative number of votes means that\n"
       "  so many people have rejected the proposal.\n";
    s+="- You can give your vote if a vote is marked 'open' and 'deny' a\n"
       "  proposal if you want it to be revised before voting.\n";
  }
  msg_object(who,CMSG_GENERIC,s);

  return 1;
}

// list the details of the proposals or the votes
int InfoVote(string vote,int no,int flag)
{
  mixed *data;
  string s;

  data=FindVote(vote,no,VOTE_DONT_CARE ); // "wizards",#of vote, proposal|vote
  if (!data) return 1;

  s=GetVoteData(data,1);
  if (data[VOTE_STATUS]==VOTE_STATUS_ACTIVE)
  {
    s+="\nCurrently "+sizeof(data[VOTE_VOTER])+" vote"+
       (sizeof(data[VOTE_VOTER])>1?"s have":" has")+
       " been made, resulting in a participation of "+
       (100*sizeof(data[VOTE_VOTER])/sizeof(act_wizards))+"%.\n";
    if (!HasVoted(data))
      s+="You have not yet voted in this vote.\n";
  }
  msg_write(CMSG_GENERIC,s);
  return 1;
}


// Evaluates a proposal and put it out for vote if all checks are passed
string EvaluateProposal(string vote,int no,int force)
{
  mixed *data;
  string group_name,title,text;
  int ok;

  if (!TI) force=0;   // only TI can force a vote

  ok=0;
  data=FindVote(vote,no,VOTE_IS_PROPOSAL ); // "wizards",#of vote, proposal
  if (!data) return ("Proposal "+to_string(no)+" not existant");

  group_name=VOTE_GRP+"."+data[VOTE_GROUP];

  if (!force)  // perform the checks
  {
    // proposal end day passed?
    if (time()< data[VOTE_START]+data[VOTE_PROPOSAL]*SEC_PER_DAY)
    {
      return 0;   // not yet passed
    }
    // too many denials?
    if (sizeof(data[VOTE_VOTER])>sizeof(act_wizards)*data[VOTE_PART]/100)
    {
      title="Proposal '"+data[VOTE_SHORT]+"' rejected";
      text="The proposal for the vote '"+data[VOTE_SHORT]+"' got "
           "rejected by "+sizeof(data[VOTE_VOTER])+" voters.\n";
    }
    else ok=1;
  }
  else ok=1;

  votes-=({ data });



  if (ok)
  {
    data[VOTE_STATUS]=VOTE_STATUS_ACTIVE;
    data[VOTE_START]=time();
    data[VOTE_PROPOSAL]=0;
    data[VOTE_VOTER]=({});
    votes+=({ data });

    title="New vote '"+data[VOTE_SHORT]+"'";
    text=GetVoteData(data,0);
    text+=mkstr(70,"-")+"\n";
    text+="The vote starts on "+ctime()+" and ends\non "
         +ctime(time()+data[VOTE_DURATION]*SEC_PER_DAY)+".\n";
    if (force)
      text+="%^BOLD%^%^RED%^The early start of this vote has been forced by "+
            capitalize(getuid(TI))+".%^RESET%^\n";
    text+=mkstr(70,"-")+"\n";
  }

  Save();

  log_file("voteresults.log",
    mkstr(70,"-")+"\n"+
    "title="+title+"\text="+text+"\n"+
    mkstr(70,"-")+"\n");
  if (!PostNote(group_name,title,text,data[VOTE_LEVEL]))
  {
    if (TP) msg_write(CMSG_ERROR,"Cannot post note.\n");
    else log_file("vote_server.log","Cannot post note\n");

  }
  return title;
}

// Evaluates a vote and put its result out on the board if it is finished
string EvaluateVote(string vote,int no,int force)
{
  mixed *data;
  string *alts;
  string group_name,title,text;
  int vote_no,i,winnumber;
  int willmail;
  int *winner;

  if (!TI) force=0;   // only TI can force a vote

  data=FindVote(vote,no,VOTE_IS_VOTE ); // "wizards",#of vote, vote
  if (!data) return ("Vote "+to_string(no)+" not existant");

  group_name=VOTE_GRP+"."+data[VOTE_GROUP];

  vote_no=sizeof(data[VOTE_VOTER]); // how many people did take part?
  willmail=0;

  // perform the checks
  // proposal end day passed or forced and not more than 90% of votes have
  // been made?
// TODO: Implement the following if case if all wizards have been logged in
//  if  ((time()< data[VOTE_START]+data[VOTE_DURATION]*SEC_PER_DAY) && !force
//      && (100*vote_no/sizeof(act_wizards)<VOTE_FINAL_PERCENTAGE) )
  if  ((time()< data[VOTE_START]+data[VOTE_DURATION]*SEC_PER_DAY) && !force )
  {
    return 0;   // not yet passed
  }

  // too little participation but not a deciding ballot (prolonged)?
  if (100*vote_no<sizeof(act_wizards)*data[VOTE_PART]
      && data[VOTE_STATUS]!=VOTE_STATUS_PROLONGED )
  {
    votes-=({ data });
    data[VOTE_STATUS]=VOTE_STATUS_CLOSED;
    title="Vote '"+data[VOTE_SHORT]+"' aborted";
    text="%^BOLD%^%^RED%^The vote '"+data[VOTE_SHORT]+"' got aborted due to too little "
         "participation.%^RESET%^ Only "+vote_no+" voters took "
         "part ("+(100*vote_no/sizeof(act_wizards))+"%). "
         "Necessary would have been "+data[VOTE_PART]+"%.\n";
    text+=mkstr(70,"-")+"\n";
    text+=GetVoteData(data,0);
    text+=mkstr(70,"-")+"\n";
    willmail=1;
  }
  else
  {
    // find the winner of the vote
    winner=({});
    winnumber=VOTE_MIN_VOTES;  // minimal number of votes for a winner

    for (i=0;i<sizeof(data[VOTE_VOTES]);i++)
    {
      if (data[VOTE_ALT][i]==VOTE_DONT_KNOW) continue; // omit don't knows

      if (data[VOTE_VOTES][i][0]>winnumber)       // found a winner
      {
        winnumber=data[VOTE_VOTES][i][0];
        winner=({i});
      }
      else if (data[VOTE_VOTES][i][0]>=winnumber) // found a drawn
      {
        winnumber=data[VOTE_VOTES][i][0];
        winner+=({i});
      }
    }

printf("winnumber=%d vote_no=%d %%=%d\n",100*winnumber,vote_no,data[VOTE_PERCENTAGE]);

    // Too little votes for winning alternative ?
    if (  (100*winnumber<vote_no*data[VOTE_PERCENTAGE]
           && data[VOTE_PERCENTAGE]>0) // no relativ vote
        || sizeof(winner)==0 )
    {
      votes-=({ data });
      data[VOTE_STATUS]=VOTE_STATUS_CLOSED;
      title="Vote '"+data[VOTE_SHORT]+"' cancelled";
      text="%^BOLD%^%^RED%^The vote '"+data[VOTE_SHORT]+
      "' got cancelled due to too little "
           "votes for any alternative.%^RESET%^ No alternative did get "+
           VOTE_MIN_VOTES+" votes or "+data[VOTE_PERCENTAGE]+"% of valid "
           "votes.\n";
      text+=mkstr(70,"-")+"\n";
      text+=GetVoteData(data,0);
      text+=mkstr(70,"-")+"\n";
      willmail=1;
    }
    // A drawn
    else if (sizeof(winner)>1)
    {
      votes-=({ data });
      title="Vote '"+data[VOTE_SHORT]+"' is drawn";
      // keep only the winning alternatives
      alts=({});
      for (i=0;i<sizeof(data[VOTE_ALT]);i++)
      {
        if (data[VOTE_VOTES][i][0]==winnumber) alts+=({data[VOTE_ALT][i]});
      }
      data[VOTE_ALT]=alts;
      data[VOTE_VOTES]=allocate(sizeof(alts));
      data[VOTE_VOTER]=({});
      data[VOTE_PART]=0;        // a pronlonged vote will have 0 here
      data[VOTE_PERCENTAGE]=0;
      data[VOTE_DURATION]=7;
      data[VOTE_PROPOSAL]=0;
      data[VOTE_START]=time();
      data[VOTE_STATUS]=VOTE_STATUS_PROLONGED;

      text="%^BOLD%^%^RED%^The vote '"+data[VOTE_SHORT]+"' is drawn and will be "
           "prolonged for one week as a deciding ballot.%^RESET%^\n\n";
      text+=mkstr(70,"-")+"\n";
      text+=GetVoteData(data,0);
      text+=mkstr(70,"-")+"\n";
      votes+=({data});
    }
    // A clear win
    else
    {
      votes-=({ data });
      data[VOTE_STATUS]=VOTE_STATUS_CLOSED;

      title="Vote result for '"+data[VOTE_SHORT]+"'";
      text ="\n%^BOLD%^%^RED%^Winning alternative of the vote '"+data[VOTE_SHORT]+
            "' is '"+data[VOTE_ALT][winner[0]]+
            "' with "+winnumber+" vote"+(winnumber>1?"s":"")+
            " out of "+vote_no+" vote"+(vote_no>1?"s":"")+
            " ("+(100*winnumber/vote_no)+")%.%^RESET%^\n\n";
      text+=mkstr(70,"-")+"\n";
      text+=GetVoteData(data,0);
      text+=mkstr(70,"-")+"\n";
      willmail=1;
    }
  } // end else

  Save();
  if (force)
  {
    text+="%^BOLD%^%^RED%^The early end of this vote has been forced by "+
          capitalize(getuid(TI))+".%^RESET%^\n";
    text+=mkstr(70,"-")+"\n";
  }

  log_file("voteresults.log",
    mkstr(70,"-")+"\n"+
    "title="+title+"\text="+text+"\n"+
    mkstr(70,"-")+"\n");

  // Post the outcome of the vote
  if (!PostNote(group_name,title,text,data[VOTE_LEVEL]))
  {
    if (TP) msg_write(CMSG_ERROR,"Cannot post note.\n");
    else log_file("vote_server.log","Cannot post note\n");
  }

  // mail a mail to all not participating wiazrds
  if (willmail)
  {
    for (i=0;i<sizeof(act_wizards);i++)
    {
      if (!HasVoted(data,act_wizards[i]))
      {
        // lib/string is inherited
        MailTo(act_wizards[i],"Vote '"+data[VOTE_SHORT]+"'",
           wrap(
               "Dear "+capitalize(act_wizards[i])+",\n\n"
               "This is an automated message from me, your humble voteserver. "
               "I noticed that you did not take part in the vote "
               "concerning '" + data[VOTE_SHORT] + "'. "
               "Since you are listed as an active voter, I just wanted to "
               "remind you that a vote can only be considered valid if "
               "enough people vote. You can always vote 'I don't know' "
               "if you aren't interested in any particular topic, but "
               "want to remain an active voter.\n\n"

               "If you never want to vote, please "
               "change your voting status to inactive with 'voting off'. That "
               "way, you won't be counted in the participation percentage "
               "(and you won't get any more annoying messages from me, either!).\n\n"

               "Thanks!\n"
               "Your 'voteserver'"
               "\n\n"
               "PS: Useful commands:\n"
               "xclone /obj/tool/voter:  Clone the votingbox\n"
               "help votingbox:     Show all help for the votingbox\n"
               "voting on/off:      Toggle your status as active voter\n"
               "voting show/noshow: Show show new votes at login or not\n"
               "voting:             Show what you have configured\n"
               "list votes:         Show all ongoing votes (* did vote there)\n"
               "list vote 2:        Show details for vote/proposal number 2\n"
               "vote for 2 in 3:    Vote for alternative 2 in vote number 3\n"
               , 60, 5) );
      }
    }
  }

  return title;
}

// Removes a proposal or a vote from the list
int RemoveVote(string vote,int no,int flag)
{
  mixed *data;
  string text,title,group_name,what;

  data=FindVote(vote,no,flag ); // "wizards",#of vote, vote|proposal
  if (!data) return 1;

  group_name=VOTE_GRP+"."+data[VOTE_GROUP];

  if (data[VOTE_CREATOR]!=getuid(TI))
  {
    msg_write(CMSG_ERROR,
      "You are not allowed to remove a vote or proposal of "+
      capitalize(data[VOTE_CREATOR])+".\n");
      return 1;
  }

  votes-=({data});

  if (data[VOTE_STATUS]==VOTE_STATUS_PROPOSED) what="proposal for a vote";
  else what="vote";

  msg_write(CMSG_GENERIC,
    "You removed the "+what+" '"+data[VOTE_SHORT]+"'.\n");

  Save();

  title=what+" '"+data[VOTE_SHORT]+"' removed";
  text=capitalize(getuid(TI))+" removed the "+what+
       " '"+data[VOTE_SHORT]+"'.\n";

  if (!PostNote(group_name,title,text,data[VOTE_LEVEL]))
  {
    if (TP) msg_write(CMSG_ERROR,"Cannot post note.\n");
  }

  return 1;
}


// Denies a proposal
int DenyProposal(string vote,int no)
{
  mixed *data;
  string name;

  if (({int})TI->QueryIsNoVoter())
  {
    msg_write(CMSG_GENERIC,
     "%^BOLD%^%^RED%^You are no active voter!"
     " Change it with the voting command.%^RESET%^\n");
    return 1;
  }
  data=FindVote(vote,no,VOTE_IS_PROPOSAL ); // "wizards",#of vote, proposal
  if (!data) return 1;

  if (HasVoted(data))
  {
    msg_write(CMSG_ERROR,
      "You already have denied that proposal.\n");
    return 1;
  }

  name=lower_case(getuid(TI));
  name=crypt(name);

  votes-=({data});
  data[VOTE_VOTER]+=({name});
  votes+=({data});

  msg_write(CMSG_GENERIC,
    "You deny the proposal '"+data[VOTE_SHORT]+".\n");

  Save();

  return 1;
}

// Remove an alternative from a proposal (if vreator)
int RemoveAlternative(string vote,int no,int alt)
{
  mixed *data;
  string text,title,group_name;
  string oldalt;

  data=FindVote(vote,no,VOTE_IS_PROPOSAL ); // "wizards",#of vote, proposal
  if (!data) return 1;

  if (data[VOTE_CREATOR]!=getuid(TI))
  {
    msg_write(CMSG_ERROR,
      "You are not allowed to remove an alternative from the proposal of "+
      capitalize(data[VOTE_CREATOR])+".\n");
      return 1;
  }

  group_name=VOTE_GRP+"."+data[VOTE_GROUP];
  alt--;
  if (alt<0 || alt>=sizeof(data[VOTE_ALT]) )
  {
      msg_write(CMSG_ERROR,
        "There is no alternative "+(alt+1)+" on '"+data[VOTE_SHORT]+"'.\n");
    return 1;
  }
  votes-=({data});
  oldalt=data[VOTE_ALT][alt];
  data[VOTE_ALT]-=({oldalt});
  data[VOTE_VOTES]= allocate(sizeof(data[VOTE_ALT]));
  votes+=({data});


  msg_write(CMSG_GENERIC,
    "You remove the alternative "+(alt+1)+" ("+oldalt+") from the proposal '"+
    data[VOTE_SHORT]+"'.\n");

  Save();

  title="Alternative removed from proposal '"+data[VOTE_SHORT]+"'";
  text=capitalize(getuid(TI))+" removes the alternative:\n'"+
       oldalt+"'\n"+
       "from the proposal for a vote '"+data[VOTE_SHORT]+"'\n";

  if (!PostNote(group_name,title,text,data[VOTE_LEVEL]))
  {
    if (TP) msg_write(CMSG_ERROR,"Cannot post note.\n");
  }
  return 1;
}


// Add another alternative to a proposal
int AddAlternative(string vote,int no,string alt)
{
  mixed *data;
  string text,title,group_name;
  int i;

  if (!alt) return 0;
  data=FindVote(vote,no,VOTE_IS_PROPOSAL ); // "wizards",#of vote, proposal
  if (!data) return 1;

  group_name=VOTE_GRP+"."+data[VOTE_GROUP];

  votes-=({data});
  data[VOTE_ALT]+=({alt});
  data[VOTE_VOTES]= allocate(sizeof(data[VOTE_ALT]));
  for (i=0;i<sizeof(data[VOTE_VOTES]);i++)
  {
    data[VOTE_VOTES][i]=({0});  // 0 votes for each alternative
  }
  votes+=({data});


  msg_write(CMSG_GENERIC,
    "You add the alternative '"+alt+" to the proposal '"+
    data[VOTE_SHORT]+"'.\n");

  Save();

  title="Alternative added to proposal '"+data[VOTE_SHORT]+"'";
  text=capitalize(getuid(TI))+" adds the alternative:\n'"+
       alt+"'\n"+
       "to the proposal for a vote '"+data[VOTE_SHORT]+"'\n";

  if (!PostNote(group_name,title,text,data[VOTE_LEVEL]))
  {
    if (TP) msg_write(CMSG_ERROR,"Cannot post note.\n");
  }
  return 1;
}

// Prolongs a vote by a couple of days (if creator)
// flag: VOTE_IS_VOTE, VOTE_IS_PROPOSAL
int ProlongVote(string vote, int no, int days, int flag)
{
  mixed *data;
  string text,title,group_name;
  string s;

  if (days<1 && flag!=VOTE_IS_PROPOSAL) return 0;
  if (flag != VOTE_IS_PROPOSAL && flag !=VOTE_IS_VOTE) return 0;
  data=FindVote(vote,no,flag); // "wizards",#of vote, vote|proposal
  if (!data) return 1;

  if (flag==VOTE_IS_PROPOSAL) s="proposal";
  else s="vote";

  if (data[VOTE_CREATOR]!=getuid(TI))
  {
    msg_write(CMSG_ERROR,
      "You are not allowed to prolong a "+s+" of "+
      capitalize(data[VOTE_CREATOR])+".\n");
      return 1;
  }

  group_name=VOTE_GRP+"."+data[VOTE_GROUP];

  if (flag==VOTE_IS_PROPOSAL) data[VOTE_PROPOSAL]+=days;
  else data[VOTE_DURATION]+=days;

  msg_write(CMSG_GENERIC,
    "You prolong the "+s+" '"+data[VOTE_SHORT]+"' by "+days+" day"+
    (days>1?"s":"")+".\n");

  Save();

  title=capitalize(s)+" '"+data[VOTE_SHORT]+"' prolonged";
  text="The ongoing "+s+" '"+data[VOTE_SHORT]+"' got prolonged by "+
       capitalize(getuid(TI))+" by "+days+" day"+(days>1?"s":"")+".\n";

  if (!PostNote(group_name,title,text,data[VOTE_LEVEL]))
  {
    if (TP) msg_write(CMSG_ERROR,"Cannot post note.\n");
  }
  return 1;
}

// Vote for an alternative in a vote
int Vote(string vote,int no,int alt,int secret)
{
  mixed *data;
  string name,s;
  int i,j,found;

  if (({int})TI->QueryIsNoVoter())
  {
    msg_write(CMSG_GENERIC,
     "%^BOLD%^%^RED%^You are no active voter!"
     " Change it with the voting command.%^RESET%^\n");
    return 1;
  }
  data=FindVote(vote,no,VOTE_IS_VOTE ); // "wizards",#of vote, vote
  if (!data) return 1;

  alt--;

  if (alt<0 || alt>=sizeof(data[VOTE_VOTES]) )
  {
      msg_write(CMSG_ERROR,
        "Alternative "+(alt+1)+" does not exist in the vote for '"+
        data[VOTE_SHORT]+"'.\n");
      return 1;
  }

  name=lower_case(getuid(TI));

  votes-=({data});
  found=0;
  for (i=0;i<sizeof(data[VOTE_ALT]) && !found;i++)
  {
    for (j=1;j<sizeof(data[VOTE_VOTES][i]) && !found;j++)
    {
      if (crypt(name,data[VOTE_VOTES][i][j])==data[VOTE_VOTES][i][j])
      {
        // found alternative
        found=1;
        data[VOTE_VOTER]-=({data[VOTE_VOTES][i][j]});
        data[VOTE_VOTES][i]-=({data[VOTE_VOTES][i][j]});
        data[VOTE_VOTES][i][0]--;
        msg_write(CMSG_GENERIC,
          "Removing old vote ("+(i+1)+") in vote "+no+
          " ("+data[VOTE_SHORT]+").\n");
      }
    }
  }
  if (HasVoted(data) && !found )
  {
    msg_write(CMSG_ERROR,
      "You already have voted in that vote. "
      "As you have voted securely in vote "+no+" ("+data[VOTE_SHORT]+
      "), it is not possible to remove your vote.\n");
  }
  else
  {
    name=crypt(name);
    data[VOTE_VOTER]+=({name});
    // prevent finding out who voted what when...
    data[VOTE_VOTER]=sort_array(data[VOTE_VOTER],#'> /*'*/);
    data[VOTE_VOTES][alt][0]++;
    // if no secret voting, write the voters name to the alternative
    if (!secret) data[VOTE_VOTES][alt]+=({name});

    s="You vote for alternative "+(alt+1)+" ("+data[VOTE_ALT][alt]+
      ") in vote "+no+" ("+data[VOTE_SHORT]+").";
    if (secret)
      s+=" You have performed a especially secret vote, which means that "
         "you will not be shown what you have voted afterwards and will "
         "not be able to change your vote.\n";
    else s+="\n";
    msg_write(CMSG_GENERIC,s);
  }
  votes+=({data});
  Save();

  return 1;
}

void UpdateVoting()
{
  int i;
  string s;
  mixed *data;

  // Slow quasi-periodic update
  call_out("UpdateVoting",7200+random(7200));

  // Get an array of active wizards, which have the vote flag set to
  // active and are no longer than VOTE_MAX_OFFLINE_DAYS offline
  act_wizards=({string *})ACTIVE_VOTER->GetActiveWizards(0,VOTE_MAX_OFFLINE_DAYS,0);
  if (!act_wizards || !sizeof(act_wizards)) act_wizards=({"dummy"});

  for (i=0;i<sizeof(votes);i++)
  {
    data=votes[i];
    if (data[VOTE_STATUS]==VOTE_STATUS_PROPOSED)
       s=EvaluateProposal(data[VOTE_GROUP],i+1,0);
    else if (data[VOTE_STATUS]==VOTE_STATUS_ACTIVE ||
             data[VOTE_STATUS]==VOTE_STATUS_PROLONGED )
      s=EvaluateVote(data[VOTE_GROUP],i+1,0);
    if (s) log_file("vote_server.log","- "+s+"\n");
  }

  log_file("vote_server.log","Update voting with "+
         sizeof(act_wizards)+" active wizards on "+ctime()+".\n");
}

void create()
{
  ::create();
  if (is_clone())
  {
    destruct(TO);
    return;
  }
  seteuid(getuid());
  if (file_size(SAVEFILE+".o")>=0)
  {
    restore_object(SAVEFILE);
  }
  else
  {
    votes=({});
  }
  act_wizards=({"dummy"});
//  log_file("vote_server.log","Initalizing vote server  on "+ctime()+".\n");

  // Calculate data and evaluate votings
  UpdateVoting();
  //  call_out("UpdateVoting",10);

}

public varargs int remove(int arg)
{
  Save();
  log_file("vote_server.log","Removing vote server on "+ctime()+".\n");
  remove_call_out("UpdateVoting");
  return ::remove(arg);
}

// ------------------------------------------------------------------------
//                    Debug functions
// ------------------------------------------------------------------------
string *GetActWizards() {return act_wizards;}
mixed *GetVotes()  {return votes;}


