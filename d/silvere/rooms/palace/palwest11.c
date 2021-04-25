// The Royal Guards armoury in western part of Silvere's palace

// created:       Sonic    [21-Jul-1997]
// last changed:  Taurec   [11-May-1998]

// TODO: Make "takeable" weapons etc, but the guards who guard all
// TODO: the stuff prevent you from leaving the room with the robbed
// TODO: stuff.

#include <silvere.h>
#include <properties.h>
#include <msgclass.h>
#include <coordinates.h>
#include <regions.h>
#include <moving.h>
#include <rooms.h>
#include <path.h>

inherit BASEROOM;

string v_sword,v_spear,v_cudgel,v_axe,v_knife;
string v_mail,v_shield,v_helmet;

string look_sword() {
  int amount;
  switch (amount=VQuery(v_sword,P_AMOUNT)) {
    case 0: return 0;
    case 1: return "A shiny sword.\n";
    default: return capitalize(itoa(amount))+" shiny swords.\n";
  }
  return capitalize(itoa(amount))+" shiny swords.\n";
}

string look_axe() {
  int amount;
  switch (amount=VQuery(v_axe,P_AMOUNT)) {
    case 0: return 0;
    case 1: return "An ordinary battleaxe.\n";
    default: return capitalize(itoa(amount))+" ordinary battleaxes.\n";
  }
  return capitalize(itoa(amount))+" ordinary battleaxes.\n";
}

string look_knife() {
  int amount;
  switch (amount=VQuery(v_knife,P_AMOUNT)) {
    case 0: return 0;
    case 1: return "An ordinary knife.\n";
    default: return capitalize(itoa(amount))+" ordinary knifes.\n";
  }
  return capitalize(itoa(amount))+" ordinary knifes.\n";
}

string look_cudgel() {
  int amount;
  switch (amount=VQuery(v_cudgel,P_AMOUNT)) {
    case 0: return 0;
    case 1: return "An ordinary cudgel.\n";
    default: return capitalize(itoa(amount))+" ordinary cudgels.\n";
  }
  return capitalize(itoa(amount))+" ordinary cudgels.\n";
}

string look_spear() {
  int amount;
  switch (amount=VQuery(v_spear,P_AMOUNT)) {
    case 0: return 0;
    case 1: return "A sharppeaked spear.\n";
    default: return capitalize(itoa(amount))+" sharppeaked swords.\n";
  }
  return capitalize(itoa(amount))+" sharppeaked swords.\n";
}

string make_rack_string(string weap){
  int amount;
  string w;
  w=VQuery(weap,P_IDS)[0];
  switch(amount=VQuery(weap,P_AMOUNT)){
    case 0: return "";
    case 1: return "one "+w;
    default: return itoa(amount)+" "+w+"s";
  }
  return itoa(amount)+" "+w+"s";
}

// look at the racks
string look_racks() {
  string *str;
  int i,s;
  string *weapons;
  weapons=({});
  weapons = ({v_sword,v_axe,v_knife,v_cudgel,v_spear});
  str = map(weapons[0..sizeof(weapons)],#'make_rack_string /*'*/);
  str-=({""});
  s=sizeof(str);
  switch(s){
    case 0: return "The racks are totally empty. Someone must have "
                   "stolen the whole equipment.\n";
    case 1: return "The racks contain "+str[0]+".\n";
    default: return "The racks contain "+
      implode(str[0..s-2],", ")+" and "+str[s-1]+".\n";
  }
  return "The racks contain "+
      implode(str[0..s-2],", ")+" and "+str[s-1]+".\n";
}

string look_mail() {
  int amount;
  switch (amount=VQuery(v_mail,P_AMOUNT)) {
    case 0: return 0;
    case 1: return "A shiny platemail.\n";
    default: return capitalize(itoa(amount))+" shiny platemails.\n";
  }
  return capitalize(itoa(amount))+" shiny platemails.\n";
}

string look_shield() {
  int amount;
  switch (amount=VQuery(v_shield,P_AMOUNT)) {
    case 0: return 0;
    case 1: return "A heavy towershield.\n";
    default: return capitalize(itoa(amount))+" heavy towershields.\n";
  }
  return capitalize(itoa(amount))+" heavy towershields.\n";
}

string look_helmet() {
  int amount;
  switch (amount=VQuery(v_helmet,P_AMOUNT)) {
    case 0: return 0;
    case 1: return "A feathered helmet.\n";
    default: return capitalize(itoa(amount))+" feathered helmets.\n";
  }
  return capitalize(itoa(amount))+" feathered helmets.\n";
}

string make_shelf_string(string armo){
  int amount;
  string a;
  a=VQuery(armo,P_IDS)[0];
  switch(amount=VQuery(armo,P_AMOUNT)){
    case 0: return "";
    case 1: return "one "+a;
    default: return itoa(amount)+" "+a+"s";
  }
  return itoa(amount)+" "+a+"s";
}

// look at the shelves
string look_shelves() {
  string *str;
  int i,s;
  string *armour;
  armour=({});
  armour = ({v_mail,v_shield,v_helmet});
  str = map(armour[0..sizeof(armour)],#'make_shelf_string /*'*/);
  str-=({""});
  s=sizeof(str);
  switch(s){
    case 0: return "Besides dust there's nothing lying on the "
                   "shelves. Someone must have "
                   "stolen the whole equipment.\n";
    case 1: return "On the shelves you see "+str[0]+".\n";
    default: return "On the shelves you see "+
      implode(str[0..s-2],", ")+" and "+str[s-1]+".\n";
  }
  return "On the shelves you see "+
      implode(str[0..s-2],", ")+" and "+str[s-1]+".\n";
}

varargs void create()
{
   string v_racks,v_shelves;
   ::create();
   Set(P_INDOORS,1);
   Set(P_INT_SHORT,"The royal guard's armory");
   Set(P_INT_LONG,
     "This is the armory of the royal guard. Racks stuffed with lots "
     "of sharp-bladed weapons fill the entire room, and the walls are "
     "covered with large shields and shelves that contain shiny armour "
     "and helmets. A whole army could be equipped with all these martial "
     "items. A flight of winding stairs leads up to the headquarters "
     "of the royal guard.\n");
   Set(P_INT_WIZMSG,
" Up   : To the guards headquarter. \n"
" South: back to the other rooms of the western part of the palace.\n");

   AddDetail(({"staircase","stairs","flight"}),"The staircase leads "
     "upwards in a tight spiral.\n");
   AddDetail(({"wall","walls"}),"The walls are almost fully covered by "
     "shelves of armour. There's nothing special to them.\n");
   AddDetail(({"weapons","weapon"}),"There are swords, axes, crossbows, "
     "longbows, cudgels... weapons for each kind of taste.\n");
   AddDetail(({"shields","shield"}),"Mostly towershields bearing the "
     "insignia of the four griffins.\n");
   AddDetail(({"armour","helmets"}),"There are feathered helmets and "
     "platemails lying around on the shelves.\n");

//********* TAKEABLE VITEMS **********

// Things you get from the racks

   v_racks=AddVItem( ([
     P_IDS:  ({"racks","rack"}),
     P_ADS:  ({"stuffed"}),
     P_PREPS: ({"in","on","from"}),
     P_LONG: #'look_racks /*'*/]) );
   v_sword=AddVItem( ([
     P_IDS: ({"sword"}),
     PV_ITEM_LOCATION: WEAPONS("sil_rgsword"),
     P_AMOUNT: 2,
     P_LONG: #'look_sword /*'*/]),v_racks);
   v_axe=AddVItem( ([
     P_IDS: ({"battleaxe","axe"}),
     PV_ITEM_LOCATION: WEAPONS("norm_axe"),
     P_AMOUNT: 3,
     P_LONG: #'look_axe /*'*/]),v_racks);
   v_knife=AddVItem( ([
     P_IDS: ({"knife"}),
     PV_ITEM_LOCATION: "/std/weapon",
     P_AMOUNT: 10,
     P_LONG: #'look_knife /*'*/]),v_racks);
   v_cudgel=AddVItem( ([
     P_IDS: ({"cudgel"}),
     PV_ITEM_LOCATION: WEAPONS("norm_cudgel"),
     P_AMOUNT: 2,
     P_LONG: #'look_cudgel /*'*/]),v_racks);
   v_spear=AddVItem( ([
     P_IDS: ({"spear"}),
     PV_ITEM_LOCATION: WEAPONS("norm_spear"),  // file to clone,two swords
     P_AMOUNT: 4,                      // 2 swords available
     P_LONG: #'look_spear /*'*/]),v_racks);

// Things you get from the shelves
   v_shelves=AddVItem( ([
     P_IDS:  ({"shelves","shelf"}),
     P_ADS:  ({"stuffed"}),
     P_PREPS: ({"in","on","from"}),
     P_LONG: #'look_shelves /*'*/]) );
   v_mail=AddVItem( ([
     P_IDS: ({"platemail","mail"}),
     PV_ITEM_LOCATION: ARMOUR("sil_rgarmour.c"),
     P_AMOUNT: 3,
     P_LONG: #'look_mail /*'*/]),v_shelves);
   v_shield=AddVItem( ([
     P_IDS: ({"shield","towershield"}),
     PV_ITEM_LOCATION: ARMOUR("sil_towshield.c"),
     P_AMOUNT: 2,
     P_LONG: #'look_shield /*'*/]),v_shelves);
   v_helmet=AddVItem( ([
     P_IDS: ({"helmet","helm"}),
     PV_ITEM_LOCATION: ARMOUR("sil_rghelmet.c"),
     P_AMOUNT: 5,
     P_LONG: #'look_shield /*'*/]),v_shelves);

//************** END OF VITEMS **************

   AddItem(NPC("royal_guard"),REFRESH_REMOVE,2);

   Set(P_INT_MAP,"palace");
   Set(P_REGION,REGION_CITY);
   Set(P_COORDINATES, ({ ({ -30,20,0 }), C_SILVERE }) );

   Set(P_NOWAY_MSG,
    "You walk into the wall like an idiot. Somehow you have the "
    "feeling, that everyone laughs at you.\n");
   AddExit("south", SIL_PALACE("palwest10"));
   AddExit("up", SIL_PALACE("palwesttow1"));
}
