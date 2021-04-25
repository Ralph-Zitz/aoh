
/* Changed to warn players that hey, we're not open yet :)  */

#include <config.h>
#include <secure/wizlevels.h>
#include <moving.h>
#include <rooms.h>
#include <stdrooms.h>
#include <living.h>
#include <races.h>
#include <properties.h>

#include "path.h"

#define ESF(x) symbol_function(x)
#define LSF(x) symbol_function(x,this_object())
inherit "/std/room";
inherit "/lib/money";

#define PL this_player()

#define COST 100000

private static object scroll;

/*------------------------------------------------------------------------*/

int do_east() {
  if (!this_player()) return 0;
  if (this_player()->QueryRace()=="visitor") return notify_fail(
  "Please type   'info races'  first.\n"),0;
  return this_player()->move(CITY("boul1"),M_GO,"east");
}

public string *QueryRacenames()
{
  string *rn;
  
  rn =  (string *) RACEMASTER->AllRaceNames();
  rn -= regexp(rn,"vampire$|^visitor$|^slave$");
  // these races should not occur in any list and the player should not
  // be able to morph into them
  // Don't change the "vampire$"-regexp! It's written as it is meant. All
  // races which end on "vampire" should not occur in the list
  return rn;
}

public string detail_floor()
{
  if (scroll)
    return "You find nothing.\n";
  scroll=clone_object(MAGIC_SCROLL_DIR + "meta");
  scroll->move(this_object(),M_SILENT);
  return "You found a scroll on the floor. Just get it.\n";
}


public int cmd_info(string str)
{
  string res;
  if (!str)
  {
    write(
      "  Every player can choose a (new) race here.\n"
      "  For guests and players with less than "+
      (RACEMASTER->XPfromLevel(3))+" XP the race is free;\n"
      "  all other players have to pay 1/2 of their experience and\n"
      "  "+COST+" coins cash.\n"
      "  You'll have ALL your stats reset to the new race's starting values\n"
      "  as you are not familiar with your new body.\n"
      "  Type 'info races' for information about the races.\n");
      return notify_fail("",NOTIFY_NOT_VALID),0;
  }
  if ((str == "races") || (str == "race"))
    {
      write("\n"+
            sprintf("    %-*#s\n",40,
              implode(
                    sort_array(map(QueryRacenames(), #'capitalize), #'>)
                     , "\n") )+
            "  For more information about a race type 'info <race>'.\n"
            "  To change your race type   'morph <race> <gender>',   where\n"
            "  the <gender> is either 'male' or 'female'.\n");
      return 1;
    }
  if ((member(QueryRacenames(),lower_case(str))==-1)
      ||!res = (string)RACEMASTER->RaceDescr(lower_case(str)))
    return notify_fail(
      "What race?\n",
      NOTIFY_NOT_VALID),0;
  write (" -- The "+capitalize(str)+" race --\n"+res);
  return 1;
}

public int cmd_morph(string str)
{
  string race, gender, home, portal;
  int gen;
  status newbie;
  mixed *rdata;
  
  if (!str
      ||sscanf(lower_case(str),"%s %s",race,gender)!=2)
    return notify_fail(
      "Put both the race and the gender, bub.\n",NOTIFY_NOT_VALID),0;
  if (member(({"neuter","male","female"}),race)!=-1)
    {
      str = race;
      race = gender;
      gender = str;
    }
  else
    if (member(QueryRacenames(),gender)!=-1)
      {
	str = race;
	race = gender;
	gender = str;
      }
  switch(gender)
    {
     case "neuter":
      gen = NEUTER;
      break;
     case "male":
      gen = MALE;
      break;
     case "female":
      gen = FEMALE;
      break;
     default:
      gen = -1;
    }
  if (gen==-1)
    if (member(QueryRacenames(),race)==-1)
      return notify_fail(
        "Put both the race and gender, bub.\n",NOTIFY_NOT_VALID),0;
    else
      return notify_fail(
        "Just put morph human male or something.\n",
        NOTIFY_NOT_VALID),0;
  if (member(QueryRacenames(),race)==-1
      ||!RACEMASTER->RaceObject(race))
    return notify_fail(
      "What race? ",
      NOTIFY_NOT_VALID),0;
  if (race == (string)PL->QueryRace())
    return notify_fail(
      "You are that already.\n",
      NOTIFY_NOT_VALID),0;
  if (gen==NEUTER)
    return notify_fail(
      "Sorry, can't make a neuter "+race+".\n",
      NOTIFY_NOT_VALID),0;

  newbie = 0;

  if (!IS_WIZARD(PL))
    if (PL->QueryGhost()
        && PL->QueryRace()!="visitor")
      return notify_fail(
        "You're in ghost form.\n",NOTIFY_NOT_VALID),0;
    else
      if (!PL->QueryGuest()
          &&(string)PL->QueryRace()!="visitor"
          && (int) PL->QueryXP() > RACEMASTER->XPfromLevel(3))
        if (QuerySomeonesValue(PL) <= COST)
          return notify_fail(
            "Not enough money.\n",
            NOTIFY_NOT_VALID),0;
        else
	  {
	    write(
              "Ok.\n");
            PayValue (PL, COST);
            PL->SetXP((int)PL->QueryXP() / 2);
          }
      else
	{
          write("Neat.\n");
	  newbie = 1;
	}

  if (sizeof(PL->Query(P_WEAPONS)))
    PL->command_me("unwield all");
  if (sizeof(PL->Query(P_ARMOURS)))
    PL->command_me("unwear all");

  write(
    "Wow.\n");
  display (capitalize((string)PL->QueryName())
    +" says: Wow.\n",({ PL }));
home = "/d/archwiz/common/room/city/froffice";
  PL->SetGender(gen);
  PL->SetGhost(0);
  RACEMASTER->InitRace(PL,race);
  RACEMASTER->AdjustStats(PL);
  PL->SetLevel(1);  // else the player won't be able to raise
		    // his stats again. (Cadra)
  rdata = RACEMASTER->RaceData(race, PL);
  if (!IS_WIZARD(PL))
    {
      if (!newbie)
        {
          PL->SetHP(5);
          PL->SetFood(5);
          PL->SetDrink(5);
        }
      else
        {
          PL->SetHP(PL->QueryMaxHP());
          PL->SetFood(PL->QueryMaxFood());
          PL->SetDrink(PL->QueryMaxDrink());
        }
      PL->SetAlcohol(0);
    }
  else 
    PL->SetHome(home);
  write(
    "\nYou have chosen.\n\n");
  display(
    "\n"
    +capitalize((string)PL->QueryName())+" chooses a body.\n"
    "Yay.\n",
    ({ PL }) );

  RACEMASTER->EquipPlayer(PL);

  if (!(home = (string)PL->QueryHome())
      ||home == ""
      ||catch(PL->move(home,M_TELEPORT)))
    PL->move(IS_IMMORTAL(PL)
             ?CENTER
             :FROFFICE);

  portal = sizeof(rdata) >= RD_PORTAL ? rdata[RD_PORTAL] : 0;
  if (portal)
  {
    if (-1 == strstr(portal, "archwiz"))
    {
      if (object_name(environment(PL)) != home)
        write(
"\nBlah.\n"
             );
      else
        write(
"\nBlah.\n"
             );
    }
    else
    {
      if (object_name(environment(PL)) != home)
        write(
"\nBlah.\n"
             );
      else
        write(
"\nBlah.\n"
             );
    }
  }

  return 1;
}

public void reset()
{
  (::reset());
  scroll=0;
}

public void create()
{
  (::create());
  SetIntShort("The office of Dr. Frankenstein");
  SetIntLong(
"Type 'info races' for information on choosing a race and gender.  Please\n"
"keep in mind that nothings ready yet in this world, and that we'll\n"
"eventually remove all existing characters.  You can come in and talk and\n"
"express your ideas. Things won't be ready for a while, but when they "
"are, this city (NFCity) will disappear.\n");
  SetIntLight(20);
  AddExit("east", #'do_east);
  AddRoomCmd("info", LSF("cmd_info"));
  AddRoomCmd("morph",LSF("cmd_morph"));
  reset();
}
