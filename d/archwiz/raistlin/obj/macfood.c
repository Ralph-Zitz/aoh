/* Special food object for MacFood, as you`ll get sick when eating
   it :) (a socalled Uebelkeit :)
   Checks also, if already a mir_is_schlecht object exists and if yes
   updates the values of this object.
*/

inherit "/std/food.c";
#include <properties.h>
#include <moving.h>
#define PL this_player()
#define THIS this_object()


create()
{
    ::create();
    SetFoodStuff(10);
    Set(P_VALUE,30);
    seteuid(getuid());
}


init()
{
    ::init();
    add_action("do_eat", "eat",0);
}


do_eat(str)
{
    object dummy;
    int maxstuff;
    int macstuff;
    int wert;


    if(!str || present(str, PL) != THIS)
	return 0;


    macstuff=PL->QueryFood() + THIS->QueryFoodStuff();
    maxstuff=PL->QueryMaxFood();

    if(macstuff<=maxstuff && QueryFoodKind()=="food")
    {
	write ("You don`t feel to well when you look at this food.\n");
        wert=THIS->QueryValue();

        if(!(dummy=present("uebelkeit",PL)))
	{
	    write("You weren`t sick bevor. Now you are :)\n");
	    dummy=clone_object("/d/archwiz/raistlin/obj/uebelkeit");
            if(dummy->move(PL,M_SILENT)!=ME_OK)
            {
		write("You didn`t get sick!!\n");
		dummy->remove();
            }
            dummy->AddSickLevel(wert);
	    dummy->sick_msg();
	}
        else
        {
	    write("You were already sick... But your sickness increases!!\n");
            dummy->AddSickLevel(wert);
        }
    }
    consume(str);
    return 1;
}
