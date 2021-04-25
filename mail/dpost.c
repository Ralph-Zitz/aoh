/*
******************************************************************************
** Post office and basefile for the portable mailbox
**
** Creation: Nightfall
** Changes
**  06Jun97 [Mica@OSB] added support for intermud mail
**  09Oct98 [Softbyte@OSB] MSG_STATUS implementation
******************************************************************************
*/

#include <config.h>
#include <mail.h>
#include <moving.h>
#include <secure/wizlevels.h>
#include <secure/domaininfo.h>

#define TI this_interactive()
#define TP this_player()
#define TO this_object()

inherit "/std/thing";
inherit "/lib/string";

//static mixed *drafta;	/* alist for mailing user */
//static mixed *foldera;	/* alist for reading user */

nosave mapping drafta;
nosave mapping foldera;

mixed *folders;		/* used for restore_object */

int get_menu(mixed str);
void list_a_folder(int num);
void list_folders();
void print_help_menu();
varargs int reply_mail(int mode, int fold, int num, string towhom);
int show_mail(int fold, int msg);
string print_help_menu_wiz();
string ra(mixed str, int n);
string la(mixed str, int n);
int reply(string user);

/*-------------------------------------------------------------------*/
varargs void create() {
  thing::create();
  SetIds(({"mailbox", "dpost"}));
  SetShort("the new mailbox");
  AddAdjective("new");
  SetNoGet(1);
  drafta  = ([ ]);
  foldera = ([ ]);

  /* for tests */
  seteuid(getuid(TO));
}
/*-------------------------------------------------------------------*/
void init() {
  ::init();
  add_action("mail_user","mail");
  add_action("mail_domain","dmail");
  add_action("read_mail","read");
  add_action("dump_mail","dump");
  add_action("help_mail","help");
}
/*-------------------------------------------------------------------*/
varargs mixed QueryLong(string arg) {
  string rc;
  rc =
"This is the new Mailbox. You can read and write mail here.\n"+
"The following commands are available:\n"+
"mail <name>    Write mail to player <name>\n"+
"read mail      Go into interactive read mode\n";
  if (IS_WIZARD(TP))
    rc +=
"dump mail      Write out the whole mail file to /log/<username>.maildump\n";
  return rc;
}
/*-------------------------------------------------------------------*/
int mail_user(string user) {
  string from_user;

  if (!TI || !TP || TI != TP)
    return 0;

  if (!user || sizeof(explode(user, " ")) > 1)
    return notify_fail("Mail whom?\n", NOTIFY_ILL_ARG);

  user = lower_case(user);

  /* only check for user existance if we are write local mail */
  if ( sscanf( user, "%~s@%~s" ) != 2 ) {
    if (!({mixed *})MASTER->find_userinfo(user)) {
      drafta[from_user] = 0;
      notify_fail("There is no player named "+user+".\n", NOTIFY_ILL_ARG);
      return 0;
    }
  }

  from_user = ({string})TP->QueryRealName();
  foldera[from_user] = ({-1, -1});
  drafta[from_user] = ({user});

  write("Subject: ");
  input_to("get_subject");
  return 1;
}

int mail_domain(string domain) {
  string from_user;
  mixed domain_data;

  if (!TI || !TP || TI != TP)
    return 0;

  if (({int})DOMAININFO->QueryState()!=DI_INITED) {
    notify_fail("Database isn't loaded up to now. Please wait and try again later.\n", NOTIFY_NOT_VALID);
    return 0;
  }
  if (!domain || sizeof(explode(domain, " ")) > 1)
    return notify_fail("Mail which domain?\n", NOTIFY_ILL_ARG), 0;
  domain = lower_case(domain);
  if (!(domain_data = ({mixed})"/secure/domaininfo"->QueryDomain(domain))) {
    notify_fail("There is no such domain "+domain+".\n", NOTIFY_ILL_ARG);
    drafta[from_user] = 0;
    return 0;
  }
  from_user = ({string})TP->QueryRealName();
  foldera[from_user] = ({-1, -1});
  drafta[from_user] = ({domain_data[1][0]});
  write("Subject: ");
  input_to("get_subject");
  return 1;
}
/*-------------------------------------------------------------------*/
int get_subject(string str) {
  string from_user;
  string recipient;

  if (!TI || !TP || TI != TP)
    return 0;

  from_user = ({string})TP->QueryRealName();
  recipient = drafta[from_user][0];
  if (!str) str = "";
  drafta[from_user] = ({recipient, str, ""});
  write("Give message.  Finish message with '**', or '~q' to cancel\n");
  write("]");
  input_to("get_body");
  return 1;
}
/*-------------------------------------------------------------------*/
int get_body(string str) {
  string from_user;
  string recipient;
  string subject;
  string body;

  from_user = ({string})TP->QueryRealName();
  recipient = drafta[from_user][0];
  subject = drafta[from_user][1];
  body = drafta[from_user][2];

  if (str == "~q") {
    write("Message cancelled\n");
    drafta[from_user] = 0;
    if (foldera[from_user][0] >= 0) get_menu(1);
    return 1;
  }
  if (str == "**" || str == ".") {
    drafta[from_user] = ({recipient, subject, body});
    write("Cc: ");
    input_to("get_cc");
    return 1;
  }
  if (!str) str="";
  body += str + "\n";
  drafta[from_user] = ({recipient, subject, body});
  write("]");
  input_to("get_body");
  return 1;
}
/*-------------------------------------------------------------------*/
int get_cc(string cc) {
  string from_user;
  mixed recipient;
  string subject;
  string body;
  //string cc2;
  string *cca;
  //int date;
  //string id;

  if (!TI || !TP || TI != TP)
    return 0;

  from_user = ({string})TP->QueryRealName();
  recipient = drafta[from_user][0];
  subject = drafta[from_user][1];
  body = drafta[from_user][2];

#if 1
  cca = regexplode(cc, "[, ]") - ({0, "", " ", ","});
  if (pointerp(recipient)) {
    cca = recipient[1..] + cca;
    recipient = recipient[0];
  }
#else
  cc2 = cc;
  cca = ({ });
  if (pointerp(recipient)) {
    cca += recipient[1..];
    recipient = recipient[0];
  }
  while (cc && cc != "") {
    int n;
    string next_pern;

    n = sscanf(cc,"%s,%s",next_pern,cc);
    if (n == 0) {
      /* cc2 held remaining line; do last person. */
      next_pern = cc2;
      /* set cc to "" to leave loop after this    */
      cc = "";
    }
    cc2 = cc;
    cca += ({next_pern});
  }
#endif
  drafta[from_user] = ({recipient, subject, body, cca});
  write("Bcc: ");
  input_to("get_bcc");
  return 1;
}
/*-------------------------------------------------------------------*/
int get_bcc(string bcc) {
  string from_user, subject, body, *cc, *cca;
  mixed recipient;
  //int date;
  //string id;
  string *delivered_to;
  int n;

  if (!TI || !TP || TI != TP)
    return 0;

  from_user = ({string})TP->QueryRealName();
  recipient = drafta[from_user][0];
  subject = drafta[from_user][1];
  body = drafta[from_user][2];
  cc = drafta[from_user][3];

#if 1
  cca = regexplode(bcc, "[, ]") - ({0, "", " ", ","});
  if (pointerp(recipient)) {
    cca = recipient[1..] + cca;
    recipient = recipient[0];
  }
#else
  cc2 = bcc;
  cca = ({ });
  while (bcc && bcc != "") {
    string next_pern;

    n = sscanf(bcc,"%s,%s",next_pern,bcc);
    if (n == 0) {
      /* cc2 held remaining line; do last person. */
      next_pern = cc2;
      /* set bcc to "" to leave loop after this    */
      bcc = "";
    }
    cc2 = bcc;
    cca += ({next_pern});
  }
#endif
  /* create final message and send it off */
  delivered_to = ({string *})MAILDEMON->DeliverMail( ({
    from_user,
    "",
    recipient,
    cc,
    cca,
    subject,
    0,
    0,
    body
  }) );
  drafta[from_user] = 0;
  if (!delivered_to) write("Could not deliver mail.\n");
  else {
    write("Ok, mail delivered to");
    for (n = 0; n < sizeof(delivered_to); n++) write(" "+delivered_to[n]);
    write(".\n");
  }
  if (foldera[from_user][0] >= 0) get_menu(1);
  return 1;
}

/* Softbyte 09oct98 */
int set_status(int sta, int foldnum,int msg,string user)
{
  mixed *msgs;

  msgs = folders[1][foldnum][msg];
  while (sizeof(msgs)<=MSG_STATUS)
  {
    msgs+=({0});
  }

  if ( (msgs[MSG_STATUS] & sta) == 0)
  {
    msgs[MSG_STATUS]|=sta;
    folders[1][foldnum][msg]=msgs;
    MAILER->save(user,folders);
  }

  return 1;
}


/*-------------------------------------------------------------------*/
int dump_mail(string str) {
  string user;
  string lf;
  mixed *msg;
  int i, j;

  if (!TI || !TP || TI != TP)
    return 0;

  if (!(IS_WIZARD(TP))) return 0;
  if (!str || str != "mail") {
    notify_fail("To dump mail, use 'dump mail'.\n", NOTIFY_ILL_ARG);
    return 0;
  }
  user = ({string})TP->QueryRealName();
  if (!restore_object(MAILPATH+user[0..0]+"/"+user)) {
    folders = ({({ }), ({ })});
  }
  lf = user+".maildump";
  for (i = 0; i < sizeof(folders[0]); i++) {
    log_file(lf,"\n*** Folder "+folders[0][i]+" ***\n\n");

    for (j = 0; j < sizeof(folders[1][i]); j++) {
      msg = folders[1][i][j];
      log_file(lf,"From:       "+msg[MSG_FROM]+"\n");
      log_file(lf,"Sender:     "+msg[MSG_SENDER]+"\n");
      log_file(lf,"To:         "+(msg[MSG_RECIPIENT] || "")+"\n");
      log_file(lf,"Cc:         "+implode(msg[MSG_CC]," ")+"\n");
      log_file(lf,"Subject:    "+msg[MSG_SUBJECT]+"\n");
      log_file(lf,"Date:       "+ctime(msg[MSG_DATE])+"\n");
      log_file(lf,"Message-ID: "+msg[MSG_ID]+"\n");
      log_file(lf,"\n"+msg[MSG_BODY]+"\n");
    }
  }
  write("Mail dump finished\n");
  return 1;
}
/*-------------------------------------------------------------------*/
int read_mail(string str) {
  int curfoldnum,mess;
  string user;

  if (!TI || !TP || TI != TP)
    return 0;

  if (!str || str != "mail") {
    notify_fail("To read mail in the post office, use 'read mail'.\n", NOTIFY_ILL_ARG);
    return 0;
  }

  user = ({string})TP->QueryRealName();
  foldera[user] = ({0, 0});
  if (!restore_object(MAILPATH+user[0..0]+"/"+user) ||
      !sizeof(folders[0])) {
    write("*** You have no mail.\n");
    return 1;
  }

  /* list new messages if there are any, otherwise list mail folders */
  curfoldnum = member(folders[0], "newmail");
  if (curfoldnum!=-1 && folders[1][curfoldnum] &&
      sizeof(folders[1][curfoldnum]) > 0) {
    list_a_folder(curfoldnum);
    mess=sizeof(folders[1][curfoldnum]);
  }
  else {
    list_folders();
    curfoldnum=0;
  }

  write("Mail command ("+
        (mess ? "1-"+mess+" . " +
          ((mess>1) ? "+ " : "") +
          "m r R f F d " +
          ((IS_WIZARD(TP)) ? "l " : "") : ""
        ) +
        "? c D q) -> ");

  /* save profile */
  foldera[user] = ({curfoldnum, 0});
  input_to("get_menu");
  return 1;
}
/*-------------------------------------------------------------------*/
int get_menu(mixed str) {
  int curfoldnum,foldmsg,must_save,must_show,mess;
  string from_user;
  string name;

  if (!TI || !TP || TI != TP)
    return 0;

  from_user = ({string})TP->QueryRealName();
  curfoldnum = foldera[from_user][0];
  foldmsg = foldera[from_user][1];
  restore_object(MAILPATH+from_user[0..0]+"/"+from_user);

  must_save=0; must_show=0;
  /* check menu */
  if (str=="") str=".";
  if (intp(str)) {}
  else if (str==".") must_show=1;
  else if (str=="+" || str=="n") {
    if (foldmsg+1<sizeof(folders[1][curfoldnum])) {
      must_save=1;
      must_show=(str=="+");
      foldmsg++;
    }
    else write("You have reached the end of this folder.\n");
  }
  else if (str=="-" || str=="p") {
    if (foldmsg) {
      must_save=1;
      must_show=(str=="-");
      foldmsg--;
    }
    else write("You have reached the beginning of this folder.\n");
  }
  else if (str=="?") print_help_menu();
  else if (str=="r")
  {
    set_status(MSG_STATUS_ANSWERED,curfoldnum,foldmsg,from_user);
    reply_mail(0,curfoldnum,foldmsg);
    return 1;
  }
  else if (str=="R")
  {
    set_status(MSG_STATUS_ANSWERED,curfoldnum,foldmsg,from_user);
    reply_mail(1,curfoldnum,foldmsg);
    return 1;
  }
  else if (str=="f")
  {
    set_status(MSG_STATUS_ANSWERED,curfoldnum,foldmsg,from_user);
    reply_mail(2,curfoldnum,foldmsg);
    return 1;
  }
  else if (str=="F")
  {
    set_status(MSG_STATUS_ANSWERED,curfoldnum,foldmsg,from_user);
    reply_mail(3,curfoldnum,foldmsg);
    return 1;
  }
  else if (sscanf(str,"f %s",name)==1)
  {
    set_status(MSG_STATUS_ANSWERED,curfoldnum,foldmsg,from_user);
    reply_mail(2,curfoldnum,foldmsg,name);
    return 1;
  }
  else if (sscanf(str,"F %s",name)==1)
  {
    set_status(MSG_STATUS_ANSWERED,curfoldnum,foldmsg,from_user);
    reply_mail(3,curfoldnum,foldmsg,name);
    return 1;
  }
  else if (str=="d") {
    mess=sizeof(folders[1][curfoldnum]);
    if (mess) {
      mess--;
      if (!mess) {                      /* aha, folder is empty then */
        /*folders[1][curfoldnum]=({});*/
        /*save_object(MAILPATH+from_user[0..0]+"/"+from_user);*/
        return get_menu("D");           /* delete the folder */
      }
      else if (!foldmsg)                /* delete first message */
        folders[1][curfoldnum]=folders[1][curfoldnum][1..mess];
      else if (foldmsg==mess) {         /* delete last message */
        folders[1][curfoldnum]=folders[1][curfoldnum][0..mess-1];
        foldmsg--;
        must_save=1;
      }
      else                              /* delete message between */
        folders[1][curfoldnum]=folders[1][curfoldnum][0..foldmsg-1] +
                               folders[1][curfoldnum][foldmsg+1..mess];
      MAILER->save(from_user,folders);
    }
  }
  else if (str=="D") {
   mess=(folders&&folders[0]) ? sizeof(folders[0])-1 : 0;
    if (!mess) {
      folders=({ ({ }), ({ }) });
    }
    else if (!curfoldnum) {
      folders[0]=folders[0][1..mess];
      folders[1]=folders[1][1..mess];
    }
    else if (curfoldnum==mess) {        /* delete last folder */
      curfoldnum--;
      folders[0]=folders[0][0..curfoldnum];
      folders[1]=folders[1][0..curfoldnum];
    }
    else {
      folders[0]=folders[0][0..curfoldnum-1] +
                 folders[0][curfoldnum+1..mess];
      folders[1]=folders[1][0..curfoldnum-1] +
                 folders[1][curfoldnum+1..mess];
    }
    foldmsg=0;
    must_save=1;
    MAILER->save(from_user,folders);
  }
  else if (str=="i") {}
  else if (str=="q") return 1;
  else if (sscanf(str,"c %s",name)==1) {
    mess=member(folders[0], name);
    if (mess==-1) write("Folder does not exist.\n");
    else {
      foldmsg=0;
      curfoldnum=mess;
      must_save=1;
    }
  }
  else if (sscanf(str,"m %s",name)==1) {
    mess=member(folders[0], name);
    if (mess==-1) {
      mess=sizeof(folders[0]);
      folders[0]+=({name});
      folders[1]+=({ ({ }) });
    }
    if (mess==curfoldnum) write("This mail is already in the folder.\n");
    else {
      folders[1][mess]+=({ folders[1][curfoldnum][foldmsg] });
      must_save=1;

      mess=sizeof(folders[1][curfoldnum]);
      if (mess) {
        mess--;
        if (!mess) {                      /* aha, folder is empty then */
          /*folders[1][curfoldnum]=({});*/
          MAILER->save(from_user,folders);
          return get_menu("D");           /* delete the folder */
        }
        else if (!foldmsg)                /* delete first message */
          folders[1][curfoldnum]=folders[1][curfoldnum][1..mess];
        else if (foldmsg==mess) {         /* delete last message */
          folders[1][curfoldnum]=folders[1][curfoldnum][0..mess-1];
          foldmsg--;
        }
        else                              /* delete message between */
          folders[1][curfoldnum]=folders[1][curfoldnum][0..foldmsg-1] +
                                 folders[1][curfoldnum][foldmsg+1..mess];
        MAILER->save(from_user,folders);
      }
    }
  }
  else if ((IS_WIZARD(TP)) && (sscanf(str,"l %s",name)==1)) {
    /* write("Sorry, this command is not implemented in this version.\n"); */
    mixed *msgs; int i;

    name = "/players/"+from_user+"/"+name;
    msgs = folders[1][curfoldnum][foldmsg];
    write_file(name, "\nFrom:    "+capitalize(msgs[MSG_FROM])+"\n");
    if (msgs[MSG_RECIPIENT] &&
	(({string})TP->QueryRealName()!=msgs[MSG_RECIPIENT]))
      write_file(name,"To:      "+capitalize(msgs[MSG_RECIPIENT])+"\n");
    for (i=0; i<sizeof(msgs[MSG_CC]); i++)
      msgs[MSG_CC][i]=capitalize(msgs[MSG_CC][i]);
    write_file(name,"Cc:      "+implode(msgs[MSG_CC]," ")+"\n");
    write_file(name,"Subject: "+msgs[MSG_SUBJECT]+"\n");
    write_file(name,"Date:    "+ctime(msgs[MSG_DATE])+"\n");
    write_file(name,"\n"+msgs[MSG_BODY]+"\n");
  }
  else if (sscanf(str,"%d",mess)==1) {
    if (mess>0 && folders[1][curfoldnum] && mess<=sizeof(folders[1][curfoldnum])) {
      must_show=1;
      must_save=1;
      foldmsg=mess-1;
    }
    else write("Cannot find this mail.\n");
  }
  else write(" Unknown command.\n");

  if (!sizeof(folders[0])) return 1;

  /* Softbyte: mark message as read */
  if (must_show)
    set_status(MSG_STATUS_READ,curfoldnum,foldmsg,from_user);

  /* save profile */
  if (must_save)
    foldera[from_user] = ({curfoldnum, foldmsg});
  if (must_show) {
    show_mail(curfoldnum,foldmsg);
    return 1;
  }
  if (stringp(str) && str=="i") { list_folders(); }
  else if (folders[1][curfoldnum] && sizeof(folders[1][curfoldnum]))
    list_a_folder(curfoldnum);
  else list_folders();

  if (folders[1][curfoldnum]) mess=sizeof(folders[1][curfoldnum]);
  else mess=0;
  write("\nMail command ("+
        (mess ? "1-"+mess+" . " +
	 (foldmsg ? "- " : "") +
	 ((foldmsg<mess-1) ? "+ " : "") +
	 "m r R f F d " +
	 (IS_WIZARD(TP) ? "l " : "")  : ""
         ) +
	"i ? c D q) -> ");

  input_to("get_menu");

  return 1;
}

int get_menu2() {
  get_menu(1);
  return 1;
}
/*-------------------------------------------------------------------*/
void print_help_menu() {
  write(
    "The following commands are available:\n"+
    ".              show current message\n"+
    "+/n            show next message (with/without printing)\n"+
    "-/p            show previous message (with/without printing)\n"+
    "c <folder>     go to new <folder>\n"+
    "m <folder>     move current message to <folder>\n"+
    "r              reply to current message\n"+
    "R              reply to current message, include old message\n"+
    "f              forward current message\n"+
    "F              forward current message, but allow for additional text\n"+
    "d              delete current message\n"+
    "D              delete current folder\n"+
    "i              show list of folders\n"+
    "q              quit\n"+
    print_help_menu_wiz());
}
/*-------------------------------------------------------------------*/
string print_help_menu_wiz() {
  if (!(IS_WIZARD(TP))) return "";
  return "l <filename>   log current message to <filename>\n";
}
/*-------------------------------------------------------------------*/
void list_a_folder(int num) {
  int i,foldmsg;
  int sta;
  string from_user,stastr;

  from_user = ({string})TP->QueryRealName();
  foldmsg = foldera[from_user][1];

  write("\n*** Messages in folder: "+folders[0][num]+" ***\n");
  for (i = 0; i < sizeof(folders[1][num]); i++)
  {
    if (sizeof(folders[1][num][i])>MSG_STATUS)
    {
      sta=folders[1][num][i][MSG_STATUS];
      if (sta & MSG_STATUS_ANSWERED) stastr="R";
      else if (sta & MSG_STATUS_READ) stastr=" ";
      else stastr="N";
    }
    else stastr="N";

    write( stastr+" "+((i==foldmsg) ? "* " : "  " ) +
          ra(i+1,3)+" "+la(capitalize(folders[1][num][i][MSG_FROM]),12)+" "+
    la(folders[1][num][i][MSG_SUBJECT],28)+"  "+
    ctime(folders[1][num][i][MSG_DATE])+"\n");
  }
}
/*-------------------------------------------------------------------*/
void list_folders() {
  int i,curfoldnum,j;
  string from_user;

  from_user = ({string})TP->QueryRealName();
  curfoldnum = foldera[from_user][0];

  write("\n*** Mail folders: ***\n");
  for (i=0; i<sizeof(folders[0]); i++) {
    if (folders[1][i]) j=sizeof(folders[1][i]);
    else j=0;
    write(((i==curfoldnum) ? "* " : "  ") +
      la(folders[0][i],20)+ra(j,4)+"\n");
  }
}
/*-------------------------------------------------------------------*/
string la(mixed str, int n) {
  return (str+"                           ")[0..n];
}
/*-------------------------------------------------------------------*/
string ra(mixed str, int n) {
  return ("                          "+str)[<n..<1];
}
/*-------------------------------------------------------------------*/
int show_mail(int fold, int msg) {
  mixed *msgs; int i;
  string mail_text;

  if (!TI || !TP || TI != TP)
    return 0;

  msgs = folders[1][fold][msg];
  mail_text = "";
  mail_text += "\nFrom:    "+capitalize(msgs[MSG_FROM])+"\n";
  if ((msgs[MSG_RECIPIENT]) &&
      (({string})TP->QueryRealName()!=msgs[MSG_RECIPIENT]))
    mail_text += "To:      "+capitalize(msgs[MSG_RECIPIENT])+"\n";
  for (i=0; i<sizeof(msgs[MSG_CC]); i++)
    msgs[MSG_CC][i]=capitalize(msgs[MSG_CC][i]);
  mail_text += "Cc:      "+implode(msgs[MSG_CC]," ") + "\n";
  mail_text += "Subject: "+msgs[MSG_SUBJECT] + "\n";
  mail_text += "Date:    "+ctime(msgs[MSG_DATE]) + "\n";
  mail_text += "\n" + msgs[MSG_BODY] + "\n";
  smore(mail_text,({int})TP->QueryPageSize(),TO,"get_menu2");
  return 1;
}
/*-------------------------------------------------------------------*/
varargs int reply_mail(int mode, int fold, int num, string towhom) {
  string from_user;
  string subject;
  string body;
  mixed *msg;

  if (!folders[1][fold] || !sizeof(folders[1][fold])) {
    write("Impossible.\n");
    get_menu(1);
    return 1;
  }

  msg=folders[1][fold][num];
  from_user = ({string})TP->QueryRealName();
  if (mode<2) subject="Re: ";
  else subject="Fw: ";
  subject += msg[MSG_SUBJECT];
  if (!mode) body="";
  else if (mode==1) body="> --- Replying mail: --------------------------\n";
  else body="> --- Forwarding mail: ------------------------\n";
  if (mode) {
    int i; string *b;
    b=explode(msg[MSG_BODY],"\n");
    for (i=0; i<sizeof(b); i++)
      b[i]="> "+b[i];
    body += implode(b,"\n")
            + "\n> ---------------------------------------------\n";
  }
  drafta[from_user] = ({mode, subject, body});
  if (mode<2) {
    reply(msg[MSG_FROM]);
  }
  else {
    if (towhom) reply(towhom);
    else {
      write("Forward to: ");
      input_to("reply");
    }
  }
  return 1;
}
/*-------------------------------------------------------------------*/
int reply(string user) {
  string from_user;
  string subject;
  string body;
  int mode;

  if (!TI || !TP || TI != TP)
    return 0;

  if (!user) {
    write("Mail whom?\n");
    drafta[from_user] = 0;
    get_menu(1);
    return 1;
  }

  from_user = ({string})TP->QueryRealName();
  mode = drafta[from_user][0];
  subject = drafta[from_user][1];
  body = drafta[from_user][2];
  user = lower_case(user);
  if (!({mixed *})MASTER->find_userinfo(user)) {
    write("There is no player named "+user+".\n");
    drafta[from_user] = 0;
    get_menu(1);
    return 1;
  }

  drafta[from_user] = ({user, subject, body});

  if (mode==2) {
    write("Cc: ");
    input_to("get_cc");
  }
  else {
    write("Give message.  Finish message with '**', or '~q' to cancel\n");
    write("]");
    input_to("get_body");
  }
  return 1;
}
/*-------------------------------------------------------------------*/
int help_mail(string str) {
  if (str == "mail" || str == "dmail" || id(str)) {
    write("\
mail <player>\tsend a mail to the <player>\n\
dmail <domain>\tsend mail to all members of the <domain>\n");
    return 1;
  }
  return 0;
}
