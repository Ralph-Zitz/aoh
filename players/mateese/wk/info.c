TODO: Rename IGROUP_BANISH to IGROUP_RESERVED, just one savefile for them.
TODO: addMember/removeMember: resume updaten!

/*--------------------------------------------------------------------------
 * /secure/info.c -- User information data base.
 *
 * Copyright (C) 1997 by One Step Beyond.
 * All Rights Reserved.
 * Permission granted for non-commercial use.
 *--------------------------------------------------------------------------
 * This object is the only authoritative instance about users, their
 * wizlevels, passwords, group memberships, etc.
 *
 * Besides the mere user data, the info also saves some meta entries
 * in /secure/save, e.g. one entry for each domain, to make lookups simpler.
 * However, in cases of inconsistencies the data in the user savefiles
 * takes precedence, with the exception of the meta files 'banish-a'..
 * 'banish-z'.
 * These meta files are predefined:
 *   groups  : 'member' holds the names of all other metafiles.
 *   banished: 'member' lists of all banished active users
 *   wizards : 'member' lists all known wizards.
 *   frozen  : 'member' lists all frozen users.
 *   banish-a..banish-z: 'member' lists all banished login names for which
 *                       no user exists/existed.
 *
 * For every user <name>, the savefile <name>.o is constructed as
 * I_USERPATH/<name>[0..0]/<name>.o. The savefiles for the
 * <meta> entries are constructed as I_INFOPATH/<meta>.o
 * The info object tries to create all missing subdirectories below
 * SECURESAVEPATH if necessary.
 *--------------------------------------------------------------------------
 */

#include <secure/info.h>

#define TIME_CACHE 1800  // Holdtime in [secs] of an entry in the cache

/*--------------------------------------------------------------------------*/
// These are the variables saved/restored from and to the savefiles.

string   password;  // The password.
int      level;     // The 'wizlevel'.
string * member;    // The list of memberships/members.
  // For normal users, this is the list of groups/domains they are member of.
  // For group entries, this is the list of members.
mixed  * logins;    // A collection of information about the last logins.
mixed  * resume;    // An array of the becomings of the user.
  // Each entry of this array is an array of its own, in chronologial order.
  // The format of each entry is ({ time(), <code>, ... }).
  // Defined entries are:
  //   ({ time(), IRESUME_CREATE }) 
  //      Users: first login; Groups: creation.
  //   ({ time(), IRESUME_LEVEL, oldlevel, newlevel, name }) : level change
  //      Users 'wizlevel' was changed by <name>.
  //   ({ time()  IRESUME_BANISH, newstate, name, reason })
  //      User was (un)banished by <name> for <reason>.
  //   ({ time()  IRESUME_MUMMY, newstate, name })
  //      User was (un)mummified by <name>.
  //   ({ time()  IRESUME_FROZEN, 1, name, duration })
  //      User was frozen by <name> for <duration> seconds.
  //   ({ time()  IRESUME_FROZEN, 0, name })
  //      User was deliberately unfrozen by <name>.
  //   ({ time()  IRESUME_FROZEN, 0, 0 })
  //      User melted automatically.
  //   ({ time(), IRESUME_JOIN, name })
  //      Users: User joined group <name>
  //      Groups: Users <name> joined
  //   ({ time(), IRESUME_LEAVE, name })
  //      Users: User left group <name>
  //      Groups: Users <name> left.
int bBanished; // True if name is banished for users.
int bMummy;    // True if user is exempt from auto-removal.
int tFrozen;   // If non-zero, it's the time until the character is frozen.
int bRemoved;  // Set by external program when wizard is removed.

/*--------------------------------------------------------------------------*/
// Internally, the data is held in a cache - a mapping index by the names.
// Each data is kept TIME_CACHE seconds after the last use.
// For each variable in the savefile exists one mapping data field,
// plus some additional ones for internal use.

static mapping data;

#define DATA_PWD       0  // 'password'
#define DATA_LEVEL     1  // 'level'
#define DATA_MEMBER    2  // 'member'
#define DATA_LOGINS    3  // 'logins'
#define DATA_RESUME    4  // 'resume'
#define DATA_BANISH    5  // 'bBanished'
#define DATA_MUMMY     6  // 'bMummy'
#define DATA_REMOVED   7  // 'bRemoved'
#define DATA_FROZEN    8  // 'tFrozen'
#define DATA_TYPE      9  // internal: 'type'
#define DATA_TMEMBER  10  // internal: temporary member
#define DATA_TOUCH    11  // time() of last use. A negative value keeps it
                          // in the cache forever.

#define DATA_SIZE     12  // Number of fields in one data line.

// the list of cache entries which need to be written back to disk.
static string * dirty;

// the list of predefined groupnames
static string * predefNames;

/*--------------------------------------------------------------------------*/

// Forward declarations - private.

private void addDirty(string name);
private void saveDirty();
private void createDirs();

// Forward declarations - public.
public int TypeOf (string name);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/* === Internal functions === */

/*--------------------------------------------------------------------------*/
public void create()

// Initialize the info, do some checks, create the directories if needed.

{
  int i, j;
  string fname;

  if (file_name(this_object()) != USERINFO)
  {
    raise_error("Illegal use of " USERINFO ".\n");
    destruct(this_object());
    return;
  }
  seteuid(getuid(this_object()));

  data = m_allocate(0, DATA_SIZE);

  // Initialize the list of predefined groupnames.
  predefNames = ({ IGROUP_BANISHED, IGROUP_FROZEN, IGROUP_WIZARDS
                 , IGROUP_GROUPS });
  for (j = 0; j < 26; j++)
  {
    fname = IGROUP_BANISH+"abcdefghijklmnopqrstuvwxyz"[j..j];
    predefNames += ({ fname });
  }

  createDirs();
}

public nomask prevent_shadow(object shadow) { return 1; }

public int clean_up (int ref) {
  destruct(this_object());
  return 0;
}

public int heart_beat() {
  saveDirty();
  set_heart_beat(0);
}

/*--------------------------------------------------------------------------*/
public void reset()

// Remove all entries from the data cache which hasn't been touched
// for TIME_CACHE seconds. Only exception are entries for online users.

{
  string * names, name;
  int i, stamp;
  object obj;

  names = m_indices(data);
  for (i = sizeof(names); i--; )
  {
    name = names[i];
    if (ITYPE_USER == data[name,DATA_TYPE]
     && (obj = find_player(name))
     && (interactive(obj))
       )
      continue;
    stamp = data[name,DATA_TOUCH];
    if (stamp >= 0 && time()-stamp > TIME_CACHE)
      m_delete(data, name);
  }
}

/*--------------------------------------------------------------------------*/
private void createDirs()

// Create all the savefile directories needed.

{
  int i, j;
  string fname;

  // Create SECURESAVEPATH if necessary.
  i = file_size(SECURESAVEPATH);
  if (i != -2)
  {
    if (i != -1)
      catch(rm(SECURESAVEPATH));
    mkdir(SECURESAVEPATH);
  }

  // Create I_INFOPATH if necessary.
  i = file_size(I_INFOPATH);
  if (i != -2)
  {
    if (i != -1)
      catch(rm(I_INFOPATH));
    mkdir(I_INFOPATH);
  }

  // Create I_USERPATH if necessary.
  i = file_size(I_USERPATH);
  if (i != -2)
  {
    if (i != -1)
      catch(rm(I_USERPATH));
    mkdir(I_USERPATH);
  }

  // Create I_USERPATH/[a-z] if necessary.
  for (j = 0; j < 26; j++)
  {
    fname = I_USERPATH+"abcdefghijklmnopqrstuvwxyz"[j..j];
    i = file_size(fname);
    if (i != -2)
    {
      if (i != -1)
        catch(rm(fname));
      mkdir(fname);
    }
  }
}

/*--------------------------------------------------------------------------*/
static int loadInfo (string iname)

// Load the info for <name> into the data cache (unless it's already there).
//
// Argument:
//   name: the name of the saved data to load (can be uppercase).
//
// Result:
//   True if the name could successfully be loaded.
//
// If it is the old savefile for a player, do some conversions, too.

{
  int typ, rc;
  string fname;

  name = lower_case(name);
  if (!m_contains(&typ, data, name) || ITYPE_NONE == typ)
  {
    // Determine the suiting savefilename.
    typ = ITYPE_USER;
    fname = I_USERPATH +name[0..0]+"/"+name;
    if (0 >= file_size(fname+".o")
    {
      typ = ITYPE_GROUP;
      fname = I_INFOPATH+name;
    }
    if (0 >= file_size(fname+".o")
     || catch(rc = restore_object(fname))
     || !rc
       )
    {
      data[name,DATA_TYPE] = ITYPE_NONE;
    }
    else
    {
      data += ([name:password;level;member;logins;resume;bBanished;bMummy;bRemoved;tFrozen;typ;({});0]);

      // Normalize info data
      if (!pointerp(member))
        data[name,DATA_MEMBER] = ({});
      if (!pointerp(logins))
        data[name,DATA_LOGINS] = allocate(ILOGIN_SIZE);
      if (!pointerp(member))
        data[name,DATA_RESUME] = ({});
    }
  }

  data[name,DATA_TOUCH] = time();
  return ITYPE_NONE != data[name,DATA_TYPE];
}

/*--------------------------------------------------------------------------*/
static void saveInfo (string iname)

// Save the info for <name> if it exists in the data cache.
//
// Argument:
//   name: the name of the data to save (can be uppercase).

{
  string fname;

  name = lower_case(name);
  if (-1 != member(m_indices(data), name)
   && ITYPE_NONE != data[name,DATA_TYPE]
     )
  {
    password  = data[name,DATA_PWD];
    level     = data[name,DATA_LEVEL];
    member    = data[name,DATA_MEMBER];
    logins    = data[name,DATA_LOGINS];
    resume    = data[name,DATA_RESUME];
    bBanished = data[name,DATA_BANISH];
    bMummy    = data[name,DATA_MUMMY];
    bRemoved  = data[name,DATA_REMOVED];
    tFrozen   = data[name,DATA_FROZEN];
    if (ITYPE_USER == data[name,DATA_TYPE])
      fname = I_USERPATH+name[0..0]+"/"+name";
    else
      fname = I_INFOPATH+name";
    save_object(fname);
    data[name,DATA_TOUCH] = time();
  }
}

/*--------------------------------------------------------------------------*/
static int loadGroupInfo (string name)

/* Load the info for usergroup <name> into the data cache.
 *
 * Arguments:
 *   name: the usergroup name to be loaded.
 *
 * Result:
 *   True if the data could be loaded.
 *
 * If <name> is one of the predefined names and the savefile doesn't exist,
 * a default entry is made put into the cache, and it's tagged as 'dirty'.
 */

{
  int rc;

  rc = loadInfo(name);
  if (!rc && -1 == member(predefNames, name))
  {
    m_delete(data, name);
    data[name, DATA_TYPE] = ITYPE_GROUP;
    data[name, DATA_MEMBER] = ({});
    data[name, DATA_TOUCH] = time();
    // Remaining fields can be 0
    rc = 1;
  }

  if (ITYPE_GROUP != data[name, DATA_TYPE])
  {
    raise_error("Inconsistent database: '"+name+"' should be a group entry.\n");
    rc = 0;
  }
  return rc;
}

/*--------------------------------------------------------------------------*/
static int saveGroupInfo (string name)

/* Save the info for usergroup <name> from the data cache.
 *
 * Arguments:
 *   name: the usergroup name to be saved.
 *
 * Result:
 *   True if the data could be written.
 */

{
  if (ITYPE_GROUP != data[name, DATA_TYPE])
  {
    raise_error("Inconsistent database: '"+name+"' should be a group entry.\n");
    return 0;
  }
  saveInfo(name);

  return 1;
}

/*--------------------------------------------------------------------------*/
static int loadUserInfo (string name)

/* Load the info for user <name> into the data cache.
 *
 * Arguments:
 *   name: the user name to be loaded.
 *
 * Result:
 *   True if the data could be loaded.
 *
 * The function also checks the info/banish-* files and adds a ITYPE_RESERVED
 * entry if <name> is found there. In that case, true is returned, too.
 * TODO: Remove the compat code for old player savefiles.
 */

{
  int rc;
  string bname;

  rc = loadInfo(name);
  if (!rc)
  {
    int rc2;

    // Maybe it's an old player savefile.
    bname = SECURESAVEPATH+name[0..0]+"/"+name;
    if (0 < file_size(bname+".o")
     && !catch(rc2 = restore_object(bname))
     && rc2
       )
    {
      m_delete(data, name);

      data += ([name:password;level;member;logins;resume;bBanished;bMummy;bRemoved;tFrozen;ITYPE_USER;({});0]);

      // Normalize info data
      if (!pointerp(member))
        data[name,DATA_MEMBER] = ({});
      if (!pointerp(logins))
        data[name,DATA_LOGINS] = allocate(ILOGIN_SIZE);
      if (!pointerp(member))
        data[name,DATA_RESUME] = ({});
      // TODO: catch(rm(bname+".o"));
      addDirty(name);    
      rc = 1;
    }
  }

  if (!rc)
  {
    // Check if the name itself was banished.
    bname = IGROUP_BANISH+name[0..0];
    if (loadGroupInfo(bname)
     && pointerp(data[bname, DATA_MEMBER])
     && -1 != member(data[bname, DATA_MEMBER], name)
       )
    {
      m_delete(data, name);
      data[name, DATA_TYPE] = ITYPE_RESERVED;
      data[name, DATA_BANISHED] = 1;
      data[name, DATA_TOUCH] = time();
    }
  }
  if (rc
   && ITYPE_USER != data[name, DATA_TYPE]
   && ITYPE_RESERVED != data[name, DATA_TYPE]
     )
  {
    raise_error("Inconsistent database: '"+name+"' should be a user entry.\n");
    rc = 0;
  }
  return rc;
}

/*--------------------------------------------------------------------------*/
static int saveUserInfo (string name)

/* Save the info for user <name> from the data cache.
 *
 * Arguments:
 *   name: the user name to be saved.
 *
 * Result:
 *   True if the data could be saved.
 *
 * If the <name> denotes a reserved/banished name, it is automatically
 * synchronized with the associated 'banish-?' groups, which is then saved
 * if necessary.
 */

{
  if (ITYPE_USER != data[name, DATA_TYPE]
   && ITYPE_RESERVED != data[name, DATA_TYPE]
     )
  {
    raise_error("Inconsistent database: '"+name+"' should be a user entry.\n");
    return 0;
  }

  if (ITYPE_USER == data[name, DATA_TYPE])
  {
    saveInfo(name);
  }
  else
  {
    string bname;
    int dirty;

    bname = IGROUP_BANISH+name[0..0];
    if (!loadGroupInfo(bname))
      return 0;
    dirty = 0;

    if (!pointerp(data[bname, DATA_MEMBER]))
    {
      data[bname, DATA_MEMBER] = ({ name });
      dirty = 1;
    }
    else if (-1 == member(data[bname, DATA_MEMBER], name))
    {
      data[bname, DATA_MEMBER] += ({ name });
      dirty = 1;
    }
    if (dirty && !saveGroupInfo(bname))
      return 0;
  }
  return 1;
}

/*--------------------------------------------------------------------------*/
private void addDirty (string name)

// Add <name> to the list <dirty> of data entries to be written back.

{
  if (!dirty)
    dirty = ({ name });
  else if (-1 == member(dirty, name))
    dirty += ({ name });
  set_heart_beat(1);
}

/*--------------------------------------------------------------------------*/
static void saveDirty()

// Save all the data entries listed in *dirty.

{
  int num;
  string name;

  for (num = sizeof(dirty); num--; )
  {
    name = dirty[num];
    if (ITYPE_USER  == data[name, DATA_TYPE]
     || ITYPE_RESERVED == data[name, DATA_TYPE]
       )
      saveUserInfo(name);
    else if (ITYPE_GROUP == data[name, DATA_TYPE])
      saveGroupInfo(name);
  }
  dirty = 0;
}

/*--------------------------------------------------------------------------*/
public int addMember (string name, string group, int bTemp)

// Add user <name> to the usergroup <group>.
//
// Arguments:
//   name:  the name of the user to add.
//   group: the name of the group to add the user to.
//   bTemp: true if the change shall be just temporary.
//
// Result:
//   True if the name was added.
//
// The function updates the member fields in both the <name> and the
// <group> info.

{
  if (ITYPE_USER != TypeOf(name)
   || ITYPE_GROUP != TypeOf(group)
     )
    return 0;

  if (!bTemp) // Add permanent member
  {
    if (-1 == member(data[group, DATA_MEMBER], name))
    {
      data[group, DATA_MEMBER] += ({ name });
      addDirty(group);
    }
    if (-1 != member(data[group, DATA_TMEMBER], name))
      data[group, DATA_TMEMBER] -= ({ name });

    if (-1 == member(predefNames, group))
    {
      if (-1 == member(data[name, DATA_MEMBER], group))
      {
        data[name, DATA_MEMBER] += ({ group });
        addDirty(name);
      }
      if (-1 != member(data[name, DATA_TMEMBER], group))
        data[name, DATA_TMEMBER] -= ({ group });
    }
  }
  else // Add temporary member
  {
    if (-1 == member(data[group, DATA_MEMBER], name)
     && -1 == member(data[group, DATA_TMEMBER], name)
       )
      data[group, DATA_TMEMBER] += ({ name });
    if (-1 == member(predefNames, group)
     && -1 == member(data[name, DATA_MEMBER], group)
     && -1 == member(data[name, DATA_TMEMBER], group)
       )
      data[name, DATA_TMEMBER] += ({ group });
  }
  return 1;
}

/*--------------------------------------------------------------------------*/
public int removeMember (string name, string group, int bTemp)

// Remove user <name> from the usergroup <group>.
//
// Arguments:
//   name:  the name of the user to remove.
//   group: the name of the group to remove the user from.
//   bTemp: true if just a temporary membership shall be removed.
//
// Result:
//   True if the name was removed.
//
// The function updates the member fields in both the <name> and the
// <group> info.

{
  if (ITYPE_USER != TypeOf(name)
   || ITYPE_GROUP != TypeOf(group)
     )
    return 0;

  if (!bTemp) // Remove permanent member
  {
    if (-1 != member(data[group, DATA_MEMBER], name))
    {
      data[group, DATA_MEMBER] -= ({ name });
      addDirty(group);
    }
    if (-1 == member(predefNames, group)
     && -1 != member(data[name, DATA_MEMBER], group)
       )
    {
      data[name, DATA_MEMBER] -= ({ group });
      addDirty(name);
    }
  }
  
  // Remove temporary membership in any case.
  if (-1 != member(data[group, DATA_TMEMBER], name))
    data[group, DATA_TMEMBER] -= ({ name });
  if (-1 == member(predefNames, group)
   && -1 != member(data[name, DATA_TMEMBER], group)
     )
    data[name, DATA_TMEMBER] -= ({ group });

  return 1;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/* === Public functions === */

/*--------------------------------------------------------------------------*/
public varargs int AddMember (string name, string group, int bTemp)

// Add user <name> to usergroup <group>.
//
// Arguments:
//   name:  the name of the user to add.
//   group: the name of the group to add the user to.
//          It must be none of the predefined groups.
//   bTemp: true if only a temporary membership is achieved.
//
// Result:
//   True if the name could be added.

{
  if (-1 != member(predefNames, group))
    return 0;
  return addMember(name, group, bTemp)
}

/*--------------------------------------------------------------------------*/
public varargs int RemoveMember (string name, string group, int bTemp)

// Remove user <name> from usergroup <group>.
//
// Arguments:
//   name:  the name of the user to remove.
//   group: the name of the group to remove the user from.
//          It must be none of the predefined groups.
//   bTemp: true if only a temporary membership is to be removed.
//
// Result:
//   True if the name could be removed.

{
  if (-1 != member(predefNames, group))
    return 0;
  return removeMember(name, group, bTemp)
}

/*--------------------------------------------------------------------------*/
public int TypeOf (string name)

// Return what kind of name <name> is.
//
// Arguments:
//   name: the name to check.
//
// Result:
//   ITYPE_NONE:     name is not known.
//   ITYPE_USER:     name is a (possibly banished) user.
//   ITYPE_GROUP:    name is a usergroup.
//   ITYPE_RESERVED: name is reserved or banished.
//
// This function can also be used as a 'load info' function.
{
  int rc;

  if (!loadUserInfo(name))
    loadGroupInfo(name);
  // TODO: else: sync info of user
  return data[name, DATA_TYPE];
}

/*--------------------------------------------------------------------------*/
public int Wizlevel (string name)

// Return the wizlevel of name <name>.
//
// Arguments:
//   name: the name to check.
//
// Result:
//   LVL_PLAYER:  name is a player, not known, or a user group.
//   LVL_ANCIENT: name is a retired player.
//   LVL_LEARNER: name is an apprentice wizard.
//   LVL_WIZARD:  name is an accepted wizard.

{
  int lvl;

  if (ITYPE_USER != TypeOf(name))
    return 0;

  return data[name, DATA_LEVEL];
}

/*--------------------------------------------------------------------------*/
public int Banished (string name)

// Check if <name> is banished or not.
//
// Arguments:
//   name: the name to check.
//
// Result:
//   True if <name> is a banished user or a banished/reserved name.

{
  int type;

  type = TypeOf(name);
  if (ITYPE_RESERVED == type)
    return 1;
  if (ITYPE_USER != type)
    return 0;

  return data[name, DATA_BANISH];
}

/*--------------------------------------------------------------------------*/
public int Frozen (string name)

// Check if user <name> is frozen or not.
//
// Arguments:
//   name: the name to check.
//
// Result:
//   0 if <name> is an unfrozen user, or no user.
//   The time until user <name> is still frozen.
//
// If the stored time is smaller than time(), i.e. the frozen period is
// over, the user is 'melted' automatically.

{
  int ft;

  if (ITYPE_USER != TypeOf(name);
    return 0;

  ft = data[name, DATA_FROZEN];
  if (ft <= time())
  {
    ft = 0;
    data[name, DATA_FROZEN] = 0;
    removeMember(name, IGROUP_FROZEN, 0);
    data[name, DATA_RESUME] += ({ ({ time(), IRESUME_FREEZE, 0, 0 }) });
    addDirty(name);
  }
  return ft;
}

TODO: syncInfo(): checks membership entries
TODO: query functions for all that data
TODO: Translationfunction for the resume entries.
TODO: WalkEntries(closure/object)
TODO: TryLogin()

#define DATA_PWD       0  // 'password'
#define DATA_LEVEL     1  // 'level'
#define DATA_TYPE      2  // 'type'
#define DATA_MEMBER    3  // 'member'
#define DATA_LOGINS    4  // 'logins'
#define DATA_RESUME    5  // 'resume'
#define DATA_BANISH    6  // 'bBanished'
#define DATA_MUMMY     7  // 'bMummy'
#define DATA_REMOVED   8  // 'bRemoved'
#define DATA_FROZEN    9  // 'bRemoved'
#define DATA_TMEMBER  10  // internal: temporary member
#define DATA_TOUCH    11  // time() of last use. A negative value keeps it
                          // in the cache forever.

#define DATA_SIZE     12  // Number of fields in one data line.
/****************************************************************************/
