/*--------------------------------------------------------------------------
 * /mail/inet_mailer.c -- The intermud mailer daemon
 *
 * Copyright (C) 1997 by One Step Beyond.
 * All Rights Reserved.
 * Permission granted for non-commercial use.
 *
 * Originally written by Alvin@Sushi
 *--------------------------------------------------------------------------
 */

#include <inetd.h>
#include <daemons.h>
#include <msgclass.h>

#pragma strict_types

mapping spool_item;

nosave string *spool;

void create() {
  seteuid(getuid());
}

private int match_mud_name(string mudname, string match_str) {
  return mudname[0..sizeof(match_str)-1] == match_str;
}

static void save_spool_item() {
  string name;
  int count;

  if(!spool_item || !mappingp(spool_item) || spool_item==([]))
    return;

  do {
    ++count;
    name=spool_item[UDPMS_DEST]+"-"+to_string(count);
  } while(spool && member(spool, name)!=-1);

  save_object(UDPM_SPOOL_DIR+name);

  if(!spool || !sizeof(spool))
    spool = ({ name });
  else
    spool += ({ name });
}

/* forward declaration */
void deliver_mail(string recipient,string mud,string from,
	string subj,string mail_body,int status_flag,string spool_name);

/* forward declaration */
static void start_retry_callout();

static void remove_from_spool(string spool_file) {
  int idx;

  if(spool && (idx=member(spool, spool_file))!=-1) {
    spool -= ({ spool_file });
    if(!sizeof(spool))
      spool=({});
  }

  if(file_size("/"+UDPM_SPOOL_DIR+spool_file+".o")>0)
    if(!rm("/"+UDPM_SPOOL_DIR+spool_file+".o"))
      log_file(INETD_LOG_FILE,"UPD_MAIL: Can't delete spool file "+
	       "/"+UDPM_SPOOL_DIR+spool_file+".o");
}

static void retry_send() {
  int i;
  string msg;

  if(!spool || !sizeof(spool)) return;

  for(i=0;i<sizeof(spool);++i) {
    if(!restore_object(UDPM_SPOOL_DIR+spool[i])) {
      log_file(INETD_LOG_FILE,"inet_mailer: Falied to restore spool file "+
	       UDPM_SPOOL_DIR+spool[i]);
      continue;
    }

    if(time() - spool_item[UDPMS_TIME] > UDPM_SEND_FAIL*60) {
      msg="Reason: Unable to connect to site \""+spool_item[UDPMS_DEST]+
	"\"\n\nINCLUDED MESSAGE FOLLOWS :-\n\n"+
	"To: "+spool_item[UDPMS_TO]+"\n"+
	"Subject: "+spool_item[UDPMS_SUBJECT]+"\n"+
	spool_item[UDPMS_BODY];

      MAILER_D->deliver_mail(spool_item[UDPMS_FROM],	/* TO */
			     "Mailer@"+LOCAL_NAME,	/* FROM */
			     "Bounced Mail",		/* SUBJECT */
			     msg			/* MAIL BODY */
			     );
      remove_from_spool(spool[i]);
      return;
    }

    deliver_mail(spool_item[UDPMS_TO],
		 spool_item[UDPMS_DEST],
		 spool_item[UDPMS_FROM],
		 spool_item[UDPMS_SUBJECT],
		 spool_item[UDPMS_BODY],
		 UDPM_STATUS_IN_SPOOL,
		 spool[i]);
  }

  start_retry_callout();
}

static void start_retry_callout() {
  if(find_call_out("retry_send")!= -1 ) return;

  call_out("retry_send",UDPM_RETRY_SEND*60);
}

static void failed_to_deliver(mapping data) {
  string msg;
  object obj;

  if(!data[I2H_SYSTEM] || data[I2H_SYSTEM] != I2S_TIME_OUT) {
    msg="Reason: Error in connection to remote site \""+
      data[I2H_NAME]+"\"\n\n"+
      "INCLUDED MESSAGE FOLLOWS :-\n\n"+
      "To: "+data[I2H_RECIPIENT]+"\n"+
      "Subject: "+data[UDPM_SUBJECT]+"\n"+data[I2H_DATA];

    MAILER_D->deliver_mail(data[UDPM_WRITER],	        /* TO */
			   "Mailer@"+LOCAL_NAME,	/* FROM */
			   "Bounced Mail",		/* SUBJECT */
			   msg			        /* MAIL BODY */
			   );
    return;
  }

  /* OK transmission timed out.. place in mail spool */

  if((obj=find_player(data[UDPM_WRITER]))) {
    msg_object( obj, CMSG_GENERIC,
		"Mail delivery to "+data[I2H_RECIPIENT]+"@"+
		data[I2H_NAME]+" timed out. Placing mail in spool.\n");
  }

  spool_item=([
	       UDPMS_TIME:	time(),
	       UDPMS_TO:	data[I2H_RECIPIENT],
	       UDPMS_DEST:	data[I2H_NAME],
	       UDPMS_FROM:	data[UDPM_WRITER],
	       UDPMS_SUBJECT:	data[UDPM_SUBJECT],
	       UDPMS_BODY:	data[I2H_DATA]
  ]);

  save_spool_item();

  start_retry_callout();
}

static void get_pending_deliveries() {
  string *entries;
  int i;

  entries=get_dir(UDPM_SPOOL_DIR+"*.o");
  if(!entries || !sizeof(entries)) return;

  spool=allocate(sizeof(entries));
  for(i=0;i<sizeof(entries);++i)
    spool[i]=entries[i][0..<3];

  start_retry_callout();
}

void reset(int started) {
  if(started) return;

  get_pending_deliveries();
}

/*
 * Public routines
 */

int query_valid_mail_host(string hostname) {
  string *match;

  match = filter( m_indices( ({mapping})INET_D->query(I2RQ_HOSTS) ),
                        #'match_mud_name /*'*/,lower_case(hostname) );

  return sizeof(match)==1;
}

void deliver_mail(string recipient,string mud,string from,
		  string subj,string mail_body,int status_flag,
		  string spool_name) {
  mapping data;
  string msg;
  object pl;

  data=([
	 I2H_REQUEST: "mail",
	 I2H_RECIPIENT: recipient,
	 I2H_SENDER: this_object(),
	 UDPM_STATUS: status_flag,
	 UDPM_WRITER: lower_case(from),
	 UDPM_SUBJECT: subj,
	 UDPM_SPOOL_NAME: spool_name,
	 I2H_DATA: mail_body
  ]);

  if ( msg = ({string})INET_D->send_udp(mud,data,1) ) {
    msg="Reason: "+msg+"\n"+
      "INCLUDED MESSAGE FOLLOWS :-\n\n"+
      "To: "+data[I2H_RECIPIENT]+"@"+mud+"\n"+
      "Subject: "+data[UDPM_SUBJECT]+"\n"+data[I2H_DATA];

    MAILER_D->deliver_mail( data[UDPM_WRITER],	/* TO */
			    "Mailer@"+LOCAL_NAME,	/* FROM */
			    "Bounced Mail",		/* SUBJECT */
			    msg			/* MAIL BODY */
			    );
  }
}

void udp_reply(mapping data) {
  object sender;

  switch(data[UDPM_STATUS]) {
  case UDPM_STATUS_TIME_OUT:
    failed_to_deliver(data);
    break;

  case UDPM_STATUS_DELIVERED_OK:
    if((sender=find_player(data[UDPM_WRITER]))) {
      msg_object( sender, CMSG_GENERIC,
		  "Mailer@"+data[I2H_NAME]+": "+
		  "Mail to "+capitalize(data[I2H_DATA])+" delivered ok.\n");
    }
    if(data[UDPM_SPOOL_NAME])
      remove_from_spool(data[UDPM_SPOOL_NAME]);

    break;

  case UDPM_STATUS_UNKNOWN_PLAYER:
    MAILER_D->deliver_mail(data[UDPM_WRITER],	        /* TO */
			   "Mailer@"+data[I2H_NAME],	/* FROM */
			   "Bounced Mail",		/* SUBJECT */
			   data[I2H_DATA]		/* MAIL BODY */
	);
    if(data[UDPM_SPOOL_NAME])
      remove_from_spool(data[UDPM_SPOOL_NAME]);
    break;

  case UDPM_STATUS_IN_SPOOL:
    /* Do nothing */
    break;
  }
}

/* ********************************************************************** */

