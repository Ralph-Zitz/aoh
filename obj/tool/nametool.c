/*
   (c) 1994 Raistlin shit produktions incorporated
   Enables a Wizard to have nicknames he can define himself.
   Checks for existing player names etc.
*/

inherit "/std/thing.c";
#include <properties.h>
#include <secure/wizlevels.h>
#define TP this_player()
#define TO this_object()

private int update_nicknames();

private string nickfile;
private string nickdata;
private string *nicknames;
private int removeflag;
private object creator;

varargs void create()
{
    seteuid(getuid());

    (::create());

/* Added by Pumuckel to check for blueprint. */
    if(!is_clone()) return;

    Set(P_AUTOOBJECT,1);
    SetWeight(0);
    SetValue(0);
    SetNoDrop("You can't drop your nametool!!!\n");
    Set(P_NOBUY,"You can't sell your nametool!!!\n");
    Set(P_NOSELL,"You can't buy the nametool!!!\n");
    SetNoGive("You can't give away your nametool!!!\n");
    SetIds(({"name","tool","nametool"}));

/* Removed by Pumuckel and mad a new QueryShort function.
    SetShort(add_gen(TP->QueryName())+" nametool"); */

    SetLong("You see a nice little tool coded by Raistlin with which\n"+
            "you can add and delete nicknames for you which then can\n"+
            "be used by other players to tell you things etc...\n"+
            "\n"+
            "Usage: nickadd <name>    Adds <name> to your nickname list.\n"+
            "       nickdel <name>    Deletes <name> from your nickname list.\n"+
            "       nicklist          Lists all your current nicknames.\n"+
            "       nickremove        Removes the nicktool completely.\n");

    creator=TP;

    nickfile="/players/"+({string})TP->QueryRealName()+"/.nickfile";
    removeflag=0;
    update_nicknames();
}


void init()
{
    if(TP && ({int})TP->QueryIsPlayer() && IS_IMMORTAL(TP))
    {
        add_action("do_nickadd","nickadd");
        add_action("do_nickdel","nickdel");
        add_action("do_nicklist","nicklist");
        add_action("do_nickremove","",1);
    }
    else
        if(!remove())
            destruct(TO);
}


varargs string QueryShort()
{
    if(TP && ({string})TP->QueryName())
        return add_gen(({string})TP->QueryName())+" nametool";

    return "Someone's nametool";
}


int query_prevent_shadow() { return 1;}


private int check()
{
    if(TP!=creator)
        return 0;

    return 1;
}


private int read_data()
{
    if(file_size(nickfile)<0)
    {
        write("Couldn't find the .nickfile in your home dir. Will create it.\n");
        if(!write_file(nickfile,""))
        {
            write("Couldn't write the file "+nickfile+". Aborting.\n");
            return 0;
        }
    }

    nickdata = read_file(nickfile);
    if(nickdata)
    {
        nicknames = explode(nickdata,"\n");
        nicknames = nicknames[0..sizeof(nicknames)-2];
    }
    else
    {
        nickdata = "";
        nicknames = ({});
    }

    return 1;
}


private int write_data()
{
    int i;

    if(!rm(nickfile))
    {
        write("Couldn't remove your nickname file. Aborting.\n");
        return 0;
    }

    if(!write_file(nickfile,""))
    {
        write("Couldn't recreate the file "+nickfile+". Aborting.\n");
        return 0;
    }

    for(i=0;i<sizeof(nicknames);i++)
    {
        if(!write_file(nickfile, nicknames[i]+"\n"))
        {
            write("Couldn't add the new nickname to your nickname file. Aborting.\n");
            return 0;
        }
    }

    return 1;
}


private int update_nicknames()
{
    if(!read_data())
        return 1;

    nicknames = ({({string})TP->QueryRealName()})+nicknames;
    TP->SetIds(nicknames);

    return 1;
}


int do_nickadd(string str)
{

    if(query_verb()!="nickadd" || !check())
        return 0;

    if(!str)
    {
        write("Hey, you have to specify a nickname to add, bummer!\n");
        return 1;
    }

    str = lower_case(str);

    if(file_size("/save/"+str[0..0]+"/"+str+".o")>=0)
    {
        write("Hey bummer, this player already exists in this mud!\n");
        return 1;
    }

    if(restore_object("/secure/banish/" + str))
    {
        write("Sorry, but this but this name is banished in this mud.\n");
        return 1;
    }

    if(!read_data())
        return 1;

    if(member(nicknames, str)>=0)
    {
        write("This name is already in your nickname list.\n");
        return 1;
    }

    write("Adding "+str+" to your nicknames.\n");
    nicknames = nicknames + ({str});

    if(!write_data())
        return 1;

    update_nicknames();

    return 1;
}


int do_nickdel(string str)
{
    int pos;

    if(query_verb()!="nickdel" || !check())
        return 0;

    if(!str)
    {
        write("Hey, you have to specify a nickname to delete, bummer!\n");
        return 1;
    }

    str = lower_case(str);

    if(!read_data())
        return 1;

    if((pos=member(nicknames, str))<0)
    {
        write("You don't have such a nickname in your list.\n");
        return 1;
    }

    write("Deleting "+str+" from your nickname file.\n");
    nicknames = nicknames[0..pos-1] + nicknames[pos+1..sizeof(nicknames)-1];

    if(!write_data())
        return 1;

    update_nicknames();

    return 1;
}


int do_nicklist()
{
    int i;
    string header;

    if(query_verb()!="nicklist" || !check())
        return 0;

    if(!read_data())
        return 1;

    header=add_gen(({string})TP->QueryName())+" nicknames:\n";
    write(header);
    for(i=0;i<sizeof(header)-1;i++)
        write("-");
    write("\n");

    for(i=0;i<sizeof(nicknames);i++)
        write(nicknames[i]+"\n");

    return 1;
}


int do_nickremove()
{
    if(query_verb()!="nickremove" || !check())
    {
        removeflag=0;
        return 0;
    }

    if(removeflag)
    {
        write("OK. Removing now the tool and setting your name to "
              +({string})TP->QueryName()+" only again.\n");

        TP->SetIds(({({string})TP->QueryRealName()}));
        if(!remove())
            destruct(TO);
    }
    else
    {
        removeflag=1;
        write("Type nickremove once again to really remove the tool and set your name\n"
             +"back again.\n");
    }

    return 1;
}
