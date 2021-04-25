/*--------------------------------------------------------------------------
 * /p/money/obj/designer/description.c -- part of the money designer
 *
 * Copyright (C) 1997 by One Step Beyond.
 * All Rights Reserved.
 * Permission granted for non-commercial use.
 *--------------------------------------------------------------------------
 */

mapping         materials, cointypes, domains;
string         *standard;

static mapping SetMaterials(mixed s)  { return materials = s; }
static mapping SetCoinTypes(mixed s)  { return cointypes = s; }
static string *SetStandard(mixed s)   { return standard = s; }
static mapping SetDomains(mixed s)    { return domains = s; }

public mapping QueryMaterials()  { return materials; }
public mapping QueryCoinTypes()  { return cointypes; }
public string *QueryStandard()   { return standard; }
public mapping QueryDomains()    { return domains; }

public mapping QueryFuns()
{
  return (["show":   "show_money";
	             "show [<cointype>|money|material[s]|domain[s]]\
 [name|val[ue]] [up|down]\n                            ";
	             "Shows the current settings.";
	             "\
        If you give a coin type as argument you get all settings for this\n\
        coin.\n\
        show with 'money' as argument or without any argument lists all\n\
        available coins and some of their values.\n\
        The argument 'materials' gives you a list of all available materials\n\
        and their values.\n\
        The argument 'domains' gives you a list of the coins available in\n\
        the domains.\n\
        For the last three situations you can specify the sorting by adding\n\
        'name' to sort alphabetically, 'val[ue]' to sort by value, 'up' or\n\
        'down' to change the order of sorting.\n", 
	   "save":   "save_design";
	             "save [design]";
	             "Saves the new settings.";
                     "\
        save forces the money designer to create a new database file for the\n\
        coin types. The old file will be saved as backup with the current\n\
        date as additional suffix.\n",
	   "exit":   "exit_design";
	             "exit [design]";
	             "Finishs designing and saves the new settings.";
                     "\
        exit forces the money designer to save the current settings (see\n\
        'save') and to remove the lock for the database file. Now other\n\
        people have the chance to customize the coin types. You should\n\
        always exit the designer, when you aren't doing anything at the\n\
        database for a longer time.\n",
           "abort":  "abort_design";
                     "abort [design]";
	             "Aborts designing and returns to the old settings.";
                     "\
        abort exits the designer without saving the changes. The old\n\
        database will be restored, the lock will be removed. This can also\n\
        happen, if you aren't doing anything at the money database for a\n\
        longer time or are netdead.\n",
	   "load":   "load_database";
                     "load [database]";
                     "Loads the money database.";
                     "\
        load loads the current database file into the money designer. It\n\
        locks the database for other people. So you are able to do your\n\
        changes exclusively and without fear of interferences with others\n", 
	   "set":    "set_values";
                     "set <type> [<value>]";
                     "Sets the value of the type <type>.";
                     "\
        set changes the values of the last coin type you looked at with\n\
        'show' or created with 'new'.\n\
        You can change every of the values 'show' lists to you except value\n\
        and weight. They are determined by the contents you can change with\n\
        'add'.\n", 
	   "new":    "new_cointype";
	             "new <name>";
                     "Creates a new cointype.";
	             "\
        new checks first if the database already contents the new coin.\n\
        If not, it adds the coin type and sets it to the initial values\n\
        (almost zero ones].\n\
        You should then use 'set' to customize it.\n",
           "add":    "add_material";
	             "add <material> <amount>";
		     "Adds (or subtracts, if amount is negative) the\n\
                              material.";
                     "\
        add changes the contents of a coin. You can add predefined materials\n\
        only. If you wish to have new materials, ask the Archwizzes of\n\
        Finance.\n\
        You can also subtract the contents by giving negative amounts.\n",
	   "delete": "delete_cointype";
	             "delete <name>";
                     "Deletes the cointype.";
	             "\
        delete removes a coin type from the database file. Please be careful\n\
        with this command, it may lead into inconsistencies.\n",
	   "domain": "domain_set_money";
	             "domain [[+|-]<domain>]";
                     "Changes the domains of the current cointype.";
                     "\
        domain changes the availability of the current cointype to a domain.\n\
        If you set a '-' (minus sign) before the name of the domain, you\n\
        remove the cointype from the list of the types of <domain>.\n\
        A '+' (plus sign) or just the domain name adds the coin type to\n\
        that domain.\n\
        domain without any argument shows you the domains the coin is\n\
        available.\n",
	   "help":   "help_command";
                     "help [designer|<command>]";
                     "Shows a short helptext.";
                     "\
        help designer gives you a short overview about the available\n\
        commands.\n\
        help <command> gives you more detailed information about the single\n\
        commands.\n",
	   "design": "design_money";
	             "design [command]";
                     "Prefix for the designer commands.";
                     "\
        To avoid conflicts with the commands of other objects you can write\n\
        'design' before every designer command.\n\
        design without arguments loads the database, if not loaded.\n"]);
}
