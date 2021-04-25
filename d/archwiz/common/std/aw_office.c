/*
  This is the standard office to be inherited
  for the special archwiz offices.

  In a normal office stands a board to discuss the topics of the department.
  Also there are placed rules or lists in some offices. A stairway leads
  down to some examples rooms related to the office task.

  Call the following functions in the create of your office, after
  calling '::create' to configure your office:

  - SetOfficeName(string name)

  - SetOfficeDesc(string desc)

  - SetArchwiz(string *wiznames)
    Array of archwiz and vices, the first is the archwiz,
    (after each name maybe the last time the wizard visited this room)

  07-10-94 [c] Cleaned the file from the CreateBoard and so on

  [c] Cadra
*/

inherit "/std/room";

#include <secure/wizlevels.h>


string 	*visits;

static 	string	office_name, office_desc, *wiznames;


void SetOfficeName(string name)	 { office_name = name; }


void SetOfficeDesc(string desc)	 { office_desc = desc; }


void SetArchwiz(string *wizzes) {
  int 	iw, iv, ok;

  wiznames = wizzes;

  /* Compare with saved wiznames in visits.           */
  /* New Archwizzes will be added, old AW not erased. */

  if(!visits) visits = ({ });
  for(iw = 0; iw < sizeof(wiznames); iw++) {
    ok = 0;
    for(iv = 0; !ok && iv < sizeof(visits); iv += 2) {
      if(lower_case(visits[iv]) == lower_case(wiznames[iw])) ok = 1;
    }
    if(!ok) {
      visits = ({ lower_case(wiznames[iw]), "" }) + visits;
    }
  }
}

string QueryIntShort() {  return("Archwiz office of " + office_name); }


string lformat(string str, int len) {
  return sprintf("%-*s", len, str);
}


varargs string QueryIntLong(string arg) {
  string 	desc, wizzes;
  int 	ind, iv;

  if(!wiznames || !sizeof(wiznames)) {
    wizzes = "There is no Archwizard responsible for this Department!\n";
  }
  else {
    if(sizeof(wiznames) > 1)
      wizzes = "The following Archwizards are responsible for this Department:\n";
    else
      wizzes = "The following Archwizard is responsible for this Department:\n";
    for(ind = 0; ind < sizeof(wiznames); ind++) {
      if(ind == 0)
	wizzes += "Archwiz ";
      else
	wizzes += "Vice    ";
      wizzes += lformat(capitalize(wiznames[ind]) + " ", 15);
      for(iv = 0; iv < sizeof(visits); iv += 2) {
	if(lower_case(wiznames[ind]) == visits[iv]) {
	  wizzes += "[" + visits[iv + 1] + "]";
	}
      }
      wizzes += "\n";
    }
  }
  desc = "You are in the Office of " + office_name + ".\n" +
    office_desc +
      wizzes;
  return process_string(desc);
}


void init() {
  int 	i;
  ::init();
  if(this_player() &&
     query_ip_name(this_player()) &&    /* Check on player */
     IS_ARCH(this_player())) {
    for(i = 0; i < sizeof(visits); i += 2) {
      if(lower_case((string)this_player()->QueryRealName()) ==
	 lower_case(visits[i])) {
	visits[i+1] = ctime(time())[0..9];
	save_object(object_name(this_object()));
      }
    }
  }
}


void create() {
  ::create();
  restore_object(object_name(this_object()));
  if(!visits) visits = ({ });
}
