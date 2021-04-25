/* the voting room
 * (c) 1993/12/13 - 1994-01-26 by taube
 */

/* TODO: wherever you find ___
 */

/* Concept:
 * How voting is done with this object:
 *
 * This object claims to be a table. Place it in a room it fits into.
 * It may not be taken nor be moved.
 *
 * If you have a subject you want to be voted on and you are a wizard,
 * you may type 'add subject'.
 * After this you are asked for a title (on line) and a description of
 * the topic the vote is on.
 *
 * Whenever this is done, every wizard (you too) may propose alternatives.
 * You do this by typing 'add alternative #', where '#' stands for the
 * number of the subject you want to add an alternative to.
 * After this you are asked for a short text.
 *
 * But you may not only add alternatives, you may also give a vote.
 * During this state votes are also short texts that state your opinion.
 * Type 'vote #' to vote on the subject '#'.
 * Everybody may vote, and he/she may do so as often as he/she wants to.
 *
 * I think that this is the normal way of voting. Of course, there is also
 * a possibility for free, secret, equal and democratic voting.
 * If you want to start a secret voting, there has to be at least one
 * alternative, although this seems to me to be quite nonsense.
 * Then you type 'secretize #', where '#' stands for the subject. Remember
 * that only the creator of a subject may change to secret voting.
 * And there is another thing: Adding of alternatives is not allowed when
 * secret voting has begun.
 * During secret voting, everybody may vote ONLY ONCE. You vote by entering
 * the number of the preferred alternative. Your input is hidden and cannot
 * be snooped. There is no possibility to trace back one's vote.
 *
 * If you changed to secret voting by mistake, and there are no secret
 * votes yet, you may go back to public voting by typing 'unsecretize #'.
 * '#' stands for the desired subject.
 *
 * After all, you may remove a subject by typing 'remove #', where '#'
 * stands for the subject you want to remove. Only the creator of a subject
 * or an archwiz may remove it. A backup protocoll of the voting is written
 * to disk.
 *
 * The 'show' commands are self-explanatory, I think.
 */



inherit "/std/room";
inherit "/obj/lib/string";

#include "path.h"
#include <config.h>
#include <moving.h>
#include <properties.h>
#include <rooms.h>
#include <secure/wizlevels.h>
#include <npc.h>
#include "/obj/lib/string.h"
#include "voting.h"


static object player;        /* the player that is in this room */
static object origin;        /* the room the player came from */

static int    subject;       /* temporary variable for input_to() */
static int    alternative;   /* temporary variable for input_to() */

static string buffer;        /* output buffer for smore() */

mixed         votes;

/* 'votes' is a list of structures for each subject:                      */
#define TITLE 0   /* str  Title of the subject                            */
#define TEXT  1   /* str  The subject's description                       */
#define ALT   2   /* mix* An array with the alternatives                  */
#define VOTES 3   /* mix* The non-secret votes                            */
#define SECR  4   /* int  Flag for secretization (0=pub, 1=secret votes)  */
#define TGUE  5   /* int  Total number of guests' votes on this subject   */
#define TPLA  6   /* int  Total number of players' votes on this subject  */
#define TWIZ  7   /* int  Total number of wizards' votes on this subject  */
#define VERN  8   /* str* The players' encrypted real names that voted    */
#define NAME  9   /* str  Name of the creator of this subject             */
#define DATE 10   /* str  Date and time of the creation of this subject   */

/* alternatives and votes */
#define TEXTS 0   /* str  The alternative or vote                         */
#define NAMES 1   /* str  Name of the creator                             */
#define DATES 2   /* str  Date of creation                                */

/* alternatives */
#define GUE   3   /* int  Number of secret votes from guests              */
#define PLA   4   /* int  Number of secret votes from players             */
#define WIZ   5   /* int  Number of secret votes from wizards             */



LeaveTable()
{
  write("You stand up.\n");
  if(!origin) {
    write("\
Argl! The room around you has vanished... you are transferred somwhere...\n");
    call_other(VOID, "???");
    origin = find_object(VOID);
  }
  tell_room(origin,
	    this_player()->QueryName()+" stands up from the voting table.\n");
  this_player()->move(origin, M_SILENT);
  write(this_player()->LookRoom(SENSE_MOVE|SENSE_SILENT));
   return 1;
}

PlayerNetdead(int arg)
{
  if (!origin)
  {
    call_other(VOID, "???");
    origin = find_object(VOID);
  }
  this_player()->move(origin, M_TELEPORT);
}

create()
{
  (::create());
  seteuid(getuid());	/* thanks, Organimedes */
  SetIntShort("the voting table");
  AddExit("exit", #'LeaveTable);
  SetIndoors(1);
  SetBright(MAX_SUNBRIGHT);
  Set(P_TPORT, TPORT_NO);
  restore();      /* load subjects */
  subject = -1;   /* no player is busy with the table */
  player  = 0;    /* no player in here */
}



init()
{
  (::init());
  add_action("_show", "show");
  add_action("_add", "add");
  add_action("_secretize", "secretize");
  add_action("_unsecretize", "unsecretize");
  add_action("_vote", "vote");
  add_action("_remove", "remove");
}



allow_enter(mixed extra)
{
  if(!living(previous_object()) || !(previous_object()->QueryIsPlayer()))
    return ME_NO_ENTER;
  if(player)
    return ME_NO_ENTER;
  else
    return ME_OK;
}



notify_enter(object extra)
{
  player = previous_object();
  origin = extra;
}



notify_leave()
{
  player = 0;
}



QueryIntLong()
{
  return "\
You are sitting at the voting table. You see lots of papers with\n\
important notes lying around. Commands are:\n\
\tshow sub[jects]              Show the titles of all subjects\n\
\tshow sub[ject] #             Show the subject #\n\
\tshow inf[o] [#]              Show some information [about subject #]\n\
\tshow alt[ernatives] #        Show the alternatives to subject #\n\
\tshow vot[es] #               Show the non-secret votes on subject #\n\
\tadd sub[ject]                Add a new subject to take a vote on\n\
\tadd alt[ernative] #          Add a new alternative to subject #\n\
\tsecretize #                  Start secret voting on subject #\n\
\tunsecretize #                Go back to public voting on subject #\n\
\tvote #                       Vote on subject #\n\
\tremove sub[ject] #           Remove subject #\n\
\tremove alt[ernative] #1 #2   Remove alternative #2 from subject #1\n\
\tremove vot[e] #a #b          Remove vote #a from subject #b\n\
\n\
\texit                         leave the table.\n\
\n\t\tdon't remain silent - vote for your rights!\n";
}



QueryOccupied()
{
  return player!=0;
}



restore()
{
  string file, filename;
  
  if(!restore_object(SAVEFILE)) {
    /* if something went wrong - save the old save file */
    file = read_file(SAVEFILE+".o");
    if(file) {
      do {
	filename = SAVEFILE+"."+random(1000000);
      } while(file_size(filename)!=-1);
      rm(filename);
      write_file(filename, file);
    }
    votes = ({});
    save();
  }
}



save()
{
  catch(rename(SAVEFILE+".o", SAVEFILE+".o.old"));
  save_object(SAVEFILE);
}



_show(string str)
{
  int    sub, alt;
  string s;
  
  if(!str) {
    notify_fail("'show' needs an argument.\n");
    return 0;
  }
  switch(str[0..2]) {
    
  case "sub":   /* Show the titles of all subjects */
    if(sscanf(str, "%s %d", s, sub) != 2) {
      if(!sizeof(votes)) {
        write("Currently there are no subjects to vote on.\n");
        return 1;
      }
      write("You may vote on the following subjects:\n" +
	    "=======================================\n");
      for(sub=0; sub<sizeof(votes); sub++)
	write((sub+1) + "\t: " + votes[sub][TITLE] + "\n");
      
    } else {   /* Show the subject # */
      sub--;
      if(sub<0 || sub>=sizeof(votes)) {
        write("There is no subject with this number.\n");
        return 1;
      }
      buffer = (sub+1) + ": " + votes[sub][TITLE] + "\n" +
	"========================================\n" +
	  votes[sub][TEXT];
      smore(buffer);
      buffer = "";
    }      
    return 1;
    
  case "inf":   /* Show some information */
    if(sscanf(str, "%s %d", s, sub) < 2) {
      write("Currently there are " + sizeof(votes) + " subjects.\n");
      
    } else {   /* Show some information about subject # */
      sub--;
      if(sub<0 || sub>=sizeof(votes)) {
        write("There is no subject with this number.\n");
        return 1;
      }
      write("Information to subject " + (sub+1) + ":\n" +
            votes[sub][TITLE] + "\n" +
            "========================================\n" +
            "This subject was put to the vote by " +
            capitalize(votes[sub][NAME]) + " on " + votes[sub][DATE] + ".\n" +
            "There are " + sizeof(votes[sub][ALT]) +
	    " alternatives available.\n" +
            sizeof(votes[sub][VOTES]) + " players voted publicly.\n" +
	    "The voting is " + (votes[sub][SECR] ? "secret" : "public") +
	    ".\n");
      if(votes[sub][SECR]) {
        write("\nSecret votes:\n" +
              "Altern.\tGuests\tPlayers\tWizards\n" +
              "--------------------------------\n");
        for(alt=0; alt<sizeof(votes[sub][ALT]); alt++)
          write((alt+1) +
                "\t" + votes[sub][ALT][alt][GUE] +
                "\t" + votes[sub][ALT][alt][PLA] +
                "\t" + votes[sub][ALT][alt][WIZ] + "\n");
	write("================================\n" +
              "Total:\t" + votes[sub][TGUE] + "\t" + votes[sub][TGUE] +
              "\t" + votes[sub][TGUE] + "\n");
      }
    }
    return 1;
    
  case "alt":   /* Show the alternatives to subject # */
    if(sscanf(str, "%s %d", s, sub)<2 || sub<1 || sub>sizeof(votes)) {
      write("There is no subject with this number.\n");
      return 1;
    }
    sub--;
    buffer = "Alternatives to subject " + (sub+1) + ":\n" +
      votes[sub][TITLE] + "\n" +
	"========================================\n";
    for(alt=0; alt<sizeof(votes[sub][ALT]); alt++)
      buffer += "\nAlternative " + (alt+1) + " from " +
	capitalize(votes[sub][ALT][alt][NAMES]) + " on " +
	  votes[sub][ALT][alt][DATES] + ":\n" +
	    votes[sub][ALT][alt][TEXTS];
    smore(buffer);
    buffer = "";
    return 1;
    
  case "vot":   /* Show the non-secret votes on subject # */
    if(sscanf(str, "%s %d", s, sub)<2 || sub<1 || sub>sizeof(votes)) {
      write("There is no subject with this number.\n");
      return 1;
    }
    sub--;
    buffer = "Votes on subject " + (sub+1) + ":\n" +
      votes[sub][TITLE] + "\n" +
	"========================================\n";
    for(alt=0; alt<sizeof(votes[sub][VOTES]); alt++)
      buffer += "\nVote " + (alt+1) + " from " +
	capitalize(votes[sub][VOTES][alt][NAMES]) + " on " +
	  votes[sub][VOTES][alt][DATES] + ":\n" +
	    votes[sub][VOTES][alt][TEXTS];
    smore(buffer);
    buffer = "";
    return 1;
    
  default:
    notify_fail("Wrong syntax for 'show'.\n");
  }
  return 0;
}



_add(string str)
{
  int    sub, alt;
  string s;
  
  if(!str) {
    notify_fail("'add' needs an argument.\n");
    return 0;
  }
  switch(str[0..2]) {
    
  case "sub":   /* Add a new subject to take a vote on */
    if(!IS_WIZARD(this_player())) {
      write("Sorry! Only wizards are allowed to do that.\n" +
            "Please ask a wizard to help you.\n");
      return 1;
    }
    if(subject!=-1) {
      write("Sorry! The table is currently used by another player - please wait.\n");
      return 1;
    }
    subject = sizeof(votes);   /* occupy this object */
    write("You want to add a new subject to take a vote on.\n" +
          "Now type in the title (one line). Type '~q' to abort.\n" +
          "] ");
    input_to("add_subject_title");
    return 1;
    
  case "alt":   /* Add a new alternative to subject # */
    if(sscanf(str, "%s %d", s, sub)<2 || sub<1 || sub>sizeof(votes)) {
      write("There is no subject with this number.\n");
      return 1;
    }
    if(!IS_WIZARD(this_player())) {
      write("Sorry! Only wizards are allowed to do that.\n" +
            "Please ask a wizard to help you.\n");
      return 1;
    }
    sub--;
    if(votes[sub][SECR]) {
      write("Sorry! This subject is already set to secret voting.\n" +
            "No more new alternatives are allowed.\n");
      return 1;
    }
    if(subject!=-1) {
      write("Sorry! The table is currently used by another player - please wait.\n");
      return 1;
    }
    subject = sub;   /* occupy this object */
    alternative = sizeof(votes[sub][ALT]);
    write("You want to add a new alternative to subject " + (sub+1) + ":\n" +
          votes[sub][TITLE] + "\n" +
          "Now type. Use '.' to finish, '~q' to abort.\n" +
          "] ");
    votes[sub][ALT] += ({({"", this_player()->QueryRealName(), ctime(time()),
			     0, 0, 0})});
    input_to("add_alternative");
    return 1;
    
  default:
    notify_fail("Wrong syntax for 'add'.\n");
  }
  return 0;
}



_secretize(string str)   /* Start secret voting on subject # */
{
  int sub;
  
  if(!str || !sscanf(str, "%d", sub)) {
    notify_fail("'secretize' needs a number as argument.\n");
    return 0;
  }
  sub--;
  if(sub<0 || sub>=sizeof(votes)) {
    write("There is no subject with this number.\n");
    return 1;
  }
  if(this_player()->QueryRealName() != votes[sub][NAME] &&
     !IS_WIZARD(this_player())) {
    write("Sorry! Only the creator of a subject or an archwiz may change\nto secret voting.\n.");
    return 1;
  }
  if(!sizeof(votes[sub][ALT])) {
    write("Sorry! There are no alternatives to vote on.\n");
    return 1;
  }
  if(votes[sub][SECR]) {
    write("Subject " + (sub+1) + " is already set to secret voting.\n");
    return 1;
  }
    if(subject!=-1) {
      write("Sorry! The table is currently used by another player - please wait.\n");
      return 1;
    }
  votes[sub][SECR] = 1;
  save();
  write("Subject " + (sub+1) + " set to secret voting.\n");
  return 1;
}



_unsecretize(string str)   /* Go back to public voting subject # */
{
  int sub;
  
  if(!str || !sscanf(str, "%d", sub)) {
    notify_fail("'unsecretize' needs a number as argument.\n");
    return 0;
  }
  sub--;
  if(sub<0 || sub>=sizeof(votes)) {
    write("There is no subject with this number.\n");
    return 1;
  }
  if(this_player()->QueryRealName() != votes[sub][NAME] &&
     !IS_WIZARD(this_player())) {
    write("Sorry! Only the creator of a subject or an archwizard may change\nto public voting.\n.");
    return 1;
  }
  if(!votes[sub][SECR]) {
    write("Subject " + (sub+1) + " is still set to public voting.\n");
    return 1;
  }
  if(subject!=-1) {
    write("Sorry! The table is currently used by another player - please wait.\n");
    return 1;
  }
  subject = sub;
  if(votes[sub][TGUE] || votes[sub][TPLA] || votes[sub][TWIZ]) {
    write("There are already secret votes.\n" +
	  "They will be deleted if you go back to public voting.\n");
  }
  write("If you want to go back to public voting, enter now 'unsecretize':\n" +
	">>> ");
  input_to("unsecretize_subject");
  return 1;
}



_vote(string str)   /* Vote on subject # */
{
  int sub;
  
  if(!str || !sscanf(str, "%d", sub)) {
    notify_fail("'vote' needs a number as argument.\n");
    return 0;
  }
  sub--;
  if(sub<0 || sub>=sizeof(votes)) {
    write("There is no subject with this number.\n");
    return 1;
  }
  if(subject!=-1) {
    write("Sorry! The table is currently used by another player - please wait.\n");
    return 1;
  }
  write("You want to vote on subject " + (sub+1) + ":\n" +
        votes[sub][TITLE] + ".\n");
  if(votes[sub][SECR]) {   /* secret voting */
    write("This subject is set to secret voting.\n");
    if(!sizeof(votes[sub][ALT])) {
      write("Sorry! There are no alternatives to vote on.\n");
      return 1;
    }
    if(getuid(this_player())!="guest" &&
       member(SEED, crypt(this_player()->QueryRealName()),
		    votes[sub][VERN]) != -1) {
      write("Sorry! You already voted on this subject.\nEverybody may only vote once.\n");
      return 1;
    }
    write("Please enter the number of the alternative you vote for.\n" +
          "Enter nothing if you don't want to vote.\n" +
          "Your input will be hidden.\n" +
          ">>> ");
    subject = sub;
    alternative = -1;
    input_to("secret_vote", 1);   /* hidden input */
  } else {   /* public voting */
    write("This subject is set to public voting.\n" +
          "Please enter some lines to state your opinion.\n" +
          "Now Type. Use '.' to finish or '~q' to abort.\n" +
          "] ");
    subject = sub;
    alternative = sizeof(votes[subject][VOTES]);
    votes[sub][VOTES] += ({({"", this_player()->QueryRealName(),
			       ctime(time())})});
    input_to("public_vote");
  }
  return 1;
}



_remove(string str)
{
  int    sub, alt;
  string s;
  
  switch(str[0..2]) {
    
  case "sub":   /* Remove subject # */
    if(sscanf(str, "%s %d", s, sub)!=2) {
      notify_fail("'remove subject' needs a number as argument.\n");
      return 0;
    }
    sub--;
    if(sub<0 || sub>=sizeof(votes)) {
      write("There is no subject with this number.\n");
      return 1;
    }
    if((this_player()->QueryRealName() != votes[sub][NAME]) &&
       !IS_WIZARD(this_player())) {
      write("Sorry! Only the creator of a subject or an archwizard may remove it.\n.");
      return 1;
    }
    if(subject!=-1) {
      write("Sorry! The table is currently used by another player - please wait.\n");
      return 1;
    }
    subject = sub;
    write("You want to remove the subject " + (sub+1) + ":\n" +
	  votes[sub][TITLE] + "\n========================================\n" +
	  votes[sub][TEXT] + "\n----------------------------------------\n\n" +
	  "If you REALLY want to remove this subject, then enter now 'remove'.\n" +
	  "Did you note the result of the voting? It will be lost!\n" +
	  ">>> ");
    input_to("remove_subject");
    return 1;
    
  case "alt":   /* Remove alternative # from subject # */
    if(sscanf(str, "%s %d %d", s, sub, alt)!=3) {
      notify_fail("'remove alternative' needs two numbers as arguments.\n");
      return 0;
    }
    sub--;
    if(sub<0 || sub>=sizeof(votes)) {
      write("There is no subject with this number.\n");
      return 1;
    }
    alt--;
    if(alt<0 || alt>=sizeof(votes[sub][ALT])) {
      write("There is no alternative with this number.\n");
      return 1;
    }
    if(votes[sub][SECR]) {
      write("Sorry! This subject is set to secret voting.\nYou may not remove any alternatives.\n");
      return 1;
    }
    if((this_player()->QueryRealName() != votes[sub][ALT][alt][NAMES]) &&
       !IS_WIZARD(this_player())) {
      write("Sorry! Only the creator of an alternative or an archwizard may remove it.\n.");
      return 1;
    }
    if(subject!=-1) {
      write("Sorry! The table is currently used by another player - please wait.\n");
      return 1;
    }
    subject = sub;
    alternative = alt;
    write("You want to remove alternative " + (alt+1) + " from subject " +
	  (sub+1) + ":\n" +
	  votes[sub][TITLE] + "\n========================================\n" +
	  votes[sub][ALT][alt][TEXTS] +
	  "\n----------------------------------------\n\n" +
	  "If you REALLY want to remove this alternative, then enter now 'remove'.\n" +
	  ">>> ");
    input_to("remove_alternative");
    return 1;
    
  case "vot":
    if(sscanf(str, "%s %d %d", s, sub, alt)!=3) {
      notify_fail("'remove vote' needs two numbers as arguments.\n");
      return 0;
    }
    sub--;
    if(sub<0 || sub>=sizeof(votes)) {
      write("There is no subject with this number.\n");
      return 1;
    }
    alt--;
    if(alt<0 || alt>=sizeof(votes[sub][VOTES])) {
      write("There is no vote with this number.\n");
      return 1;
    }
    if(votes[sub][SECR]) {
      write("Sorry! This subject is set to secret voting.\nYou may not remove any votes.\n");
      return 1;
    }
    if(this_player()->QueryRealName() != votes[sub][VOTES][alt][NAMES] &&
       !IS_WIZARD(this_player())) {
      write("Sorry! Only the creator of a vote or an archwizard may remove it.\n.");
      return 1;
    }
    if(subject!=-1) {
      write("Sorry! The table is currently used by another player - please wait.\n");
      return 1;
    }
    subject = sub;
    alternative = alt;
    write("You want to remove vote " + (alt+1) + " from subject " +
	  (sub+1) + ":\n" +
	  votes[sub][TITLE] + "\n========================================\n" +
	  votes[sub][VOTES][alt][TEXTS] +
	  "\n----------------------------------------\n\n" +
	  "If you REALLY want to remove this vote, then enter now 'remove'.\n" +
	  ">>> ");
    input_to("remove_vote");
    return 1;
    
  default:
    notify_fail("Wrong syntax for 'remove'.\n");
  }
  return 0;
}



add_subject_title(string str)
{
  if(!str || str=="" || str=="." || str=="~q") {
    write("You decide not to add a new subject.\n");
    subject = -1;
    return 1;
  }
  votes += ({({str, "", ({}), ({}), 0, 0, 0, 0, ({}),
		 this_player()->QueryRealName(), ctime(time())})});
  write("You want to add a new subject with the title\n" + str + "\n" +
        "Now enter the subject (some lines). Use '.' to finish or '~q' to abort.\n" +
        "] ");
  input_to("add_subject");
}



add_subject(string str)
{
  if(!str) str = "";
  if(str == ".") {
    write("You added a new subject. It's number is " + (subject+1) + ".\n");
    save();
    subject = -1;
    return;
  } else if(str=="~q") {
    write("You aborted adding a new subject.\n");
    votes = votes[0..subject-1];
    subject = -1;
    return;
  } else {
    votes[subject][TEXT] += str + "\n";
    write("] ");
    input_to("add_subject");
  }
}



add_alternative(string str)
{
  if(!str) str = "";
  if(str == ".") {
    write("You added a new alternative to subject " + (subject+1) + ".\n");
    save();
    subject = -1;
    return;
  } else if(str=="~q") {
    write("You aborted adding a new alternative.\n");
    votes[subject][ALT] = votes[subject][ALT][0..alternative-1];
    subject = -1;
    return;
  } else {
    votes[subject][ALT][alternative][TEXTS] += str + "\n";
    write("] ");
    input_to("add_alternative");
  }
}



unsecretize_subject(string str)
{
  int sub, alt;
  
  if(str != "unsecretize") {
    write("You decide not to go back to public voting.\n");
    subject =-1;
    return;
  }
  sub = subject;
  votes[sub][SECR] = 0;
  for(alt=0; alt<sizeof(votes[sub][ALT]); alt++)
    votes[sub][ALT][alt][GUE] = votes[sub][ALT][alt][PLA] =
      votes[sub][ALT][alt][WIZ] = 0;
  votes[sub][TGUE] = votes[sub][PLA] = votes[sub][TWIZ] = 0;
  save();
  write("Subject " + (sub+1) + " set to public voting.\n");
  subject = -1;
  return 1;
}



secret_vote(string str)
{
  int alt;
  
  write("*** voted ***\n");
  if(!str) str="";
  if(!sscanf(str, "%d", alt)) {
    write("Your vote is illegal. It will not be counted. But you may vote again.\n");
    subject = -1;
    return;
  }
  alt--;
  if(alt<0 || alt>=sizeof(votes[subject][ALT])) {
    write("There are not as many alternatives. Your vote is illegal.\n" +
          "It will not be counted. But you may vote again.\n");
    subject = -1;
    return;
  }
  if(alternative==-1) {
    write("Please re-enter the number of the alternative you vote for.\n" +
          "Enter nothing if you want to abort voting.\n" +
          ">>> ");
    alternative = alt;
    input_to("secret_vote", 1);   /* hidden input */
    return;
  }
  if(alt!=alternative) {
    write("You entered two different numbers. Your vote is illegal.\n" +
          "It will not be counted. But you may vote again.\n");
    subject = -1;
    return;
  }
  if(getuid(this_player())=="guest") {
    votes[subject][ALT][alt][GUE]++;
    votes[subject][TGUE]++;
  } else {
    votes[subject][VERN] += ({crypt(this_player()->QueryRealName(), SEED)});
    if(IS_WIZARD(this_player())) {
      votes[subject][ALT][alt][WIZ]++;
      votes[subject][TWIZ]++;
    } else {
      votes[subject][ALT][alt][PLA]++;
      votes[subject][TPLA]++;
    }
  }
  save();
  subject = -1;
}



public_vote(string str)
{
  if(!str) str = "";
  if(str == ".") {
    write("You added a new public vote to subject " + (subject+1) + ".\n");
    save();
    subject = -1;
    return;
  } else if(str=="~q") {
    write("You aborted adding a new vote.\n");
    votes[subject][VOTES] = votes[subject][VOTES][0..alternative-1];
    subject = -1;
    return;
  } else {
    votes[subject][VOTES][alternative][TEXTS] += str + "\n";
    write("] ");
    input_to("public_vote");
  }
}



remove_subject(string str)
{
  string text;
  int    alt;
  
  if(str != "remove") {
    write("You decide not to remove this subject.\n");
    subject = -1;
    return;
  }
  text =
    ("Title:\n " + votes[subject][TITLE] +
     "\n\nText:\n" + votes[subject][TEXT] +
     "\nAuthor: " + votes[subject][NAME] +
     "\nDate: " + votes[subject][DATE] + "\n");
  for(alt=0; alt<sizeof(votes[subject][ALT]); alt++)
    text +=
      ("\nAlternative " + (alt+1) +
       ":\nText:\n" + votes[subject][ALT][alt][TEXTS] +
       "Author: " + votes[subject][ALT][alt][NAMES] +
       "\nDate: " + votes[subject][ALT][alt][DATES] +
       "\nGuests: " + votes[subject][ALT][alt][GUE] +
       "   Players: " + votes[subject][ALT][alt][PLA] +
       "   Wizards: " + votes[subject][ALT][alt][WIZ] + "\n");
  text +=
    ("\nTotal:\nGuests: " + votes[subject][TGUE] + "   Players: " +
     votes[subject][TPLA] + "   Wizards: " + votes[subject][TWIZ] + "\n");
  for(alt=0; alt<sizeof(votes[subject][VOTES]); alt++)
    text += ("\nVote " + (alt+1) +
	     ":\nText:\n" + votes[subject][VOTES][alt][TEXTS] +
	     "Author: " + votes[subject][VOTES][alt][NAMES] +
	     "\nDate: " + votes[subject][VOTES][alt][DATES] + "\n");
  text += "\nHaving Voted Players Encrypted Real Names:\n";
  for(alt=0; alt<sizeof(votes[subject][VERN]); alt++)
    text += votes[subject][VERN][alt] + "\n";
  text += "\n########################################\n\n";
  if(!write_file(SAVEFILE + ".save", text)) {
    write("Error: Could not write backup file.\n");
    return 1;
  }
  votes = votes[0..subject-1] + votes[subject+1..sizeof(votes)-1];
  save();
  write("Subject " + (subject+1) + " removed.\n");
  subject = -1;
}



remove_alternative(string str)
{
  if(str != "remove") {
    write("You decide not to remove this alternative.\n");
    subject = -1;
    return;
  }
  votes[subject][ALT] = votes[subject][ALT][0..alternative-1] +
    votes[subject][ALT][alternative+1..sizeof(votes[subject][ALT])-1];
  save();
  write("Alternative " + (alternative+1) + " from subject " + (subject+1) +
	" removed.\n");
  subject = -1;
}



remove_vote(string str)
{
  if(str != "remove") {
    write("You decide not to remove this vote.\n");
    subject = -1;
    return;
  }
  votes[subject][VOTES] = votes[subject][VOTES][0..alternative-1] +
    votes[subject][VOTES][alternative+1..sizeof(votes[subject][VOTES])-1];
  save();
  write("Vote " + (alternative+1) + " from subject " + (subject+1) +
	" removed.\n");
  subject = -1;
}
