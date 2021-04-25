/*--------------------------------------------------------------------------
 * /p/money/obj/designer/file.c -- part of the money designer
 *
 * Copyright (C) 1997 by One Step Beyond.
 * All Rights Reserved.
 * Permission granted for non-commercial use.
 *--------------------------------------------------------------------------
 */

#include <search.h>
#include <money.h>

#define THIS this_object()
#define USER (this_player() == environment()?"":\
             sprintf("%O's designer: ",environment()))

static mapping SetMaterials(mixed s);
static mapping SetCoinTypes(mixed s);
static string *SetDomains(mixed s);
static string *SetStandard(mixed s);

public mapping QueryMaterials();
public mapping QueryCoinTypes();
public mapping QueryFuns();
public int     QueryLastAction();
public string *QueryStandard();
public string *QueryDomains();

static string backup;

public int save_design(string str)
{
  if(!THIS->security()) return 0;
  if((!str) || str = "money" || str = "design") {
    if(!QueryCoinTypes()) {
      notify_fail("Not started yet.\n");
      return 0;
    }
    save_object(MONEY_CONFIG);
    write("Saving new database...\n");
    if(!catch("/obj/money_demon"->remove()))
      write("Updating money demon...\n");
    else
      write("Error while updating money demon!\n");
    return 1;
  }
  notify_fail("Unrecognized argument: " + str + ".\n");
  return 0;
}

public int exit_design(string str)
{
  if(!THIS->security()) return 0;
  if(save_design(str)) {
    rm(backup);
    rm(MONEY_CONFIG + ".lock");
    write("Finishing design...\n");
    SetCoinTypes(0);
    SetMaterials(0);
    SetStandard(0);
    return 1;
  }
  notify_fail("Unrecognized argument: " + str + ".\n");
  return 0;
}

public int abort_design(string str)
{
  /* Here we don't check for THIS->security. If someone destructs the
   * money designer we want to have the chance to remove our settings.
   * But we allow other money designers to ask for permissions to remove
   * the lock. */

  if(this_player() && this_player() != environment() &&
     blueprint(previous_object()) == blueprint(this_object()) &&
     interactive(environment()) && query_idle(environment()) < 600
     && time() - QueryLastAction() < 600)
    return 0;
  if((!str) || str = "money" || str = "design") {
    if(!QueryCoinTypes()) {
      notify_fail("Not started yet.\n");
      return 0;
    }
    if(!backup || file_size(backup) <= 0)
      write(USER + "Attention! Missing backup!\nDesign aborted...\n");
    else {
      rm(MONEY_CONFIG + ".o");
      write_file(MONEY_CONFIG + ".o",read_file(backup));
      rm(backup);
      write(USER + "Aborting design, restoring old database...\n");
      if(environment() != this_player() && interactive(environment()))
	tell_object(environment(),
	  sprintf("%O forces you to stop design. All changes are lost.\n",
		  this_player()||"Gamedriver Cleanup"));
    }
    rm(MONEY_CONFIG + ".lock");
    SetCoinTypes(0);
    SetMaterials(0);
    SetStandard(0);
    return 1;
  }
  notify_fail("Unrecognized argument: " + str + ".\n");
  return 0;
}

static void restore_database() { restore_object(MONEY_CONFIG); }

public int load_database(string str)
{
  object user, *designer;
  
  if(!THIS->security()) return 0;
  if(str && str[0..3] != "data") {
    notify_fail("** Usage: [design] " + QueryFuns()["load",1] + "\n");
    return 0;
  }
  if(QueryCoinTypes()) {
    notify_fail("Data already loaded.\n");
    return 0;
  }
  if(file_size(MONEY_CONFIG + ".lock") >= 0) { 
    write("Checking for locks...\n");
    if(!(user = find_object("/" + explode(read_file(MONEY_CONFIG +
						    ".lock"),",")[0]))) {
      write("Removing lock file of a not existing user...\n");
      rm(MONEY_CONFIG + ".lock");
    } else {
      write("Checking for the user...\n");
      if(!sizeof(designer = search(user,"money designer",SM_LOCATE))) {
	write("Removing lock file of an user without designer...\n");
	rm(MONEY_CONFIG + ".lock");
      } else {
	write("Checking, if the user is still active...\n");
	designer = filter(designer,
				lambda(({'ob,'file}),
				       ({#'==,'file,({#'blueprint,'ob})})),
				blueprint(this_object()));
	if(!sizeof(filter(designer,#'call_other,"abort_design"))) {
	  write("The config file is locked currently by " +
		read_file(MONEY_CONFIG + ".lock"));
	  return 1;
	}
        write("Removing lock of an inactive user...\n");
      }
    }
  }
  write_file(backup =
	     MONEY_CONFIG + ".o." +
	     implode(explode(implode(explode(ctime(time())," "),"."),
			     ".."),".")+".old",
	     read_file(MONEY_CONFIG + ".o"));
  restore_database();
  write_file(MONEY_CONFIG + ".lock",capitalize(getuid(this_player())) +
	     ", " + ctime(time()) + "\n");
  write("Loading money data from config file...\n");
  return 1;
}
public int design_money(string str)
{
  string cmd, arg, *tmp;

  if(!THIS->security()) return 0;
  if(!str || str == "money") return load_database(0);
  cmd = (tmp = explode(str," "))[0];
  arg = implode(tmp[1..]," ");
  if(QueryFuns()[cmd]) return call_other(this_object(),QueryFuns()[cmd],(arg == "")?0:arg);
  notify_fail("Unknown arguments: " + str + ".\n");
  return 0;
}
