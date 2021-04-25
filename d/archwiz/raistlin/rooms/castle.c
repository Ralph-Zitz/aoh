#define DEST "room/southave1"
#define ENTER "room/church"
/*
 * This is just the facade to a castle. If you want to enable the
 * "enter" command, move the player to a hall or something (which
 * you have to design yourself).
 * The predefined string DEST is where a player should come when he
 * leaves the castle.
 *
 * This file is loaded automatically from "init_file". We have to move
 * ourself to where we are supposed to be.
 */

id(str) { return str == "castle"; }

short() {
    return "Strange looking castle of Raistlin";
}

long() {
    write(
"As you look closer to the castle of Raistlin you are suddenly "
"flashed by a colored and glooming sign. "
"It seems to be very familar to you.\n"
"The doors are opened wide.\n");
}

init() {
    add_action("enter", "enter");
    add_action("south", "south");
    add_action("look", "look");
}

enter(str) {
    if (!id(str))
        return 0;
    this_player()->move_player("south#players/raistlin/entrance");
    return 1;
}

south() {
    this_player()->move_player("south#players/raistlin/entrance");
    return 1;
}

look(str) {
   if (str != "at castle")
        return 0;
   long();
   make_sign();
   return 1;
}

object reklame;
object reklame2;

make_sign() {
    reklame = clone_object("/obj/treasure.c");
    call_other (reklame, "set_id", "sign");
    call_other (reklame, "set_alias", "sign");
    call_other (reklame, "set_short", "A big glooming sign at Raist's Castle");
    call_other (reklame, "set_long",
        "\n  M   M\n"
         +" M M M M\n"
         +" M  M  M\n"
         +"M       M\n"
         +"M       M\n"
         +"M       Mc Donalds of Raistlin\n\n"
         +"Opening soon...\n\n" );
    call_other (reklame, "set_value", 10);
    call_other (reklame, "set_weight", 10);
    call_other (reklame, "set_info", "It looks VERY precious");
    move_object(reklame, DEST);
    return 1;
}

reset(arg) {
    if (arg)
        return;
    
    reklame2 = clone_object("/obj/treasure.c");
    reklame2->set_id ("sign");
	/* id changed .. -xio- ( war total falsch (set_id("A huge sign.") .. 
		( oh gott :-)    ) */
    reklame2->set_alias ("sign");
    reklame2->set_short ("A big glooming sign");
    reklame2->set_long(
        "\nCome and visit:\n"
         +"  M   M\n"
         +" M M M M\n"
         +" M  M  M\n"
         +"M       M\n"
         +"M       M\n"
         +"M       Mc Donalds of Raistlin\n\n"
         +"Opened recently!\n"
         +"-Easy and endless monsters (especially for beginners)\n"
         +"-Something to eat\n"
         +"-And very much fun!!!\n"
         +"do s,s,s,s,w,w,s to visit it!\n");

    reklame2->set_value (10);
    reklame2->set_weight (50);
    reklame2->set_info ("It looks VERY precious");
    move_object(reklame2, ENTER);
    
    move_object(this_object(), DEST);
}


