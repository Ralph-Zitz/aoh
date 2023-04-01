#include <properties.h>
#include <msgclass.h>
#include <living.h>

inherit "/std/npc";

void create()
{
  ::create();

  SetName("Ephred");
  SetLong("Ephred owns the Bakery Shoppe here in Sykorath. He is a slender, "
    "middle-aged man of medium stature, wears a simple white pants and "
    "white apron, and has spots of flour in his beard. He seems to be quite "
    "busy preparing the next batch of dough.\n");

  SetLevel(20);
  SetRace("elf");

  AddId( ({ "shopkeeper", "baker" }));
  SetGender(GENDER_MALE);

  SetAlign(200);

  InitChats(4, ({
    "Ephred bustles about the small shop, preparing dough for various types\n"+
      "of baked goodies.\n",
    "Ephred throws some wood onto the fire under the hot oven.\n",
    "Ephred rolls out some dough for bread loaves.\n",
    "Ephred looks at you briefly, wondering if you'd like to purchase\n"+
      "anything special.\n"
  }) );

}

void init()
{
  ::init();
  if (interactive(this_player()))
    call_out("greet",2,this_player());
}

void greet(object guest)
{
  if(({string})this_player()->QueryName() != "Someone")
  {
      msg_room(environment(), CMSG_ROOM,
          "Ephred smiles: Welcome, "+
	       capitalize(({string})guest->QueryName()) + ". Don't forget to check "
	      "today's specials.\n");
  }
}
