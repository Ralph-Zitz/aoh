inherit "/std/room";

create() {
  (::create());
  SetIntShort("Crossing of Garph Shipper and Upper Ave");
  SetIntLong(
"Coming up from south, the Garph Shipper ends here in the Upper Ave.\n"
"The avenue leads to the east, away from the village gate.\n"
            );
  AddDetail(({"gate","village"}),
({ "The village gate looks strong, securing the innards of the village.\n"
  "The guards make sure that noone will enter this village without allowance.\n"
, "A small piece of paper stucks to the gate, firmly glued to it by the paint.\n"
})
           );
  AddDetail("paper",
"The paper seems to once having been part of a floorplan. Rain and sun\n"
"bleached it, just a few typewritten words are still readable: \"The the west\n"
"you can mention the village gate\". Someone marked this line and added \"And\n"
"we wonder why no one goes to Ireland! T.\".\n"
           );
  AddDetail("guard",
"Glancing too long at one of these is guards might have detrimental effects\n"
"to your health.\n"
           );
  AddDetail("guards",
"Tough, sturdy, these guards are not someone to joke with.\n"
           );
  AddExit("south","./raum2f.c");
  AddExit("west","./raum1g.c");
  AddExit("east","./raum3g.c");
}
