// LAST EDIT: Wed Nov  2 18:16:12 1994 by Joern Preine (wolf/Uni-OL)

#include <secure/wizlevels.h>
#include "./wizbelt/wizbelt-defaults.h"

inherit "/std/thing";

inherit "/obj/tool/wizbelt/editor";
inherit "/obj/tool/wizbelt/filecomm";
inherit "/obj/tool/wizbelt/viewcomm";

#include <properties.h>
#include <config.h>
#include <combat.h>
#include <moving.h>
#include <secure/wizlevels.h>

object store, ob, merk;
string wname, wsay, edit_file, income_msg;

mixed *QueryAutoObject() { return ({ newmacros, turned_on, income_msg }); }

void SetAutoObject(mixed *args) {
   if (!pointerp(args) || sizeof(args) < 3) {
      write("Can't initalize belt of wizards.\n");
      newmacros = ([]);
      turned_on =
        ({"title", "environment", "attributes", "messages", "armours",
          "weapons", "quests", "domains", "mails", "plan", "project"});
      income_msg = "";
      return;
   }
   if (!mappingp(args[0])) newmacros = ([]); else newmacros = args[0];
   if (!pointerp(args[1])) turned_on = 
        ({"title", "environment", "attributes", "messages", "armours",
          "weapons", "quests", "domains", "mails", "plan", "project"});
   else turned_on = args[1];
   if (!stringp(args[2])) income_msg=""; else income_msg = args[2];
   if (sizeof(args) > 3) write("Belt couldn't understand all the data.\n");
}

int won(string str) {
  string *possible;
  int i;
  possible = ({"title", "environment", "attributes", "messages", "armours",
               "weapons", "quests", "domains", "mails", "plan", "project"});
  if (!str) {
     write("Turned output on for:\n");
     for (i=0; i<sizeof(turned_on); i++) write(turned_on[i]+"\n");
     return 1;
  }
  for (i=0; i<sizeof(possible); i++) 
      if ((({string})"/lib/string"->ladjcut(possible[i],3) == str) ||
          possible[i] == str) { 
            write("Ok.\n"); 
            turned_on += ({possible[i]}); 
            return 1;
          }
  write("Invalid argument. Valid arguments are:\n");
  for (i=0; i<sizeof(possible); i++) write(possible[i]+"\n");
  return 1;
}

int woff(string str) {
  string *possible, *rest;
  int i;
  possible = ({"title", "environment", "attributes", "messages", "armours",
               "weapons", "quests", "domains", "mails", "plan", "project"});
  rest = possible - turned_on;
  if (!str) {
     write("Turned output off for:\n");
     for (i=0; i<sizeof(rest); i++) write(rest[i]+"\n");
     return 1;
  }
  for (i=0; i<sizeof(turned_on); i++) 
      if ((({string})"/lib/string"->ladjcut(turned_on[i],3) == str) ||
          turned_on[i] == str) { 
            write("Ok.\n"); 
            turned_on -= ({turned_on[i]}); 
            return 1;
          }
  write("Invalid argument. Valid arguments are:\n");
  for (i=0; i<sizeof(turned_on); i++) write(turned_on[i]+"\n");
  return 1;
}

void create() {
 ::create();
 if (!is_clone()) return;
 if (!geteuid() || blueprint(this_object()) == "/players/joern/wizbelt") {
    write("\
For security reasons this object may NOT be cloned\n\
from a wizard's directory. Try /obj/tool.\n");
    remove();
    if (this_object()) destruct(this_object());
    return;
 }
 SetShort("Your belt of wizards");
 SetLong("This is yet another small wiztool. Type 'whelp' for help.\n");
 SetWeight(500);
// SetAC(1);
 SetInfo("Belt of wizards V2.0 by Joern\n");
// SetArmourType(AT_AMULET);
 SetNoDrop(1);
 SetNoGive(1);
 SetNoGet(1);
 AddId("belt");
 AddId("wizbelt");
 AddId("wizards belt");
 wsay="Player";
 seteuid(getuid(this_object()));
 wname = add_gen(({string})this_player()->QueryName()) + " belt of wizards";
 if (sizeof(explode(object_name(this_object()),"#"))==2) {
   if (file_size("/players/"+({string})this_player()->QueryRealName()+"/startup.c")>=0) 
      call_other("/players/"+({string})this_player()->QueryRealName()+"/startup",
      "startup");
 }
}

varargs string QueryShort() { return wname; }

void init() {
  ::init();
  add_action("stat","wexam");
  add_action("stat","wx");
  add_action("wlook","wlook");
  add_action("wlook","wl");
  add_action("wload","wload");
  add_action("wgrep","wgrep");
  add_action("wquit","wquit");
  add_action("whelp","whelp");
  add_action("wman","wman");
  add_action("wmore","wmore");
  add_action("wclone","wclone");
  add_action("wrml","wrml");
  add_action("wmacro","wmacro");
  add_action("wlist","wlist");
  add_action("wfind","wfind");
  add_action("wrepl","wrepl");
  add_action("wedit","wedit");
  add_action("whead","whead");
  add_action("recho","recho");
  add_action("schalt","turn");
  add_action("lsay","lsay");
  add_action("lsay","#");
  add_action("lecho","lecho");
  add_action("bright","bright");
  add_action("ac","ac");
  add_action("wmess","wmess");
  add_action("won","won");
  add_action("woff","woff");
  if (income_msg && income_msg != "") shout(income_msg);
  SetAutoObject(({newmacros, turned_on, income_msg}));
}

void get_message_line(string str) {
   if (!str || str == "." || str == "**") return;
   income_msg += str + "\n";
   input_to("get_message_line");
}

int wmess(string arg) {
    if (arg) { income_msg = arg + "\n"; return 1; }
    write("Enter the lines. Type '.' or '**' to end.\n");
    income_msg = "";
    input_to("get_message_line");
    return 1;
}

int wmacro(string arg) {
    int ret;
    ret = editor::macro(arg);
    SetAutoObject(({newmacros, turned_on, income_msg}));
    return ret;
}

int wquit() {
 object *objs;
 int i;
 if (file_size("/players/"+({string})this_player()->QueryRealName()+"/startup.c")>=0)
     call_other("/players/"+({string})this_player()->QueryRealName()+"/startup","quit");
 objs = deep_inventory(this_player());
 for (i=0; i<sizeof(objs); i++) {
     if (!({mixed})objs[i]->Query(P_AUTOOBJECT)) objs[i]->remove();
 }
 if (({string})this_player()->QueryHome())
     this_player()->move(this_player()->QueryHome(), M_TELEPORT);
 this_player()->command_me("quit");
 return 1;
}

int wman(string str) {
    string dir, file;
    dir = "/obj/tool/wizbelt/";
    if (!str) { notify_fail("Wman <command>\n"); return 0; }
    file = dir + str;
    if (file_size(file) > 0) {
       this_player()->more(file, 0, 0);
       return 1;
    }
    if (present("xtool", this_player()))
       this_player()->command_me("xman "+str);
    else
       this_player()->command_me("man "+str);
    return 1;
}

int recho(string str) {
  string a,b;
  object who;
  if (!str) { notify_fail("REcho what?\n"); return 0; }
  if (sscanf(str, "%s %s", a, b)<2) {
     notify_fail("REcho to whom and what?\n");
     return 0;
  }
  if (!who = find_living(a)) {
         notify_fail("Failed to find "+a+".\n");
         return 0;
  }
  if (!environment(who)) return 0;
  tell_room(environment(who), b+"\n");
  write("You echo: "+b+"\n");
  return 1;
}

int bright(string str) {
  int i;
  if (!Allowed()) return 1;
  if (!str) {write("Bright is "+QueryBright()+".\n"); return 1;}
  if (sscanf(str,"%d",i)>0) SetBright(i);  
  write("Bright set to: "+i+".\n");
  return 1;
}

int ac(string str) {
  if (!Allowed()) return 1;
//  if (!str) {write("AC is "+QueryAC()+".\n"); return 1; }
//  if (sscanf(str,"%d",i)>0) SetAC(i);
//  write("AC set to: "+i+".\n");
  return 1;
}

int whelp() {
   if (!Allowed()) return 1;
   this_player()->more("/obj/tool/wizbelt/beltdoc", 0 ,0);
   return 1;
}

int lsay(string str) {
   string text;
   if (!Allowed()) return 1;
   if (!str) text = ""; else text = str;
   say(({string})this_player()->QueryName()+" says: "+text+"\n");
   write("Started lsay. Give '.' to stop.\n");
   write("#");
   input_to("lsay_more");
   return 1;
 }

void lsay_more(string str) {
   if (!Allowed()) return;
   if (str == "**" || str == ".") {
     write("Ok.\n");
     return;
   }
   if (str && str != "") say(str+"\n");
   write("#");
   input_to("lsay_more");
 }

int lecho(string str) {
   if (!Allowed()) return 1;
   if (str) shout(str+"\n");
   write("Started lecho. Give '.' to stop.\n");
   write("#");
   input_to("lecho_more");
   return 1;
 }

void lecho_more(string str) {
  if (!Allowed()) return;
  if (str == "**" || str == ".") {
    write("Ok.\n");
    return;
  }
  if (str && str != "") shout(str+"\n");
  write("#");
  input_to("lecho_more"); 
}

int wedit(string str) {
  string fname;
  fname = WFindFile(str);
  edit_file = fname;
  say(({string})this_player()->QueryName()+" starts editing a file. Don't disturb!\n");
  editor::start_ed(edit_file, this_player());
  return 1;
}

void SaveFile(string name, string *lines) {
  int i;
  write("Writing file to "+name+".\n");
  if (file_size(name) > 0) rm(name);
  for (i=0; i<sizeof(lines); i++) write_file(name, lines[i]);
}

void SaveAs(string name, string *lines) {
  if (!name || name=="") {
     write("Save as? ");
     input_to("SaveAs", 0, lines);
     return;
  }
  edit_file = name;
  SaveFile(edit_file, lines);
}

void end_edit(string *lines, object user) {
  say(({string})user->QueryName()+" finished editing a file.\n");
  if (sizeof(lines) < 1) {
     write("File is 0 bytes long. It's not saved.\n");
     return;
  }
  if (!edit_file || edit_file == "") {
     write("Save as? ");
     input_to("SaveAs", 0, lines);
     return;
  }
  SaveFile(edit_file, lines);
  return;
}

int do_it() {
 object who;
 if (({object *})this_player()->QueryEnemies()) who=(({object *})this_player()->QueryEnemies())[0];
 if (!who) {call_out("do_it",2); return 1; }
 tell_object(this_player(),"  ["+({int})who->QueryHP()+" hp, ");
 tell_object(this_player(),({int})who->QuerySP()+" sp] ");
 tell_object(this_player()," Yours: ["+({int})this_player()->QueryHP()+" hp, ");
 tell_object(this_player(),({int})this_player()->QuerySP()+" sp]\n");
 call_out("do_it",2);
 return 1;
}

int check_inv() {
 object who;
 who=first_inventory(environment(this_player()));
 if (environment(this_player())!=store) {
   while(who) {
     if (({string})who->QueryName()=="Someone") {
       say(capitalize(({string})this_player()->QueryName())+"'s belt shoots a ray of strange red light through the room!\n");
       say("Now you can see "+capitalize(({string})who->QueryRealName())+"'s shadow on the wall!\n");
       write("Invisible here: "+capitalize(({string})who->QueryRealName())+"!\n");
       store=environment(this_player());
     } /* if */
     who=next_inventory(who);
   } /* while(who) */
 } /* if */
 call_out("check_inv",2);
 return 1; 
}

int schalt(string str) {
 if (!Allowed()) return 1;
 if (sscanf(str,"%~s on")==1) {
    call_out("do_it",2);
    call_out("check_inv",2);
    write("You turn your belt on!\n");
    return 1;
 }
 if (sscanf(str,"%~s off")==1) {
    remove_call_out("do_it");
    remove_call_out("check_inv");
    write("You turn your belt off!\n");
    return 1;
 }
 return 0;
}
