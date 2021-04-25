/*
 * /d/archwiz/common/room/city/inn.c  -  The Eastroad Inn.
 * Changed to fit in new NF, Dec 1992 (Gubert)
 *
 * food 1:  heal 8,  stuff 12,  cost 20
 * food 2:  heal 16, stuff 24,  cost 50
 * food 3:  heal 24, stuff 36,  cost 90
 */

inherit"std/room";
inherit "lib/money";

#include "path.h"
#include <config.h>		/* for racemaster */
#include <moving.h>
#include <health.h>		/* for food costs (still not avail) */

#define PL		this_player()
#define Level		(RACEMASTER->XPtoLevel (PL->QueryXP()))
#define Name		(PL->QueryName())
#define Speak(s)	write ("Innkeeper says: "+s+"\n")

static int cm, mm, rmm;
static string last_eater;
int scroll;

look_floor() {
  if (scroll) {
  return "You find nothing.\n";
  }
  scroll=clone_object(SCROLLS("rpoison"));
  scroll->move(this_object(),M_SILENT);
  scroll=1;
  return "You found a scroll on the floor. Just get it.\n";
}
			 

QueryIntLong() {
  return "You are in the Eastroad Inn. Here you can buy food to still your "
"hunger, but only a limited selection is available. "
"The floor looks strange.\n"+show_menu();
}

reset ()
{
::reset ();
        cm = random (12) + 2;
        mm = random (8) + 1;
        rmm = random (5) + 1;
  scroll=0;
}

show_menu ()
{
	string menu;
	menu = "\n";
        if (! (cm || mm || rmm)) 
           menu += 
"The Inkeeper says: We have completely sold out...come back later.\n";
	else {
		menu += 
"1: Commoner's Meal     "+(cm)+" at 20 coins\n"
"2: Merchant's Meal     "+(mm)+" at 50 coins\n"
"3: Rich Man's Meal     "+(rmm)+" at 90 coins\n"
"4: A Mug of Beer       "+"      4 coins\n";
		menu += "\n\
Use 'buy <number>' to buy the desired Food. The food will "
"be consumed at once. Good appetite.\n\n";
	}
	return menu;
}

no_food () 
{
	Speak ("Sorry - we have sold out of that.");
	if (cm || mm || rmm)
		Speak ("Why don't you try something else ?");
	else
		Speak ("Why don't you come back later ?");
	return 1;
}

pays (n) 
{
	if (!PL) return 0;
	if (QuerySomeonesValue (PL) < n)
	{
		Speak ("You cannot afford that.");
		return 0;
	}
	PayValue (PL, n);
	return 1;
}

tease (n, cost)
{
	/* is it too rich ? */
	if (!PL) return 0;
	if (! PL->AddFood (3 * n))
	{
		Speak (
"You do not look hungry to me... let me take it back.");
		PayValue (PL, -cost);
		return 0;
	}

	if (Name == last_eater)
		Speak ("My - Are we hungry today.");
	last_eater = Name;

	heal (2 * n);
	return 1;
}

heal (n)
{
	int hp, maxhp;

	hp = PL->QueryHP (), maxhp =  PL->QueryMaxHP ();
	if (hp < maxhp)
	{
                if (hp + n > maxhp)
                        n = maxhp - hp;
                PL->SetHP (hp + n);
        }
}

/*
 * /doc/concepts/health speaks about /sys/health.h : MIN_FOOD_COST()
 * this would be very useful.
 */

buy (s) 
{
	if (! s)
	{
		Speak ("What do you want to buy ?");
		return 1;
	}
	/* commoner's meal, price 20, heals 4, preferably for levels 1-6 */ 

	if (s == "1" || s== "<1>") 
	{
		if (! cm) return no_food();
		if (! pays (20)) return 1;
		if (Level > 6)
		{
			Speak("You don't look like a commoner to me.");
			if (mm || rmm)
			{
				Speak(
			"You should eat food more suited for you.");
				return 1;
			}
			Speak("But as we have no better food - here you are.");
		}
		write (
"You are served a commoner's meal - very nourishing.\n");
		say (Name+" orders a commoner's meal\n");
		if (tease (4, 20))
			cm = cm - 1;
		return 1;
	}

	/* merchant's meal, price 50, heals 8, preferably for levels 7-12 */ 
	if (s == "2" || s== "<2>") 
	{
		if (! mm) return no_food ();
		if (! pays (50)) return 1;
		if (Level > 12)
		{
			Speak ("You look more like a richman to me.");
			if (rmm)
			{
				Speak (
			"You should eat food more suited for you.");
				return 1;
			}
			Speak("But as we have no better food - here you are.");
		}
		write ("You are served a merchant's meal - very good.\n");
		say (Name+" orders a merchant's meal\n");
		if (tease (8, 50))
			mm = mm - 1;
		return 1;
	}
	/* rich man's meal, price 90, heals 12, preferably for levels 13+ */ 
	if (s == "3" || s == "<3>") 
	{
		if (! rmm) return no_food ();
		if (! pays (90)) return 1;
		write ("You are served a rich man's meal - very delicious.\n");
		say (Name+" orders a rich man's meal\n");
		if (tease (12, 90))
			rmm = rmm - 1;
		return 1;	
	}
	if (s == "4" || s == "<4>" || s == "mug" || s == "beer") 
	{
		if (! pays (4)) return 1;
		if (! PL->AddDrink(8))
		{
			Speak(
"You do not look thirsty to me... let me take it back.");
                        PayValue (PL, -4);
                        return 1;
		}
		if (! PL->AddAlcohol(4))
		{
			Speak(
"You look a little too drunk for that... let me take it back.");
			PayValue (PL, -4);
			return 1;
		}
		write(
"You drink a mug of first class beer - That feels good.\n");
		say (Name+" drinks a beer.\n");
		heal (2);
		return 1;
	}
	Speak ("We have no such number on the menu, try 1, 2 or 3.");
	return 1;
}


create ()
{
::create ();
	SetIntShort ("Eastroad Inn");
        AddExit ("east", "./eastroad5");
        AddDetail ("menu", #'show_menu);
        AddRoomCmd ("buy", #'buy);
        AddRoomCmd ("order", #'buy);
        AddDetail(({"floor","ground"}),#'look_floor);
	reset ();
}
