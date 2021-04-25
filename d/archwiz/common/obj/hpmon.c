/*
** Hitpoint-Monitor
** Author: Kaanae@Nightfall
** Rewrite: Thragor@Nightfall
** Update: Analena@Nightfall, 18feb97
*/

#include <properties.h>
#include <nightday.h>
#include <moving.h>
#include <living.h>
#include <magic.h>
#include <config.h>
#include <secure/domaininfo.h>
#include "path.h"

#define TP this_player()

#define SF(x) #'x //'
#define SYM(x) 'x //'

#define P_REM_HP "RememberHP" // Remember the HP before this combat-turn
                              // in the enemie-object
#define STATE_OFF     0
#define STATE_ON      1
#define STATE_COMBAT  2

#define DATA_HP    0
#define DATA_SP    1
#define DATA_EXP   2
#define DATA_STATE 3
#define DATA_DELAY 4

#define DATA_SIZE 5

inherit "/std/thing";
inherit STD_TELEPORT;
inherit "/obj/lib/string";

private static int state,hp,sp,exp,delay,maxdelay;
private static string talkpartner;

public string QueryNoGive()
{
  return environment()&&interactive(environment())
         ?"As you want to give the strange black thing away you suddenly\n"
          "think that you'll need it and shouldn't give it away.\n"
         :0;
}

public varargs void SetAutoObject(mixed data)
{
  if (!pointerp(data)||sizeof(data)!=DATA_SIZE)
    {
      if (TP)
        {
          hp = TP->QueryHP();
          sp = TP->QuerySP();
          exp = TP->QueryXP();
        }
      else
        hp = sp = exp = 0;
      delay = maxdelay = 1;
      state = STATE_OFF;
    }
  else
    {
      hp    = data[DATA_HP];
      sp    = data[DATA_SP];
      exp   = data[DATA_EXP];
      state = data[DATA_STATE];
      delay = maxdelay = data[DATA_DELAY];
    }
  if (state&STATE_ON)
    set_heart_beat(1);
}

public int *QueryAutoObject()
{
  int *data;
  data = allocate(DATA_SIZE);
  data[DATA_HP] = hp;
  data[DATA_SP] = sp;
  data[DATA_EXP] = exp;
  data[DATA_STATE] = state;
  data[DATA_DELAY] = maxdelay;
  return data;
}

public void create()
{
  (::create());
  if (!is_clone(this_object())) return;
  seteuid(getuid());
  SetLong(
"This is a strange thing, made out of an unknown material. The material feels "
"very hard, but it is warm. This thing is completely black and has a strange "
"aura. Somehow you feel, that it tries to manipulate your thoughts. "
"You think: I should type 'hpmon'.\n");
  SetHelpMsg("Type 'hpmon'.\n");
  SetInfo("This nice thing was brought to you by Kaanae and Thragor\n");
  SetShort("a black something");
  SetIds(({"monitor","hitpoint-monitor","hpmon","hpmonitor",
           "something","thing"}));
  SetAds(({"special","hitpoint","strange","warm","black"}));
  SetValue(7104);
  SetAutoObject();
  SetNoDrop("You want to drop this strange black thing, but suddenly\n"
            "you think: Hey, why toss this nice thing away. Keep it\n"
            "in your backpack!\n");
}


public void init()
{
  (::init());
  add_action("cmd_hpmon","hpmon");
  add_action("cmd_expcheck", "expcheck");
  add_action("cmd_expcheck", "ec");
  add_action("cmd_go","go");
  add_action("cmd_einfo","einfo");
  add_action("cmd_ptell","ptell");
  add_action("cmd_partner","partner");
}

public object get_pl()
{
  object pl;
  if (   !(pl = environment())
      || !interactive(pl)
     )
    return 0;
  return pl;
}

public object get_env()
{
  object pl,env;
  if (   !(pl = get_pl())
      || (!env = environment(pl))
     )
    return 0;
  return env;
}

public int cmd_ptell(string str)
{
  if (!TP) return 0;
  if (!talkpartner)
    return notify_fail("You haven't defined any talkpartners!\n"
		"Use 'partner' to do so.\n",
		NOTIFY_NOT_VALID),0;
  return TP->command_me("tell "+talkpartner+" "+str);
}

public int cmd_partner(string str)
{
  if (!TP) return 0;
  if (str)
    talkpartner = str;
  write("Your talkpartners are: "+(talkpartner||"none")+".\n");
  return 1;
}

public int cmd_go(string str)
{
  object env,newenv;

  if (str!="home")
    return (int)notify_fail("Edi tells you: You want to go home?\n")&&0;
  switch(CheckTeleport(TP,60,20))
    {
     case TP_NO_SP:
      return notify_fail(
        "Edi tells you: Hm, well. I need your magic energy to do this.\n"
        "  But you don't have enough of it.\n"),0;
     case TP_NO_LEVEL:
      return notify_fail(
        "Edi tells you: Hm. I would like to help you, but I fear you\n"
        "  won't survive the journey, because we will travel through\n"
        "  another sphere. Better ask when you are more experienced.\n"),0;
     case TP_IS_FIGHTING:
       return notify_fail(
         "Edi tells you: Hey, don't leave this thrilling fight.\n"),0;
    }
  if (!str = TP->QueryHome())
    {
      RestoreSP(60);
      return notify_fail(
        "Edi tells you: Whoops. You have no home! *comfort*\n"),0;
    }
  if (!newenv = load_object(str))
    {
      RestoreSP(60);
      return notify_fail(
        "Edi tells you: Oh, oh. Your home seems to be destroyed.\n"),0;
    }
  if (!env = environment(TP))
    {
      RestoreSP(60);
      return notify_fail(
        "Edi tells you: It's nice to be in the fabric of space.\n"),0;
    }
  if (env==newenv)
    {
      RestoreSP(60);
      return notify_fail(
        "Edi tells you: You seem to be a little bit confused. You ARE\n"
        "  at home.\n"),0;
    }

  if (!(env->QueryTPort()&TPORT_OUT))
    {
      RestoreSP(60);
      return notify_fail(
        "Edi tells you: Hm. I can't open the gate to the other sphere\n"
           "  in here.\n"),0;
    }

  return TP->move(newenv,M_SPECIAL,
    ({"enters a shimmering field which suddenly appeared in front of you",
      "leave a shimmering field which suddenly appeared in front of you",
      "see a shimmering field in the air and enter it. For some seconds\n"
      "everything is black, you feel a bit disoriented and after some time\n"
      "you step through another field and the place looks quite familiar\n"
      "to you.\n"
      "Edi tells you: Here we are"}));
}

public int cmd_hpmon(string str)
{
  int value;

  if (!TP) return 0;

  if (!str||str=="?"||str=="help")
    {
      write(wrap("Suddenly you hear a strange voice in your mind:\n"
		 "Hello, I'm Edi, your very personal hitpoint-monitor. "
		 "You don't know how you feel? Well, just ask me and I'll "
		 "tell it to you.\n"
		 "If you want me to tell you if your hitpoints or spellpoints "
		 "changed, just type 'hpmon on', or if you want me to be "
		 "silent again type 'hpmon off'. If I should just inform you "
		 "during combat about type 'hpmon combat'. To tell me to "
		 "return to the old display, just type 'hpmon combat' again. "
		 "You may also tell me not to tell you the HP and SP with "
		 "every change. Just give the delay by typing 'hpmon delay "
		 "<nr>', where 1 is the default.\n"
		 "I can also tell you how much experience you gained since "
		 "your last check. Just type 'expcheck' or 'ec' and I'll tell "
		 "it to you.\n"
		 "Another thing is, that I might tell you who's responsible "
		 "for the area you're in. Just type 'einfo'.\n"
		 "Finally I will help you talking to persons. Just define "
		 "your current talkpartners with 'partner <name(s)> just as "
		 "if you would list them after 'tell <name(s)>'. Then you "
		 "may use 'ptell <what-you-want-to-say>' to talk to your "
		 "partners. 'partner' will reveal your current talkpartners.\n"
		 "Well, ok. Don't want to disturb you any longer. Hope to "
		 "contact you soon again.\n"
		 "  Bye.\n",0,2));
      if (TP->QueryRealLevel()>=20)
        write(
          "Edi tells you: Oh, I forgot. As you have so many XP, you may\n"
          "  ask me to teleport you to your home. Just type 'go home'.\n");
      return 1;
    }
  str = lower_case(str);
  if (str=="on")
    if (state&STATE_ON)
      {
        write("Edi tells you: Hey, don't bother me. I already know that\n"
              "  I shall tell you your hitpoints and spellpoints.\n");
        return 1;
      }
    else
      {
        write("Edi tells you: Hi "+capitalize(getuid(TP))+"!\n"
              "  Ok, I'll tell you about how you feel.\n");
        state |= STATE_ON;
        set_heart_beat(1);
        return 1;
      }
  if (str=="off")
    if (!(state&STATE_ON))
      {
        write("You hear something snoring in your mind.\n"
              "Uuuuuuaaaaaaaaahhhhhh.\n"
              "Edi tells you: Hey, what's up? I had such a nice dream.\n"
              "  Oh, no. You just waked me up to tell me again, that I\n"
              "  should not tell you your status? Bah.\n"
              "Edi yawns and falls silent again.\n");
        return 1;
      }
    else
      {
        write("Edi tells you: Ah, time to have a little sleep. Thanx!\n");
        state &= ~STATE_ON;
        set_heart_beat(0);
        return 1;
      }
  if (str=="combat")
    if (!(state&STATE_COMBAT))
      {
        if (!state&STATE_ON)
          write("You hear something snoring in your mind.\n"
                "Uuuuuuaaaaaaaaahhhhhh.\n"
                "Edi tells you: Hey, what's up? A, I see. I should only\n"
                "  inform you about your status when you're fighting as soon\n"
                "  as you tell me to inform you about your status.\n");
        else
          write("Edi tells you: Ok, I'll now inform you only in combat.\n");
        state |= STATE_COMBAT;
        return 1;
      }
    else
      {
        if (!state&STATE_ON)
          write("You hear something snoring in your mind.\n"
                "Uuuuuuaaaaaaaaahhhhhh.\n"
                "Edi tells you: Hey, what's up? I had such a nice dream.\n"
                "  Oh, I see. Ok, I will inform you about each change of\n"
                "  HP and SP as soon as you turn me on again.\n");
        else
          write("Edi tells you: Ok. Here we go. I'll inform you about\n"
                "  every change.\n");
        state &= ~STATE_COMBAT;
        return 1;
      }
  if (sscanf(str,"delay %d",value))
    {
      if (!value)
        write("Edi tells you: Sorry, I need at least a rest of 1.\n");
      else
        {
          delay = maxdelay;
          write("Edi tells you: Ok. Delay changed from "+to_string(delay)+
                " to "+to_string(value)+".\n");
          maxdelay = delay = value;
          return 1;
        }
    }
  write("Edi tells you: What do you want me to do? Hey, don't talk\n"
        "  such stupid nonsense.\n");
  return 1;
}

protected object *get_enemies()
{
  object env;

  if ((!env = environment())||
      !query_once_interactive(env)||
      !environment(env))
    return ({});
  return filter(env->QueryEnemies()||({}),
    lambda(({SYM(v)}),
      ({SF(!=),
        ({SF(member),
          ({SF(all_inventory),
            ({SF(environment),env})
          }),
          SYM(v)
        }),
        -1
      }) ) );
}

protected string get_status(object e)
// Descriptions taken from /std/living/descriptions.c
{
  string str;
  if (!e) return 0;
  if (e->Query(P_REM_HP)==e->QueryHealth()) return 0;
  switch(e->QueryHealth())
    {
     case 0..3:
      str = " is next to Death";
      break;
     case  4..10:
      str = " is in very bad shape";
      break;
     case 11..15:
      str = "'s blood covers the whole ground";
      break;
     case 16..20:
      str = " is in bad shape";
      break;
     case 21..30:
      str = " is more dead than alive";
      break;
     case 31..40:
      str = "'s wounds pour blood everywhere";
      break;
     case 41..50:
      str = " is severely wounded";
      break;
     case 51..60:
      str = " has many wounds";
      break;
     case 61..70:
      str = " has some wounds";
      break;
     case 71..80:
      str = " is wounded";
      break;
     case 81..90:
      str = " is slightly hurt";
      break;
     case 91..99:
      str = " is in good shape";
      break;
     default:
      if (e->QueryHealth() < 0) str = " is living dead";
      else str = " is perfectly healthy";
    }
  e->Set(P_REM_HP,e->QueryHealth());
  return "  "+(e->QueryName()||"Someone")+str+".";
}

protected void display_stats()
{
  object pl;
  string *en_info;
  int newhp,newsp;

  if (!(pl = get_pl())) return;
  newhp = pl->QueryHP();
  newsp = pl->QuerySP();
  if (hp!=newhp || sp!=newsp)
    tell_object(pl,sprintf("Edi tells you: HP: %d (%d), SP: %d (%d)\n",
                           newhp,pl->QueryMaxHP(),newsp,pl->QueryMaxSP()));
  hp = newhp;
  sp = newsp;
  if (sizeof((en_info = map(get_enemies(),SF(get_status)))-({0})))
    {
      tell_object(pl,
        "Edi tells you: Well, let's see how your enemies look:\n");
      tell_object(pl,implode(en_info,"\n")+"\n");
    }
}

public string get_domain()
// Returns the domain someone is actually in.
{
  object env;
  string domain,h;

  if (   !(env = get_env())
      || sscanf(object_name(env),"/d/%s/%s",domain,h)!=2
     )
    return 0;
  return domain;
}

public string get_wizard()
// Returns the wizard who coded the environment of the player.
{
  object env;
  string domain,h,wizard;

  if (   !(env = get_env())
      || (   sscanf(object_name(env),"/players/%s/%s",wizard,h)!=2
          && sscanf(object_name(env),"/d/%s/%s/%s",domain,wizard,h)!=3
         )
     )
    return 0;
  return find_savefile(wizard)?wizard:0;
}

protected string get_state(string name,string domain)
{
  object ob;
  int gender;
  string *data;
  mixed state;

  ob=find_player(name);
  if(!ob = find_player(name))
    {
      data=explode(read_file(find_savefile(name)+".o"),"\n");
      gender=to_int((regexp(data,"^aGender")+({""}))[0][8..]);
    }
  else
    gender = ob->QueryGender();
  state = DOMAININFO->QueryWizard(name,domain);
  if (gender!=GENDER_FEMALE)
    switch(state)
      {
       case D_MEMBER:
        return "Member";
       case D_VICE:
        return "Vice Lord";
       case D_LORD:
        return "Lord";
      }
  else
    switch(state)
      {
       case D_MEMBER:
        return "Member";
       case D_VICE:
        return "Vice Lady";
       case D_LORD:
        return "Lady";
      }
  return "no member";
}

public mapping get_resp(string domain)
// Return the responsible persons for the area a player is in
{
  mixed f;
  mapping res,*wizards;
  int i;

  i = sizeof(wizards = DOMAININFO->QueryDomain(domain)[DD_WIZ][DW_NAME]);
  res = ([]);
  res[D_VICE] = ({});
  res[D_LORD] = ({});
  while(i--)
    {
      if (D_VICE==DOMAININFO->QueryWizard(wizards[i],domain))
        res[D_VICE] += ({wizards[i]});
      else if (D_LORD==DOMAININFO->QueryWizard(wizards[i],domain))
        res[D_LORD] += ({wizards[i]});
    }
  return res;
}

protected string get_list(string *w)
{
  return implode(map(w[0..<2],SF(capitalize)),", ")+" and "+
    capitalize(w[<1]);
}

public int cmd_einfo(string str)
// Informations for HLPs about their environment.
{
  string res,h;
  mapping resp;
  int lord,vlord;
  if (str||!TP)
    return notify_fail("Just type 'einfo'.\n",NOTIFY_ILL_ARG),0;
  if (!get_env())
    return notify_fail("You don't have a valid environment.\n",
                       NOTIFY_NOT_VALID),0;
  res = "";
  if (h = get_wizard())
    res+="The wizard who coded this room is "+capitalize(h)+".\n";
  if (h = get_domain())
    if (DOMAININFO->QueryState()!=DI_INITED)
      res+="There's more information available about this room,\n"
           "But currently I'm not able to get it. Try it again later.\n";
    else
      {
        resp=get_resp(h);
        if (lord = sizeof(resp[D_LORD]))
          if (lord==1)
            res+=get_state(resp[D_LORD][0],h)+" "+capitalize(resp[D_LORD][0]);
          else
            res+="The Lords/Ladies "+get_list(resp[D_LORD]);
        if (!(vlord = sizeof(resp[D_VICE])))
          if(!lord)
            res+="Noone";
          else;
        else
          {
            if (lord) res+=" and ";
            if (vlord==1)
              res+=get_state(resp[D_VICE][0],h)+" "+
                capitalize(resp[D_VICE][0]);
            else
              res+=(lord?"t":"T")+"he Vice Lords/Ladies "+
                get_list(resp[D_VICE]);
          }
        res+=((lord+vlord>1)?" are ":" is ")+
          "responsible for this area.\n";
      }
  if (res=="")
    res+="I don't know anything about this area.\n";
  else
    res = "\n"+res;

  write("Edi tells you: "+wrap("  "+res,0,2));
  return 1;
}

protected void domain_chat()
{
  string comment;

  switch(get_domain())
    {
      case "archwiz":
        comment = "Oh, you are at the source of Nightfall. "
                  "You should know that this area was all, the old "
                  "Nightfall consisted of. "
                  "It feels good to be here again.\n";
        break;
      case "atlantis":
        comment = "Hm, strange area here. Might this be a part of the "
                  "sunken Atlantis I heard from in so many stories?\n";
        break;
      case "avalon":
        comment = "Hey, what about visiting King Arthur? I would like to "
                  "see his wife Genevre.\n";
        break;
      case "daydream":
        comment = "Your dream is really nice. Thanks that I may take part "
                  "in it :-)\n";
        break;
      case "drannor":
        comment = "A mysterious area, that is.\n";
        break;
      case "halfling":
        comment = "Oh, wow. Didn't know that there are so many small people "
                  "around here.\n";
        break;
      case "hell":
        comment = "I have such an evil feeling in here. Perhaps we should "
                  "leave this place.\n";
        break;
      case "ireland":
        comment = "Would you buy me an Irish Coffee? Thanks.\n";
        break;
      case "orient":
        comment = "The land where the elephants live ... It's quite hot here "
                  "but somehow I like this area.\n";
        break;
      case "shadows":
        comment = "Wasn't there just a shadow behind you? Oh no. Think I was "
                  "wrong. Sorry, this area is really confusing and it "
                  "looks quite dangerous.\n";
        break;
      case "valhalla":
        comment = "Oh, the land where the old gods live. Perhaps we'll meet "
                  "Thor here. I would like to see his hammer.\n";
        break;
       case "vecna":
         comment = "I would suggest you better leave this area. I wouldn't "
                   "like to see you dying.\n";
         break;
       case "wasteland":
         comment = "Phew, what an odeur ... This must be the place where "
                   "a big explosion took place years ago. You can still "
                   "smell it.\n";
         break;
       case "woodland":
         comment = "What a nice area. Where the elves and half-elves life. "
                   "Did you know that elves have very nice and beautiful "
                   "voices?\n";
         break;
       default:
         comment = "This is a strange area. I didn't know that.\n";
    }
  if (get_pl())
    tell_object(get_pl(),wrap("Edi tells you: "+comment,0,2));
}

protected void nightday_chat()
{
  object pl,env;
  string comment,domain,h;

  if (!pl = get_pl()) return;
  if (!env = get_env()) return;
  if (env->QueryIndoors()) return;
  switch(env->QueryDayState())
    {
     case ND_NIGHT:
      comment = "Oh, what a cosy night. Let's watch the moon. "
                "Do you see the beautiful stars at the sky?\n";
      break;
     case ND_DAWN:
      comment = "Uuuaaaah. I feel that this will be a wonderful day.\n";
      break;
     case ND_SUNRISE:
      comment = "See how the sun is rising at the horizon. "
                "Isn't that beautiful?\n";
      break;
     case ND_PRENOON:
      comment = "I'm getting hungry. Let's search for something to it for "
                "lunch.\n";
      break;
     case ND_DAY:
      comment = "The sun is quite bright today.\n";
      break;
     case ND_POSTNOON:
      comment = "What about an after-dinner nap?\n";
      break;
     case ND_FALL:
      comment = "I always like to see how the sun sinks into the ocean.\n";
      break;
     case ND_PRENIGHT:
      comment = "Think it's time to get some sleep.\n";
    }
  if (!comment) return;
  tell_object(pl,wrap("Edi tells you: "+comment,0,2));
}

protected void stat_chat()
{
  object pl;
  string comment;
  int i;

  if (!(pl = get_pl())) return;

  switch(random(3))
    {
     case 0:
      if (pl->QueryGhost())
        comment = "You look interesting as ghost.\n";
      else if (pl->QueryHP()<10)
        comment = "Hey, "+pl->QueryName()+" be careful! "
                  " You're facing death.\n";
      else if (pl->QueryHP()==pl->QueryMaxHP())
        comment = "You really look good today.\n";
      break;
     case 1:
      if (!i = pl->QueryWimpy())
        comment = "Hey, you're a real hero!\n";
      else if (i>(pl->QueryMaxHP()/100)*75)
        comment = "Bah, you're a whimp!\n";
      break;
     case 2:
      i = sizeof((pl->QueryEnemies()||({}))
                -(pl->QueryHunters()||({}))
                +(pl->QueryHunters()||({})) );
      if (!i)
        comment = "You have a peaceful life. Noone who's hunting for you.\n";
      else if (i>10)
        comment = "Uh, oh. You have a lot of enemies. Don't you think "
                  "you should choose a more peaceful life?\n";
      break;
    }
  if (!comment) return;
  tell_object(pl,wrap("Edi tells you: "+comment,0,2));
}

protected void quest_chat()
{
  object pl;
  string comment,*quests;
  int i;

  if (!(pl = environment())||!interactive(pl)) return;

  if (!comment = pl->QueryQuests()) return;
  quests = explode(comment,"#");
  comment = 0;
  switch(quests[random(sizeof(quests))])
    {
     case "LAG-Monster":
       comment = "Ah, you slayed the evil LAG-Monster. Great!\n";
       break;
     case "bear_claw":
        comment = "You fought against a strong bear? "
                  "You seem to be a good fighter.\n";
        break;
     case "deerquest":
        comment = "The deer with the cross is nice, isn't it?\n";
        break;
     case "insulin_quest":
        comment = "You take care of other people I see, as for example "
                  "the ferryman who needed his insulin.\n";
        break;
     case "go_problem":
        comment = "You solved some go-problems? Perhaps you can help me "
                  "with this one:\n"
                  "     9|..........\n"
                  "     8|..........\n"
                  "     7|..OOOOOO..\n"
                  "     6|..O@@@@O..\n"
                  "     5|.O@...@O..\n"
                  "     4|.O@...@O..\n"
                  "     3|.O@@@@O...\n"
                  "     2|.O.OOOO...\n"
                  "     1|..........\n"
                  "      +----------\n"
                  "       abcdefghik\n"
                  "      Next move: O\n";
        break;
     case "erniequest":
        comment = "You helped the bard-guildmaster to retrieve the lost "
                  "composition of Ernie? If you meet Viola next time "
                  "tell her greetings from me.\n";
        break;
     case "clean_phishs_castle":
        comment = "I see you're experienced using a broom. Perhaps you "
                  "should use it to clean your brain a little bit.\n";
        break;
     case "orc_slayer":
        comment = "This orc-shaman is really evil, isn't he?\n";
        break;
     case "dying_beggar":
        comment = "You have a heart for the poor and weak. I like that!\n";
        break;
     case "bounty_quest":
        comment = "Fletcher Christian is a poor zombie, isn't he?\n";
        break;
     case "find_kirk":
        comment = "Uh, I see strange pictures in your brain about a big "
                  "grey something floating in the air. It has a circle "
                  "at one side and ends in two ... hm, pencils? "
                  "Oh, there are some words on it: USS Enterprise. "
                  "Strange thing that is.\n";
        break;
     case "feed_ice_monster":
        comment = "You fed the ice monster? Well, perhaps you met Vanilla "
                  "Ice, did you?\n";
        break;
     case "greet King Arthur":
        comment = "You seem to know very important persons, like King Arthur.\n";
        break;
     case "choking_cok":
        comment = "Bah, you helped this damn cock. I was glad that he "
                  "was so silent all the time.\n";
        break;
     case "desp_wanderer":
        comment = "You know how to find your way through Lake Mosken? "
                  "Perhaps you can help me: I need the way from the "
                  "castle made of sand to the place where you can find "
                  "this beautiful stone.\n";
        break;
     case "lighthouse":
        comment = "How boring. Now no ship will drive towards the cliff "
                  "just because you lighted the beacon.\n";
        break;
     case "25th_anniversary":
        comment = "You visited 'Bean's Beautiful Bones'? Did you already "
                  "buy the skull-candleholder there? That thing is really "
                  "nice. You can even darken a room with it.\n";
        break;
     case "alwis_asks":
        comment = "Helping this stupid Alwis? Hey, come on, tell me, why "
                  "doesn't he go around and searches the answers by "
                  "himself? I but he's just to lazy to do that.\n";
        break;
     case "dreamquest":
        comment = "Ah, now I know why I have my nice dreams back. Thanks!\n";
        break;
     case "Alice":
        comment = "You freed this cute girl Alice? That's nice.\n";
        break;
     case "sculpturesort":
        comment = "How did you like the statue of Deepthought in the museum?\n";
        break;
     case "postquest":
        comment = "Ugh, you delivered this stupid message for King Arthur? "
                  "I hate codes like: ?firfub baaartiku cest\n";
        break;
     case "electric_ladyland":
        comment = "You saw The Who in Electric Ladyland? Could you do me a "
                  "favour? I want to have their autographes.\n";
        break;
     case "circus":
        comment = "You know Monthy Pythons Flying Circus? *grin* "
                  " Well, and now something completely different.\n";
        break;
     case "dragonslayer":
        comment = "Wow, you survived a contact with a wild dragon? "
                  "Great job.\n";
        break;
     case "elementals":
        comment = "Oh, I have a lack of memory. What are the four elementals?\n";
        break;
     case "become_a_knight":
        if (pl->QueryGender()==MALE)
          comment = "Oh, a real knight! My respect, Sir!\n";
        else
          comment = "Oh, a real knight! My respect, Mylady!\n";
        break;
     case "destruct_creature":
        comment = "Great that you destructed this strange creature\n";
        break;
    }
  if (!comment) return;
  tell_object(pl,wrap("Edi tells you: "+comment,0,2));
}

protected void check_chat()
{
  switch(random(4))
    {
     case 0: domain_chat(); break;
     case 1: nightday_chat(); break;
     case 2: stat_chat(); break;
     case 3: quest_chat(); break;
    }
}


public int cmd_expcheck()
{
  object pl;
  int diff;

  if (!(pl = get_pl())) return 0;
  diff = pl->QueryXP()-exp;
  exp = pl->QueryXP();

  if (!diff)
    write("Edi tells you: Bah, that's boring. Nothing new here.\n");
  else if (diff<0)
    write("Edi tells you: Uh, you look less experienced today.\n"
          "  You lost "+to_string(diff*-1)+" XP.\n");
  else if (diff>0)
    write("Edi tells you: Wow, seems you learned a lot.\n"
          "  You have now "+to_string(diff)+" XP more than before.\n");
  return 1;
}

public void heart_beat()
{
  object pl;

  if (!pl = get_pl()) return;

  if (!--delay)
    {
      delay = maxdelay;
      if (!(state&STATE_COMBAT))
        display_stats();
      else
        if (sizeof((pl->QueryEnemies()||({}))-({0})))
          display_stats();
    }
  if (!sizeof((pl->QueryEnemies()||({}))-({0})))
    if (!random(70)) check_chat();
}

public void PlayerNetdead(int i)
{
  if (i)
    set_heart_beat(0);
  else
    if (state&STATE_ON)
      set_heart_beat(1);
}
