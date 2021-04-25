/*
******************************************************************************
** The mail demon. Receives mail from users and delivers it into
** the mail directory.
**
** Creation: Nightfall
** Changes:
**  06Jun97 [Mica@OSB] added intermud mail
**  09Oct98 [Softbyte@OSB] Newmail support
**
******************************************************************************
**
**  Save file format (sort of formal notation)
**
**  mixed *folders = (
**     ({ string name1; string name2; ... string nameN; }
**     ({ mixed *msgs1; mixed *msgs2; ... mixed *msgsN; }
**  }
**
**  Each msgs field is an array of messages
**
**  mixed *msgs = ({ mixed *message1; ... mixed *messageM }
**
**  A message is represented as an array with the following fields
**
**  mixed *message = (
**     string from
**     string sender
**     string recipient
**     string *cc
**     string *bcc
**     string subject
**     string date
**     string id
**     string body
**     int    status
**   )
**
**  The mailer demon (/secure/mailer, or /obj/mailer) provides
**  the following functions:
**
**  string *DeliverMail(mixed *message
**    Hand a mail message over to the mailer demon. The mailer
**    demon extracts recipients from the recipient, cc and bcc
**    fields and removes the bcc information. It then deposits
**    the message to the mail files of all recipients. A valid
**    message is shown above. Returns a list of successfull
**    delivered recipients.
**
**  int FingerMail(string user)
**    Gives the number of unread messages a user has
******************************************************************************
*/

#include <mail.h>
#include <daemons.h>
#include <driver/object_info.h>

/*
******************************************************************************
** Globals
******************************************************************************
*/

mixed *folders; /* used for save and restore of mail file */

/*
******************************************************************************
** Prototypes
******************************************************************************
*/

static void save_msg( mixed * msg, mixed user);

/*
******************************************************************************
** Creation / prevent shadowing
******************************************************************************
*/

public void create() {
  seteuid(getuid());
}

public int query_prevent_shadow() {
  return 1;
}

/*
******************************************************************************
** Checks to see if player <name> exists. Return 0 if not
**  this is used by INETD_CMD_DIR/mail too
******************************************************************************
*/

public int query_recipient_ok(string name) {
  return (file_size(SAVEPATH+name[0..0]+"/"+name+".o") > 0);
}

/*
******************************************************************************
** DeliverMail - main delivery function
******************************************************************************
*/

public mixed * DeliverMail( mixed * msg ) {
  string sender, *recok, reciuser, recimud;
  mixed *newmsg, *recipients;
  int i;
  object pl;

  /* dont accept invalid formats */

  if (!pointerp(msg) || sizeof(msg) != 9) return 0;

  /* determine the real sender */

  sender = ({string})MASTER->creator_file(previous_object());

  /* make a list of all recipients */

  if (msg[MSG_RECIPIENT])
    recipients = ({ msg[MSG_RECIPIENT] });
  else
    recipients = ({});
  recipients += msg[MSG_CC]+msg[MSG_BCC];

  /* build the new message */

  newmsg = ({ msg[MSG_FROM],
              sender,
              msg[MSG_RECIPIENT],
              msg[MSG_CC],
              "",
              msg[MSG_SUBJECT],
              time(),
              MUDNAME ":"+time(),
              msg[MSG_BODY]
           });

  /* sent if off */

  recok = ({ });
  for (i = 0; i < sizeof(recipients); i++) {
    recipients[i] = lower_case( implode( explode( recipients[i], " " ),"" ) );
    recipients[i] = lower_case( implode( explode( recipients[i], "." ), "" ) );

    if ( recipients[i] ) {
      if ( sscanf( recipients[i], "%s@%s", reciuser, recimud ) == 2 ) {
	if ( member(recok, recipients[i]) == -1 ) {
	  call_other( INET_MAILER_D, "deliver_mail",
		      reciuser,
		      recimud,
		      newmsg[MSG_FROM],
		      newmsg[MSG_SUBJECT],
		      newmsg[MSG_BODY] );
	  recok += ({ recipients[i] });
	}
      }
      else if ( query_recipient_ok( recipients[i] ) ) {
	if ( member(recok, recipients[i]) == -1 ) {
	  save_msg(newmsg,recipients[i]);
	  recok += ({ recipients[i] });
	  pl=find_player(recipients[i]);
	  if (pl && query_once_interactive(pl) && geteuid(pl) == recipients[i])
	    tell_object(pl,"*** Postman: you received mail.\n");
	}
      }
      else
	write("Cannot deliver mail to player '"+recipients[i]+"'.\n");
    }
  }

  return recok;
}

/*
******************************************************************************
** FingerMail - return the number of new mails for the given user
******************************************************************************
*/

public int FingerMail( mixed user ) {
  int newfolder;

  if ( objectp( user ) )
    user = ({string})user->QueryRealName();

  if (!restore_object(MAILPATH+user[0..0]+"/"+user)) return 0;
  newfolder = member(folders[0], "newmail");
  if (newfolder == -1) {
    return 0;
  }
  return sizeof(folders[1][newfolder]);
}

/*
******************************************************************************
** FingerUnreadMail - return the number of unread mails for the given user
******************************************************************************
*/

public int FingerUnreadMail( mixed user ) {
  int newfolder;
  int i,cnt;
  mixed *msgs;


  if ( objectp( user ) )
    user = ({string})user->QueryRealName();

  if (!restore_object(MAILPATH+user[0..0]+"/"+user)) return 0;
  newfolder = member(folders[0], "newmail");
  if (newfolder == -1) {
    return 0;
  }

  cnt=0;
  for (i=0;i<sizeof(folders[1][newfolder]);i++)
  {
    msgs = folders[1][newfolder][i];
    if (sizeof(msgs)<=MSG_STATUS)
    {
      cnt++;
    }
    else if ( (msgs[MSG_STATUS]&MSG_STATUS_READ)==0)
    {
      cnt++;
    }

  }

  return cnt;
}

/*
******************************************************************************
** save a message to the users folder, save / load functions
******************************************************************************
*/

static void save_msg( mixed * msg, mixed user) {
  int newfolder;

  if ( objectp( user ) )
    user = ({string})user->QueryRealName();

  /*write(MAILPATH+user[0..0]+"/"+user+"\n");*/
  if (!restore_object(MAILPATH+user[0..0]+"/"+user)) {
    folders = ({({ }), ({ })});
  }

  /* if folder 'newmail' doesn't exist, create it */
  newfolder = member(folders[0], "newmail");
  if (newfolder == -1) {
    folders[0] += ({ "newmail" });
    folders[1] += ({ ({ }) });
    newfolder = sizeof(folders[0])-1;
  }
  folders[1][newfolder] += ({ msg });
  save_object(MAILPATH+user[0..0]+"/"+user);
}


public int save( string user, mixed folder ) {
  folders = folder;
  user = lower_case(user);
  if (this_interactive()==find_player(user)) {
    save_object("/mail/"+user[0..0]+"/"+user);
    return 1;
  }
  else
    write("You aren't permitted to save such a mail.\n");
  return 0;
}

public mixed * load( string user ) {
  user = lower_case(user);
  if (this_interactive()==find_player(user) ||
      (explode(object_name(this_interactive()),"#")[0] == "/secure/login" &&
       this_player() == find_player(user))) {
    if (!restore_object("/mail/"+user[0..0]+"/"+user))
      folders = ({});
    return ({}) + folders;
  }
  else
    write("Could not load requested mail.\n");
  return ({});
}

/*
******************************************************************************
** functions for intermud mail - these get called by INETD_CMD_DIR/mail.c
**  deliver_mail calls DeliverMail
******************************************************************************
*/

public void deliver_mail(
	string recipient,	/* the local players real name*/
	string from,		/* A string depicting the sender */
	string subject,		/* The mail subject/header */
	string mail_body	/* The actual mail message */
) {
  mixed * newmsg;

  /* create final message and send it off */

  newmsg = ({ from,
              "",
              recipient,
              ({}),
              ({}),
              subject,
              0,
              0,
              mail_body
           });

  DeliverMail( newmsg );
}
