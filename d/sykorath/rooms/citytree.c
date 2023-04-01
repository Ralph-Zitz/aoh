// part of the town
// made by magictcs - 6 jul 97

// TODO: climb command, finish it
// TODO: add search relax and and and
// TODO: update coords

#include <properties.h>
#include <moving.h>
#include <npc.h>
#include <skills.h>
#include <regions.h>
#include <coordinates.h>
#include <sykorath.h>

inherit BASEROOM;
//inherit "/obj/lib/skills";

// names in the tree
string *names;

//***********
// carve name
//***********
// TODO: make it nicer - very ugly code actually
int carve(string str) {
  notify_fail("Carve where?\n");
  if (!str) return 0;
  if (member(({"oak","huge oak","very huge oak","city oak","tree"}),str)!=-1) {
    write("You carve your name into the tree.\n");
    show(({string})NAME+" carves a name into the tree.\n");
    if (names==0) {
      names=({({string})TP->QueryName()});
    } else {
      if (member(names,({string})TP->QueryName())==-1) {
        names+=({({string})TP->QueryName()});
      }
    }
    return 1;
  }
  return 0;
}

//***********
// climb tree
//***********
int climb(string str) {
  int suc,percentage = 0;
  string ret;
  notify_fail("Climb tree\n");
  if (member(({"oak","huge oak","very huge oak","city oak","tree"}),str)!=-1) {
//    suc=UseSkill(TP,SK_CLIMB,TP->QueryInt()*25,&percentage,1);
    suc=1;
    if (suc) {
      write(
      "You catch one of the lower branches of the tree and climb up.\n");
      TP->move(SYKORATH("????????"),M_GO,"into the branches");
      return 1;
    } else {
      switch (percentage) {
        case 0..30:  ret="You try to catch the lowest branch, but miss it.\n";
                     break;
        case 31..66: ret="You jump to catch the lowest branch, but miss it.\n";
                     break;
        default:     ret="You catch the lowest branch, but the branch is too "
                         "smooth and you slide down to the earth.\n";
      }
      write(ret);
      show(({string})NAME+" tries to climb the tree, but fails.\n");
      return 1;
    }
  }
  return 0;
}

//***************
// detailhandling
//***************
string looknames()  {
  string ret;
  int i;
  if (!names) return "It is a nice place to carve your name into the tree.\n";
  ret="You see some names carved into the tree:\n";
  for (i=0;i<sizeof(names);i++) ret+=names[i]+" ";
  ret+="\n";
  return ret;
}

//*******
// create
//*******
varargs void create() {
  ::create();
  SetIntShort("The city oak");
  SetIntLong(
    "You are standing near a very huge oak. The oak is one of the oldest\n"
    "trees that can be found in this city.\n");
  SetIndoors(0);
  SetCoordinates( ({ ({0,0,0}), C_SYKORATH }));
  Set(P_REGION,REGION_CITY);

  /* details */
  AddDetail("city","Well you are standing in it.\n");
  AddDetail(({"oak","huge oak","very huge oak","city oak","tree","older tree",
    "oldest tree"}),
    "The oak is a very old tree. It is standing here since centuries.\n"
    "During the great war some people tried to burn the oak. But other men\n"
    "extinguished the fire, but you can still see some marks.\n");
  AddDetail(({"trees","older trees","oldest trees"}),
    "A single oak is standing here.\n");
  AddDetail(({"mark","marks","some marks","marks of oak","marks of tree"}),
    "You see some marks of fire and some scratches (looks like that some\n"
    "people carved their name into the tree). You can take a closer look to\n"
    "the names.\n");
  AddDetail(({"names","name","names of tree","names in tree","name of tree",
    "name in tree"}),#'looknames /*'*/);
  AddReadMsg(({"names","name","names of tree","names in tree","name of tree",
    "name in tree"}),#'looknames /*'*/);

  AddRoomCmd("carve","carve");
  AddRoomCmd("climb",#'climb /*'*/);

  /* exits */
  // TODO AddExit();
}

/*
reset() {
  ::reset();
  if (names) {
    tell_room(TO,
    "An old worker comes along the way and takes a closer view to the oak.\n"
    "The worker says: 'Ah these young people here. All have to carve their\n"
    "  names into the oak.'\n"
    "And with these words he takes some special equipment and heals the\n"
    "tree. The names vanishes in a magical way.\n");
    names=0;
  }
}
*/
