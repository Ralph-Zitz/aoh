/*---------------------------------------------------------------------------
**  /secure/info
**
** Server of the security relevant data of players: Password, Wizlevel
** and domain membership.
**
** The server distinguishes between valid entries (describing existing
** players) and invalid entries.
** Both types of entries are held in an internal cache to minimize
** disk accesses, but only valid entries are loaded/saved.
**
** 04-Sep-93 [Mateese]
** 02-Jan-94 [Mateese] Introduced pseudo domains to ease the computation
**                     of the wizlevels.
** 10-Jan-94 [Mateese] Simplified level handling.
**                     Added _HONORS and _BANISH fields and reserved names.
** 26-Feb-94 [Mateese] Adapted for new security method.
** 27-Nov-94 [Mateese] Wizards may promote new wizards as well. Promotors
**                     may also demote their pupils again.
** 14-Jul-96 [Mateese] PD flag removed, LORD flag added.
**                     Pseudo domain SYSTEM introduced.
**---------------------------------------------------------------------------
*/

#include "/i/sys/config.h"
#include "/i/sys/info.h"

#define THIS     this_object()
#define PREV     previous_object()
#define PREVN(n) previous_object(n)
#define TP       this_player()
#define TI       this_interactive()

#define SAVEPATH "/secure/save"

//---------------------------------------------------------------------------
// The mapping describing one user has these fields, corresponding with the
// saved variables, and is indexed by the user name:
//
// info = RECORD
//   int     SINFO_LEVEL    level
//   mapping SINFO_DOMAIN   domain
//   string* SINFO_HONORS   honors
//   string  SINFO_BANISH   banish
//   int     SINFO_FLAGS    flags
//   mixed*  SINFO_PROMO    promo
//   string  SINFO_PASSWORD password
//   int     SINFO_STAMP
// END
//
// The domain mapping is indexed by the domain names and contains the
// wizard level for that domain.
// No domain wizlevel may exceed the range [MEMBER_LVL..LORD_LVL]
// For each domain an info records exists, marked with the level
// SILEVEL_DOMAIN. This record holds the names of the members as indices of
// the SINFO_DOMAIN entry to store the domain wizlevels.
// The pseudo domain "system" holds lists of known arches, admins, banished
// and reserved names, stored as SINFO_DOMAIN under the appropriate indices.
//---------------------------------------------------------------------------

// The data from/to the savefiles.

int     level;
mapping domain;
string  *honors;
string  banish;
int     flags;
mixed   *promo;
string  password;

// The internal info-cache, indexed by name.

static mapping data;

//---------------------------------------------------------------------------
nomask void create()

// Set up the server.

{ int i;

  if (file_name(THIS) != SECUREINFO)
  {
    raise_error("Illegal attempt.\n");
    destruct(THIS);
    return 0;
  }
  seteuid(getuid(THIS));
  i = file_size(SAVEPATH);
  if (i != -2) {
    if (i != -1) catch(rm(SAVEPATH));
    mkdir(SAVEPATH);
  }
  data = m_allocate(0, SINFO_NO_FIELDS);
}

nomask int prevent_shadow(object shadow) { return 1; }

//---------------------------------------------------------------------------
void reset()

// Remove all entries of non-interactives from the data cache which weren't
// touched for at least half an hour.

{ string *names, name;
  int i, stamp;
  object ob;

  names = m_indices(data);
  for (i = sizeof(names); i--;)
  {
    name = names[i];
    if ((name[0] != '!') && (ob = find_player(name)) && interactive(ob))
      continue;
    stamp = data[name,SINFO_STAMP];
    if (stamp >= 0 && time()-stamp > 1800)
      m_delete(data, name);
  }
}

//---------------------------------------------------------------------------
static int check (int arch, string name)

// Check if the current caller has admin (or arch) rights,
// or is <name> itself.
//
// Arguments:
//   arch: flag if arches shall pass as well.
//   name: name to accept.
//
// Result:
//   0: operation denied.
//   1: caller has admin (or arch) rights.
//   2: this_player() is <name>

{
  string eperm;

  eperm = (string) MASTER->_effective_perm(0, PREV);
  if ((int)MASTER->_compare_perms(eperm, arch ? SIDOM_ARCHID : SIDOM_ADMINID) >= 0)
    return 1;
  if ((int)MASTER->_compare_perms(eperm, PRIVATEID+":"+name) >= 0)
    return 2;
  return 0;
}

//---------------------------------------------------------------------------
static void loadSystemInfo ()

// Load the info for the system lists into the cache.

{ int rc;
  int lvl;

  if (!m_contains(&level, data, SIDOM_SYSTEM) || lvl == SILEVEL_ILLEGAL)
  {
    if (    name[0] == '!' || file_size(SAVEPATH+"/"+name[0..0]) != -2
         || catch(rc = efun::restore_object(SAVEPATH+"/"+name[0..0]+"/"+name))
         || !rc
       )
    {
      data += ([name:SILEVEL_DOMAIN;0;0;0;0;0;0;0]);
    }
    else
      data += ([name:level;domain;0;0;0;0;0;0]);
  }
  if (!mappingp(data[name, SINFO_DOMAIN]))
    data[name, SINFO_DOMAIN] = ([]);
  if (!pointerp(data[name, SINFO_DOMAIN][SIDOM_ADMIN]))
    data[name, SINFO_DOMAIN][SIDOM_ADMIN] = ({});
  if (!pointerp(data[name, SINFO_DOMAIN][SIDOM_ARCH]))
    data[name, SINFO_DOMAIN][SIDOM_ADMIN] = ({});
  if (!pointerp(data[name, SINFO_DOMAIN][SIDOM_BANISH]))
    data[name, SINFO_DOMAIN][SIDOM_BANISH] = ({});
  if (!pointerp(data[name, SINFO_DOMAIN][SIDOM_RESERVED]))
    data[name, SINFO_DOMAIN][SIDOM_RESERVED] = ({});
}

//---------------------------------------------------------------------------
static int loadInfo (string name)

// Load the info for a name into the cache.
//
// Argument:
//   name: the name to load
//
// Result:
//   True if info for the name exists; false, if the name is not in use.
//
// If the info for the name is already loaded, just the existance is
// returned.
// If there is no info to load, a dummy-entry is made and tagged as such,
// then false is returned.
// The pseudo domain SIDOM_SYSTEM is loaded as well.

{ int rc;
  int lvl;

  loadSystemInfo();

  name = lower_case(name);
  if (SIDOM_SYSTEM == name)
    return 1;

  if (!m_contains(&level, data, name) || lvl == SILEVEL_ILLEGAL)
  {
    if (    name[0] == '!' || file_size(SAVEPATH+"/"+name[0..0]) != -2
         || catch(rc = efun::restore_object(SAVEPATH+"/"+name[0..0]+"/"+name))
         || !rc
       )
    {
      data += ([name:SILEVEL_ILLEGAL;0;([]);({});0;({});0;0]);
    }
    else
      data += ([name:level;domain;honors;banish;flags;promo;password;0]);
  }
  if (!mappingp(data[name, SINFO_DOMAIN]))
    data[name, SINFO_DOMAIN] = ([]);
  if (!pointerp(data[name, SINFO_HONORS]))
    data[name, SINFO_HONORS] = ({});
  if (!pointerp(data[name, SINFO_PROMO]))
    data[name, SINFO_PROMO] = ({});
  data[name,SINFO_STAMP] = time();
  return data[name,SINFO_LEVEL] != SILEVEL_ILLEGAL;
}

//---------------------------------------------------------------------------
static void saveInfo (string name)

// Write the info for a name from the cache.
//
// Argument:
//   name: the name to save
//
// If the info for name is real, it is saved.

{
  name = lower_case(name);
  if (    member(m_indices(data), name) != SILEVEL_ILLEGAL
       && data[name,SINFO_LEVEL] != SILEVEL_ILLEGAL
     )
  {
    if (file_size(SAVEPATH+"/"+name[0..0]) != -2)
      mkdir(SAVEPATH+"/"+name[0..0]);
    level    = data[name, SINFO_LEVEL];
    domain   = data[name, SINFO_DOMAIN];
    honors   = data[name, SINFO_HONORS];
    banish   = data[name, SINFO_BANISH];
    flags    = data[name, SINFO_FLAGS];
    promo    = data[name, SINFO_PROMO];
    password = data[name, SINFO_PASSWORD];
    efun::save_object(SAVEPATH+"/"+name[0..0]+"/"+name);
    data[name, SINFO_STAMP] = time();
  }
}

//---------------------------------------------------------------------------
int isValid (string name)

// Return if for the given name valid info exists.

{
  return loadInfo(name);
}

//---------------------------------------------------------------------------
varargs int Wizlevel (mixed arg, string domain)

// Return the wizlevel for one name, optionally the domainlevel.
//
// Argument:
//   arg   : the object/name of interest.
//   domain: a domain affected or 0.
//
// Result:
//   The wizlevel for this name, at least 0.
//   If 'domain' is given, the wizlevel is returned in respect to that
//   domain.
//
// If a non-existing domain is given, it is deleted from the players info record.

{
  if (!arg)
    return PLAYER_LVL;
  if (objectp(arg))
    arg = geteuid(arg);
  else
    arg = lower_case(arg);

  if (arg[0] == '!')
    return PLAYER_LVL;

  if (!loadInfo(arg) || data[arg, SINFO_LEVEL] < PLAYER_LVL)
    return PLAYER_LVL;

  if (   domain && stringp(domain) && domain[0] != '!' && SIDOM_SYSTEM != domain
      && (!loadInfo(domain) || data[domain, SINFO_LEVEL] != SILEVEL_DOMAIN)
      && member(data[arg, SINFO_DOMAIN], domain) != -1
     )
  {
    m_delete(data[arg, SINFO_DOMAIN], domain);
    saveInfo(arg);
    domain = 0;
  }

  if (domain)
    return data[arg, SINFO_DOMAIN][domain];

  if (data[arg, SINFO_LEVEL] < SIDOM_ARCH_LVL && (SIFLAG_LORD & data[arg, SINFO_FLAGS]))
    return SIDOM_ARCH_LVL;

  return data[arg, SINFO_LEVEL];
}

//---------------------------------------------------------------------------
varargs string Permission (string arg, string domain)

// Return the permission for a user, optionally the domain permissions..
//
// Argument:
//   arg   : the object/name of interest.
//   domain: a domain affected or 0.
//
// Result:
//   The permission for this name, at least 0.
//
// If a non-existing domain is given, it is deleted from the players info record.

{
  string rc;
  int lvl;

  if (!arg)
    return BOTTOMID;

  arg = lower_case(arg);

  if (arg[0] == '!')
    return BOTTOMID;

  if (!loadInfo(arg) || data[arg, SINFO_LEVEL] < PLAYER_LVL)
    return BOTTOMID;

  if (   domain && stringp(domain) && domain[0] != '!' && SIDOM_SYSTEM != domain
      && (!loadInfo(domain) || data[domain, SINFO_LEVEL] != SILEVEL_DOMAIN)
      && member(data[arg, SINFO_DOMAIN], domain) != -1
     )
  {
    m_delete(data[arg, SINFO_DOMAIN], domain);
    saveInfo(arg);
    domain = 0;
  }

  if (domain)
    lvl = data[arg, SINFO_DOMAIN][domain];
  else {
    lvl = data[arg, SINFO_LEVEL];
    if (lvl < SIDOM_ARCH_LVL && (SIFLAG_LORD & data[arg, SINFO_FLAGS]))
      lvl = SIDOM_ARCH_LVL;
  }

  if (domain && lvl >= MEMBER_LVL) {
    rc = ({PLAYERID, MEMBERID, LORDID})[lvl];
    rc += ":"+domain+":"+arg;
  }
  else {
    rc = ({PLAYERID, WIZARDID, WIZARDID, SIDOM_ARCHID, SIDOM_ADMINID})[lvl];
    if (lvl < SIDOM_ARCH_LVL)
      rc += ":"+arg;
  }

  return rc;
}

//---------------------------------------------------------------------------
int CheckPassword (string name, string pw)

// Compare the password for a valid info entry with a given one.
//
// Argument:
//   name: the name to add
//   pw  : the password to check against.
//
// Result:
//   -1: if no password was set.
//    0: if the passwords don't match.
//    1: if the passwords match.

{ string oldpw;

  name = lower_case(name);
  if (   !check(0, name) || !loadInfo(name) || name[0] == '!'
      || data[name, SINFO_LEVEL] < 0
     )
    return 0;
  oldpw = data[name,SINFO_PASSWORD];
  if (!oldpw || oldpw == "") return -1;
  return oldpw == crypt(pw, oldpw);
}

//---------------------------------------------------------------------------
mixed * Info (string name)

// Return the info for a given name.
//
// Argument:
//   name: the name of interest.
//
// Result:
//   An array with the public info for the name, or 0 if the name is not
//   valid.

{
  name = lower_case(name);
  if (!loadInfo(name)) return 0;
  return ({ data[name,SINFO_LEVEL]
          , copy(data[name, SINFO_DOMAIN])
          , data[name, SINFO_HONORS]+({})
          , data[name, SINFO_BANISH]
          , data[name, SINFO_FLAGS]
          , data[name, SINFO_PROMO]+({})
         });
}

//---------------------------------------------------------------------------
int NewPlayer (string name, string pw)

// Add a new player to the info database.
//
// Argument:
//   name: the name to add
//   pw  : the password to use
//
// Result:
//   True on success.

{
  name = lower_case(name);
  if (   !check(0, name) || loadInfo(name) || name[0] == '!'
      || data[name, SINFO_LEVEL] < 0
     )
    return 0;
  data[name, SINFO_LEVEL]  = PLAYER_LVL;
  data[name, SINFO_DOMAIN] = ([]);
  data[name, SINFO_HONORS] = ({});
  data[name, SINFO_BANISH] = 0;
  data[name, SINFO_FLAGS]  = 0;
  data[name, SINFO_PROMO]  = ({});
  if (!pw || pw == "")
    data[name, SINFO_PASSWORD] = 0;
  else
    data[name, SINFO_PASSWORD] = crypt(pw);
  data[name, SINFO_STAMP]    = time();
  saveInfo(name);
  return 1;
}

//---------------------------------------------------------------------------
int NewDomain (string name)

// Add a new domain to the info database.
//
// Argument:
//   name: the domain to add
//
// Result:
//   True on success.

{
  name = lower_case(name);
  if (   !check(0, name) || loadInfo(name) || name[0] == '!'
      || data[name, SINFO_LEVEL] < 0
     )
    return 0;
  data[name, SINFO_LEVEL]  = SILEVEL_DOMAIN;
  data[name, SINFO_DOMAIN] = ([]);
  data[name, SINFO_HONORS] = ({});
  data[name, SINFO_BANISH] = 0;
  data[name, SINFO_FLAGS]  = 0;
  data[name, SINFO_PROMO]  = ({});
  data[name, SINFO_PASSWORD] = 0;
  data[name, SINFO_STAMP]    = time();
  saveInfo(name);
  return 1;
}

//---------------------------------------------------------------------------
varargs int SetLevel (string name, int lvl, string domain)

// Set a new (domain) wizlevel for a valid info entry.
//
// Argument:
//   name  : the name to add
//   lvl   : the new level
//   domain: the domain affected, or 0 for the general level.
//
// Result:
//   True on success.

{
  int oldlvl, dlvl;
  string eperm;

  name = lower_case(name);
  domain = lower_case(domain);
  if (   !loadInfo(name) || name[0] == '!'
      || data[name, SINFO_LEVEL] < 0
      || !loadInfo(SIDOM_SYSTEM)
     )
    return 0;

  // Is there such a domain?
  if (   domain
      && (   !loadInfo(domain) || domain[0] == '!' || SIDOM_SYSTEM == domain
          || data[domain, SINFO_LEVEL] != SILEVEL_DOMAIN
         )
     )
   return 0;

  eperm = (string) MASTER->_effective_perm(domain, PREV);

  oldlvl = data[name, SINFO_LEVEL];

  // The function is quite critical, so an abundance of checks
  // is needed here. It'd be fine if we could live without it :-(

  if (!domain) {

    if (oldlvl == lvl)
      return 1;

    while (1) {
      // Root or adminobjects may do everything
      if ((int)MASTER->_compare_perms(eperm, SIDOM_ADMINID) >= 0)
        break;

      // Archobjects may promote up to wizard or demote wizards.
      if ((int)MASTER->_compare_perms(eperm, SIDOM_ARCHID) >= 0
          && (   (oldlvl < lvl && lvl < SIDOM_ARCH_LVL)
              || (oldlvl > lvl && oldlvl < SIDOM_ARCH_LVL)
             )
         )
        break;

      // Wizards may promote players and learners.
      // Promotors may also demote their promotees again.
      if ((int)MASTER->_compare_perms(eperm, WIZARDID) >= 0)
      {
        if (oldlvl < lvl && (lvl == LEARNER_LVL || lvl == WIZARD_LVL))
          break;
        if (oldlvl > lvl && oldlvl == LEARNER_LVL
            && geteuid(TI) == data[name, SINFO_PROMO][1]
           )
          break;
      }

      // Everybody may demote itself
      if ((int)MASTER->_compare_perms(eperm, PRIVATEID+":"+name) >= 0
          && oldlvl < lvl
         )
        break;

      // Everything else is forbidden
      return 0;
    }

    data[name, SINFO_LEVEL] = lvl;

    // Extra actions
    if (oldlvl < lvl && lvl == LEARNER_LVL) {
      data[name, SINFO_PROMO] = ({ time(), geteuid(TI) });
    }
    if (oldlvl < lvl && lvl == WIZARD_LVL) {
      mixed tmp;
      tmp = data[name, SINFO_PROMO];
      data[name, SINFO_PROMO] = tmp[0..1] + ({ time(), geteuid(TI) });
    }
    if (lvl < LEARNER_LVL) {
      string * domains;
      domains = m_indices(data[name, SINFO_DOMAIN]));

      if (sizeof(domains) { // Delete all domain memberships
        int i;
        for (i = sizeof(domains); i--; ) {
          domain = domains[i];
          if (loadInfo(domain))
          {
            m_delete(data[domain, SINFO_DOMAIN], name);
            saveInfo(domain);
          }
        }
      }

      data[name, SINFO_DOMAIN] = ([]);
      data[name, SINFO_PROMO] = ({ });
    }
    if (oldlvl != lvl && oldlvl == SIDOM_ADMIN_LVL)
      data[SIDOM_SYSTEM, SINFO_DOMAIN][SIDOM_ADMIN] -= ({ name });
    if (oldlvl != lvl && oldlvl == SIDOM_ARCH_LVL)
      data[SIDOM_SYSTEM, SINFO_DOMAIN][SIDOM_ARCH] -= ({ name });
    if (oldlvl != lvl && lvl == SIDOM_ADMIN_LVL)
      data[SIDOM_SYSTEM, SINFO_DOMAIN][SIDOM_ADMIN] += ({ name });
    if (oldlvl != lvl && lvl == SIDOM_ARCH_LVL)
      data[SIDOM_SYSTEM, SINFO_DOMAIN][SIDOM_ARCH] += ({ name });
  }
  else {
    // For domains only a restricted set of levels is allowed.
    // Only learners and above may be dealt with.
    if (lvl > LORD_LVL || oldlvl < LEARNER_LVL)
      return 0;

    dlvl = data[name, SINFO_DOMAIN][domain];
    if (dlvl == lvl)
      return 1;

    while (1) {
      // Admins, arches and lords may do everything.
      if ((int)MASTER->_compare_perms(eperm, LORDID+":"+domain) >= 0)
        break;

      // Everybody may demote itself
      if ((int)MASTER->_compare_perms( (string)MASTER->_effective_perm(0, PREV)
                                     , PRIVATEID+":"+name) >= 0
          && lvl < dlvl
         )
        break;

      // Everything else is forbidden
      return 0;
    }

    if (dlvl < MEMBER_LVL)
    {
      m_delete(data[name, SINFO_DOMAIN], domain);
      m_delete(data[domain, SINFO_DOMAIN], name);
    }
    else
    {
      data[name, SINFO_DOMAIN][domain] = dlvl;
      if (oldlvl < WIZARD_LVL)
      {
        mixed tmp;
        tmp = data[name, SINFO_PROMO];
        data[name, SINFO_PROMO] = tmp[0..1] + ({ time(), geteuid(TI) });
        data[name, SINFO_LEVEL] = WIZARD_LVL;
      }
      if (LORD_LVL == dlvl)
        data[name, SINFO_FLAGS] = data[name, SINFO_FLAGS] | SIFLAG_LORD;
      else
      {
        string * domains;
        int i;
        domains = m_indices(data[name, SINFO_DOMAIN]);
        for (i = sizeof(domains); i--; )
          if (LORD_LVL == data[name, SINFO_DOMAIN][domains[i]])
            break;
        if (i >= 0)
          data[name, SINFO_FLAGS] = data[name, SINFO_FLAGS] & (~SIFLAG_LORD);
      }
    }
    saveInfo(domain);
  }
  saveInfo(SIDOM_SYSTEM);
  saveInfo(name);
  return 1;
}

//---------------------------------------------------------------------------
int SetPassword (string name, string pw)

// Set a new password for a valid info entry.
//
// Argument:
//   name: the name to add
//   pw  : the new password
//
// Result:
//   True on success.

{
  name = lower_case(name);
  if (   !check(0, name) || !loadInfo(name) || name[0] == '!'
      || data[name, SINFO_LEVEL] < 0
     )
    return 0;
  if (!pw || pw == "")
    data[name,SINFO_PASSWORD] = 0;
  else
    data[name,SINFO_PASSWORD] = crypt(pw);
  saveInfo(name);
  return 1;
}

//---------------------------------------------------------------------------
int ChangeFlags (string name, int mask, int val)

// Change the flag settings.
//
// Argument:
//   name: the name to add
//   mask: the bit mask of flags to change.
//   val : the bit mask of new flag values
//
// Result:
//   True on success.

{
  int old;

  name = lower_case(name);
  if (   !check(0, name) || !loadInfo(name) || name[0] == '!'
      || data[name, SINFO_LEVEL] < 0
     )
    return 0;

  old = data[name,SINFO_FLAGS];
  data[name, SINFO_FLAGS] = ((old & ~mask) | (val & mask)) & SIFLAG_ALL;
  if (data[name, SINFO_FLAGS] != old)
    saveInfo(name);
  return 1;
}

//---------------------------------------------------------------------------
int Delete (string name)

// Delete a valid entry from the info database.
//
// Argument:
//   name : the name of the entry to delete.
//
// Result:
//   True on success.

{
  int chk, i;
  string * domains, domain;

  name = lower_case(name);
  chk = check(1, name);
  if (!chk || !loadInfo(name) || name[0] == '!' || !loadInfo(SIDOM_SYSTEM))
    return 0;

  // Only arches, admins or gods may delete someone, but only
  // if the victim is of same or lower level.
  // Suicide is of course allowed.
  if (chk == 1 && Wizlevel(TI) <= Wizlevel(name))
    return 0;

  // Remove any (pseudo)domain memberships.
  data[SIDOM_SYSTEM, SINFO_DOMAIN][SIDOM_ADMIN] -= ({ name });
  data[SIDOM_SYSTEM, SINFO_DOMAIN][SIDOM_ARCH] -= ({ name });
  domains = m_indices(data[name, SINFO_DOMAIN]);
  for (i = sizeof(domains); i--; ) {
    domain = domains[i];
    if (loadInfo(domain))
    {
      m_delete(data[domain, SINFO_DOMAIN], name);
      saveInfo(domain);
    }
  }

  m_delete(data, name);
  catch(rm(SAVEPATH+"/"+name[0]+"/"+name+".o"));
  return 1;
}

//---------------------------------------------------------------------------
int Rename (string name, string newname)

// Rename a valid info entry.
//
// Argument:
//   name    : the name to change.
//   newname : the new name.
//
// Result:
//   True on success.

{
  int i, chk;

  name = lower_case(name);
  newname = lower_case(newname);
  if (   !loadInfo(name) || name[0] == '!'
      || !loadInfo(SIDOM_SYSTEM)
      || data[name, SINFO_LEVEL] < 0
      || !loadInfo(newname) || newname[0] == '!'
      || data[newname, SINFO_LEVEL] < 0
      || data[newname, SINFO_BANISH]
      || !check(1, name)
     )
    return 0;
  for (i = SINFO_LEVEL; i <= SINFO_STAMP; i++)
    data[newname,i] = data[name,i];

  // Change any (pseudo)domain membership.
  if (-1 != member(data[SIDOM_SYSTEM, SINFO_DOMAIN][SIDOM_ADMIN], name))
  {
    data[SIDOM_SYSTEM, SINFO_DOMAIN][SIDOM_ADMIN] -= ({ name });
    data[SIDOM_SYSTEM, SINFO_DOMAIN][SIDOM_ADMIN] += ({ newname });
  }
  if (-1 != member(data[SIDOM_SYSTEM, SINFO_DOMAIN][SIDOM_ARCH], name))
  {
    data[SIDOM_SYSTEM, SINFO_DOMAIN][SIDOM_ARCH] -= ({ name });
    data[SIDOM_SYSTEM, SINFO_DOMAIN][SIDOM_ARCH] += ({ newname });
  }
  domains = m_indices(data[name, SINFO_DOMAIN]);
  for (i = sizeof(domains); i--; ) {
    domain = domains[i];
    if (loadInfo(domain))
    {
      data[domain, SINFO_DOMAIN][newname] = data[domain, SINFO_DOMAIN][name];
      m_delete(data[domain, SINFO_DOMAIN], name);
      saveInfo(domain);
    }
  }
  saveInfo(newname);
  saveInfo(SIDOM_SYSTEM);
  m_delete(data, name);
  catch(rm(SAVEPATH+"/"+name[0]+"/"+name+".o"));
  return 1;
}

//---------------------------------------------------------------------------
int Reserve (string name)

// Reserve a name for game use.
//
// Argument:
//   name : the name to reserve.
//
// Result:
//   True on success.
//
// The name mustn't be in use already.

{
  name = lower_case(name);
  if (   loadInfo(name) || name[0] == '!' || !TI
      || data[name, SINFO_LEVEL] != SILEVEL_ILLEGAL
     )
    return 0;

  data[name, SINFO_LEVEL] = SILEVEL_RESERVED;
  data[name, SINFO_BANISH] = stime()+" Reserved by "+getuid(TI)+"\n";
  data[SIDOM_SYSTEM, SINFO_DOMAIN][SIDOM_RESERVED] -= ({ name });
  data[SIDOM_SYSTEM, SINFO_DOMAIN][SIDOM_RESERVED] += ({ name });

  saveInfo(name);
  saveInfo(SIDOM_SYSTEM);
  return 1;
}

//---------------------------------------------------------------------------
int Unreserve (string name)

// Return a reserved name for free use.
//
// Argument:
//   name : the name to unreserve.
//
// Result:
//   True on success.

{
  name = lower_case(name);
  if (   loadInfo(name) || name[0] == '!' || !TI
      || (   data[name, SINFO_LEVEL] != SILEVEL_ILLEGAL
          && data[name, SINFO_LEVEL] != SILEVEL_RESERVED
         )
     )
    return 0;

  if (data[name, SINFO_LEVEL] == SILEVEL_RESERVED) {
    catch(rm(SAVEPATH+"/"+name[0]+"/"+name+".o"));
    m_delete(data, name);
    data[SIDOM_SYSTEM, SINFO_DOMAIN][SIDOM_RESERVED] -= ({ name });
    saveInfo(SIDOM_SYSTEM);
  }
  return 1;
}

//---------------------------------------------------------------------------
int Banish (string name, string reason)

// Banish a player from the game.
//
// Argument:
//   name  : the name to banish.
//   reason: the reason for the banishment.
//
// Result:
//   True on success.
//
// The name must be a player name.

{
  name = lower_case(name);
  if (   !loadInfo(name) || name[0] == '!'
      || data[name, SINFO_LEVEL] < 0
      || !check(1, name)
     )
    return 0;

  data[name, SINFO_BANISH] =   stime()+" Banished by "+getuid(TI)+"\n"
                             + (reason || "No reason given.")+"\n";
  data[SIDOM_SYSTEM, SINFO_DOMAIN][SIDOM_BANISH] -= ({ name });
  data[SIDOM_SYSTEM, SINFO_DOMAIN][SIDOM_BANISH] += ({ name });
  saveInfo(name);
  saveInfo(SIDOM_SYSTEM);
  return 1;
}

//---------------------------------------------------------------------------
int Unbanish (string name, string reason)

// Unbanish a player from the game.
//
// Argument:
//   name  : the name to unbanish.
//
// Result:
//   True on success.
//
// The name must be a player name.

{
  int chk;

  name = lower_case(name);
  if (   !loadInfo(name) || name[0] == '!'
      || data[name, SINFO_LEVEL] < 0
      || 1 != check(1, name)
     )
    return 0;

  data[name, SINFO_BANISH] = 0;
  data[SIDOM_SYSTEM, SINFO_DOMAIN][SIDOM_BANISH] -= ({ name });
  saveInfo(name);
  saveInfo(SIDOM_SYSTEM);
  return 1;
}

//---------------------------------------------------------------------------
int SetHonors (string name, string *hons)

// Set the honorifics for a nane.
//
// Argument:
//   name : the name to set the honorifics to
//   hons : the honorifics to set
//
// Result:
//   True on success.

{
  int chk;

  name = lower_case(name);
  if (   !loadInfo(name) || name[0] == '!'
      || data[name, SINFO_LEVEL] < 0
     )
    return 0;

  chk = check(0, name);
  if (chk != 1 && chk != 2)
    return 0;

  data[name, SINFO_HONORS] = hons+({});
  saveInfo(name);
  return 1;

}

//---------------------------------------------------------------------------
int AddHonor (string name, string hon)

// Add one honorific to a nane.
//
// Argument:
//   name : the name to add the honorific to.
//   hon  : the honorific to add.
//
// Result:
//   True on success.

{
  int chk;

  name = lower_case(name);
  if (   !loadInfo(name) || name[0] == '!'
      || data[name, SINFO_LEVEL] < 0
     )
    return 0;

  chk = check(0, name);
  if (chk != 1 && chk != 2)
    return 0;

  data[name, SINFO_HONORS] += ({ hon });
  saveInfo(name);
  return 1;
}

/***************************************************************************/
