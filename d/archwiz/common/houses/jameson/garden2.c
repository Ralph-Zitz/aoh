/* The garden of Jameson */
inherit "/d/avalon/common/std/meadow";
create() {
  ::create();
  SetIntShort("in the garden of Jameson");
  SetIntLong("You are standing in the garden of Jameson.\n\
To the west and south you can go deeper into the garden.\n\
But maybe you can search for herbs in this garden?\n");
  AddExit("west","./garden3");
  AddExit("south","./garden1");
  AddHerb("/d/avalon/common/obj/fruits/dandilion",2);
}
