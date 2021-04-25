/* -------------------------------------------------------------------------
 * /std/room/details.c -- compat module for old detail handling
 *
 * Copyright (C) 1997 by One Step Beyond.
 * All Rights Reserved.
 * Permission granted for non-commercial use.
 * -------------------------------------------------------------------------
 */

#include <properties.h>
#include <nightday.h>
#include <vitems.h>
#include <macros.h>

inherit "/std/base";
inherit "/std/room/vitems";

/* -------------------------------------------------------------------------
 * static helpfun for easy addin/removing
 * -------------------------------------------------------------------------
 * TODO:  the GetVItem for finds always the first but not the best match,
 * TODO:: this is wanted for 'look' etc. but is not best for add_compat.
 * TODO:: The fun only returns the first added/modified detail
 */

static mixed add_compat( mixed det, string prop, mixed desc ) {
  mapping tm;
  string * v_ids, v_id;
  int i;

  if ( ! pointerp( det ) )
    det = ({ det });

//printf("START-DET: %O\n",det);
  /* try to find matches for the given strings */
  v_ids = ({});
  for( i = 0; i < sizeof( det ); i ++ ) {
//printf("TESTING: %O\n",det[i]);
    v_id = GetVItem( det[i], 2 );
//printf("GETV?: %O\n",v_id);
    if ( v_id ) {
//printf("V_ID,V_IDS?: %O,%O\n",v_id,v_ids);
      if ( member( v_ids, v_id ) == -1 )
        v_ids += ({ v_id });
      det[i] = 0;
//printf("V_IDS: %O\n",v_ids);
    }
  }

//printf("DET,PROP,VIDs: %O,%O,%O\n",det,prop,v_ids);
  /* clean up our data */
  det -= ({ 0 });

  /* modify the found ones */
  for ( i = 0; i < sizeof( v_ids ); i ++ )
    modify_vitem( v_ids[i], prop, desc );

  /* add the not found ones as single vitem */
  if ( sizeof( det ) && ( tm = ids_n_ads( det ) ) )
    v_ids += ({ add_vitem( tm + ([ prop : desc ]) ) });

  /* just in case */
  v_ids -= ({ 0 });

  if ( sizeof( v_ids ) )
    return v_ids[0];
  else
    return 0;
}

static void remove_compat( mixed det, string prop ) {
  string * v_ids, v_id;
  int i;

  if ( ! pointerp( det ) )
    det = ({ det });

  /* try to find matches for the given strings */
  v_ids = ({});
  for( i = 0; i < sizeof( det ); i ++ ) {
    if ( v_id = GetVItem( det[i], 2 ) ) {
      if ( member( v_ids, v_id ) == -1 )
        v_ids += ({ v_id });
    }
  }

  /* remove the found ones */
  for ( i = 0; i < sizeof( v_ids ); i ++ )
    modify_vitem( v_ids[i], prop, 0 );
}

/* -------------------------------------------------------------------------
 * compat Add
 * -------------------------------------------------------------------------
 */

public varargs mixed AddDetail( mixed det, mixed desc, string exa ) {
  // typo check: AddDetail("id" "desc\n");  <-- forgotten ','
  if (desc==0) {
      MASTER->runtime_error(
          "Call to AddDetail() without 2nd parameter (forgotten ','?)\n",
          "/std/room/details",
          object_name(this_object()),
          __LINE__   );
  }
  if (stringp(desc) && stringp(exa))
    desc = ({ desc, exa });

  return add_compat( det, P_LONG, desc );
}

public mixed AddSmell( mixed det, mixed desc ) {
  // typo check: AddSmell("id" "desc\n");  <-- forgotten ','
  if (desc==0) {
      MASTER->runtime_error(
          "Call to AddSmell() without 2nd parameter (forgotten ','?)\n",
          "/std/room/details",
          object_name(this_object()),
          __LINE__   );
  }
  return add_compat( det, P_SMELL, desc );
}

public mixed AddNoise( mixed det, mixed desc ) {
  // typo check: AddNoise("id" "desc\n");  <-- forgotten ','
  if (desc==0) {
      MASTER->runtime_error(
          "Call to AddNoise() without 2nd parameter (forgotten ','?)\n",
          "/std/room/details",
          object_name(this_object()),
          __LINE__   );
  }
  return add_compat( det, P_NOISE, desc );
}

public mixed AddReadMsg( mixed det, mixed desc ) {
  // typo check: AddReadMsg("id" "desc\n");  <-- forgotten ','
  if (desc==0) {
      MASTER->runtime_error(
          "Call to AddReadMsg() without 2nd parameter (forgotten ','?)\n",
          "/std/room/details",
          object_name(this_object()),
          __LINE__   );
  }
  return add_compat( det, P_READ_MSG, desc );
}

/* -------------------------------------------------------------------------
 * compat Remove
 * -------------------------------------------------------------------------
 */

public void RemoveDetail(mixed k)  { remove_compat( k, P_LONG ); }
public void RemoveSmell(mixed k)   { remove_compat( k, P_SMELL ); }
public void RemoveNoise(mixed k)   { remove_compat( k, P_NOISE ); }
public void RemoveReadMsg(mixed k) { remove_compat( k, P_READ_MSG ); }

/* -------------------------------------------------------------------------
 * compat Get
 * -------------------------------------------------------------------------
 */

public mixed GetDetail( string str ) { return GetVItem( str, P_LONG ); }
public mixed GetSmell(string str) { return GetVItem( str, P_SMELL ); }
public mixed GetNoise(string str) { return GetVItem( str, P_NOISE ); }
public mixed GetReadMsg(string str) { return GetVItem( str, P_READ_MSG ); }

/* --------------------------------------------------------------------------
 * Old properties
 *  Set is not supported
 *  Query only returns the first id and values maybe 0
 *  TODO: Compat properties should be removed asap!
 * --------------------------------------------------------------------------
 */

private int qv_filter( string key, mapping v, string prop ) {
  return !!v[key][prop];
}

static mapping query_vprops( string prop ) {
  mapping m;
  string * v_ids, tx, *v_ads;
  int i, i2;
  mapping rc;

  /* get all vitems with the given prop defined */
  v_ids = m_indices( filter_indices( m = QueryVItems(),
                                     SF(qv_filter), m, prop ) );

  /* and try to rebuild the old details mapping partially */
  rc = ([]);
  for( i = 0; i < sizeof( v_ids ); i ++ ) {
    for( i2 = 0; i2 < sizeof( m[v_ids[i]][P_IDS] ); i2 ++ ) {
      v_ads = m[v_ids[i]][P_ADS];
      tx = sizeof(v_ads)?implode(v_ads," ")+" ":"";
      tx += m[v_ids[i]][P_IDS][i2];
      rc[tx] = m[v_ids[i]][prop];
    }
  }

  /* return the result */
  return rc;
}

public mapping QueryDetails() { return query_vprops( P_LONG ); }
public mapping QuerySmells() { return query_vprops( P_SMELL ); }
public mapping QueryNoises() { return query_vprops( P_NOISE ); }
public mapping QueryReadMsgs() { return query_vprops( P_READ_MSG ); }

/* NOTE: only the values of map are used */
public mapping SetDetails(mapping map) {
  Pvitems = 0;

  if ( ! mappingp( map ) )
    return 0;

  filter_indices( map, lambda( ({ 'v /*'*/ }),
                               ({ #'add_vitem /*'*/, 'v /*'*/ }) ) );

  return QueryVItems();
}

#if 0
public mapping SetSmells(mapping map);
public mapping SetNoises(mapping map);
public mapping SetReadMsgs(mapping map);
#endif

/* -------------------------------------------------------------------------
 * predefined vitems door, sky, heaven, star, stars and their funs
 *  In difference to the previous code no special handling for defined details
 *  of indoor rooms, because AddVItem will overwrite the predefined now.
 * -------------------------------------------------------------------------
 */

public mixed fsky(string str) {
  mixed in;

  if (in = ({int})this_object()->QueryOutdoors())
    return ({mixed})in->QueryStateDesc();

  return "What sky? There are just walls!\n";
}

public mixed fstar(string str) {
  mixed in;

  if (in = ({int})this_object()->QueryOutdoors()) {
    switch (({int})in->QueryState()) {
    case ND_NIGHT:
      return "Little spots of light against the black of the universe.\n";
    case ND_PREDAWN:
      return "They fade against the upcoming sunlight.\n";
    case ND_EVENING:
      return "Through the fading sunlight the first stars become visible.\n";
    }
    return "There are no stars visible.\n";
  }

  return "What sky? There are just walls!\n";
}

public mixed fdoors( string str ) {
  if (sizeof(({mapping})this_object()->QueryDoors()))
    return ({mixed})this_object()->Doors();

  return 0;
}

/* -------------------------------------------------------------------------
 * Create - add our predefined vitems
 * -------------------------------------------------------------------------
 */

public void create() {
  ::create();

  AddVItem( ([ P_IDS : ({ "doors" }),
             P_LONG : #'fdoors /*'*/ ]) );
  AddVItem( ([ P_IDS : ({ "sky", "heaven" }),
             P_LONG : #'fsky /*'*/ ]) );
  AddVItem( ([ P_IDS : ({ "stars", "star" }),
             P_LONG : #'fstar /*'*/ ]) );
}

/* *********************************************************************** */
