/*--------------------------------------------------------------------------
 * /std/living/chat.c -- The livings chat and message handler.
 *
 * Copyright (C) 1997 by One Step Beyond.
 * All Rights Reserved.
 * Permission granted for non-commercial use.
 *
 * [Softbyte 29nov97] Talk functions
 * 060498 [Mica] receive_msg now uses Query(P_QUESTION) instead of pQuestions
 *               directly. This makes it possible to define a custom Queryfun
 *
 *--------------------------------------------------------------------------
 * TODO:  document how to give msg-class withing chats/achats
 * TODO:: ({ (int)class, (mixed) data })
 */

#include <config.h>
#include <living.h>
#include <search.h>
#include <msgclass.h>
#include <properties.h>

#define THIS this_object()
#define TP   this_player()
#define ENV  environment
#define PREV previous_object()

/* -------------------------------------------------------------------------
 * prototypes
 * -------------------------------------------------------------------------
 */

public varargs mixed Query( string str, int sc );             /* std/base */

public void OneChat (int justSched);                           /* forward */
public void InitChats (int chance, mixed strs);               /* forward */

private void printTalk(string seq,object who);
public int cmd_talk(string str);
public int cmd_menu(string str);
/* -------------------------------------------------------------------------
 * global variables
 * -------------------------------------------------------------------------
 */

private int     pChatChance, pAChatChance;
private mixed   *pChats, *pAChats;
private mapping pQuestions;
private string  *pTalkMatch, *pTalkType;
private mixed   *pTalkFun;
private mixed   pTalkObj;
private int     *pTalkDelay;
private mixed   pShrugMsg;

private string *_buffer;                                 /* An experiment */
private int    _sema;                            /* Mutex for receive_msg */

private object m_talker;
private string m_talk_seq;
private mapping m_talk;


/* -------------------------------------------------------------------------
 * buitlin properties
 * -------------------------------------------------------------------------
 */

public int QueryChatChance ()     { return pChatChance; }
public int SetChatChance (int i)  {
  pChatChance = i;
  OneChat(1);
  return pChatChance;
}

public mixed *QueryChats ()        { return pChats; }
public mixed *SetChats( mixed c1 ) { return pChats = c1; }

public int QueryAChatChance ()     { return pAChatChance; }
public int SetAChatChance (int i)  { return pAChatChance = i; }

public mixed *QueryAChats ()       { return pAChats; }
public mixed *SetAChats (mixed *c) { return pAChats = c; }

public mapping QueryQuestions()    { return pQuestions; }
public mapping SetQuestions (mixed c) {
  mapping q;
  int i, j;

  if (!pointerp(c))
    return pQuestions = c;
  q = m_allocate(0, 2);
  for (i = 0; i < sizeof(c); i += 2)
  {
    if (!pointerp(c[i]))
      q[c[i]] = c[i+1];
    else
      for (j = 0; j < sizeof(c[i]); j++)
        q[c[i][j]] = c[i+1];
  }
  return pQuestions = q;
}

public mixed QueryShrugMsg ()      { return pShrugMsg; }
public mixed SetShrugMsg (mixed s) { return pShrugMsg = s; }

public int *QueryTalkDelay ()      { return pTalkDelay; }
public int *SetTalkDelay (int *d)  { return pTalkDelay = d; }

public string *QueryTalkMatch ()         { return pTalkMatch; }
public string *SetTalkMatch (string *c)  { return pTalkMatch = c; }

public string *QueryTalkType ()         { return pTalkType; }
public string *SetTalkType (string *c)  { return pTalkType = c; }

public mixed *QueryTalkFun ()        { return pTalkFun; }
public mixed *SetTalkFun (mixed *c)  { return pTalkFun = c; }

public mixed QueryTalkObj ()        { return pTalkObj; }
public mixed SetTalkObj (mixed c)  { return pTalkObj = c; }

// Debug functions only !
object QueryTalker() {return m_talker;}
int SetTalker(object obj) {m_talker=obj;return 1;}
string QueryTalkSequence() {return m_talk_seq;}
int SetTalkSequence(string s) {m_talk_seq=s;return 1;}
mapping QueryM_Talk() {return m_talk;} // debug only

/* -------------------------------------------------------------------------
 * property access helpfuns
 * -------------------------------------------------------------------------
 */

/* add a question/answer to the living
 */

public varargs void AddQuestion (mixed q, mixed a, int silent) {
  int i;

  if (silent && !pointerp(a))
    a = ({ a, 0 });
  if (!pointerp(q))
  {
    if (!pQuestions)
      pQuestions = ([q:a]);
    else
      pQuestions[q] = a;
  }
  else
  {
    for (i = sizeof(q); i--; )
      if (!pQuestions)
        pQuestions = ([q[i]:a]);
      else
        pQuestions[q[i]] = a;
  }
}

// Add a talk menu entry to the NPC
// Arguments: string|string *key: The talk sequence idendifier (e.g. "acab")
//            string|closure talk: The string or the function returning a
//                    string which will be shown in the menu corresponding
//                    to "acab". The closure gets the arguments object who,
//                    string talk_sequence, i.e. the player which is talking
//                    and the talk sequence "acab" to reuse one function
//           string|closure answer: The text which the NPC will answer to
//                    the player when "acab" is said. In case of a closure
//                    nothing is said but the closure executed with the
//                    same arguments as described above in 'talk'.
//           string result: 'goto' modifier - let the talk proceed as if
//                    the sequence given with 'result' (e.g. "abbb") had
//                    been entered. In this way you can jump to another
//                    part of the talk tree avoiding double writing
// return: 1: ok, 0:fault
varargs int AddTalk(mixed key,mixed talk,mixed answer,string result)
{
int i;
  if (!key || !talk || !answer) return 0;
  if (!m_talk) m_talk=([]);

  if (stringp(key)) key=({key});
  if (result) result=lower_case(result);

  for (i=0;i<sizeof(key);i++)
  {
    m_talk+=([lower_case(key[i]):talk;answer;result;0]);
  }
  return 1;
}

// Remove the key-sequence (e.g. "aaa") from the talk
// Argument: string|string *key : one or more keys to be removed
// return 1: ok, 0: fault
int RemoveTalk(mixed key)
{
int i;
  if (!key) return 0;
  if (!m_talk) m_talk=([]);

  if (stringp(key)) key=({key});
  for (i=0;i<sizeof(key);i++)
  {
    m_talk=m_delete(m_talk,lower_case(key[i]));
  }
  return 1;
}

/* ---------------------------------------------------------------------- */

/* Init the normal chats
 */

public void InitChats (int chance, mixed strs) {
  pChatChance = chance;
  pChats = pointerp(strs) ? strs : ({ strs });
}


/* ---------------------------------------------------------------------- */

/* Init the attack chats
 */

public void InitAChats (int chance, mixed *strs) {
  pAChatChance = chance;
  pAChats = pointerp(strs) ? strs : ({ strs });
}

/* ---------------------------------------------------------------------- */

/* Init the matches for message processing
 */

public varargs void InitMatch ( mixed *fun
                              , string *type
                              , string *match
                              , int    *delay
                              , mixed  ob) {

  if (sizeof(fun) != sizeof(type) || sizeof(match) != sizeof(type))
    return;
  pTalkObj = ob || THIS;
  pTalkFun = fun;
  pTalkType = type;
  pTalkMatch = match;
  pTalkDelay = delay;
}

/* ---------------------------------------------------------------------- */

/* Add a match for message processing
 */

public varargs void AddMatch ( mixed  fun
                             , string type
                             , string match
                             , int    delay
                             ) {
  pTalkFun   = (pTalkFun  ||({})) + ({ fun });
  pTalkType  = (pTalkType ||({})) + ({ type });
  pTalkMatch = (pTalkMatch||({})) + ({ match });
  pTalkDelay = (pTalkDelay||({})) + ({ delay });
}

/* -------------------------------------------------------------------------
 * chat functions
 * -------------------------------------------------------------------------
 */

/* do a chat
 * TODO: DoChat should use somthing different than CMSG_ROOM
 */

public void DoChat () {
  mixed chat;

  if (!pChats || !sizeof(pChats))
    return;
  chat = pChats[random(sizeof(pChats))];
  if (closurep(chat))
    chat = apply(chat, 0);


  if (chat) {
    if ( pointerp( chat ) ) {
      if ( ( sizeof( chat ) > 1 ) && ( intp( chat[0] ) ) )
        msg_room(ENV(), chat[0], chat[1..], ({ THIS }));
      else
        msg_room(ENV(), CMSG_ROOM|MMSG_SEE, chat, ({ THIS }));
    }
    else
      msg_room(ENV(), CMSG_ROOM, chat, ({ THIS }));
  }
}

/* ---------------------------------------------------------------------- */

/* If it's time, do one chat and restart call_out.
 */

public void OneChat (int justSched) {
  int delay;

  if (justSched < 0 && find_call_out("OneChat") != -1)
    return;
  while (remove_call_out("OneChat") != -1);
  if (!pChatChance)
    return;
  if (!justSched)
    DoChat();
  if (1 < sizeof(filter((ENV()?all_inventory(ENV()):({}))
                              +all_inventory(THIS), #'living /*'*/ ))
     ) {
    for (delay = 0; delay < 4000 && random(100) >= pChatChance; delay += 2);
    call_out("OneChat", delay);
  }
}

/* ---------------------------------------------------------------------- */

/* do an attack chat
 * TODO: DoAttackChat should use something different that CMSG_ROOM
 */

public void DoAttackChat (object enemy) {
  mixed achat;

  if (random(100) >= pAChatChance || !pAChats || !sizeof(pAChats))
    return;
  achat = pAChats[random(sizeof(pAChats))];
  if (closurep(achat))
    achat = apply(achat, enemy);

  if (achat) {
    if ( pointerp( achat ) ) {
      if ( ( sizeof( achat ) > 1 ) && ( intp( achat[0] ) ) )
        msg_room(ENV(), achat[0], achat[1..], ({ THIS }));
      else
        msg_room(ENV(), CMSG_ROOM|MMSG_SEE, achat, ({ THIS }));
    }
    else
      msg_room(ENV(), CMSG_ROOM, achat, ({ THIS }));
  }
}

/* -------------------------------------------------------------------------
 * message-match functions
 * -------------------------------------------------------------------------
 */

static void _call_talk_ob (mixed obj, mixed fun, mixed str) {
  if (closurep(fun))
    apply(fun, str);
  else
    call_other(obj, fun, str);
}

public mixed TestMatch (string str) {
  string type, match;
  mixed fun;
  int i, delay;

  for (i = 0; i < sizeof(pTalkMatch); i++) {
    type = pTalkType[i] || "";
    match = pTalkMatch[i] || "";
    if (pTalkFun[i]) fun = pTalkFun[i];
    if (   2 == sscanf(str, "%~s " + type + match + " %~s\n")
        || 1 == sscanf(str, "%~s " + type + match + "\n")
        || 2 == sscanf(str, "%~s " + type + match + "%~s\n")
        || 1 == sscanf(str, "%~s " + type + " " + match + "\n")
        || 2 == sscanf(str, "%~s " + type + " " + match + " %~s\n")
        || 2 == sscanf(str, "%~s " + type + " " + match + "%~s\n")
       )
    {
      if (pointerp(pTalkDelay) && (delay = pTalkDelay[i]) > 0) {
        if (closurep(fun))
          call_out(fun, delay, str);
        else
          call_out( #'_call_talk_ob /*'*/, delay, pTalkObj, fun, str);
        return delay;
      }
      if (closurep(fun))
        return apply(fun, str);
      return ({mixed})call_other(pTalkObj, fun, str);
    }
  }
  return 0;
}

/* -------------------------------------------------------------------------
 * receive_msg - the main message handling fun
 * -------------------------------------------------------------------------
 * class is a message class from msgclass.h
 * from is the originating object
 * data is the message data, type depends on class
 *
 * TODO:  let the programmer decide which kind of message he/she wants
 * TODO:: beeing caught (maybe SetMatchClasses( int * ) )
 */

public void ResetSema () { _sema = 0; _buffer = 0; }

private mixed handle_msg_modifiers( int class, mixed data ) {
  int l;

  if ( ( class & MMSG_SEE ) && ( environment() ) ) {
    l = ({int})environment()->Query(P_INT_LIGHT);
    if (l >= Query(P_IVISION) && l <= Query(P_UVISION) && !Query(P_BLIND)) {
      if ( pointerp( data ) && sizeof( data ) )
	return data[0];
      else
	return data;
    }
    else {
      if ( pointerp( data ) && sizeof( data ) >= 2 )
	return data[1];
      else
        return 0;
    }
  }
  else if ( class & MMSG_HEAR ) {
    if ( pointerp( data ) && sizeof( data ) )
      return data[0]; /* currently everybody hears everything */
    else
      return data;
  }
  else if ( class & MMSG_SMELL ) {
    if ( pointerp( data ) && sizeof( data ) )
      return data[0]; /* currently everybody smells everything */
    else
      return data;
  }

  return data;
}

public void receive_msg( int class, object from, mixed data ) {
  string msg, tmp;
  mixed answer;
  mapping pq;

  /* if we don't have matches defined or issued the message ourselves,
   * and we don't have catch_tell, return
   */

  if ( previous_object() == this_object() )
    return;

  switch( class & MMSG_MASKOUT ) {
  case CMSG_CHANNEL:
    /* TODO:  CMSG_CHANNEL needs an extra function for matches, or maybe
     * TODO:: just pass it through test match. Currently we ignore them,
     * TODO:: as npcs do not get them without being plugged into the
     * TODO:: CHANNEL_D anyway.
     */
    break;
  case CMSG_ASK:
    if ( ( pq = Query( P_QUESTIONS) ) && ( answer = pq[data]))
      answer = funcall(answer, data);

    if (pointerp(answer) && ("" == (answer[1] || "")))
      msg_room( ENV(), CMSG_ROOM, ({string})TP->Query(P_NAME)+" talks to "+Query(P_NAME)+".\n", ({ from, THIS}) );
    else
      msg_room( ENV(), CMSG_ROOM, ({string})TP->Query(P_NAME)+" asks "+Query(P_NAME)+" about "+data+".\n", ({ from, THIS}));

    if (!answer) {
      answer =    funcall(QueryShrugMsg(), data )
        || capitalize(Query(P_NAME))+" shrugs helplessly.\n";
    }

    if (answer) {
      tmp = funcall(pointerp(answer) ? answer[0] : answer, data );
      if (tmp && tmp != "")
        msg_object( from, CMSG_ROOM, tmp );
      tmp = funcall(pointerp(answer) ? answer[1] : answer, data );
      if (tmp && tmp != "")
        msg_room( ENV(), CMSG_ROOM, tmp, ({ from, THIS }));
    }
    break;
  case CMSG_TELL:
  case CMSG_SAY:
  case CMSG_EMOTE:
  case CMSG_ROOM:
  case CMSG_GENERIC:
    /* TODO:  CMSG_GENERIC should not be caught by livings but keep this
     * TODO:: till the message system is fully implemented and accepted.
     */

    /* check if there is anything to do */
    if ( ( ! function_exists( "catch_tell", this_object() ) ) &&
         ( ! pTalkObj ) )
      return;

    /* handle modifiers */
    msg = handle_msg_modifiers( class, data );

    /* only process strings here, we should get nothing else */
    if ( ! stringp( msg ) )
      return;

    if (_sema) {
      if (!_buffer)
        _buffer = ({});
      _buffer += ({ msg });
      return;
    }
    _sema = 1;

    do {
      /* compatiblity - call catch_tell, if it does not exist, pass the
       * msg through TestMatch
       */
      if ( ! call_resolved( & answer, this_object(), "catch_tell", msg ) )
        TestMatch( msg );

      if (_buffer && sizeof(_buffer)) {
        msg = _buffer[0]; _buffer = _buffer[1..];
      }
      else
        msg = 0;
    } while( msg );
    _sema = 0;

    break;
  default:
    /* TODO:  check which other messages classes need to be processed, maybe
     * TODO:: some combat messages too
     */
    break;
  }
}

/* -------------------------------------------------------------------------
 * init() - start chats here
 * -------------------------------------------------------------------------
 */

void init() {
  OneChat(1);
  // Talk menu
  add_action("cmd_talk","talk");
  add_action("cmd_menu","A");
  add_action("cmd_menu","a");
  add_action("cmd_menu","B");
  add_action("cmd_menu","b");
  add_action("cmd_menu","C");
  add_action("cmd_menu","c");
  add_action("cmd_menu","Q");
  add_action("cmd_menu","q");

}


/* -------------------------------------------------------------------------
 * create() - some basic inits
 * -------------------------------------------------------------------------
 */
public void create()
{
  m_talker=0;
  m_talk=([]);
}

/* -------------------------------------------------------------------------
 * internal functions
 * -------------------------------------------------------------------------
 */
// print out a talk menu to the player
// Arguments: string seq: The actual talking sequence (e.g. "acac")
//            object who: The player which will get the menu
private void printTalk(string seq,object who)
{
string s;
string talk;
int flag;
  flag=0;
  s="\n%^BLUE%^What do you want to say to "+({string})THIS->QueryName()+"?%^RESET%^\n";
  // s+="["+seq+"]\n"; // Debug only
  if (member(m_talk,seq+"a"))
  {
    talk=m_talk[seq+"a",0];
    if (closurep(talk)) talk=funcall(talk,who,seq+"a");
    if (!talk) talk="";
    talk=process_string(talk);
    s+="    A. "+talk+"\n";
    flag=1;
  }
  if (member(m_talk,seq+"b"))
  {
    talk=m_talk[seq+"b",0];
    if (closurep(talk)) talk=funcall(talk,who,seq+"b");
    if (!talk) talk="";
    talk=process_string(talk);
    s+="    B. "+talk+"\n";
    flag=1;
  }
  if (member(m_talk,seq+"c"))
  {
    talk=m_talk[seq+"c",0];
    if (closurep(talk)) talk=funcall(talk,who,seq+"c");
    if (!talk) talk="";
    talk=process_string(talk);
    s+="    C. "+talk+"\n";
    flag=1;
  }
  if (member(m_talk,"q"))
  {
    talk=m_talk["q",0];
    if (closurep(talk)) talk=funcall(talk,who,"q");
    if (!talk) talk="";
    talk=process_string(talk);
    s+="    Q. "+talk+"\n";
  }
  else
  {
    s+="    Q. Say goodbye.\n";
  }
  s+="\n";
  if (flag)
  {
    msg_object(who,CMSG_GENERIC,s);
  }
  else
  {
    s="%^BLUE%^The talk with "+({string})THIS->QueryName()+" is finished.%^RESET%^\n";
    msg_object(who,CMSG_GENERIC,s);
    m_talker=0;
    TP->Set("_talking",0);
  }
}

// Command to start the talk with a certain NPC
public int cmd_talk(string str)
{
string whom;
object talking;
  if (!TP) return 0;
  if (!str) return notify_fail(
     "Talk to whom?\n",NOTIFY_NOT_OBJ);
  // Are we meant?
  if ( (1!=sscanf(str,"to %s",whom) &&
        1!=sscanf(str,"with %s",whom)) ||
      (!whom || !({int})THIS->id(whom) ) )
  {
     return notify_fail(
       "Talk to whom?\n",NOTIFY_NOT_OBJ);
  }
  if (m_talker)
  {
    // talker left room ... delete him
    if (environment(m_talker)!=environment(THIS))
    {
      m_talker->Set("_talking",0);
      m_talker=0;
    }
    else if (m_talker==TP) return notify_fail(
     "You are already talking...type 'Q' to abort.\n",NOTIFY_ILL_ARG);
    else return notify_fail(
     "Someone else is talking with "+({string})THIS->QueryName()+
     ". Please wait a little.\n",NOTIFY_NOT_VALID);
  }
  // Has the player already an entry that he is talking with an NPC
  if ( (talking=({object})TP->Query("_talking")) && talking!=THIS)
  {
    if (environment(talking)==environment(TP)) return notify_fail(
      "You are already talking with "+({string})talking->QueryName()+
      ". Stop that talk first with 'Q'.\n",NOTIFY_NOT_VALID);
    else // stop the former talk when player left the room
    {
      talking->SetTalker(0);
    }

  }
  TP->Set("_talking",THIS);
  m_talker=TP;
  m_talk_seq="";
  printTalk(m_talk_seq,m_talker);
  return 1;
}

// Command to process the menu inputs ("aAbBcCqQ")
// This function will in principle handle arbitrarily much letters (a-z)
public int cmd_menu(string str)
{
string verb;
string talk;
mixed answer;
string result;
  if (str) return 0;   // no text

  if (TP!=m_talker) return notify_fail(
     "You are not talking with "+({string})THIS->QueryName()+".\n",NOTIFY_ILL_ARG);


  verb=lower_case(query_verb());
  result=0;
  answer=0;
  talk=0;
  switch(verb)
  {
    case "q":
      if (member(m_talk,"q"))
      {
	talk=m_talk["q",0];
	answer=m_talk["q",1];
      }
      else
      {
	talk="Goodbye "+({string})THIS->QueryName()+"!";
	answer="Goodbye "+({string})TP->QueryName()+"!";
      }
      m_talker=0;
      TP->Set("_talking",0);
    break;
    default:
      m_talk_seq+=verb;
      talk=m_talk[m_talk_seq,0];
      answer=m_talk[m_talk_seq,1];
      result=m_talk[m_talk_seq,2];
    break;
  }
  // Only 'talk' when there is something to say
  if (talk)
  {
    if (closurep(talk)) talk=funcall(talk,TP,m_talk_seq);
    if (talk)
    {
      talk=process_string(talk);
      msg_room(environment(TP),CMSG_SAY,
	  ({string})TP->QueryName()+" says: "+talk+"\n",({TP}));
      msg_object(TP,CMSG_SAY,
	  "You say: "+talk+"\n");
    }
  }
  // Only answer if there is something to say
  if (answer)
  {
      if (closurep(answer))
      {
	 funcall(answer,TP,m_talk_seq);
      }
      else
      {
        answer=process_string(answer);
	msg_room(environment(THIS),CMSG_SAY,
	  ({string})THIS->QueryName()+" says: "+answer+"\n",({THIS}));
	msg_object(THIS,CMSG_SAY,
	    "You say: "+answer+"\n");
      }
  }
  if (result)
  {
    m_talk_seq=result;   // override talk sequence ('goto')
  }
  if (verb!="q") printTalk(m_talk_seq,TP);
  return 1;
}

