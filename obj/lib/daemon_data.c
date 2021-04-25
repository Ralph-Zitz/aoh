/*--------------------------------------------------------------------------
 * daemon_data.c -- provide save-mechanism for daemons
 *
 * Copyright (C) 1997 by One Step Beyond.
 * All Rights Reserved.
 * Permission granted for non-commercial use.
 *
 * Credits: Lima Mudlib (Creation)
 *--------------------------------------------------------------------------
 * daemon_data simplifies the process of maintaining save files for daemons.
 * It automatically restores the data from the data file [if it exists] 
 * using restore_object() when the program is loaded, and provides the
 * routine save_me() to save the data.
 *
 * The filename is the name of the source file, with /save/daemons/ added
 * on to the front and the _d suffix, if any, removed from the end.
 * I.e. the file "/p/daemons/stat_d.c" will save to "/data/p/daemons/stat.o".
 *
 * TODO:  hmm, maybe this module should have a remove() routine which calls
 * TODO:: save_me() too.
 */

private nomask string create_object_name();
nomask void save_me();
nomask void restore_me();

private nomask string create_object_name() {
  string ret;

  ret = DAEMONSAVEPATH + explode(object_name( this_object() ), "/" )[<1];
  
  if (ret[<2..] == "_d")
    ret = ret[0..<3];
  
  return ret;
}

/* FUNCTION save_me
 * Save the variable data to the save file
 */

nomask void save_me() {
  save_object( create_object_name() );
}

/* FUNCTION restore_me
 * Restore the data from the save file.  Automatically called by create().
 */

nomask void restore_me() {
  string fn;

  fn = create_object_name();
  if ( file_size( fn + ".o" ) > 0 )
    restore_object( fn );
}

void create() {
  seteuid(getuid());
  restore_me();
}

