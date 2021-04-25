/*---------------------------------------------------------------------------
** b/base.c
**
** The core of all objects.
** It contains:
**  - normalized properties
**  - automagic clean_up()
**  - sensible destruction
**  - save and restore.
**
** TODO: Recursive save()/restore() for containing objects.
**
** 05-Sep-1993 [Mateese] Written, normalized properties adapted for micro.
** 16-Oct-1993 [Mateese] Apply() added.
** 02-Jan-1994 [Mateese] replace_pgm() added.
** 20-Feb-1994 [Mateese] Changed property handling to direct function calls.
** 06-Jun-1994 [Mateese] Made Set() and Query() distinct from set() and query().
** 08-Jun-1994 [Mateese] Removed the need for method().
** 01-Jul-1994 [Mateese] Splitted the propmode off into an own mapping.
** 12-Jul-1994 [Mateese] Standard effective value query implemented.
** 21-Feb-1995 [Mateese] Rename P_AutoObject to P_Autodata.
**---------------------------------------------------------------------------
** Acknowledgements:
**   This property handling is an improvement of the properties originally
**   developed in Nightfall.
**   The 'Normalized Properties', developed by Hate@MorgenGrauen, were an
**   important stepstone (teaching me how I don't like to do it).
**---------------------------------------------------------------------------
*/

#include <driver/functionlist.h>
#include <b/base.h>

#define ENV   environment()
#define THIS  this_object()
#define PREV  previous_object()

//---------------------------------------------------------------------------

private static mapping props;       // The all-in-one property mapping.
private static mapping propmode;    // The modes of the props.
private        mapping properties;  // The mapping saved/restored.

// As zero-valued entries are not stored, and most properties mode is 0,
// using two mappings in fact reduces memory usage.

//---------------------------------------------------------------------------
public nomask varargs int QueryMode (string name)

// Return the mode of the named property.
//
// Arguments:
//   name : the name of the property to query the mode.
//
// Result:
//   An integer build from the combination of the flags PM_FIXED,
//   PM_PROTECTED, PM_NOSAVE, PM_PERM_HOOK and PM_EXIST as approbiate
//   (so 0 means "prop doesn't exist").

{ mixed data;
  int mode;

  if (!m_contains(&data, props, name))
    return 0;
  return (propmode[name] & ~PMF_TYPE) | PM_EXISTS;
}

//---------------------------------------------------------------------------
public nomask varargs int SetMode (string name, int new)

// Set the mode of the named property.
//
// Arguments:
//   name : the name of the property to set the mode.
//   new  : the new mode to set (e.g. a combination of PM_PROTECTED and PM_NOSAVE).
//
// Result:
//   The mode now in use for that property.
//
// It is illegal to change the mode of an already protected property.

{ int mode;

  new &= PMF_MODE;
  if (new & PM_FIXED)
    new |= PM_NOSAVE;

  mode = propmode[name];

  if (mode & PM_FIXED)
  {
    raise_error("Property "+name+" is fixed.\n");
    return mode;
  }
  if (mode & PM_PROTECTED && PREV != THIS)
  {
    raise_error("Property "+name+" is protected.\n");
    return mode;
  }

  new = new | (mode & ~PMF_MODE);
  if (new || props[name])
    propmode[name] = new;
  else {
    m_delete(props, name);
    m_delete(propmode, name);
  }

  return new;
}

//---------------------------------------------------------------------------
// static nomask void setType (string name, int mode)

// Set a type mode of a property.
//
// name : the name of the property to set the type mode.
// value: the type mode to set.

#define setType(name,mode) propmode[name] = (propmode[name] & ~(PMF_TYPE)) | (mode & PMF_TYPE)

//---------------------------------------------------------------------------
static nomask varargs mixed set (string name, mixed value)

// Set a mapped property.
//
// Arguments:
//   name  : the name of the property to set.
//   value : the value to set
//
// Result:
//   The value set.

{ int mode;

  mode = propmode[name];

  if (mode & PM_FIXED)
  {
    raise_error("Property "+name+" is fixed.\n");
    return props[name];
  }

  // only the object itself is allowed to change protected properties
  // Check of the creator is a hack since previous_object() might be the
  // cloner during creation.
  // Check of PREV is done to catch calls by the gamedriver.
  if (    PREV && PREV != THIS && (mode & PM_PROTECTED)
       && to_string(PREV) != props[P_Creator]
     )
  {
    raise_error ("Property "+name+" is protected.\n");
    return props[name];
  }

  if (mode || value)
    props[name] = value;
  else
    m_delete(props, name);
  return value;
}

//---------------------------------------------------------------------------
public nomask varargs mixed query (string name)

// Query a mapped property.
//
// Arguments:
//   name : the name of the property to query.
//
// Result:
//   The value queried.

{
  return props[name];  // so easy...
}

//---------------------------------------------------------------------------
public nomask mixed SetProps (mapping new)

// Set all mapped properties and their modes at once.
//
// Argument:
//   new: the new property mapping with two elements per entry:
//        F_VALUE: the value itself
//        F_MODE : its mode
//
// Result:
//   The newly set property mapping, as returned by QueryProps().
//
// It is not possible to change protected properties from outside, or
// fixed properties at all.

{
  mixed *names;
  mixed name;
  int outside;
  int mode;
  int i;

  outside = PREV && PREV != THIS && to_string(PREV) != props[P_Creator];

  names = m_indices(new);
  for (i = sizeof(names); i-- > 0; )
  {
    name = names[0];
    mode = propmode[name];
    if ((mode & PM_FIXED) || (outside && mode & PM_PROTECTED))
      continue;
    if (new[name, F_VALUE] || new[name,F_MODE]) {
      props[name] = new[name, F_VALUE];
      propmode[name]  = new[name, F_MODE];
    }
    else
      m_delete(props, name);
  }
  new = 0;
  return QueryProps();
}

//---------------------------------------------------------------------------
private static void _insert_mode (string name, mixed value, mixed mode)

// Auxiliary function to QueryProps() to prepare the result mapping.
//
// Arguments:
//   name : the index of the entry under preparation
//   value: the reference of the value field for <name>
//   mode : the reference of the mode field for <name>
//
// Result:
//   <mode> is set to propmode[name].

{
  mode = propmode(name);
}

public nomask mapping QueryProps ()

// Return the complete property mapping including modes (a copy of it!).
// For each name, the subindex F_VALUE holds the value, and F_MODE its mode.

{
  mapping rc;

  rc = m_allocate(0,2) + props; // Enforce two elements per entry.
  return walk_mapping(rc, #'_insert_mode);
}

//---------------------------------------------------------------------------
public mapping getProps ()

// Return the saveable properties as one mapping, with F_VALUE holding
// the value, and F_MODE holding the mode.
// Fixed properties are not saveable per definition.

{ mixed *names;
  mixed name;
  mapping rc;
  int i, mode;

  rc = m_allocate(0,2) + props;  // Enforce two fields per entry

  names = m_indices(props);
  for (i = sizeof(names); i-- > 0; )
  {
    name = names[i];
    mode = propmode[name];
    if (!(mode & (PM_NOSAVE|PM_FIXED)))
      rc[name, F_MODE] = mode;
    else
      m_delete(rc, name);
  }
  return rc;
}

//---------------------------------------------------------------------------
public void putProps (mapping p)

// Restore the given properties.
// Fixed properties can't be restored, protected ones not from outside.

{ mixed *names;
  mixed name;
  int i;
  int outside, mode;

  outside = PREV && PREV != THIS && to_string(PREV) != props[P_Creator];

  names = m_indices(props);
  for (i = sizeof(names); i-- > 0; )
  {
    name = names[i];
    mode = propmode[name];
    if (!( (mode & PM_FIXED) || (outside && mode & PM_PROTECTED)))
      if (mode || p[name])
        props[name] = p[name];
      else
        m_delete(props, name);
  }
}

//---------------------------------------------------------------------------
public mixed QueryAutodata ()

// Return the data needed for autoloading.

{
  return Query(P_NoAutoload) ? 0 : getProps();
}

//---------------------------------------------------------------------------
public mixed SetAutodata (mixed p)

// Restore the data saved over autoloading.

{
  if (mappingp(p))
    putProps(p);
  p = 0;
  return QueryAutodata();
}

//---------------------------------------------------------------------------
public int SetCloned (int v)

// Set the Cloned flag.
// This call is used by the clones to notify the blueprint that it has
// been cloned.

{
  if (!PREV || PREV == THIS)
    return v;
  if (v < 0 && file_name(THIS) == blueprint(THIS)
            && file_name(PREV) == blueprint(THIS)
     )
    v = THIS->Set(P_Cloned, v);
  return v;
}

//---------------------------------------------------------------------------
public void save (string file)

// Save the object into <file>.o.
//
// Argument:
//   file: the name of the savefile.
//
// This is in fact a save_object(), but takes care of saving the saveable
// properties. Fixed properties aren't saved.

{
  properties = getProps();
  efun::save_object(file);
  properties = 0;    // Conserve memory.
}

//---------------------------------------------------------------------------
public int restore (string file)

// Restore the object from <file>.o.
//
// Argument:
//   file: the name of the savefile.
//
// Result:
//   True on success.
//
// This is in fact a restore_object(), but takes care of restoring of saved
// and unfixed properties.

{
  int rc;

  if (rc = efun::restore_object(file))
    putProps(properties);
  properties = 0;
  return rc;
}

//---------------------------------------------------------------------------
public int remove()

// Selfdestruct the object.
//
// Use of this function ensures that the environment is updated correctly.

{
  if (ENV)
  {
    // TODO: Adapt this to the actual move() handling.
    ENV->NotifyLeave(0, M_DESTRUCT, 0);
  }
  destruct(THIS);
  return 1;
}

//---------------------------------------------------------------------------
public int clean_up (int ref)

// Allow the object to clean up itself.
//
// Argument:
//   ref < 0: the environment calls the object to clean up.
//       = 0: object is a clone, or a blueprint with replaced program.
//       = 1: object is swapped and/or unreferenced blueprint.
//       > 1: object has <ref> references.
//
// Result:
//   0: object will never clean up (or did selfdestruct with this call).
//   1: object does not clean up now, but wants to be asked again later.
//
// The function calls Query(P_Cleanup, ref) and analyzes the result:
//   <= 0: object selfdestructs if it has no environment.
//    = 1: object will never clean up.
//    > 1: object might clean up later.


{
  int rc;

  rc = THIS->Query(P_Cleanup, ref);
  if (rc < 1) {
    if (!ENV)
      remove();
    rc = 2;
  }
  return THIS && (rc > 1);
}

//---------------------------------------------------------------------------
public varargs string replace_pgm (string *except)

// Replace this objects program by one parents program.
//
// Argument:
//   except: one string/an array of strings denoting the names of functions
//           which may be ignored during the checks.
//
// Result:
//   The file_name() of the object providing the replace program, or 0 if
//   no replace was possible.
//
// The function checks if this object is able to replace its program (it
// mustn't be a clone, and mustn't contain other functions than create()
// and those given in 'except'.

{
  mixed *arr;
  object ob;
  int s;
  string rc;

  if (Query(P_Inherited) || Query(P_Cloned) || member(file_name(THIS), '#') >= 0)
    return 0;

  if (stringp(except))
    except = ({ except, "create", "create_inh", "create_clone", "create_obj" });
  else if (pointerp(except))
    except += ({ "create", "create_inh", "create_clone", "create_obj" });
  else
    except = ({ "create", "create_inh", "create_clone", "create_obj" });

  // Check all non-except functions if they are inherited.
  arr = functionlist(THIS, 3);
  for (s = sizeof(arr) - 1; s > 0; s -= 2)
    if (!(arr[s] & NAME_INHERITED) && member(except, arr[s-1]) < 0)
      return 0;

  // Check if we have just one direct parent and replace with that.
  arr = inherit_list();
  s = sizeof(arr) - 1;
  rc = 0;
  if (s && (ob = find_object(arr[1])) && s == sizeof(inherit_list(ob)))
  {
    rc = file_name(ob);
    replace_program(rc);
  }

  return rc;
}

//---------------------------------------------------------------------------
public varargs mixed Query ( string name
                           , mixed arg1, mixed arg2, mixed arg3, mixed arg4
                           , mixed arg5, mixed arg6, mixed arg7, mixed arg8
                           )

// Default function to query properties.
//
// Arguments:
//   fun       : the name of the property to query.
//   arg1..arg8: the arguments given to the call.
//
// Result:
//   Whatever it should be, default is 0.
//
// The function is called by other object to query a property.
// The basic value is retrieved from the property mapping, but may be
// modified by a lfun Query<name>(<value>, <arg1>..<arg8>). If it exists,
// its result value is returned instead of the basic value.

{
  mixed rc, rc2;

  rc2 = query(name);
  // TODO: If mode == PM_STRING, rc = basisstring(rc);
  if (call_resolved(rc = call_other( THIS, "Query"+name, rc2, arg1, arg2, arg3
                                   , arg4, arg5, arg6, arg7, arg8))
     )
    return rc;
  return rc2;
}

//---------------------------------------------------------------------------
public varargs mixed Set ( string name
                         , mixed arg1, mixed arg2, mixed arg3, mixed arg4
                         , mixed arg5, mixed arg6, mixed arg7, mixed arg8
                         )

// Default function to set properties.
//
// Arguments:
//   fun       : the name of the property to set.
//   arg1      : the value to set, normally.
//   arg2..arg8: other arguments given to the call.
//
// Result:
//   Whatever it should be, default is 0.
//
// The function is called by other object to set a property. Basically
// it sets the value into the property mapping, but before it may be
// filtered through a lfun Set<name>(&<arg1>, <arg2>..<arg8>). This function
// has to modify <arg1> into the value to set in the mapping. The
// result from the Set()-call is then the return value.

{
  mixed rc;

  if (call_resolved(rc = call_other( THIS, "Set"+name, &arg1, arg2, arg3
                                    , arg4, arg5, arg6, arg7, arg8))
     )
    set(name, arg1);
  else
    rc = set(name, arg1);

  if (stringp(arg1))
    setType(name, PM_STRING);
  else
    setType(name, 0);
  return rc;
}

//---------------------------------------------------------------------------
public varargs mixed SetHook ( string name
                             , mixed arg1, mixed arg2, mixed arg3, mixed arg4
                             , mixed arg5, mixed arg6, mixed arg7, mixed arg8
                             )

// Default function to set hook properties.
//
// Arguments:
//   fun       : the name of the property to set.
//   arg1      : the value to set, normally.
//   arg2..arg8: other arguments given to the call.
//
// Result:
//   Whatever it should be, default is 0.
//
// The function is called by other object to set a property. Basically
// it sets the value into the property mapping, but before it may be
// filtered through a lfun Set<name>(&<arg1>, <arg2>..<arg8>). This function
// has to modify <arg1> into the value to set in the mapping. The
// result from the Set()-call is then the return value.
// The difference to Set() is that the PM_HOOK mode is set if <arg1> is
// a closure.

{
  mixed rc;

  if (call_resolved(rc = call_other( THIS, "Set"+name, &arg1, arg2, arg3
                                    , arg4, arg5, arg6, arg7, arg8))
     )
    set(name, arg1);
  else
    rc = set(name, arg1);

  if (stringp(arg1))
    setType(name, PM_STRING);
  else if (closurep(arg1))
    setType(name, PM_HOOK);
  else
    setType(name, 0);
  return rc;
}

//---------------------------------------------------------------------------
public varargs mixed Get ( string name
                         , mixed arg1, mixed arg2, mixed arg3, mixed arg4
                         , mixed arg5, mixed arg6, mixed arg7, mixed arg8
                         )

// Dispatch function to query the effective value of a property.
//
// Arguments:
//   fun       : the name of the property to query.
//   arg1..arg8: the arguments given to the call.
//
// Result:
//   Whatever it should be, default is 0.
//
// The function is called by other object to query a property. It recognizes
// the existance of <name>() or Query<name>() functions and filters the value
// from the property mapping through them.

{
  mixed rc, rc2;
  int   mode;

  rc = query(name);
  mode = propmode[name] & PMF_XTYPE;
  if (mode & PM_STRING)
  {
    // TODO:
    // if (stringp(rc))
    //   rc = set(name, compile_dynstring(rc));
    // if (stringp(rc))
    //   setType(name, 0);
    setType(name, 0);
  }
  else if (mode & (PM_HOOK|PM_PERM_HOOK))
    rc = funcall(rc, name, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8));
  if (call_resolved(rc2 = call_other( THIS, name, arg1, arg2, arg3
                                    , arg4, arg5, arg6, arg7, arg8))
     )
    return rc2;
  if (call_resolved(rc2 = call_other( THIS, "Query"+name, arg1, arg2, arg3
                                    , arg4, arg5, arg6, arg7, arg8))
     )
    return rc2;
  return rc;
}

//---------------------------------------------------------------------------
public void create ()

// Initialise the object and set some initial properties.

{
  if (!props)
  {
    props    = m_allocate(0);   // setup empty mappings
    propmode = m_allocate(0);
  }

  // The creator, fixed -- must come first!

  Set(P_Creator, to_string(PREV || "<unknown>"));
  SetMode(P_Creator, PM_FIXED|PM_NOSAVE);
  Set(P_ICreator, to_string(TI) || "<unknown>");
  SetMode(P_ICreator, PM_FIXED|PM_NOSAVE);
  Set(P_CreaTime, time());
  SetMode(P_CreaTime, PM_FIXED|PM_NOSAVE);
}

//---------------------------------------------------------------------------
public void create_inh ()

// Initialise the inherited object and set some initial properties.

{
  if (!props)
  {
    props    = m_allocate(0);   // setup empty mappings
    propmode = m_allocate(0);
  }

  Set(P_Inherited, 1);
  SetMode(P_Inherited, PM_FIXED|PM_NOSAVE);
  Set(P_Cloned, 0);
  SetMode(P_Cloned, PM_PROTECTED|PM_NOSAVE);

  // Normal initialisation
  create();
}

//---------------------------------------------------------------------------
public void create_clone ()

// Initialise the inherited object and set some initial properties.

{
  if (!props)
  {
    props    = m_allocate(0);   // setup empty mappings
    propmode = m_allocate(0);
  }

  Set(P_Inherited, 0);
  SetMode(P_Inherited, PM_FIXED|PM_NOSAVE);
  Set(P_Cloned, 1);
  SetMode(P_Cloned, PM_PROTECTED|PM_NOSAVE);

  blueprint()->Set(P_Cloned, -1);

  // Normal initialisation
  create();
}

//---------------------------------------------------------------------------
public void create_obj ()

// Initialise the inherited object and set some initial properties.

{
  if (!props)
  {
    props    = m_allocate(0);   // setup empty mappings
    propmode = m_allocate(0);
  }

  Set(P_Inherited, 0);
  SetMode(P_Inherited, PM_FIXED|PM_NOSAVE);
  Set(P_Cloned, 0);
  SetMode(P_Cloned, PM_PROTECTED|PM_NOSAVE);

  // Normal initialisation
  create();
}

//---------------------------------------------------------------------------
public int prevent_shadow (object shadow)

{
   string fname;

   fname = file_name(THIS);
   if (find_object(explode(fname, "#")[0]) != THIS)
     return 0;
   return (   fname[0..2] == "/b/"
           || fname[0..4] == "/adm/"
           || fname[0..4] == "/g/"
           || fname[0..4] == "/std/"
           || fname[0..7] == "/secure/"
          );
}

/***************************************************************************/
