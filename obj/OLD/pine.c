/*  Pine V 1.0, coded by Pumuckel@Nightfall, 1994
 *
 *  Each msgs field is an array of messages
 *
 *  mixed *msgs = ({ mixed *message1; ... mixed *messageM }
 *
 *  A message is represented as an array with the following fields
 *
 *  mixed *message = (
 *     string from
 *     string sender
 *     string recipient
 *     string *cc
 *     string *bcc
 *     string subject
 *     string date
 *     string id
 *     string body
 *   )
 *
 *  The mailer demon (/secure/mailer, or /obj/mailer) provides
 *  the following functions:
 *
 */

#include <config.h>
#include <properties.h>
#include <secure/wizlevels.h>
#include <string.h>
#include <mail.h>

inherit "/std/thing";
inherit "/lib/string";

#define TP        this_player()
#define TO        this_object()
#define TI        this_interactive()

static int screen, curfoldnum, curmsg, rmsg, update_status;
static string tmp_str1, tmp_str2;
static string rsubject, rbody;
static mixed *msg;
static string *cur_folder;
mixed  *folders;  /* used for restore_object */

#define MAINMENU      1
#define ADDRESSBOOK   2
#define FOLDERMENU    3
static mapping aliases;

static string check_for_new_mail();

create() {
  (::create());
  if (!is_clone()) return;	/* Blueprint shouldn't do anything */
  SetShort("another fine mailtool");
  Set(P_NODROP, "Don't even think about that!\n");
  Set(P_NOGIVE, "Better you don't do that.\n");
  Set(P_NOSELL, "Selling mailboxes ? How silly!\n");
  Set(P_AUTOOBJECT, 1);
  AddId("pine");
  AddId("mailbox");
  AddId("mailtool");
  seteuid(getuid());
  write(check_for_new_mail());
}

query_prevent_shadow() { return 1;}

QueryLong() {
  return "This is another fine mailtool which is programmed by Pumuckel.\n\
It is a try to simplify mailing, add more and better features to the\n\
existing mailbox and to reduce memory resources.\n\
Please remember that you are using a program in testing phase. So bugs may\n\
occur all the time and could destruct existing mail data. Use on your own risk.\n\
Pine runs in command mode if you enter 'pine'.\n" + check_for_new_mail();
}

init() {
  (::init());
  add_action("firstmenu", "pine");
}

static a2s(string *arr) {
  return implode(map(sort_array(arr, #'>), /* '),/**/ 
                 #'capitalize), "\n"); /*'));/**/
}
static int get_mail_status(int index) {
  /* Returns the status of the mail, 
   * -1 for unknown status
   */
  if (sizeof(folders[1][curfoldnum][index-1]) == 9)
    return MSG_STATUS_NEW;
  if (sizeof(folders[1][curfoldnum][index-1]) == 10)
    return folders[1][curfoldnum][index-1][MSG_STATUS];
  return -1;
}
static int set_mail_status(int index, int stat) {
  /* Set status of mail:
   * Allow only setting of higher stat
   * NEW < READ < ANSWERED
   */
  if (sizeof(folders[1][curfoldnum][index-1]) == 9) {
    folders[1][curfoldnum][index-1] += ({MSG_STATUS_NEW});
  }
  if (sizeof(folders[1][curfoldnum][index-1]) != 10) return 0;
  if (stat > folders[1][curfoldnum][index-1][MSG_STATUS]) {
    folders[1][curfoldnum][index-1][MSG_STATUS] = stat;
    MAILER->save(TP->QueryRealName(), folders);
  }
  return 1;
}

static Input() {
  write("Enter command: ");
  input_to("parse_commands");
  return 1;
}

static pine_update(string str) {
  if (str == "update") {
    object ob;
    
    while (ob = find_object(blueprint(TO))) {
      if (!ob->remove()) destruct(ob);
    }
    if (ob = clone_object(blueprint(TO))) {
      ob->move(environment());
      remove();
      return 1;
    }
    else
      write("Argh. Couldn't update!\n");
  }
  return 0;
}

static firstmenu(str) {
  read_aliases();
  return menu(str);
}
static menu(string str) {
  screen = MAINMENU;

/* ?               HELP               -  Get help using foldermenu\n\ */
  if (pine_update(str)) return 1;
  write("\n\
W               WRITE MAIL           -  Send a mail to someone\n\
R|F|D [n]       REPLY|FORWARD|DELETE -  Reply/forward/delete a mail\n\
I [n][<folder>] FOLDER INDEX         -  View messages >= <n> in <folder>\n\
M               FOLDER MENU          -  Go to folder menu\n");
  if (IS_IMMORTAL(TP))
    write ("\
A               ADDRESS BOOK         -  Update address book\n\
S <file>        SAVE MESSAGE         -  Save your current message to <file>\n\
");
  write("\
.|+|-           SHOW MESSAGE         -  Show cur./prev./next message\n\
<n>                                     Show message number <n>\n\
N                                       Show next new mail\n\
<RETURN>        SHOW MENU AGAIN      -  Show this menu again\n\
Q               QUIT                 -  Exit the Pine program\n\
");
  return Input();
}

static address_book(string str) {
  screen = ADDRESSBOOK;
  write("\n\
M         MAIN MENU                  -  Go back to the main menu\n\
S         SHOW ALIASES               -  Shows all your aliases\n\
A         ADD ALIAS                  -  Add an alias to your addressbook\n\
D         DELETE ALIAS               -  Delete an alias from the book\n\
Z         ADD TO ALIAS               -  Add another recipient to the alias\n\
E         ERASE FROM ALIAS           -  Erase someone from an alias\n\
R         RENAME ALIAS               -  Rename an existing alias\n\
");
  return Input();
}

/* Alias handling of the addressbook */

static read_aliases() {
  string aliastext;
  string *text;
  string object_name;
  
  aliases = ([]);
  object_name = "/players/"+TP->QueryRealName()+"/.pinealias";
  if ((aliastext = read_file(object_name)) &&
      sizeof(text = explode(aliastext, "\n"))) {
    int i;
    string text1, text2;

    for (i=0; i<sizeof(text); i++) {
      if (sscanf(text[i],"%s: %s", text1, text2) != 2) {
	if (text[i] != "")
	  printf("Error in alias file at line %d\n", i+1);
      }
      else {
	aliases[text1] = explode(text2,", ")-({});
      }
    }
  }
}

static save_aliases() {
  string object_name;
  string aliastext;
  string text;
  string *a_index;
  int i;
  
  object_name = "/players/"+TP->QueryRealName()+"/.pinealias";
  text = "";
  a_index = m_indices(aliases);
  for (i=0; i<sizeof(a_index); i++) {
    text += sprintf("%s: %s\n", a_index[i], implode(aliases[a_index[i]],", "));
  }
  if (file_size(object_name)>=0) {
    rm(object_name+".old");
    if (rename(object_name, object_name+".old")) {
      write("Couldn't backup old alias file.\n");
      return 0;
    }
  }
  if (!write_file(object_name, text)) {
    write("Couldn't write new alias file.\n");
    return 0;
  }
  return 1;
}

static show_aliases() {
  string *a_index;
  
  if (sizeof(a_index = m_indices(aliases))) {
    string text;
    int i;
    a_index = sort_array(a_index, #'>); /*'); /* emacs bluff */
    text = "\nYour current addressbook\n========================\n\n";
    for (i=0; i<sizeof(a_index); i++) {
      text += sprintf( "%-10s:  %-70#s\n",
		      a_index[i],
		      a2s(aliases[a_index[i]]));
    }
    smore(text+"\n", TP->QueryPageSize(), TO, "parse_commands", 1);
  }
  else {
    write("No aliases defined.\n");
    Input();
  }
  return 1;
}

static add_alias() {
  write("Enter new nickname (one word and easy to remember) : ");
  input_to("add_alias2");
  return 1;
}

static add_alias2(string str) {
  tmp_str1 = str;
  if (!tmp_str1 || tmp_str1 == "") {
    write("Aborted.\n");
    return Input();
  }
  write("To whom should your mail go to: ");
  input_to("add_alias3");
  return 1;
}

static add_alias3(string str) {
  if (!str || str == "") {
    write("Aborted.\n");
    return Input();
  }
  str = lower_case(str);
  str = implode(explode(str, " ") - ({""}), ""); /* delete spaces */
  aliases[tmp_str1] = map(explode(str,",") - ({""}), #'capitalize);
  /* ' );
  /* emacs bluff */
  printf("Alias '%s' for %s added.\n", tmp_str1, a2s(aliases[tmp_str1]));
  save_aliases();
  return Input();
}

static delete_alias() {
  write("Enter alias to delete: ");
  input_to("delete_alias2");
  return 1;
}

static delete_alias2(string str) {
  if (!str || str == "") {
    write("Aborted.\n");
    return Input();
  }
  if (member_array(str, m_indices(aliases)) == -1) {
    write("No such alias.\n");
    return Input();
  }
  aliases = m_delete(aliases, str);
  save_aliases();
  printf("Alias '%s' deleted from addressbook.\n", str);
  return Input();
}

static add_to_alias() {
  write("Add to which alias: ");
  input_to("add_to_alias2");
  return 1;
}

static add_to_alias2(string str) {
  if (!str || str == "") {
    write("Aborted.\n");
    return Input();
  }
  if (member_array(str, m_indices(aliases)) == -1) {
    write("No such alias.\n");
    return Input();
  }
  tmp_str1 = str;
  printf("Add whom to alias '%s': ", str);
  input_to("add_to_alias3");
  return 1;
}

static add_to_alias3(string str) {
  string *arr;
  
  if (!str || str == "") {
    write("Aborted.\n");
    return Input();
  }
  str = lower_case(str);
  str = implode(explode(str, " ") - ({""}), ""); /* delete spaces */
  aliases[tmp_str1] += (arr = 
			map(explode(str,",") - ({""}), #'capitalize));
  /* ' ));
  /* emacs bluff */
  printf("'%-10s' expanded to: %-55#s\n", tmp_str1, 
	 a2s(aliases[tmp_str1]));
  save_aliases();
  return Input();
}

static erase_from_alias() {
  write("Erase from which alias: ");
  input_to("erase_from_alias2");
  return 1;
}

static erase_from_alias2(string str) {
  if (!str || str == "") {
    write("Aborted.\n");
    return Input();
  }
  if (member_array(str, m_indices(aliases)) == -1) {
    write("No such alias.\n");
    return Input();
  }
  tmp_str1 = str;
  printf("Erase whom from alias '%s': ", str);
  input_to("erase_from_alias3");
  return 1;
}

static erase_from_alias3(string str) {
  string *arr;
  int i;

  if (!str || str == "") {
    write("Aborted.\n");
    return Input();
  }
  str = lower_case(str);
  str = implode(explode(str, " ") - ({""}), ""); /* delete spaces */
  arr = map(explode(str,",") - ({""}), #'capitalize); /* ' );
  /* emacs bluff */
  for (i=0; i<sizeof(arr); i++)
    aliases[tmp_str1] -= ({ arr[i]}) ;
  if (sizeof(aliases[tmp_str1]))
    printf("'%-10s' shrunked to: %-55#s\n", tmp_str1,
	   a2s(aliases[tmp_str1]));
  else {
    aliases = m_delete(aliases, tmp_str1);
    printf("Alias '%s' removed.\n", tmp_str1);
  }
  save_aliases();
  return Input();
}

static rename_alias() {
  write("Which alias to rename: ");
  input_to("rename_alias2");
  return 1;
}

static rename_alias2(string str) {
  if (!str || str == "") {
    write("Aborted.\n");
    return Input();
  }
  if (member_array(str, m_indices(aliases)) == -1) {
    write("No such alias.\n");
    return Input();
  }
  tmp_str1 = str;
  printf("Rename '%s' to: ", str);
  input_to("rename_alias3");
  return 1;
}

static rename_alias3(string str) {
  if (!str || str == "") {
    write("Aborted.\n");
    return Input();
  }
  aliases[str] = aliases[tmp_str1];
  aliases = m_delete(aliases, tmp_str1);
  save_aliases();
  printf("Alias '%s' renamed to '%s'.\n", tmp_str1, str);
  return Input();
}
/****************************************************************************/

static compose_mail(string str) {
  rsubject = 0;
  rbody = "";
  if (stringp(str))
    return compose_mail2(str);
  write("Send mail to: ");
  input_to("compose_mail2");
  return 1;
}

static compose_mail2(string str) {
  if (!str) str = "";
  msg = allocate(9);
  msg[MSG_FROM] = getuid(TI);
  msg[MSG_CC] = ({});
  msg[MSG_RECIPIENT] = str;
  write("CC to: ");
  input_to("compose_mail3");
  return 1;
}

static compose_mail3(string str) {
  int i;
  string *cc;
    
  if (!str) str = "";
  cc = explode(implode(explode(str," ") - ({""}), ""), ",") - ({""});
  for (i=0; i<sizeof(cc); i++) {
    if (aliases[cc[i]])
      msg[MSG_CC] += aliases[cc[i]];
    else
      msg[MSG_CC] += ({ cc[i] });
  }
  if (aliases[msg[MSG_RECIPIENT]]) {
    msg[MSG_CC] += aliases[msg[MSG_RECIPIENT]];
    msg[MSG_RECIPIENT] = msg[MSG_CC][0];
  }
  write("BCC to: ");
  input_to("compose_mail4");
  return 1;
}
static compose_mail4(string str) {
  int i;
  string *bcc;

  if (!str) str = "";
  bcc = explode(implode(explode(str," ") - ({""}), ""), ",") - ({""});
  msg[MSG_BCC] = ({});
  for (i=0; i<sizeof(bcc); i++) {
    if (aliases[bcc[i]])
      msg[MSG_BCC] += aliases[bcc[i]];
    else
      msg[MSG_BCC] += ({ bcc[i] });
  }
  if (rsubject)
    return compose_mail5(rsubject);
  write("Subject: ");
  input_to("compose_mail5");
  return 1;
}
static compose_mail5(string str) {
  if (!str) str = "";
  msg[MSG_SUBJECT] = str;
  
  msg[MSG_BODY] = rbody;
  rsubject = 0; rbody = "";
  write("Give message.  Finish message with '**' or '.', or '~q' to cancel.\n");
  write("]");
  input_to("compose_mail6");
  return 1;
}
static compose_mail6(string str) {
  int i;
  string *delivered_to;

  if (!str) str = "";
  
  if (str == "~q") {
    msg = 0; /* Do not need the msg array anymore */
    write("Message cancelled.\n");
    return Input();
  }
  if (str == "**" || str == ".") {
    if (!(delivered_to = MAILDEMON->DeliverMail(msg))) {
      write("Could not deliver mail.\n");
    }
    else {
      delivered_to = map(delivered_to, #'capitalize); /* ');
      /* emacs bluff */
      printf("Ok, mail delivered to: %-55#s\n", a2s(delivered_to));
    }
    msg = 0; /* Do not need the msg array anymore */
    if (rmsg && update_status) {
      if (read_mail())
	set_mail_status(rmsg, MSG_STATUS_ANSWERED);
      rmsg = 0;
    }
    return Input();
  }
  msg[MSG_BODY] += str + "\n";
  write("]");
  input_to("compose_mail6");
  return 1;
}
/****************************************************************************/
static folder_menu() {
  screen = FOLDERMENU;
  write("\n\
M         MAIN MENU                  -  Go back to the main menu\n\
L         LIST FOLDERS               -  Shows all your folders\n\
A         ADD                        -  Add a folder\n\
D         DELETE FOLDER              -  Delete a folder\n\
C         CHANGE FOLDER              -  Choose another folder as current\n\
R         RENAME FOLDER              -  Rename an existing folder\n\
");
  return Input();
}

static read_mail() {

  if (!TP) return 0;

  folders = MAILER->load(TP->QueryRealName());
  if (!sizeof(folders) || !sizeof(folders[1])) {
    write("You have no mail.\n");
    return 0;
  }
  return 1;
}

static show_folders() {
  string text;
  int i;
  
  if (!read_mail()) 
    return Input();
  text = sprintf("Folders: %-70#s\n", implode(folders[0],"\n"));
  folders = 0;
  smore(text, TP->QueryPageSize(), TO, "parse_commands", 1);
  return 1;
}
    
static read_folder(string name) {
  int tmp;
  if (!read_mail()) 
    return 0;
  /*   write(val_to_str(folders)); */
  if ((tmp = member_array(name, folders[0])) == -1) {
    write("Folder does not exist.\n");
    return 0;
  }
  if (!sizeof(folders) ||
      !sizeof(folders[1])) {
    folders = ({ ({}), ({}) });
  }
  curfoldnum = tmp;
  curmsg = 1;
  rmsg = 0;
  folders = 0;  
  return 1;
}

static string check_for_new_mail() {
  if (read_folder("newmail") && read_mail()) {
    int i, n;
    for (i=0, n=0;i<sizeof(folders[1][curfoldnum]);i++)
      n += (get_mail_status(i+1) == MSG_STATUS_NEW);
  }
  if (n)
    return "You have "+n+" new mail"+(n==1?"":"s")+".\n";
  return "";
}

static change_folder(string str) {
  if (stringp(str)) 
    return change_folder2(str);
  write("Change to folder: ");
  input_to("change_folder2");
}

static change_folder2(string name) {
  if (!read_folder(name))
    return Input();
  printf("Ok.\n");
  return Input();
}
/****************************************************************************/
static show_index(string str) {
  int i, start, stat, foldmsg;
  string from_user;
  string text, fold;

  start = 1;
  if (str && 
      !((sscanf(str, "%d %s", start, fold) == 2) && read_folder(fold)) &&
      (sscanf(str, "%d", start) != 1) &&
      !read_folder(str))
    return Input();
  else
    if (!read_mail()) 
      return Input();
  if ((start < 1) || (start >sizeof(folders[1][curfoldnum]))) {
    write("No such mailnumber.\n");
    return Input();
  }
  text = "\n*** Messages in folder: "+folders[0][curfoldnum]+" ***\n";
  for (i = --start; i < sizeof(folders[1][curfoldnum]); i++) {
    stat = get_mail_status(i+1);
    text += sprintf("%c%c%3d %-12.12s %-26.26s [%5d] %s\n", 
		    ((i+1 == curmsg)?'*':' '),
		    (stat==MSG_STATUS_NEW?'N':
		     (stat==MSG_STATUS_READ?' ':
		      (stat==MSG_STATUS_ANSWERED?'A':'X'))),
		    i+1,
		    capitalize(folders[1][curfoldnum][i][MSG_FROM]),
		    folders[1][curfoldnum][i][MSG_SUBJECT],
		    strlen(folders[1][curfoldnum][i][MSG_BODY]),
		    ctime(folders[1][curfoldnum][i][MSG_DATE]));
  }
  folders = 0;
  smore(text+"\n", TP->QueryPageSize(), TO, "parse_commands", 1);
  return 1;
}
/****************************************************************************/
static string get_mail_text(int index) {
  string text;
  msg = folders[1][curfoldnum][index-1];
  text = "";
  text += "\nFrom:    "+capitalize(msg[MSG_FROM])+"\n";
  if (msg[MSG_RECIPIENT] &&
      (TP->QueryRealName()!=msg[MSG_RECIPIENT]))
    text += "To:      "+capitalize(msg[MSG_RECIPIENT])+"\n";
  msg[MSG_CC]=map(msg[MSG_CC], #'capitalize); /* ');
  /* emacs bluff */
  if (sizeof(msg[MSG_CC]))
    text += sprintf("Cc:      %-68#s\n", a2s(msg[MSG_CC]));
  text += "Subject: "+msg[MSG_SUBJECT] + "\n";
  text += "Date:    "+ctime(msg[MSG_DATE]) + "\n";
  text += "\n" + msg[MSG_BODY] + "\n";
  msg = 0;
  return text;
}
static show_mail(int index, int unread) {
  int i;
  
  if (!read_mail()) 
    return Input();
  
  if (index < 1) {
    write("You have reached the beginning of this folder.\n");
    return Input();
  }
  if (index > sizeof(folders[1][curfoldnum])) {
    write("You have reached the end of this folder.\n");
    return Input();
  }
  if (unread) {
    index = 1;
    while((index <= sizeof(folders[1][curfoldnum])) &&
	  (sizeof(folders[1][curfoldnum][index-1]) == 10) &&
	  (folders[1][curfoldnum][index-1][MSG_STATUS] != MSG_STATUS_NEW))
      index++;
    if (index > sizeof(folders[1][curfoldnum])) {
      write("No new mail in this folder.\n");
      return Input();
    }
  }
  curmsg = index;
  smore(get_mail_text(index), TP->QueryPageSize(), TO, "parse_commands",1);
  set_mail_status(index, MSG_STATUS_READ);
  folders = 0;
  return 1;
}
/****************************************************************************/
static reply_or_forward_mail(string str, int forward) {

  if (!read_mail())
    return Input();
  if (!str || (str && (sscanf(str, "%d", rmsg) != 1)))
    rmsg = curmsg;
  update_status = !forward;
  rsubject = (forward?"Fw: ":"Re: ") + 
    folders[1][curfoldnum][rmsg-1][MSG_SUBJECT];
  if (forward) {
    rbody = "---- Forwarded mail begin ----\n"+
      folders[1][curfoldnum][rmsg-1][MSG_BODY] +
	"---- Forwarded mail end ----\n";
    write("Forward to: ");
    input_to("compose_mail2");
    return 1;
  }
  rbody = "";
  write("\
Reply to: "+capitalize(folders[1][curfoldnum][rmsg-1][MSG_FROM])+"\n\
Subject: "+rsubject+"\n");
  return compose_mail2(folders[1][curfoldnum][rmsg-1][MSG_FROM]);
}  
/****************************************************************************/
static delete_mail(string str) {
  int i, size;
  
  if (!read_mail()) 
    return Input();
  i = 0;
  if (!str) i = curmsg;
  if (i || sscanf(str, "%d", i) == 1)  {
    if ((i < 1) ||
	(i > (size = sizeof(folders[1][curfoldnum])))) {
      write("No such mail number.\n");
      return Input();
    }
    i --;
    folders[1][curfoldnum] = 
      folders[1][curfoldnum][0..i-1] + folders[1][curfoldnum][i+1..];
    if (!sizeof(folders[1][curfoldnum]))
      write("Need to delete folder.\n");
    MAILER->save(TP->QueryRealName(), folders);
  }
  curmsg -= (curmsg > sizeof(folders[1][curfoldnum]));
  folders = 0;
  return Input();
}
/****************************************************************************/
static save_message(string str) {
  if (stringp(str))
    return save_message2(str);
  write("Save message, enter filename: ");
  input_to("save_message2");
  return 1;
}    
static save_message2(string str) {
  if (!read_mail())
    return Input();
  if (!str || str == "") {
    write("Aborted.\n");
    return Input();
  }
  if (!write_file(str, get_mail_text(curmsg))) {
    printf("Couldn't save mail to '%s'\n",str);
  }
  else
    write("Message saved.\n");
  return Input();
}
/****************************************************************************/
static uninitialize() {
  aliases = 0;
  write("Ok, quitting.\n");
  return 1;
}
static parse_commands(string str) {
  int i;
  string tmp, arg;
  
  if (!str) {
    return Input();
  }
  if (sscanf(str, "%s %s", tmp, arg) == 2)
    str = tmp;
  str = lower_case(str);
  switch (screen) {
  case MAINMENU: 
    {
      switch (str) {
      case "?":
	write("Not implemented yet, sorry.\n");
	return Input();
      case "m":
	return folder_menu();
      case "+":
	return show_mail(curmsg+1, 0);
      case "-":
	return show_mail(curmsg-1, 0);
      case ".":
	return show_mail(curmsg, 0);
      case "n":
	return show_mail(curmsg, 1);
      case "":
	return menu();
      case "r":
	return reply_or_forward_mail(arg, 0);
      case "f":
	return reply_or_forward_mail(arg, 1);
      }
      if (IS_IMMORTAL(TP)) {
	switch(str) {
	case "a":
	  return address_book();
	case "s":
	  return save_message(arg);
	}
      }
    }
    break;
  case ADDRESSBOOK: 
    {
      switch (str) {
      case "?":
	write("Not implemented yet, sorry.\n");
	return Input();
      case "a":
	return add_alias();
      case "d":
	return delete_alias();
      case "z":
	return add_to_alias();
      case "e":
	return erase_from_alias();
      case "r":
	return rename_alias();
      case "s":
	return show_aliases();
      case "":
	return address_book();
      }
    }
    break;
  case FOLDERMENU:
    {
      switch (str) {
      case "?":
      case "r":
      case "d":
      case "a":
	write("Not implemented yet, sorry.\n");
	return Input();
      case "":
	return folder_menu();
      }
    }
    break;
  }
  switch (str) {
  case "c":
    return change_folder(arg);
  case "d":
    return delete_mail(arg);
  case "i":
    return show_index(arg);
  case "l":
    return show_folders();
  case "m":
    return menu();
  case "q":
    return uninitialize();
  case "u":
    return pine_update("update");
  case "w":
    return compose_mail(arg);
  }
  if (sscanf(str, "%d", i) == 1) {
    return show_mail(i, 0);
  }
  write("Ambigious command.\n");
  return Input();
}
