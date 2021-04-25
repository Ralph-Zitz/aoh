/*
** This file is called by the guilds in whole Nightfall,
** if you want to do a special wizardcommand.
**
**  02feb95 [t] Creation
**  17sep95 [t] Changed to new /obj/lib/register
**
** [t] Thragor
*/

inherit "/lib/string";

#include <moving.h>
#include <config.h>

#define TP this_player()
#define REGISTER "/obj/lib/register"
#define LONGLIST "/open/guilds/GUILDCHECK"
#define SHORTLIST "/open/guilds/GUILDLIST"

#define SF(x) #'x //'

public void create()
{
  if (is_clone())
    destruct(this_object());
  seteuid(getuid());
}

protected object check_guild(string guild,string error)
// Returns the objectpointer of the guild or 0, if any
// error occured. <error> will contain the error-message
// if called by reference.
{
  object result;
  if (!error = catch(result = load_object(guild)))
    if (result&&member(inherit_list(result),"std/guild.c")==-1)
      error = "This file is no guild.\n";
  return result;
}

protected string print_stats(mapping stats)
{
  string *idx,res;
  int i;
  
  i = sizeof(idx = sort_array(m_indices(stats),SF(<)));
  res = "";
  while(i--)
    res+=idx[i]+": "+to_string(stats[idx[i]])+"   ";
  return res;
}

public varargs string analyze_guild(string ob,status err)
// Called in the wiz_funcs to analyze a special guild, given in the guild-
// register. It returns 1 if all is fine and 0 on error, i. e. we should
// remove this guild from the list.
{
  string error,result,*c;
  string res;
  object guild;

  err = 0;
  if (file_size(ob)<1
      &&file_size(ob+".c")<1)
    {
      err = 1;
      return "The guild doesn't exist anymore!\n";
    }
  if (!guild = check_guild(ob,&error))
    return "Error: "+error+"\n";
  if (member(inherit_list(guild),"std/guild.c")==-1
      &&object_name(guild)!="/std/guild")
    {
      err = 1; 
      return "This file is no guild.\n";
    }
  else result =
(guild->QueryIntShort()||"<nameless room>")+".\n"+
"Guildclass:  "+guild->QueryGuildclass()+"\n"+
"Guildname:   "+guild->QueryGuildname()+"\n"+
"Guildobject: "+guild->QueryGuildobject()+"\n"+
"Creators:    "+implode((c = guild->QueryCreators())[0..<2],", ")
               +(sizeof(c[0..<2])?" and ":"")
               +(sizeof(c)?c[<1]:"none")+"\n"+
"Guildfile:   "+((res = guild->QueryGuildfile())?res:"def.")+"\n"+
"Minstats:\n"+
"  "+print_stats(guild->QueryMinStats())+"\n"+
"Maxstats:\n"+
"  "+print_stats(guild->QueryMaxStats())+"\n"+
"MaxHLPstats:\n"+
"  "+print_stats(guild->QueryMaxHLPStats())+"\n"+
"Members:     "+to_string(sizeof(guild->QueryMembers()))+"\n"+
"Guildleader: "+((res = guild->QueryGuildleader())?res:"none")+"\n"+
"Viceleader:  "+((res = guild->QueryViceGuildleader())?res:"none")+"\n";

  return result;
}

public string int_short(string file)
{
  object guild;
  string error;
  
  if (guild = check_guild(file,&error))
    return (guild->QueryIntShort()||"<no intshort>");
  return error;
}

protected string deep_update(string file)
{
  object ob;
  string *ilist,res;
  int i;
  
  if (ob = find_object(file))
    {
      // Get all inherited files of the current file and destruct all of
      // them to ensure, that the file really gets the new version.
      i = sizeof(ilist = inherit_list(ob));
      while(i--)
        if (ob = find_object(ilist[i]))
          destruct(ob);
    }
  else
    {
      // The guild wasn't loaded up to now, so try to do it.
      // If no error occurs call deep_update again to ensure,
      // that even all inherited files get updated.
      // This is no chance for a 'too deep recursion' unless
      // the object doesn't destruct itself again and again.
      if (res = catch(load_object(file)))
        return res;
      return deep_update(file);
    }
  return (res = catch(load_object(file)))?res:0;
}

public string update(string file)
{
  string error;
  object guild,*pls;
  int i;

  if (guild = find_object(file))
    {
      i = sizeof(pls = filter(deep_inventory(guild),
                 SF(query_once_interactive)));
      while(i--)
        {
          tell_object(pls[i],"The guild gets updated.\n");
          pls[i]->move("/std/void",M_SILENT);
        }
      guild->clean_up(-1);
      if (guild) guild->remove();
      if (guild) destruct(guild);
      i = sizeof(pls);
      if (!guild = check_guild(file,&error))
        {
          while(i--)
            tell_object(pls[i],"An error occured in the guild.\n");
          return "Error: "+error;
	}
      if (!i)
	{
          // Don't keep the (now loaded) guild in memory, if it isn't
          // used by a player right now anyway.
	  guild->clean_up(-1);
          if (guild) guild->remove();
          if (guild) destruct(guild);
          return "Guild destructed as no players present.\n";
	}
      while(i--)
	{
	  pls[i]->move(guild,M_SILENT);
          tell_object(pls[i],"Guild is updated.\n");
	}
    }
  else
    {
      // The guild doesn't exist, so just load it to check for errors and
      // destruct it again.
      if (!guild = check_guild(file,&error))
        return error;
      guild->clean_up(-1);
      if (guild) guild->remove();
      if (guild) destruct(guild);
      return "Guild destructed as no players present.\n";
    }
  return "Update ok.\n";
}

public int startfun(object player,string str,string regfile)
// Called with action 'guild'. These are special commands for wizards to check
// this and other guilds.
{
  int i,number;
  string file,*files;
  if (!TP) return 0;
  if (str=="analyze")
    {
      write(analyze_guild(blueprint(previous_object()),&i));
      if (i)
        REGISTER->RemoveEntry(blueprint(previous_object()),regfile);
      return 1;
    }
  if (sscanf(str,"analyze %d",number)==1)
    {
      files = m_indices(REGISTER->QueryRegister(regfile));
      if (!number||number>sizeof(files))
        return notify_fail("Range of guilds from 1 to "+
                           to_string(sizeof(files))
                           +".\n",NOTIFY_ILL_ARG),0;
      file = files[--number];
      write(analyze_guild(file,&i));
      if (i)
        REGISTER->RemoveEntry(file,regfile);
      return 1;
    }
  if (sscanf(str,"analyze %s",file)==1)
    {
      files = m_indices(REGISTER->QueryRegister(regfile));
      file = TP->get_path(file);
      if (file[<2..]==".c")
        file = file[0..<3];
      if (member(files,file)==-1)
        return notify_fail("No guild "+file+" known to the guildregister.\n",
			   NOTIFY_ILL_ARG),0;
      write(analyze_guild(file,&i));
      if (i)
        REGISTER->RemoveEntry(file,regfile);
      return 1;
    }
  switch(str)
    {
     case "shortlist":
      write("Short list of guilds will be evaluated.\n");
      REGISTER->PrintList(regfile,SHORTLIST,SF(int_short));
      break;
     case "list":
      write("Long list of guilds will be evaluated.\n");
      REGISTER->PrintList(regfile,LONGLIST,SF(analyze_guild));
      break;
     case "update":
      write("All loaded guilds will be updated.\n");
      REGISTER->PrintList(regfile,0,SF(update));
      break;
     default:
      write(
       "Here are some useful 'guild' commands:\n"
       "  help                   This text\n"
       "  shortlist              Short list of guilds in file " SHORTLIST "\n"
       "  list                   List of shops in file " LONGLIST "\n"
       "  analyze [<nr>|<file>]  Analyze a special (or current) guild\n"
       "  update                 Update all guilds\n");
    }
 return 1; 
}
