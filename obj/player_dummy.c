/*--------------------------------------------------------------------------
 * /obj/player_dummy -- dummy object for player-savefile access
 *
 * Copyright (C) 1997 by One Step Beyond.
 * All Rights Reserved.
 * Permission granted for non-commercial use.
 *--------------------------------------------------------------------------
 * NOTE: this file should always be used as an interface if you need data
 *       from a player that can be offline. It handles both, if the player is
 *       online it queries the properties, if not, it reads the savefile
 */

#include <properties.h>
#include <attributes.h>

private static string loaded;
private static mapping properties;

public mapping pAttrs;
public int aAlcohol, aAlign, Con, Dex, aDrink, aFood, aGender,
  aHP, Int, aInvis, aIVision, aLevel, aMaxAlcohol, aMaxDrink,
  aMaxFood, aMaxHP, aMaxSP, aSP, Str, aUVision, aWimpy,
  aXP, age, pagesize, aLState, pLastSave, pLastLogin, pFirstLogin;

public mixed aMsgIn, aMsgOut, aMMsgIn, aMMsgOut, aTargets;
public string aRace, * channels, aCorpse, explored, guild,
  home, presay, quests, title, tty, mailaddr, called_from_ip,
  aName, called_from_ipname;
mapping faliases;

public mapping QueryProperties(mixed pl);

public status LoadData(string pl)
{
  string file;
  if (!({mixed *})MASTER->find_userinfo(pl)) return 0;
  if (!file = find_savefile(pl)) return 0;
  seteuid(getuid());

  pAttrs = aAlcohol = aAlign = Con = Dex = aDrink = aFood = aGender =
    aHP = Int = aInvis = aIVision = aLevel = aMaxAlcohol = aMaxDrink =
    aMaxFood = aMaxHP = aMaxSP = aSP = Str = aUVision = aWimpy = aXP =
    age = pagesize = aLState = aMsgIn = aMsgOut = aMMsgIn = aMMsgOut =
    aTargets = aRace = channels = aCorpse = explored = guild =
    home = presay = quests = title = tty = pLastSave = mailaddr =
    called_from_ip = called_from_ipname = pLastLogin = faliases = 0;

  restore_object(file);
  seteuid(0);
  loaded = pl;
  return 1;
}

public mixed trans_pl(mixed pl)
{
  if (!pl) return 0;
  if (objectp(pl)) return pl;
  if (find_player(pl)) return find_player(pl);
  return pl;
}

public mixed QueryProperty(mixed pl, string what)
{
  mapping m = QueryProperties(pl);
  return m[what];
}

public mapping QueryProperties(mixed pl)
{
  pl = trans_pl(pl);
  if (objectp(pl)) return ({mapping})pl->QueryProperties();
  if (pl==loaded&&properties) return properties+([]);
  if (!pl||!LoadData(pl)) return ([]);
  properties = ([]);
  if (pAttrs)
    properties+=pAttrs;
  properties[P_ALCOHOL] = aAlcohol; // int
  properties[P_ALIGN] = aAlign; // int
  properties[P_CON] = Con; // int
  properties[P_DEX] = Dex; // int
  properties[P_DRINK] = aDrink; // int
  properties[P_FOOD] = aFood; // int
  properties[P_GENDER] = aGender; // int
  properties[P_HP] = aHP; // int
  properties[P_INT] = Int; // int
  properties[P_INVIS] = aInvis; // int
  properties[P_IVISION] = aIVision; // int
  properties[P_LEVEL] = aLevel; // int
  properties[P_MAX_ALCOHOL] = aMaxAlcohol; // int
  properties[P_MAX_DRINK] = aMaxDrink; // int
  properties[P_MAX_FOOD] = aMaxFood; // int
  properties[P_MAX_HP] = aMaxHP; // int
  properties[P_MAX_SP] = aMaxSP; // int
  properties[P_MMSGIN] = aMMsgIn; // mixed
  properties[P_MMSGOUT] = aMMsgOut; // mixed
  properties[P_MSGIN] = aMsgIn; // mixed
  properties[P_MSGOUT] = aMsgOut; // mixed
  properties[P_RACE] = aRace; // string
  properties[P_SP] = aSP; // int
  properties[P_STR] = Str; // int
  properties[P_UVISION] = aUVision; // int
  properties[P_WIMPY] = aWimpy; // int
  properties[P_XP] = aXP; // int
  properties[P_LSTATE] = aLState; // int
  properties[P_ATTRS] = pAttrs; // mapping
  properties[P_CHANNELS] = channels; // *string
  properties[P_CORPSE] = aCorpse; // string
  properties[P_FALIASES] = faliases; // mixed
  properties[P_GUILD] = guild; // string
  properties[P_HOME] = home; //string
  properties[P_PAGESIZE] = pagesize; // int
  properties[P_PRESAY] = presay; // string
  properties[P_QUESTS] = quests; // string
  properties[P_TARGETS] = aTargets; // ???
  properties[P_TITLE] = title; // string
  properties[P_TTY] = tty; // string
  properties[P_LAST_SAVE] = pLastSave;
  properties[P_FIRST_LOGIN] = pFirstLogin;
  properties[P_LAST_LOGIN] = pLastLogin;
  properties[P_CALLED_FROM_IP] = called_from_ip;
  properties[P_CALLED_FROM_IPNAME] = called_from_ipname;
  properties[P_MAILADDR] = mailaddr;
  properties[P_NAME] = aName;
  properties[P_REALNAME] = lower_case( pl );
  properties[P_NETDEAD] = 0;
  properties[P_NETDEAD_ROOM] = 0;
  properties[P_AWAY] = 0;

  properties[P_AGE] = age; // int
  properties[P_EXPLORED] = explored; // string

  switch( aGender ) {
  case GENDER_MALE:
    properties[P_PRONOUN] = "he";
    properties[P_OBJECTIVE] = "him";
    properties[P_POSSESSIVE] = "his";
    break;
  case GENDER_FEMALE:
    properties[P_PRONOUN] = "she";
    properties[P_OBJECTIVE] = "her";
    properties[P_POSSESSIVE] = "her";
    break;
  default:
    properties[P_PRONOUN] = "it";
    properties[P_OBJECTIVE] = "it";
    properties[P_POSSESSIVE] = "its";
    break;
  }

  return properties+([]);
}
