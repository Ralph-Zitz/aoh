/*
Anzeige des /log/<domain>.rep einbauen
*/
/*
  This is the standard office to be inheritet for the special domain offices.

In a normal office stands a board to discuss the topics of the department.
Also there are placed rules or lists in some offices. A stairway leads
down to some examples rooms related to the office task.

Call the following functions in the create of your office, after
calling '::create' to configure your office:

  - SetOfficeName(string name)

  - SetDomainDirectory(string subdir_name)
      Give the subdir name, e.g. "testdomain" for "/d/testdomain".
      This will also be given as default-domain to the domaintool provided
      in the office.
  07-10-94 [c] removed the Board from this room
  
   [c] Cadra
*/

inherit "/std/room";

#include <config.h>;
#include <secure/wizlevels.h>;

#include <secure/domaininfo.h>

#define DOMAIN_PATH "/d/"

#define PL this_player()
#define PL_RNAME this_player()->QueryRealName()

string  *visits;
string  desc_file, theme_file;

static  string  office_name, domaindir, domainsubdir;
static  string* wiznames;


void SetOfficeName(string name)  {  office_name = name; }


  /* We can't simply clone a deposit a dtool since then the
  ** uid's won't be right.
  */
int clone_dtool (string arg) {
  object dtool;
  if (!arg || arg != "dtool" && arg != "domaintool") return 0;
  if (PL->command_me("clone /obj/tool/domaintool")) {
    write ("You receive a domaintool.\n");
    say (PL->QueryName()+" takes something out of the air.\n");
    if (domainsubdir && (dtool = present ("dtool", PL)) )
      dtool->Set("def_domain", domainsubdir);
  }
  else
    write ("Funny, Nightfall is short of domaintools.\n");
  return 1;
}

void SetDomainDirectory(string subdir)   {
  domainsubdir = subdir;
  domaindir = DOMAIN_PATH + subdir + "/";
}


string QueryIntShort() {  return("Office of the " + office_name); }


/* Checks if PL is a Lord. */
int check_lord() {
  int size;
  if(!PL) return 0;
  if(wiznames) size = sizeof(wiznames);
  else size = 0;
  return(   (size > 0) && (lower_case((string)PL_RNAME) == lower_case(wiznames[0]))
         || (size > 1) && (lower_case((string)PL_RNAME) == lower_case(wiznames[1]))
        );
}


string lformat(string str, int len) {
  return sprintf("%-*s", len, str);
}


string _return_memberlist() {
  string rc, wname;
  mixed *ddata;
  int state, size, i, iv, what;

  if (!domainsubdir) return "This office has no domaindirectory.\n";

  state = (int) DOMAININFO->QueryState();
  rc = 0;
  if (state == DI_INITING)
    rc = "The memberlist is quite blurry and achanging.\n";
  if (state == DI_INITED)
    rc = "Members of "+capitalize(domainsubdir)+" are:\n";
  if (!rc) return "The memberlist didn't materialize yet.\n";

  ddata = (mixed *) DOMAININFO->QueryDomain(domainsubdir);
  if (!ddata || !sizeof(ddata))
    return "  <No information available>\n";
  ddata = ddata[DD_WIZ];

  if (!(size = sizeof(ddata[DW_NAME]))) {
    rc += "  There are no Wizards responsible for this Domain!\n";
    rc += "  We need Wizards!!!\n";
    return rc;
  }

  for (what = D_LORD; what >= D_MEMBER; what--) {
    for (i = 0; i < size; i++) {
      if (ddata[DW_DSTAT][i] != what) continue;
      switch (what) {
      case D_LORD  : rc += "  Domain Lord "; break;
      case D_VICE  : rc += "  Vice Lord   "; break;
      case D_MEMBER: rc += "  Wizard      "; break;
      default      : rc += "  Unknown     ";
      }
      rc += lformat (capitalize (ddata[DW_NAME][i])+" ",15);
      wname = lower_case (ddata[DW_NAME][i]);
      for (iv = 0; iv < sizeof(visits); iv += 2)
        if (wname == visits[iv]) {
          rc += "[" + visits[iv + 1] + "]";
          break;
        }
      rc += "\n";
    }
  }

  if (size < 3)
    rc += "There are more Wizards needed for this Domain!\n";

  return rc;
}


string QueryIntLong(int arg) {

  string  desc, wizzes;
  int     state, size;

  state = (int) DOMAININFO->QueryState();
  wizzes = "There is a faint glow at the place of the memberlist.\n";
  if (state == DI_INITING)
    wizzes = "There seems to be a memberlist hanging from the wall.\n";
  if (state == DI_INITED)
    wizzes = "A memberlist hangs from the wall.\n";

  desc = "You are in the Office of the " + office_name + ".\n";
  if((!catch(size = file_size(desc_file))) && (size > 0))
    desc += read_file(desc_file);
  desc += "For your convenience you can 'summon' a domaintool.\n";
  desc += "There is a big sign on a wall with the theme/description of the domain.\n" +
          "You can look at it or read it...\n";
  desc += wizzes;
  return(desc);
}


void init() {

  int     i;

  ::init();
  add_action("set_officedesc",    "officedesc");
  add_action("set_domaintheme",   "domaintheme");
  add_action("clone_dtool",       "summon");

/* Remove this later ... */
  add_action("read",      "read");

  if(PL && query_ip_name(PL)) {    /* Check on player */
    if(wiznames && check_lord())
      write("You can use 'officedesc <path>' and 'domaintheme <path>'\n");
    if (!visits) visits = ({});
    for(i = 0; i < sizeof(visits); i += 2) {
      if(lower_case((string)PL_RNAME) == lower_case(visits[i])) {
        visits[i+1] = ctime(time())[0..9];
        save_object(object_name(this_object()));
      }
    }
  }
}


int set_officedesc(string path) {
  if(check_lord()) {
    desc_file = path;
    write("Office description set to file '" + desc_file + "'.\n");
    save_object(object_name(this_object()));
    return(1);
  }
}


int set_domaintheme(string path) {
  if(check_lord()) {
    theme_file = path;
    write("Domain description sign set to file '" + theme_file + "'.\n");
    save_object(object_name(this_object()));
    return(1);
  }
  notify_fail("You are not allowed to do that.\n");
}


varargs string give_sign(int more) {
  int size;

  if((!catch(size = file_size(theme_file))) && (size > 0)) {
    if(more && PL) {
      call_other(PL, "more", theme_file);
      return("");
    }
    else return(read_file(theme_file));
  }
  else return(
"There is nothing written on the sign. Ask the Domain Lords to write\n" +
"something on it.\n"
             );
}


varargs int read(string str) {
  if(!str || lower_case(str) != "sign") return(0);
  write(give_sign(1));
  return(1);
}


void set_domain_wiz(string *wizzes) {
  int     iw, iv, ok;

  wiznames = wizzes;

  /* Compare with saved wiznames in visits.           */
  /* New Archwizzes will be added, old AW not erased. */
  if(!visits) visits = ({ });
  for(iw = 0; iw < sizeof(wiznames); iw++) {
    ok = 0;
    for(iv = 0; !ok && iv < sizeof(visits); iv += 2)
      if(lower_case(visits[iv]) == lower_case(wiznames[iw]))
        ok = 1;
    if(!ok) visits = ({ lower_case(wiznames[iw]), "" }) + visits;
  }
}


int check_lords(string name) {
  return(   (int)(MASTER)->domain_master(name, domainsubdir)
         || (int)(MASTER)->domain_vice(name, domainsubdir)
        );
}


int check_dirs(string name) {
  return(file_size(domaindir + "/" + name) == -2 
          && (name != "common") && (name[0] != '.'));
}


void create() {
  string *wiz;
  string *lords;
  string tmpname;

  ::create();

  /* SetIntLight(1); */
  AddDetail("sign", #'give_sign);
  AddDetail( ({ "list", "memberlist" }), #'_return_memberlist);


  if(domaindir) {
    wiz = get_dir(domaindir + "*******");
    if(wiz) {
      wiz = filter(wiz, "check_dirs", this_object());
      /* get lords */
      lords = filter(wiz, "check_lords", this_object());                        /* lords nach 'vice' sortieren */
      if(lords) {
        /* sort Lord and Vice */
        if(sizeof(lords) > 1 && MASTER->domain_vice(lords[0])) {
          tmpname = lords[1];
          lords[1] = lords[0];
          lords[0] = tmpname;
        }
        /* wiz - lords   |   to get the right order */
        wiz = wiz - lords;
        /* wiz = lords + wiz   |   to get the right order */
        wiz = lords + wiz;
      }
    }
  }

  tmpname = object_name(this_object());
  if (file_size(tmpname+".o") > 0) restore_object(tmpname);
  if(!wiz) wiz = ({ });
  set_domain_wiz(wiz);
  if(!visits) visits = ({ });
}

