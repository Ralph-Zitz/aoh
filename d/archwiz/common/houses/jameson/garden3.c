/* The garden of Jameson */
inherit "/d/avalon/common/std/meadow";
create() {
  ::create();
  SetIntShort("in the garden of Jameson");
  SetIntLong("You are standing in the garden of Jameson.\n\
To the south Jameson is building a house.\n\
To the west and east you can go deeper into the garden.\n\
But maybe you can search for herbs in this garden?\n");
  AddExit("west","./garden4");
  AddExit("east","./garden2");
  AddHerb("/d/avalon/common/obj/fruits/dandilion",2);
}
