/* -------------------------------------------------------------------------
 * vitems.c -- virtual item handling
 *
 * Copyright (C) 1997 by One Step Beyond.
 * All Rights Reserved.
 * Permission granted for non-commercial use.
 * -------------------------------------------------------------------------
 * TODO:  this is just the first step for virtual items,
 * TODO:: they should be getable and searchable too and be printed
 * TODO:: at the room desc like normal items
 * TODO:: Explorer Handling needs to be readded
 * TODO:: reactivate PV_SURFACE, take care of problems with AddDetail, maybe
 * TODO::  this one messes up the compat code for identifying similar details
 *
 * CHANGES
 *  05/28/98 Mica Improved preposition handling inside GetVItem, now uses
 *                regular expressions, a bit faster and much more elegant
 */

#include <properties.h>
#include <vitems.h>
#include <macros.h>
#include <driver/regexp.h>

/* Define VITEMS_CLEANUP_LAST to cleanup the P_LAST_-properties
 * that is done by TP->RegisterCmdFun();
 */

#define VITEMS_CLEANUP_LAST

/* Define VITEMS_SURFACE to add the surface-code (PV_SURFACE), which is
 * currently not working. Therefor that part is disabled for now
 */

#undef VITEMS_SURFACE

/* --------------------------------------------------------------------------
 * Global variables
 * --------------------------------------------------------------------------
 */

nosave mapping Pvitems;

/* --------------------------------------------------------------------------
 * Prototypes
 * --------------------------------------------------------------------------
 */

static varargs string add_vitem( mapping m, string v_id ); // forward
static void remove_vitem( string id ); // forward
public varargs mapping GetVItemData( string v_id, int lvl ); // forward
public varargs mixed Query(string pn, int sc); // /std/base
public varargs mixed Set(string pn, mixed arg, int sc); // /std/base

/* --------------------------------------------------------------------------
 * Builtin property P_VITEMS
 * --------------------------------------------------------------------------
 */

public mapping QueryVItems() {
  return Pvitems; /* we do not copy here, reason is speed. */
}

public mapping SetVItems( mapping m ) {
  return Pvitems = m;
}

#ifdef VITEMS_CLEANUP_LAST
/* --------------------------------------------------------------------------
 * helpfun to clean P_LAST_ID, P_LAST_ADS and P_LAST_PREP
 *  the fun is registered as command fun and executed after the current
 *  command ended. Purpose is to save memory
 * --------------------------------------------------------------------------
 */

public void v_clean_last_props( mixed v_ids ) {
  int i;
  mapping dat;

  if ( ! v_ids )
    return;

  for( i = 0; i < sizeof( v_ids ); i ++ ) {
    if ( ! dat = GetVItemData( v_ids[i] ) )
      continue;
    m_delete( dat, P_LAST_ID );
    m_delete( dat, P_LAST_ADS );
    m_delete( dat, P_LAST_PREP );
  }
}

private void register_cmd_fun( mixed v_ids ) {
  if ( TP ) {
    if ( ! v_ids )
      return;

    if ( ! pointerp( v_ids ) )
      v_ids = ({ v_ids });

    if ( ! sizeof( v_ids ) )
      return;

    TP->RegisterCmdFun( SF( v_clean_last_props ), v_ids );
  }
}
#endif

/* --------------------------------------------------------------------------
 * public helpfun to make an ids 'n ads mapping from det
 * --------------------------------------------------------------------------
 */

#if __VERSION__ > "3.5.0"
public mapping ids_n_ads( mixed|string det ) {
#else
public mapping ids_n_ads( mixed det ) {
#endif
  int i, ii;
  string * tx;
  mapping r;

  if ( ! pointerp( det ) )
    det = ({ det });

  r = ([ P_IDS : ({}), P_ADS : ({}) ]);

  for( i = 0; i < sizeof( det ); i ++ ) {
    tx = explode( det[i], " " ) - ({""});
    if ( member( r[P_IDS], tx[<1]) == -1 )
      r[P_IDS] += ({ tx[<1] });

    if (sizeof(tx) > 1)
      tx = tx[0..<2];
    else
      tx = ({});
    for( ii = 0; ii < sizeof( tx ); ii++ ) {
      if ( member( r[P_ADS], tx[ii] ) == -1 )
        r[P_ADS] += ({ tx[ii] });
    }
  }

  if ( ! sizeof( r[P_ADS] ) )
    m_delete( r, P_ADS ); /* remove empty ads array */

  if ( sizeof( r[P_IDS] ) )
    return r;
  else
    return 0;
}

/* ------------------------------------------------------------------------
 * helpfun for GetVItem
 * ------------------------------------------------------------------------
 */

private mixed * gv_buildarray( string str, mixed * res ) {
  string tx;
  mixed * ret;
  int i;

  tx = "\n    ("+str+", here)\n";

  /* we need to build an array for return, because res may contain
     array-entries */
  ret = ({ "", "" });
  for( i = 0; i < sizeof( res ); i ++ ) {
    if ( stringp( res[i] ) ) {
      ret[0] += ( tx + res[i] );
      ret[1] += ( tx + "You see nothing special.\n"+ res[i] );
    }
    else if ( ( pointerp( res[i] ) ) && ( sizeof ( res[i] ) ) ) {
      if ( sizeof( res[i] ) == 1 ) { /* handle some abuse */
        if ( stringp( res[i][0] ) ) {
          ret[0] += ( tx + res[i][0] );
          ret[1] += ( tx + "You see nothing special.\n"+ res[i][0] );
        }
      }
      else {
        if ( stringp( res[i][0] ) )
          ret[0] += ( tx + res[i][0] );
        if ( stringp( res[i][1] ) )
          ret[1] += ( tx + res[i][1] );
      }
    }
  }
  /* look or examine entry could be empty, handle it right */
  if ( ret[0] == "" )
    ret[0] = 0;
  if ( ret[1] == "" )
    ret[1] = 0;

  return ret;
}

/* ---------------------------------------------------------------------------
 * GetVItem - Get the value for the vitem 'str'
 *  The optional arg 'prop' defines which property you want to query, if
 *  omited, P_LONG will be used. If set to an integer, special things are
 *  done.
 * Currently defined for intp(prop):
 *  1 : the function will return 1 if the vitem exists and 0 if not.
 *  2 : the function will return the vitem mapping id.
 * ---------------------------------------------------------------------------
 * TODO:  prep handling conflicts with 'all' handling
 * TODO:: for multi-looks making vitems short from ads+ids is worse
 * TODO:: Explorer handling should be done by the vitems (P_EXPLORABLE ?)
 */

public varargs mixed GetVItem( string str, mixed prop, mapping vi ) {
  int i, i2, wall, fullmatch;
#ifdef VITEMS_SURFACE
  int is_inside;
#endif
  mapping m;
  mixed res, tx, wcount;
  string id, * ads, * keys, prep, rest, rstring;
#ifdef VITEMS_CLEANUP_LAST
  string * v_ids;
#endif

  /* fill varargs fields
   */

  if ( ! prop )
    prop = P_LONG;

  if ( ! vi )
    vi = Pvitems;

  if ( ! sizeof( vi ) )
    return 0; /* nothing to search */

  res = ({}); /* res will hold all supported preps for some time now */
  keys = m_indices( vi );
  for( i = 0; i < sizeof( vi ); i ++ ) {
    if ( ! mappingp( m = vi[keys[i]] ) )
      continue; /* malformed entry */
    if ( pointerp( m[P_PREPS] ) )
      res += m[P_PREPS];
  }

  /* use norm_id to get rid of double blanks and uppercase
   */

  str = norm_id( str );

  /* Get id to match, used preposition and rest ( subdetail ids )
   */

  if ( sizeof( res ) ) {
    /* make regular expression from preps, we use word-matching here
     */
#if __PCRE__
    rstring = "\\b(" + implode( res, "|" ) + ")\\b";
#else
    rstring = "\\<(" + implode( res, "|" ) + ")\\>";
#endif
    /* regexplode now, valid match if we have at least three entries in the
       result. The last entry must not be "", that would mean that preposition
       is used as the last word, which is invalid too
     */
    tx = regexplode( str, rstring);

    if ( ( sizeof( tx ) >= 3 ) && sizeof( tx[<1] ) ) {
      /* preposition found */
      prep = tx[<2];
      str = tx[<1];
      rest = implode( tx[0..<3], "" );
      if ( ! sizeof( rest ) )
      	rest = 0; /* this happens if preposition is used in first place */
    } else {
      /* no preposition found, str remains unchanged */
      prep = 0;
      rest = 0;
    }
  }

  /* now get id and adjectives from our id */
  wcount = 0;
  wall = 0;
  ads = explode( str, " " ) - ({ "" });
  switch( sizeof( ads ) ) {
  case 0:
    return 0;
  case 1:
    id = ads[0];
    ads = ({});
    break;
  default:
    id = ads[<1];
    ads = ads[0..<2];

    if ( ads[0] == "all" ) {
      if (sizeof(ads) > 1)
        ads = ads[1..];
      else
        ads = ({});
      wall = 1;
    }
    else if ( intp( wcount = atoi( id ) ) && ( wcount > 0 ) ) {
      id = ads[<1];
      ads = ads[0..<2];
    }
    else {
      if ( intp( wcount = atoo( ads[0] ) ) && ( wcount > 0 ) )
        ads = ads[1..];
      else if ( intp( wcount = atoo( ads[<1] ) ) && ( wcount > 0 ) )
        ads = ads[0..<2];
    }
    break;
  }

  if ( ! intp( wcount ) )
    wcount = 0; /* maybe it is a string because of atoi */

  if ( wcount < 0 )
    wcount = 0; /* this happens because of silly atoo */
  else if ( wcount )
    wcount --;

  /* rebuild str from id and ads, this is needed to get rid of a count
   * that has been given
   */

  str = ( sizeof( ads ) ? ( implode( ads, " " ) + " " ) : "" ) + id;

#ifdef VITEMS_SURFACE
  /* is_inside is set to 1 if the player is in the inventory of this_object()
   * we need it to speed up checking for PV_SURFACE
   */

  if ( ! this_player() )
    is_inside = 0;
  else
    is_inside = ( environment( this_player() ) == this_object() );
#endif

  res = ({}); /* this will contain the collected matches for all-looks */
#ifdef VITEMS_CLEANUP_LAST
  v_ids = ({});
#endif

  /* get the keys of vi and handle all of 'em */
  keys = m_indices( vi );
  for( i = 0; i < sizeof( keys ); i ++ ) {
    if ( ! mappingp( m = vi[keys[i]] ) )
      continue; /* malformed entry */
#ifdef VITEMS_SURFACE
    if ( m[PV_SURFACE] ) {
      if ( is_inside )
        continue;
    }
    else {
      if ( ! is_inside )
        continue;
    }
#endif

    /* check prepositions first
     */
    if ( ( ! prep )
         && m[P_PREP_REQUIRED] )
      continue; /* we require a proposition */

    if ( ( prep )
         && member( m[P_PREPS]||({}), prep ) == -1 )
      continue; /* preposition, but does not match for this vitem */

    /* check whole id first and then based on id/ads
     */

    if ( member( m[P_IDS]||({}), str ) == -1 ) {
      /* check based on id and adjectives
       */

      if ( member( m[P_IDS]||({}), id ) == -1 )
        continue;

      /* check if all ads match */
      tx = 0;
      for( i2 = 0; i2 < sizeof( ads ); i2 ++ ) {
        if ( member( m[P_ADS]||({}), ads[i2]) == -1 )
          tx = 1;
      }

      /* mismatch on ads, continue */
      if ( tx )
        continue;

      fullmatch = 0;
    }
    else {
      fullmatch = 1;
    }

    if ( ! wcount ) {
      if ( rest ) {
        if ( m[P_VITEMS]
             && sizeof( m[P_VITEMS] ) )
          return GetVItem( rest, prop, m[P_VITEMS] );
        else
          return 0; /* prep but no subvitems */
      }
      else {
#ifdef VITEMS_CLEANUP_LAST
        /* remember the v_ids to cleanup */
        v_ids += ({ keys[i] });
#endif

        if ( fullmatch ) {
          /* set last id, ads */
          m[P_LAST_ID] = str;
          m[P_LAST_ADS] = 0;
        }
        else {
          /* set last id, ads */
          m[P_LAST_ID] = id;
          m[P_LAST_ADS] = ads;
        }

        /* set last preposition */
        m[P_LAST_PREP] = prep;

        if ( intp( prop ) ) {
          if ( prop == 1 ) {
#ifdef VITEMS_CLEANUP_LAST
            register_cmd_fun( v_ids );
#endif
            return 1; /* prop == 1 means check for match only */
          }
          else if ( prop == 2 ) {
#ifdef VITEMS_CLEANUP_LAST
            register_cmd_fun( v_ids );
#endif
            return keys[i]; /* prop == 2 means return the mapping id */
          }
          else {
#ifdef VITEMS_CLEANUP_LAST
            register_cmd_fun( v_ids );
#endif
            return 0; /* invalid call */
          }
        }

        /* now handle the property */
        if ( wall )
          res += ({ funcall( m[prop], str, keys[i] ) });
        else {
#ifdef VITEMS_CLEANUP_LAST
          register_cmd_fun( v_ids );
#endif
          return funcall( m[prop], str, keys[i] );
        }
      }
    }
    else
      wcount --;
  }

#ifdef VITEMS_CLEANUP_LAST
  register_cmd_fun( v_ids );
#endif

  /* we do multi-look and have found some matches */
  if ( ( wall ) && sizeof( res ) )
    return gv_buildarray( str, res );

  return 0;
}

/* --------------------------------------------------------------------------
 * GetVItemData( string <id> )  - return the mapping for the given id
 * --------------------------------------------------------------------------
 */

private mapping get_vitem_data( string v_id, mapping vi, string * rest ) {
  if ( ( ! vi ) ||
       ( ! vi[v_id] ) )
    return 0;

  if ( ! sizeof( rest ) )
    return vi[v_id];
  if (sizeof(rest) < 2)
    return get_vitem_data( v_id+"#"+rest[0],
                           vi[v_id][P_VITEMS],
                           ({}) );
else
    return get_vitem_data( v_id+"#"+rest[0],
                           vi[v_id][P_VITEMS],
                           rest[1..] );
}

public varargs mapping GetVItemData( string v_id, int lvl ) {
  string * strs;

  if ( ! stringp( v_id ) )
    return 0;

  strs = explode( v_id, "#" );
  if ( ( sizeof( strs ) < 2 )
       || ( strs[0] != "vitem" ) )
    return 0; /* invalid id */
  if (sizeof(strs) < 3)
    return get_vitem_data( implode( strs[0..1], "#" ), Pvitems, ({}) );
  else
    return get_vitem_data( implode( strs[0..1], "#" ), Pvitems, strs[2..<(lvl+1)] );
}

/* --------------------------------------------------------------------------
 * static Pvitems helpfuns
 * --------------------------------------------------------------------------
 */

static varargs string get_free_id( string v_id ) {
  int idcounter;
  mapping dat, vi;

  if ( ! v_id ) {
    v_id = "vitem";
    idcounter = Query( P_VITEM_COUNTER );
    vi = Pvitems;
    dat = 0;
  }
  else {
    dat = GetVItemData( v_id );
    idcounter = dat[P_VITEM_COUNTER];
    vi = dat[P_VITEMS];
  }

  while( vi &&vi[ v_id+"#"+to_string(idcounter)] )
    idcounter++;

  /* store the counter for the next vitem
   */
  if ( dat )
    dat[P_VITEM_COUNTER] = idcounter + 1;
  else
    Set( P_VITEM_COUNTER, idcounter + 1 );

  /* return our id
   */

  return v_id+"#"+to_string(idcounter);
}

static varargs string add_vitem( mapping m, string v_id ) {
  string v_id2;
  mapping vi;

  if ( ! mappingp( m ) )
    return 0;

  if ( ! v_id ) {
    if ( ! Pvitems )
      Pvitems = ([]);

    vi = Pvitems;
  }
  else {
    vi = GetVItemData( v_id );
    if ( ! vi )
      return 0;
    if ( ! vi[P_VITEMS] )
      vi = ( vi[P_VITEMS] = ([]) );
    else
      vi = vi[P_VITEMS];
  }

  v_id2 = get_free_id( v_id );

  vi[v_id2] = m;

  return v_id2;
}

static void remove_vitem( string id ) {
  mapping m;

  if ( ! m = GetVItemData( id, 1 ) )
    return 0;

  if ( ! m[P_VITEMS] ) {
    if ( mappingp( Pvitems ) ) {
      m_delete( Pvitems, id );

      if ( ! sizeof( Pvitems ) )
        Pvitems = 0;
    }
  }
  else {
    m_delete( m[P_VITEMS], id );
    if( ! sizeof( m[P_VITEMS] ) )
      m_delete( m, P_VITEMS );
  }
}

static mixed modify_vitem( string id, string prop, mixed val ) {
  mapping dat;

  if ( ! dat = GetVItemData( id ) )
    return 0;

  if ( val ) {
    dat[prop] = val;
    return id;
  }
  else {
    m_delete( dat, prop );
    if ( ! sizeof( m_indices( dat ) -
                   ({ P_IDS, P_ADS }) ) )
      remove_vitem( id );
  }

  return 1;
}

/* -------------------------------------------------------------------------
 * AddVItem add a virtual item
 *
 *  You have to provide all data for the vitem with a mapping that
 *  contains P_IDS, P_ADS, P_LONG and whatever you wish.
 * -------------------------------------------------------------------------
 */

public varargs mixed AddVItem( mapping det, string v_id ) {
  if ( mappingp( det ) ) {
    if ( ! pointerp( det[P_IDS] ) ) {
      if (!det[P_IDS]) {                     // is there an single entry?
        MASTER->runtime_error(               // no ?! -> error
          "Call to AddVItem() with no P_IDS !\n",
          "/std/room/vitems",
          object_name(this_object()),
          __LINE__   );
        return 0; /* invalid call */
      } else {
        det[P_IDS] = ({det[P_IDS]});         // yes it is - make an array
      }
    }
    if ( ! pointerp( det[P_ADS] ) ) {        // ads array?
      if (det[P_ADS]) {                      // single ads?
        det[P_ADS] = ({det[P_ADS]});         // make array of single entry
      }
    }

    return add_vitem( det, v_id );
  }

  return 0;
}

/* -------------------------------------------------------------------------
 * RemoveVItem - remove the given vitem
 * -------------------------------------------------------------------------
 */

public void RemoveVItem( mixed det ) {
  int i;

  if ( ! pointerp( det ) )
    det = ({ det });

  for( i = 0; i < sizeof( det ); i ++ )
    remove_vitem( det[i] );
}

/* -------------------------------------------------------------------------
 * VSet - set a property for the given vitem
 *  Use val = 0 to remove the key.
 * -------------------------------------------------------------------------
 */

public mixed VSet( string id, string prop, mixed val ) {
  return modify_vitem( id, prop, val );
}

/* -------------------------------------------------------------------------
 * VQuery - query a property from the given vitem
 * -------------------------------------------------------------------------
 */

public mixed VQuery( string id, string prop ) {
  mapping dat;

  if ( ! dat = GetVItemData( id ) )
    return 0;

  return dat[prop];
}

/* -------------------------------------------------------------------------
 * room identification by virtual items
 * -------------------------------------------------------------------------
 */

public int id(string str) {
  return GetVItem( str, 1 );
}

/* -------------------------------------------------------------------------
 * Create
 * -------------------------------------------------------------------------
 */

public void create() {
  Pvitems = 0;
}

/* *********************************************************************** */
