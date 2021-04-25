/*---------------------------------------------------------------------------
** std/skillmaster.c
**
** The standard master of skills.
** It's not much more than a database.
**
** 05-Jun-1994  [Mateese]
**---------------------------------------------------------------------------
*/

#include "/neu/sys/skillmaster.h"
#include "/neu/sys/attributes.h"

#define THIS this_object()
#define TI   this_interactive()
#define TP   this_player()
#define PREV previous_object()

static mapping skills;  // Here all the data is kept.

//---------------------------------------------------------------------------
public mixed * SetSkill ( string name, mixed *data )

// Add or change skill entry.

{
  skills[name, SKX_STAT1] = data[SKX_STAT1];
  skills[name, SKX_STAT1_FACTOR] = data[SKX_STAT1_FACTOR];
  skills[name, SKX_STAT2] = data[SKX_STAT2];
  skills[name, SKX_STAT2_FACTOR] = data[SKX_STAT2_FACTOR];
  skills[name, SKX_SKILL_FACTOR] = data[SKX_SKILL_FACTOR];
  skills[name, SKX_LEARN_FACTOR] = data[SKX_LEARN_FACTOR];
  skills[name, SKX_MAX_VALUE] = data[SKX_MAX_VALUE];
  skills[name, SKX_EXTRA] = data[SKX_EXTRA];
  return data;
}

//---------------------------------------------------------------------------
public varargs 
       void AddSkill ( string name
                     , string stat1, mixed stat1factor
                     , string stat2, mixed stat2factor
                     , mixed skillfactor
                     , mixed learnfactor
                     , mixed maxvalue
                     , mixed extra
                     )

// Add or change skill entry.

{
  skills[name, SKX_STAT1] = stat1;
  skills[name, SKX_STAT1_FACTOR] = stat1factor;
  skills[name, SKX_STAT2] = stat2;
  skills[name, SKX_STAT2_FACTOR] = stat2factor;
  skills[name, SKX_SKILL_FACTOR] = skillfactor;
  skills[name, SKX_LEARN_FACTOR] = learnfactor;
  skills[name, SKX_MAX_VALUE] = maxvalue;
  skills[name, SKX_EXTRA] = extra;
}

//---------------------------------------------------------------------------
public void RemoveSkill ( string name )

// Completely remove a skill.

{
  m_delete(skills, name);
}

//---------------------------------------------------------------------------
public void create()

// Init the master and its data.

{
  if (is_clone(THIS) || blueprint(THIS) != object_name(THIS))
    raise_error("Illegal attempt.\n");

  skills = m_allocate(0,SKX_NO_FIELDS);
}

//---------------------------------------------------------------------------
public string * QuerySkillNames () 

// Return the names of all managed.

{
  return m_indices(skills);
}

//---------------------------------------------------------------------------
public mapping QuerySkills () 

// Return all skill data

{
  mapping rc;
  string *ind;
  int i, j;

  rc = copy(skills);
  ind = m_indices(rc);
  for (i = sizeof(rc); i--; )
    for (j = 0; j < SKX_NO_FIELDS; j++)
      if (mappingp(rc[i,j]))
        rc[i,j] = copy(rc[i,j]);
      else if (pointerp(rc[i,j]))
        rc[i,j] = rc[i,j] + ({});
  return rc;
}

//---------------------------------------------------------------------------
public mixed * QuerySkill (string name)

// Query the data for one skill.

{
  mixed *rc;
  int i;

  rc = allocate(SKX_NO_FIELDS);
  if (!m_contains( &(rc[0]), &(rc[1]), &(rc[2]), &(rc[3]), &(rc[4])
                       , &(rc[5]), &(rc[6]), &(rc[7]), skills, name)
     )
    return 0;

  for (i = 0; i < SKX_NO_FIELDS; i++)
    if (mappingp(rc[i]))
      rc[i] = copy(rc[i]);
    else if (pointerp(rc[i]))
      rc[i] = rc[i] + ({});
  return rc;
}

//---------------------------------------------------------------------------
public int HasSkill (string name)

// Query if this server has the the data for one skill.

{
  mixed rc;

  return member(m_indices(skills), name) != -1;
}

//---------------------------------------------------------------------------
public varargs mixed * ResolveSkill ( string name, mixed receiver)

// Query the data for one skill, resolved for a living.

{
  mixed *rc, v0;
  int i;

  if (!receiver)
    receiver = PREV;

  if (objectp(receiver))
    receiver  = receiver->QueryProp(A_GUILD) || "other";

  if (!stringp(receiver))
    return 0;

  rc = allocate(SKX_NO_FIELDS);
  if (!m_contains( &(rc[0]), &(rc[1]), &(rc[2]), &(rc[3]), &(rc[4])
                       , &(rc[5]), &(rc[6]), &(rc[7]), skills, name)
     )
    return 0;

  for (i = 0; i < SKX_NO_FIELDS; i++)
    if (mappingp(rc[i]))
      if (!m_contains(&v0, rc[i], receiver))
        rc[i] = rc[i]["other"] || 100;
      else
        rc[i] = v0;
  return rc;
}

//---------------------------------------------------------------------------
public int ComputeSkill (string name, mixed * data, object obj)

// For an object, compute the current effective skill rating using the
// the given resolved data.
// This function is to compute the current effective value, not to 'use'
// the skill (or its underlying stats)!

{
  int rating;

  if (!obj && objectp(data)) {
    obj = (object) data;
    data = 0;
  }
  else if (!obj)
    obj = PREV;
  if (!pointerp(data))
    data = ResolveSkill(name, obj);

  rating = 0;
  if (data[SKX_STAT1])
    rating = STAT_SCALE*(int)obj->QueryAttr(data[SKX_STAT1], 0) * data[SKX_STAT1_FACTOR];
  if (data[SKX_STAT2])
    rating += STAT_SCALE*(int)obj->QueryAttr(data[SKX_STAT2], 0) * data[SKX_STAT2_FACTOR];
  rating += obj->QueryAttr(name) * data[SKX_SKILL_FACTOR];
  return rating / 100;
}

/***************************************************************************/
