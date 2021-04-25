/*
** Changelog
**    Feb 97 [Mica] Creation
** 13 Mar 97 [Mica] added the possibility to disable security
** 21 Mar 97 [Mica] added the help system (binhelp)
** 05 Sep 98 [Softbyte] added bininfo
** 18 Sep 98 [Softbyte] added binpackage and helpfuns
** 21 Sep 98 [Softbyte] added heart beat support and bscan command
** 23 Sep 98 [Softbyte] Improved binlist output and package support
**
** TODO: Separate functions on inherit files
**
*/

#include <properties.h>
#include <config.h>
#include <secure/wizlevels.h>
#include <rooms.h>
#include <moving.h>
#include <string.h>
#include "bincmd.h"

inherit "/std/thing";
inherit "/lib/config";     /* config file processing */


#define RESERVED_RC ({ "rootdir","options","packagetitle","packageinfo"})

/*
** Forward declarations
*/
public int CheckHelp(string str);
public int cmd_binremove(string str);
public int cmd_binadd(string str);





/*
** Version things
*/

#define VERSION "1.07"

string pVersion;

public string QueryVersion() { return pVersion; }
public string SetVersion( string x ) { return pVersion; } /* FAKE */

/*
** Misc vars
*/

static int aliaslevel;

/*
** The pending commands mapping
*/

static object * pPending;

public object * QueryPending() { return pPending; }
public object * SetPending( object * n ) { return pPending; } /* FAKE */

/*
** The cloner object and related functions
*/

static object pCloner;

public object QueryCloner() { return pCloner; }
public object SetCloner( object nc ) { return pCloner; } /* FAKE */

/*
** Load/Save object handling
** savecnt is increased in heart_beat
** if saveneeded and savecnt > HBSTOSAVE SaveSettings is called
*/

#define HBSTOSAVE 50
static int savecnt;
static int saveneeded;

private int SaveSettings() {
  save_object( "/players/"+getuid(pCloner)+"/.binrc" );
  saveneeded = 0;
  savecnt = 0;
  return 1;
}

private int LoadSettings() {
  if ( file_size( "/players/"+getuid(pCloner)+"/.binrc.o" ) > 0 ) {
    restore_object( "/players/"+getuid(pCloner)+"/.binrc" );
    saveneeded = 0;
    savecnt = 0;
    return 1;
  }
  else
    return 0;
}

/* 
** The commands mapping
** ([ "command": ({ "filename", 
                    <disabled>,
                    0||"abrev",
		    0||"stubfile",
		    }) ])
**
*/
#define M_C_FILE       0
#define M_C_DISABLED   1
#define M_C_ABREV      2
#define M_C_STUB       3
	       
mapping pCommands;


public mapping QueryCommands() { return pCommands; }
public mapping SetCommands( mapping x ) { return pCommands; } /* FAKE */

int IsEnabled(string cmd)
{
  if (!pCommands || 0==member(pCommands,cmd) ) return 0;
  return !pCommands[cmd][M_C_DISABLED];
}

/*
** Heartbeat Handling
*/
string * pHeartbeat; /* Array holding the files with heartbeats */
string *QueryHeartbeatUser() {return pHeartbeat;}

/*
** Is the file using a heartbeat?
*/
int IsHeartUser(string cmd)
{
  if (!cmd) return 0;
  if (-1==member(pHeartbeat,cmd)) return 0;
  return 1;

}

/*
** Adds a file to the list of heartbeart files
*/
private int AddHeartbeat(string cmd)
{
  if (!cmd) return 0;
  if (IsHeartUser(cmd)) return 0;
  pHeartbeat+=({ cmd });
  return 1;
}

/*
** Removes a file of the list of heartbeat files
*/
private int RemHeartbeat(string cmd)
{
  if (!cmd) return 0;
  if (!IsHeartUser(cmd)) return 0;
  pHeartbeat-=({cmd});
  return 1;
}

/*
** Load object requiring heartbeats to avoid heartbeat errors
*/
public void VerifyHeartObjects()
{
  int i;
  string file,error;
  object ob;
  string *rem_beat;

  rem_beat=({});
  for (i=0;i<sizeof(pHeartbeat);i++)
  {
    if (!pCommands || 0==member(pCommands,pHeartbeat[i]))
    {
      rem_beat+=({pHeartbeat[i]});
    }
    file=pCommands[pHeartbeat[i]][M_C_FILE];
    if (!file || !stringp(file))
    {
      rem_beat+=({pHeartbeat[i]});
    }
    else if ( error = catch( ob = load_object( file ) ) )
    {
      if (strlen(error)<1) error="NULL";
      tell_object( pCloner,
        "["+(file||"NULL")+
        "] HeartbeatError: "+
        (error[1..]||"NULL") );
      rem_beat+=({pHeartbeat[i]});
    }
    else
    {
      if (!ob->QueryHeartbeat()) rem_beat+=({pHeartbeat[i]}); // remvoe on the fly
    }
  } 
  filter(rem_beat,#'RemHeartbeat); // Remove outdated ones 
}

/*
** The package handling functions
*/
/*
  ([package:({modules}),...])
*/
mapping pPackage;

/*
** Add a new package to the installed ones containing the given modules
*/
private int AddPackage(string package, string *mods)
{
  if (member(pPackage,package)) return 0; /* Aldready added , TODO update */
  if (!mods) return 0;
  pPackage+=([package:mods]);
  return 1;
}

/*
** Remove a package from the list of installed ones
*/
private int RemPackage(string package)
{
  if (!member(pPackage,package)) return 0; /* Not added  */
  pPackage=m_delete(pPackage,package);
  return 1;
}

/*
**  Remove modules from a package. REmove package if last module is gone
*/
private int RemModules(string package, string *mods)
{
  int i;

  for (i=0;i<sizeof(mods);i++)
  {
    cmd_binremove(mods[i]);
  }
  if (member(pPackage,package)) 
  {
    pPackage[package]-=mods;
    if (sizeof(pPackage[package])==0) RemPackage(package);
  }
  return 1;
}

/*
** Checks whether the cloner has defined an own packages.rc file
** and returns its filename
*/
string GetUserPackages()
{
  string file;

  if (!pCloner ) return 0;
  file="/players/"+getuid(pCloner)+"/packages.rc";
  if (file_size(file)>0) return file;
  return 0;
}


/*
** Returns the key value for a given key. It looks first in
** the users packagefile
*/
mixed GetPackageKeyValue(string package,string key)
{
  mixed tmp;

  if (GetUserPackages())
  {
      tmp=GetKeyValue(GetUserPackages(),package,key);
  }
  else tmp=0;

  if (!tmp)
      tmp=GetKeyValue(BINPACKAGES,package,key);
  return tmp;
}

/*
** Add modules to a package. Create new package if necessary 
*/
private int AddModules(string package, string *mods)
{
  string dir,options,file;
  int i;

    // Get a rootdir and options for all commands
    options=GetPackageKeyValue(package,"options");
    if (options) options+=" ";
    else options="";
    dir=GetPackageKeyValue(package,"rootdir");
    if (dir) dir+="/";
    else dir="";

    for (i=0;i<sizeof(mods);i++)
    {
      file=GetPackageKeyValue(package,mods[i]);
      if (!file || file=="") file=mods[i];  // the name is the module!
      /* add the file */
      cmd_binadd(options+dir+file);
    }
    if (member(pPackage,package)) 
    {
      pPackage[package]-=mods;
      pPackage[package]+=mods;
    }
    else AddPackage(package,mods);
    return 1;
}

/*
** Return an arry of names of installed packges
*/
public string *GetInstalledPackages()
{
  return m_indices(pPackage);
}

/*
** Checks whether a package is installed
*/
public int IsInstalledPackage(string package)
{
  string *pcks;

  if (!package) return 0;
  pcks=GetInstalledPackages();
  if (member(pcks,package)==-1) return 0;
  return 1;
}

/*
** Returns an array of module names of an installed package
*/
public string *GetInstalledModules(string package)
{
  string *mods;
  if (!package) return ({});
  mods=pPackage[package];
  return mods||({});
}

/*
** Returns an array of packagenames of all available packages on disk
*/
public string *GetAvailablePackages()
{
  string *tmp,*tmp2;

  if (GetUserPackages())
  {
      tmp=GetSections(GetUserPackages())||({});
  }
  else tmp=({});

  tmp2=GetSections(BINPACKAGES)||({});

  tmp2=tmp2-tmp;  // substract double sections

  return tmp+tmp2;
}

/*
** Checks whether a given packas is available on disk
*/
public int IsAvailablePackage(string package)
{
  string *pcks;

  if (!package) return 0;
  pcks=GetAvailablePackages();
  if (member(pcks,package)==-1) return 0;
  return 1;
}

/*
** Returns all modules stored in a given package
*/
public string *GetAvailableModules(string package)
{
  string *mods;
  if (!package) return ({});

  // First check for user packages
  if (GetUserPackages())
  {
      mods=GetKeys(GetUserPackages(),package);
  }
  else mods=0;

  if (!mods || !sizeof(mods) )
    mods=GetKeys(BINPACKAGES,package)||({});

  mods-=RESERVED_RC; // Exclude keywords
  return mods;
}


/*
** The abbreviation translation mapping, implemented for speed reasons
*/

mapping pAbrev;
string * pAbrevIndex;

public mapping QueryAbrev() { return pAbrev; }
public mapping SetAbrev( mapping x ) { return pAbrev; } /* FAKE */
public string * QueryAbrevIndex() { return pAbrevIndex; }
public string * SetAbrevIndex( string * x ) { return pAbrevIndex; } /* FAKE */

private int AddAbrev( string abrev, string full ) {
  if ( ! mappingp( pAbrev ) )
    pAbrev = ([ abrev : full ]);
  else
    pAbrev[ abrev ] = full;

  if ( ! pointerp( pAbrevIndex ) ) 
    pAbrevIndex = ({ abrev });
  else
    pAbrevIndex += ({ abrev });

  /* we do not set saveneeded, as this is only called by AddCommand */
  return 1;
}

private int RemoveAbrev( string abrev ) {
  if ( ! mappingp ( pAbrev ) ) return 0;

  if ( ! pAbrev[abrev] ) return 0;

  if ( pointerp( pAbrevIndex ) ) {
    pAbrevIndex -= ({ abrev });
    if ( ! sizeof( pAbrevIndex ) )
      pAbrevIndex = 0;
  }

  m_delete( pAbrev, abrev );
  if ( ! sizeof( pAbrev ) )
    pAbrev = 0;

  /* we do not set saveneeded, as this is only called by RemoveCommand */
  return 1;
}

private int StubRequired( string file ) {
  string *pathes;

  if ( ( ! file ) || ( ! pCloner ) )
    return 0;
  pathes = explode( file, "/" ) - ({ "" });
  if ( sizeof( pathes ) < 2 )
    return 0;

  if ( ( pathes[0] == "obj" ) || 
       ( ( pathes[0] == "players" ) && ( pathes[1] == getuid( pCloner ) ) ) )
    return 0; /* valid pathes for non-stubbed bin-commands */

  return 1;
}

/* stub generator, returns stub name or 0 */
private string MakeStub( string file ) {
  string basestub, *pathes, stubname, stubname2;

  if ( ( ! file ) || ( ! pCloner ) )
    return 0;

  pathes = explode( file, "/" ) - ({ "" });
  if ( sizeof( pathes ) < 2 )
    return 0;

  basestub = "/players/"+getuid( pCloner )+"/.stub";
  if ( file_size( basestub ) >= 0 ) {
    tell_object( pCloner, "Error: \""+basestub+"\" exists as a file.\n" );
    return ""; /* invalidate file */
  }
  if ( file_size( basestub ) == -1 ) {
    if ( ! mkdir( basestub ) ) {
      tell_object( pCloner, "Error: \""+basestub+"\" cannot be created.\n" );
      return ""; /* invalidate file */
    }
  }

  stubname = basestub+"/"+implode( pathes, "-" );
  stubname2 = stubname+".c"; /* we always get pathes without .c from binadd */

  if ( file_size( stubname2 ) >= 0 ) {
    tell_object( pCloner, "Removed old stub \""+stubname2+"\".\n" );
    rm( stubname2 );
  }
  
  write_file( stubname2, 
	      "/* Automatically generated stub */\n"+
	      "/* Date: "+ctime()+" */\n"+
	      sprintf( "/* Creator: [%O] */\n", this_object() )+
	      "\n"+
	      "inherit \""+file+"\";\n" );

  tell_object( pCloner, "Created stub \""+stubname2+"\".\n" );

  return stubname;
}

private void RemoveStub( string stubfile ) {
  string * pathes;

  if ( stubfile[<2..] != ".c" )
    stubfile += ".c";
  if ( file_size( stubfile ) >= 0 ) {
    pathes = explode( stubfile, "/" ) - ({ "" });
    if ( member( pathes, ".stub" ) != -1 ) {
      rm( stubfile );
      tell_object( pCloner, "Removed stub \""+stubfile+"\".\n" );
    }
  }
}

private int AddCommand( string cmd, string file, int stubmode ) {
  string abrev;
  object ob;
  string error;
  string stub;

  /* Query the blueprint for the abbreviation, cloning is not needed yet */

  if ( error = catch( ob = load_object( file ) ) ) {
    tell_object( pCloner, "["+file+"] Error: "+error[1..] );
    return 0;
  }    

  if ( ! ob ) {
    tell_object( pCloner, "["+file+"] blueprint cannot be loaded.\n" );
    return 0;
  }

  if ( ! ob->QueryIsBinCommand() ) {
    tell_object( pCloner, "["+file+"] invalid command.\n" );
    return 0;
  }

  if ( ( ob->QueryEUIDRequired() ) &&
       ( StubRequired( file ) ) ) {
    if ( ! stubmode ) {
      tell_object( pCloner, "["+file+"] stubmode required.\n" );
      return 0;
    }

    if ( stub = MakeStub( file ) ) {
      error = file;
      file = stub;
      stub = error;

      if ( error = catch( ob = load_object( file ) ) ) {
	      tell_object( pCloner, "["+file+"] Error: "+error[1..] );
	      return 0;
      }    

      if ( ! ob ) {
	      tell_object( pCloner, "["+file+"] blueprint cannot be loaded.\n" );
	      return 0;
      }
    }
    else {
      tell_object( pCloner, "["+file+"] stub cannot be generated.\n" );
      return 0;
    }
  }

  abrev = ob->QueryAbrev();

  if ( ! mappingp( pCommands ) )
    pCommands = ([ cmd : ({ file, 0, abrev, stub }) ]);
  else
    pCommands[cmd] = ({ file, 0, abrev, stub });

  if ( abrev )
    AddAbrev( abrev, cmd );

  if (ob->QueryHeartbeat()) 
    AddHeartbeat(cmd);

  saveneeded = 1;
  return 1;
}

private int RemoveCommand( string cmd ) {
  mixed * cdata;
  string * pathes, stubfile;

  if ( ! mappingp ( pCommands ) ) return 0;

  if ( ! ( cdata = pCommands[cmd] ) ) return 0;

  RemHeartbeat(cmd); // Remove heartbeart

  if ( cdata[M_C_ABREV] )
    RemoveAbrev( cdata[M_C_ABREV] );

  if ( cdata[M_C_STUB] )
    RemoveStub( cdata[M_C_FILE] );

  m_delete( pCommands, cmd );
  if ( ! sizeof( pCommands ) )
    pCommands = 0;

  saveneeded = 1;
  return 1;
}

/*
** Function to query matching commands for a given regexp
** input string 'arg'
** Returns: 0: An error occured, the string error is filled with the message
**          Keys matching the input 
*/
string *GetMatchingCommands(string arg, string error)
{
  string *keys;
  error=0;
  keys=0;

  if ( ! arg ) {
    error= "You need to supply a valid command name/regexp.\n" ;
    return 0;
  }
 
  if ( ! regexp( ({ "dummy" }), arg ) ) {
    error="Invalid regular expression.\n" ;
    return 0;
  }

  arg += "$"; /* to prevent regexp "xcall" match "xcallouts" */
  
  if ( ( ! mappingp( pCommands ) ) ||
       ( ! sizeof( keys = m_indices( pCommands ) ) ) ) {
    error= "There are currently no commands defined.\n" ;
    return 0;
  }

  keys = regexp( keys, arg );
  if ( ( ! pointerp( keys ) ) || ( ! sizeof( keys ) ) ) {
    error= "There are no commands matching to \""+arg+"\".\n" ;
    return 0;
  }
  return keys;
}


/*
** Security function, returns true if we are secure
** If lvl is true do previous_object chain checking
*/

static int pSecure;

public int QuerySecure() { return pSecure; }
public int SetSecure( int n ) { return pSecure; } /* FAKE */

private int security( int lvl ) {
  int poc;
  object prev;
    
  if ( ! pSecure ) /* command security has been disabled */
    return 1;

  if ( ( this_interactive() ) &&
       ( this_player() != this_interactive() ) ) {
    tell_object( pCloner, sprintf( "Security violation detected in [%O].\n  this_interactive is %O.\n", this_object(), this_interactive() ) );
    return 0;
  }

  if ( lvl ) {
    for ( poc = 0; prev = previous_object( poc ); poc++ ) { 
      if ( ( getuid( prev ) != ROOTID ) &&
	   ( ( getuid( prev ) != getuid() ) ||
	     ( geteuid( prev ) != geteuid() ) ) ) {
	tell_object( pCloner, sprintf( "Security violation detected in [%O].\n  previous_object( %O ) has uid %O and euid %O\n", this_object(), poc, getuid( prev ), geteuid( prev ) ) );
	return 0;
      }
    }
  }

  return 1;
}

/*
** Functions to make it bin-commands possible to store data within the tool
*/

mapping pPermData; /* to store saved values */
static mapping pTempData; /* to store discardable values */

public mapping QueryTemp() { return pTempData; }
public mapping QueryPerm() { return pPermData; }

public varargs mapping SetPermData( string pn, mixed arg ) {
  if ( ! security( 1 ) ) return 0;

  if ( arg ) {
    if ( mappingp( pPermData ) )
      pPermData[pn] = arg;
    else
      pPermData = ([ pn : arg ]);
  }
  else {
    if ( mappingp( pPermData ) ) {
      m_delete( pPermData, pn );
      if ( ! sizeof( pPermData ) )
	pPermData = 0;
    }
  }

  saveneeded = 1;
  return pPermData;
}

public varargs mixed QueryPermData( string pn ) {
  return pPermData ? pPermData[pn] : 0;
} 

public varargs mapping SetTempData( string pn, mixed arg ) {
  if ( ! security( 1 ) ) return 0;

  if ( arg ) {
    if ( mappingp( pTempData ) )
      pTempData[pn] = arg;
    else
      pTempData = ([ pn : arg ]);
  }
  else {
    if ( mappingp( pTempData ) ) {
      m_delete( pTempData, pn );
      if ( ! sizeof( pTempData ) )
	pTempData = 0;
    }
  }

  return pTempData;
}

public varargs mixed QueryTempData( string pn ) {
  return pTempData ? pTempData[pn] : 0;
} 

/*
** Nostore handling
*/

static int pNostore;

int QueryNostore() { return pNostore; }
int SetNostore( int x ) { return pNostore = x; }
int IncreaseNostore() { return ++pNostore; }
int DecreaseNostore() { return --pNostore; }

/*
** Show the ring as extralook rather than in the inventory
*/

public string QueryExtralook() {
  object en;
 
  if ( ( en = environment() ) && 
       ( IS_IMMORTAL( en ) ) )
    return capitalize(en->QueryPronoun())+" is wearing "+en->QueryPossessive()+" ring of wizards.\n";
}

/*
** Toggle heart_beat depending on netdead status
*/

void PlayerNetdead (int arg) {
  if ( arg && saveneeded )
    SaveSettings();

  set_heart_beat(!arg);
}

/*
** Define an own move function, to prevent abuse by moving it away and then
** executing anything
*/

public varargs int move( mixed dest, int method, mixed extra ) {
  move_object( this_object(), pCloner ? pCloner : dest );
  return ME_OK;
}

/*
** Define an own remove function to do saving
*/

public int remove() {
  if ( saveneeded )
    SaveSettings();
  return ::remove();
}

/*
** Forward heartbeat to user hearts
*/
int user_beat(string cmd)
{
  string file;
  if (!cmd) return 1; // remove on the fly
  file=pCommands[cmd][M_C_FILE];
  if (!file) return 1; // remove on the fly
  return !call_other(file,"HeartBeat",pCloner);
}


/*
** Float on top in heart_beat
** (move into inventory of pCloner if not there is a wanted side-effect)
*/

void heart_beat()
{
  string *rem_beat;

  if ( ( pCloner ) &&
       ( first_inventory( pCloner ) != this_object() ) )
  {
    move( pCloner );
  }

  if ( ( savecnt++ > HBSTOSAVE ) && ( saveneeded ) )
  {
    SaveSettings();
  }

  if ( ! pSecure ) /* always tell the cloner that tool is insecure */
  {
    tell_object( pCloner, "Command security is disabled!\n" );
  }

  /* Process user heartbeats */
  if (pHeartbeat && sizeof(pHeartbeat))
  {
    rem_beat=filter(pHeartbeat||({}),#'IsEnabled); 
    rem_beat=filter(rem_beat||({}),#'user_beat);
    if (rem_beat && sizeof(rem_beat) )
    {
      filter(rem_beat,#'RemHeartbeat); // Remove outdated ones 
    }
  }
}

/*
** The internal help functions
*/

/*
** Define some actions needed to control the dispatcher
*/

init() {
  mixed *cdata;
  string *cindex;
  int i;

  /* bincmd related actions */
  add_action( "cmd_bindestruct", "bindestruct" ); /* MUST BE FIRST! */
  add_action( "cmd_binadd",     "binadd" );
  add_action( "cmd_binremove",  "binremove" );
  add_action( "cmd_binlist",    "binlist" );
  add_action( "cmd_binpending", "binpending" );
  add_action( "cmd_binsave",    "binsave" );
  add_action( "cmd_binload",    "binload" );
  add_action( "cmd_bindisable", "bindisable" );
  add_action( "cmd_binenable",  "binenable" );
  add_action( "cmd_binsecure",  "binsecure" );
  add_action( "cmd_binhelp",    "binhelp" );
  add_action( "cmd_bininfo",    "bininfo" );    /* Softbyte 05sep98 */
  add_action( "cmd_binpackage", "binpack",1 );    /* Softbyte 18sep98 */
  add_action( "cmd_binheart", "binheart" );    /* Softbyte 21sep98 */

  /* alias related actions */
  add_action( "cmd_alias",      "alias" );
  add_action( "cmd_unalias",    "unalias" );

  /* add binary commands */
  if ( mappingp( pCommands ) && ( sizeof( cindex = m_indices( pCommands ))) ) {
    for( i = 0; i < sizeof( cindex ); i ++ ) {
      cdata = pCommands[ cindex[i] ];
      if ( ! cdata[M_C_DISABLED] ) {
	if ( cdata[M_C_ABREV] ) /* add command as abreviation */
	  add_action( "_parser_bin", cdata[M_C_ABREV], AA_SHORT );
	else /* add command with full name */
	  add_action( "_parser_bin", cindex[i] );
      }
    }
  }

  /* the main command parser */
  add_action( "_parser_alhis", "hist", 1 );
  add_action( "_parser_alhis", "", 1 );
}


/* 
** Check binary command function
*/

private object CheckCommand( string file, string fun ) {
  object ob;

  /* check for invalid files */
  if ( file_size( file+".c" ) <= 0 )
    return 0; /* TODO : give a message */
  
  /* try to clone the command */
  if ( ! ( ob = clone_object( file ) ) )
    return 0; /* TODO : give a message */

  if ( ! function_exists( fun, ob ) ) {
    ob->remove();
    if ( ob )
      destruct( ob );
    return 0; /* TODO : give a message */
  }

  return ob;
}

/*
** Execute binary command
*/

private int ExecuteCommand( string file, string fun, string arg ) {
  object ob;
  int rc;

  if ( ob = CheckCommand( file, fun ) ) {
    rc = call_other( ob, fun, arg ); /* funcall(symbol_f.. here causes C!*/
    /* check if object is still existing and wants to be destructed */
    if ( ( ob ) && ( ob->QueryDestruct() ) ) {
      ob->remove();
      if ( ob )
	destruct( ob );
    }
    else if ( ob ) {
      pPending += ({ ob });
      pPending -= ({ 0 }); /* remove those ones from the list */
    }

    return rc;
  }
  else
    return 0;
}

/*
** Test for abbreviated command
*/

private string GetFullCommandName( string verb ) {
  int i, l;

  if ( pAbrevIndex ) {
    for( i = 0; i < sizeof( pAbrevIndex ); i++ ) {
      if ( ( l = strlen( pAbrevIndex[i] ) ) <= strlen( verb ) ) {
	if ( pAbrevIndex[i] == verb[0..l-1] ) {
	  return pAbrevIndex[i];
	  break;
	}
      }
    }
  }

  return 0;
}

/* 
** Command parser for not abbreviated commands
*/

public int _parser_bin( string arg ) {
  string verb;
  string full;
  mixed * cdata;
  string file;

  if ( ( ! this_player() ) ||
       ( ! IS_IMMORTAL( this_player() ) ) ||
       ( ! ( verb = query_verb() ) ) )
    return 0;

  if ( ! security( 1 ) ) return 1;

  if ( ! mappingp( pCommands ) ) 
    return 0;

  if ( cdata = pCommands[verb] ) {
    file = cdata[M_C_FILE];
    return ExecuteCommand( file, "main", arg );
  }
  else { /* Handle Abreviations */
    if ( full = GetFullCommandName( verb ) ) {
      verb = pAbrev[full];
      if ( cdata = pCommands[verb] ) {
	file = cdata[M_C_FILE];
	return ExecuteCommand( file, "main", arg );
      }
    }
  }

  return 0;
}

/*
** The alias mapping and related functions
** ([ "alias" : "what" ])
**
*/

mapping pAliases;

public mapping QueryAliases() { return pAliases; }
public mapping SetAliases() { return pAliases; } /* FAKE */

private int AddAlias( string cmd, string what ) {
  if ( ! mappingp( pAliases ) )
    pAliases = ([ cmd : what ]);
  else
    pAliases[cmd] = what;

  saveneeded = 1;
  return 1;
}

private int RemoveAlias( string cmd ) {
  if ( ! mappingp ( pAliases ) ) return 0;

  if ( ! pAliases[cmd] ) return 0;

  m_delete( pAliases, cmd );
  if ( ! sizeof( pAliases ) )
    pAliases = 0;

  saveneeded = 1;
  return 1;
}

private int DoAlias(string verb, string arg) {
  int i, s, f;
  string cmd, *args, tmp;

  if ( ( ! verb ) ||
       ( ! mappingp( pAliases ) ) ||
       ( ! sizeof( pAliases ) ) )
    return 0;

  if ( cmd = pAliases[verb] ) {
    if ( arg && ( args = explode( arg, " " ) ) ) {
      s = sizeof( args );
      i = s--;
      if ( f = sizeof( regexp( ({ cmd }), "![0-9]*\\*" ) ) )
        cmd = L_STR->string_replace( cmd, "!*", arg );
      arg = "";
      while ( i-- ) {
        tmp = cmd;
        if ( f ) cmd = L_STR->string_replace( cmd, "!"+(i+1)+"*", implode( args[i..s], " " ) );
        cmd = L_STR->string_replace( cmd, "!"+(i+1), args[i]);
        if(!f&&tmp==cmd) arg=" "+args[i]+arg;
      }
      cmd+=arg;
    }
    if ( ++aliaslevel > 6)
      tell_object( pCloner, "*** Too deep recursion. Aborting execution.\n" );
    else {
      IncreaseNostore();
      pCloner->command_me( cmd );
      DecreaseNostore();
    }
    aliaslevel = 0;
    return 1;
  }
  return 0;
}

/*
** handle history
*/

#define MAX_HISTORY 25

static string * pHistory;

public string * QueryHistory() { return pHistory; }
public string * SetHistory( string * x ) { return pHistory; } /* FAKE */

private int DoHistory( string line ) {
  int i;
  string cmd, *strs;

  if ( ! line ) 
    return 1;
  else if ( line == "%!" || 0==strstr("history",line) ) {
    tell_object( pCloner, "Current command history:\n" );
    for ( i = MAX_HISTORY; i; --i ) {
      if ( pHistory[ i - 1 ] ) {
	tell_object( pCloner, 
		     sprintf( " %2.2s : %-70.70s\n", ""+i, pHistory[i-1] ) );
      }
    }
    return 1;
  } 
  else if ( ( line[0..1] == "%%" ) &&
	    ( cmd = pHistory[0]+line[2..] ) ) {
    IncreaseNostore();
    this_player()->command_me( cmd );
    DecreaseNostore();

    return 1;
  } 
  else if ( ( line[0] == '^' ) &&
	    ( strs = explode( line, "^" ) -({""}))) {
    if ( sizeof( strs ) &&
	 strs[0] && 
	 ( cmd = pHistory[0] ) ) {
      if ( sizeof( strs ) == 2 ) 
	cmd = L_STR->string_replace( cmd, strs[0], strs[1]);
      else
	cmd = L_STR->string_replace( cmd, strs[0], "" );

      DecreaseNostore();
      this_player()->command_me( cmd );
      IncreaseNostore();

      return 1;
    }
  } 
  else if ( ( line[0] == '%' ) &&
	    ( sscanf( line[1..], "%d", i ) ) ) {
    i = i > 0 ? i : 1;
    i = ( i <= MAX_HISTORY ) ? i : MAX_HISTORY;
    if ( cmd = pHistory[i-1] ) {
      IncreaseNostore();
      this_player()->command_me( cmd );
      DecreaseNostore();
    }
    return 1;
  } 
  else if ( line[0] == '%' ) {
    for ( i = 0; i < MAX_HISTORY; i++ ) {
      if ( pHistory[i] &&
	   ( pHistory[i][0..(strlen(line)-2)] == line[1..] ) ) {

	IncreaseNostore();
	this_player()->command_me( pHistory[i] );
	DecreaseNostore();

        return 1;
      }
    }
  } 
  else if ( QueryNostore() < 1 ) 
    pHistory = ({ line })+pHistory[0..MAX_HISTORY-2];

  return 0;
}

/*
** The parser for alias/history 
*/

public int _parser_alhis( string arg ) {
  string verb;
  mixed * cdata;
  mixed tx;
  int i;
  string file;
  object ob;
  int rc;

  if ( ( ! this_player() ) ||
       ( ! IS_IMMORTAL( this_player() ) ) ||
       ( ! ( verb = query_verb() ) ) )
    return 0;

  if ( ! security( 1 ) ) return 1;

  if ( DoHistory( verb+(arg ? " "+arg : "") ) )
    return 1;

  if ( DoAlias( verb, arg ) )
    return 1;

  return 0;

}

/*
** The QueryHelpMsg
*/
string QueryHelpMsg()
{
  string s,tmp;
  string *files;
  int i;

  s="********** The ring of wizards V"+VERSION+" **********\n"
    "   Written by Mica 1997/1998\n"
    "   Additional functionaliy by Softbyte 1998\n"
    "   Last changed on "+ctime(file_date(blueprint(this_object())+".c"))+
    ".\n"
    "\n"
    "You can get help on commadns or packages with the command\n"
    "'binhelp <command|package>' or with '<command> ?'.\n\n"
    "The internal functions of the bintool are:\n";

  files=get_dir(BINHELP+"/*.hlp");
  for (i=0;i<sizeof(files);i++)
  {
    sscanf(files[i],"%s.hlp",files[i]);
  }
  tmp=implode(files,", ");
  s+="  "+L_STR->wrap(tmp,75,2)+"\n";

  // Package information
  s+="The available packages are:\n";
  files=GetAvailablePackages();
  tmp=implode(files,", ");
  s+="  "+L_STR->wrap(tmp,75,2)+"\n";
  return s;
}

/*
** The extended help function
*/

private int _help_function( mixed * cdata ) {
  string fname, fdata;

  /* if help-function exists in the file, call it */
  if ( ExecuteCommand( cdata[M_C_FILE], "help", 0 ) )
    return 1;

  if ( cdata[M_C_STUB] )
    fname = cdata[M_C_STUB];
  else
    fname = cdata[M_C_FILE];

  if ( fname[<2..] == ".c" )
    fname = fname[0..<3];

  fname += ".hlp";

  if ( file_size( fname ) > 0 ) {
    fdata = read_file( fname );
    L_STR->smore( fdata, pCloner->QueryPageSize(), pCloner );
    return 1;
  } 

  tell_object( pCloner, "No help available.\n" );
  return 1;
}

/*
** Get help on the bintool's own commands
*/
private int _internal_help( string arg)
{
  string s,file;
  string *args;
  
  if (!arg) return 0;

  args=explode(arg,"/"); // Kill pathnames ... security
  if (!args || !sizeof(args)) return 0;
  arg=args[<1];
  file=BINHELP+"/"+arg+".hlp";
  if (file_size(file)>0) //  help file found
  {
    s=read_file(file);
    L_STR->SMore(s);
    return 1;
  }
  file=BINPACKAGEINFO+"/"+arg+".hlp";
  if (file_size(file)>0) //  help file found
  {
    s=read_file(file);
    L_STR->SMore(s);
    return 1;
  }
  return 0; // nothing found
}


/*
** The binhep command
*/
public int cmd_binhelp( string arg ) {
  mixed * cdata;
  string full;

  if (CheckHelp(arg)) return 1;
  if ( ! security( 1 ) )
    return 0;

  if ( ! arg ) {
    tell_object( pCloner, "Usage: binhelp <command>\n" );
    return 1;
  }

  if ( ! mappingp( pCommands ) )
  {
    if (!_internal_help(arg))
      tell_object( pCloner, "The given command does not exist.\n" );
    return 1;
  }

  arg = norm_id( arg ); /* remove spaces etc. */

  if ( cdata = pCommands[arg] )
    return _help_function( cdata );
  else 
  {
    if ( full = GetFullCommandName( arg ) ) {
      arg = pAbrev[full];
      if ( cdata = pCommands[arg] )
      {
	      return _help_function( cdata );
      }
    }
  }
  if (!_internal_help(arg))
    tell_object( pCloner,
     "The given command or package does not exist.\n" );
  return 1;
}


/*
** Do some checking in creation for security reasons .. CREATE
*/

create() {
  if (::create()) return;

  if ( object_name() == blueprint() )
    return;

  if ( ( ! this_player() ) || 
       ( ! IS_IMMORTAL( this_player() ) ) ) {
    destruct( this_object() );
    return 1;
  }

  if ( present_clone( this_object(), this_player(),1 ) ) {
    write( "You already have one.\n" );
    destruct( this_object() );
    return 1;
  }

  pCloner = this_player();
  pSecure = 1; /* the tool is always secure on creation, that is not saved */
  aliaslevel = 0;
  savecnt = 0;
  saveneeded = 0;

  AddId("ring");
  AddId("tool");
  AddId("bin");

  SetShort( "" ); /* nobody needs to see this item, for what? */
  SetLong( "This is your copy of Mica's ring of wizards.\n"+ 
	   "Use 'help ring' to get more information.\n" );

  SetProp(P_NODROP,"A wizard's ring cannot be dropped.\n" );
  SetProp(P_NOGET,"A wizard's ring cannot be taken.\n");
  SetProp(P_NOGIVE,"A wizard's ring cannot be given away.\n" );
  SetProp(P_AUTOOBJECT,1);
  Set( "IsTool", 1 ); /* do not mess with compat stuff */

  pHistory = allocate(MAX_HISTORY);
  pPending = ({});
  pHeartbeat = ({});
  pPackage = ([]);
  LoadSettings();

  if ( ! pVersion )
  {
    tell_object( pCloner, 
		 "Congratulations, you now own a ring of wizards!\n"+
		 "To get more information about it, use 'help ring'.\n" );
    pVersion = VERSION;
    SaveSettings();
  }
  else if ( pVersion != VERSION )
  {
    tell_object( pCloner, 
		 "Your ring of wizards got updated to version "+VERSION+".\n" );
    pVersion = VERSION;
    SaveSettings();
  }

  VerifyHeartObjects();

  set_heart_beat(1);
}

/* 
** The binsecure command to turn on/off tool security
*/

public int cmd_binsecure( string arg ) 
{
  if (CheckHelp(arg)) return 1;
  if ( ! security( 1 ) )
    return 1;

  if ( arg ) {
    arg = norm_id( arg );
    if ( arg == "?" ) {
      tell_object( pCloner, 
		   "Usage: binsecure [<\"on\">|<\"off\">]\n" );
      return 1;
    }
    
    if ( arg == "on" )
      pSecure = 1;
    else if ( arg == "off" )
      pSecure = 0;
  }    

  tell_object( pCloner,
	       "Command security is "+(pSecure?"enabled":"disabled")+".\n");
  
  return 1;
}
				      

/*
** The binsave command to save the tool settings manually 
*/

public int cmd_binsave( string arg ) 
{
  if (CheckHelp(arg)) return 1;
  if ( ! security( 1 ) ) return 1;

  SaveSettings();
  write( "Settings saved.\n" );
  return 1;
}

/*
** The binload command to load the tool settings manually 
*/

public int cmd_binload( string arg ) 
{
  if (CheckHelp(arg)) return 1;
  if ( ! security( 1 ) ) return 1;

  LoadSettings();
  write( "Settings loaded.\n" );
  return 1;
}

/*
** The bindestruct command to destruct the tool if nothing works anymore
*/

public int cmd_bindestruct( string arg ) 
{
  if (CheckHelp(arg)) return 1;
  if ( ! security( 1 ) ) return 1;

  if ( pCloner ) 
    tell_object( pCloner, "Self-destruct initiated...\n" );

  remove(  );
  if ( this_object() )
    destruct( this_object() );

  return 1;
}

/*
** The binadd command used to add binary commands
*/

public int cmd_binadd( string arg ) {
  object ob;
  string fname, path, * cname, cmdn, match;
  mixed * dentries;
  int i, stubmode;

  if (CheckHelp(arg)) return 1;
  if ( ! security( 1 ) ) return 1;

  stubmode = 0;
  cname = explode( arg||"", " " ) - ({ "" });
  while( ( sizeof(cname) ) &&
	 ( cname[0][0]=='-' ) ) {
    switch( cname[0][1..] ) {
    case "stub":
    case "s":
      stubmode = 1;
      break;
    }
    cname = cname[1..];
  }
  if ( ! sizeof( cname ) )
    arg = 0;
  else 
    arg = implode( cname, " " );

  if ( ! arg ) {
    write( "Usage: binadd [-s] <filename/wildcard>\n" );
    return 1;
  }
   
  match = "/secure/master"->make_path_absolute( arg );
  
  dentries = get_dir( match );
  if ( ( ! pointerp( dentries ) ) || 
       ( ! sizeof( dentries -= ({ ".", ".." }) ) ) ) {
    /* check if a single name is given without .c */
    if ( ( strlen( match ) > 2 ) && 
	 ( match[<2..] != ".c" ) &&
	 ( file_size( match+".c" ) > 0 ) ) {
      dentries = ({ explode( match+=".c", "/" )[<1] });
    }
    else {
      write( "No matching files found for \""+match+"\".\n" );
      return 1;
    }
  }

  path = implode( explode( match, "/" )[0..<2], "/" );

  for( i = 0; i < sizeof( dentries ); i++ ) {
    fname = path + "/" + dentries[i];
    switch ( file_size( fname ) ) {
    case -2:
      write( "Ignored directory \""+fname+"\".\n" );
      break;
    case -1:
      write( "Ignored not accessible \""+fname+"\".\n" );
      break;
    case 0:
      write( "Ignored zero-length \""+fname+"\".\n" );
      break;
    default:
      if ( strlen( fname ) < 2 ) {
	write( "Ignored invalid \""+fname+"\".\n" );
	break;
      }
      if ( fname[<2..] != ".c" ) {
	write( "Ignored \""+fname+"\".\n" );
	break;
      }

      fname = fname[0..<3]; /* skip trailing .c */

      cname = explode( fname, "/" );
      if ( ( ! pointerp( cname ) ) || ( sizeof( cname ) < 2 ) ) {
	write( "Ignored invalid \""+fname+"\".\n" );
	break;
      }

      if ( AddCommand( cname[<1], fname, stubmode ) )
	      write( "Added \""+fname+"\" as \""+cname[<1]+"\".\n" );
      
      break;
    }
  }

  if ( pCloner ) /* float on top to call init */
    move( pCloner );

  return 1;
}

/*
** The binremove command used to remove binary commands 
*/

public int cmd_binremove( string arg ) {
  string * keys;
  string error;
  int i;

  if (CheckHelp(arg)) return 1;
  if ( ! security( 1 ) ) return 1;

  if ( (!arg) || ( arg == "?" ) )
    return notify_fail( "Usage: binremove <module>\n" ), 0;

  keys=GetMatchingCommands(arg,&error);
  if (!keys) {  /* No matching commands */
    write(error);
    return 1;
  }

  for( i = 0; i < sizeof( keys ); i++ ) {
    if ( RemoveCommand( keys[i] ) ) 
      write( "Removed command \""+keys[i]+"\".\n" ); 
    else
      write( "Could not remove command \""+keys[i]+"\".\n" );
  }

  if ( pCloner ) /* float on top to call init */
    move( pCloner );

  return 1;
}

/*
** The bindisable command used to temporarily disable binary commands 
*/

public int cmd_bindisable( string arg ) {
  string * keys;
  string error;
  int i;

  if (CheckHelp(arg)) return 1;
  if ( ! security( 1 ) ) return 1;

  if ( (!arg) || ( arg == "?" ) )
    return notify_fail( "Usage: bindisable <module>\n" ), 0;

  keys=GetMatchingCommands(arg,&error);
  if (!keys) {  /* No matching commands */
    write(error);
    return 1;
  }

  for( i = 0; i < sizeof( keys ); i++ ) {
    pCommands[keys[i]][M_C_DISABLED] = 1;
    write( "Disabled command \""+keys[i]+"\".\n" ); 
  }

  if ( pCloner ) /* float on top to call init */
    move( pCloner );

  saveneeded = 1;

  return 1;
}

/*
** The binenable command used to enable binary commands again 
*/

public int cmd_binenable( string arg ) {
  string * keys;
  string error;
  int i;

  if (CheckHelp(arg)) return 1;
  if ( ! security( 1 ) ) return 1;

  if ( (!arg) || ( arg == "?" ) )
    return notify_fail( "Usage: binenable <module>\n" ), 0;

  keys=GetMatchingCommands(arg,&error);
  if (!keys) {  /* No matching commands */
    write(error);
    return 1;
  }


  for( i = 0; i < sizeof( keys ); i++ ) {
    pCommands[keys[i]][M_C_DISABLED] = 0;
    write( "Enabled command \""+keys[i]+"\".\n" ); 
  }

  if ( pCloner ) /* float on top to call init */
    move( pCloner );

  saveneeded = 1;

  return 1;
}

/*
** The binlist command to list all active binary commands
*/

public int cmd_binlist( string arg ) {
  string * keys;
  int i, l, cnt;
  string res;
  mixed * cdata;
  string cname, tag, tag2, tag3, tag4;
  string package;
  string * mods;
  int shortmode;

  if (CheckHelp(arg)) return 1;

  if ( (arg=="?") )
    return notify_fail( "Usage: binlist [-s] [package <package>] [<regexp<]>\n" ), 0;

  if ( ! security( 1 ) ) return 1;

  /* defaults */
  package=0;
  mods=0;
  shortmode=0;
  l=-1;

  if (arg) /* command processing */
  {
    keys=norm_id(arg,1)||({});
    for (i=0;i<sizeof(keys);i++)
    {
      if (keys[i]=="-s") shortmode=1;
      else if (keys[i]=="-p" || keys[i]=="package")
      {
        if (i<sizeof(keys)-1) 
        {
          package=keys[i+1];
          if (!IsInstalledPackage(package) )
            return notify_fail( "Package '"+package+"' not installed.\n" ), 0;
          mods=GetInstalledModules(package);
          i++;
        }
        else 
         return notify_fail( "Package name missing\n" ), 0;
      }
      else l=i;
    }
  }
  /* Remaining argument is the real one */
  if (l<0) arg=0;
  else arg=keys[l];


  if ( ( ! mappingp( pCommands ) ) || ( ! sizeof( pCommands ) ) ) {
    write( "There are currently no commands available.\n" );
    return 1;
  }

  keys = m_indices( pCommands );

  if ( arg )
  {
    if ( ! regexp( ({ "dummy" }), arg ) ) {
      write( "Invalid regular expression.\n" );
      return 1;
    }
    arg += "$";
    keys = regexp( keys, arg );
    if ( ! sizeof( keys ) ) {
      write( "There are no commands matching \""+arg+"\".\n" );
      return 1;
    }
    if (package)
      res = "Current commandset in package '"+package+
            "' matching \""+arg+"\":\n\n";
    else
      res = "Current commandset matching \""+arg+"\":\n\n";
  }
  else if (package)
    res = "Current commandset in package '"+package+"':\n\n";
  else
    res = "Current commandset:\n\n";

  keys = sort_array( keys, SF( > ) ); 

  if (!shortmode)
  {
    res+=sprintf( "%-15.15s /------- Abbreviated (*)\n", " ");
    res+=sprintf( "%-15.15s |/------ Stubbed (S)\n", " ");
    res+=sprintf( "%-15.15s ||/----- Disabled (D) \n", " ");
    res+=sprintf( "%-15.15s |||/---- Heartbeat (H)\n", " ");
    res+=sprintf( "%-15.15s ||||\n", " ");
  }

  cnt=0;
  for( i = 0; i < sizeof( keys ); i ++ )
  {
    // key not in package ..continue
    if (mods && -1==member(mods,keys[i]) ) continue;

    cdata = pCommands[keys[i]];
    cname = keys[i];
    tag = ".";

    if (cdata[M_C_FILE] && IsHeartUser(cname) && IsEnabled(cname) )
      tag4="H";
    else 
      tag4=".";

    if ( cdata[M_C_DISABLED] )
      tag3 = "D";
    else
      tag3 = ".";

    if ( cdata[M_C_STUB] )
      tag2 = "S";
    else
      tag2 = ".";

    if ( cdata[M_C_ABREV] )
    {
      if ( ( l = strlen( cdata[M_C_ABREV] ) ) <= strlen( keys[i] ) )
      {
        if ( cdata[M_C_ABREV] == keys[i][0..l-1] ) 
        {
          if ( strlen( keys[i][l..] ) )
            cname = cdata[M_C_ABREV]+"("+keys[i][l..]+")";
          else
            cname = cdata[M_C_ABREV];
          tag = "*";
        }
        else
        {
          cname = cdata[M_C_ABREV];
          tag = "?"; /* invalid abrev */
        }
      }
      else
      {
        cname = cdata[M_C_ABREV];
        tag = "?"; /* invalid abrev */
      }
    }
    
    res+=sprintf( "%-15.15'.'s %s%s%s%s [%s]\n", cname+" ",
                 tag, tag2, tag3,tag4, cdata[M_C_FILE] );
    if ( cdata[3] )
      res+=sprintf( "%21.21s [%s]\n", "", cdata[M_C_STUB] );
    cnt++;
  } /* next command */

  res += "\n";
  res += sprintf( "There are %d/%d commands available.\n",
    cnt,sizeof( m_indices(pCommands) ) );

  L_STR->smore( res, pCloner->QueryPageSize(), pCloner );
  
  return 1;
}

/*
** The binpending command to list pending commands 
*/

public int cmd_binpending( string arg ) {
  int i;

  if (CheckHelp(arg)) return 1;

  if ( ! security( 1 ) ) return 1;

  if ( arg ) {
    write( "You cannot supply an argument.\n" );
    return 1;
  }

  if ( ( ! pointerp( pPending ) ) ||
       ( ! sizeof( pPending -= ({ 0 }) ) ) ) {
    write( "There are currently no pending commands.\n" );
    return 1;
  }

  write( "Currently pending commands:\n" );
  for( i = 0; i < sizeof( pPending ); i++ )
    write( sprintf( "%2d [%O]\n", i, pPending[i] ) );

  return 1;
}

/*
** The alias command
*/

public int cmd_alias( string str ) {
  string * keys, al, cmd;
  int i;

  if (CheckHelp(str)) return 1;

  if ( ! security( 1 ) ) return 1;

  if ( ! str ) {
    if ( ( ! mappingp( pAliases ) ) || ( ! sizeof( pAliases ) ) ) {
      write( "There are currently no aliases defined.\n" );
      return 1;
    }

    keys = m_indices( pAliases );
    write("Current aliases:\n");
    for( i = 0; i < sizeof( keys ); i ++ )
      write(" "+sprintf( "%-12.12'.'s %s\n", keys[i]+" ", pAliases[keys[i]])); 
  }
  else if ( sscanf( str, "%s %s", al, cmd ) == 2 ) {
    if(!sizeof(keys=explode(al, " ")-({""}))) return 0;
    al=keys[0];

    if ( AddAlias( al, cmd ) )
      write("Setting an alias \""+al+"\" for \""+cmd+"\"\n");
  }
  else {
    if ( mappingp( pAliases ) && ( cmd = pAliases[str] ) )
      write("Alias \""+str+"\" is set to \""+cmd+"\"\n");
    else 
      write("An alias \""+str+"\" does not exist\n");
  }

  return 1;
}

/*
** The unalias command
*/

public int cmd_unalias( string str ) {
  string cmd;

  if (CheckHelp(str)) return 1;
  if ( (!str) )
    return notify_fail( "Usage: unalias <alias>\n" ), 0;

  if ( mappingp( pAliases ) && 
       ( cmd = pAliases[ str ] ) &&
       RemoveAlias( str ) )
    write("Unalias: \""+str+"\" for \""+cmd+"\"\n");
  else
    write("An alias \""+str+"\" does not exist\n");

  return 1;
}

/*
** The bininfo command
*/
public int cmd_bininfo( string str ) 
{
  string error;
  string *keys;
  string file;
  mixed *cdata;
  mixed rc;
  string *rc2;
  string s;
  object ob;
  int i,j;

  if (CheckHelp(str)) return 1;
  if ( (!str) )
    return notify_fail( "Usage: bininfo <module>\n" ), 0;

  keys=GetMatchingCommands(str,&error);
  if (!keys) {  /* No matching commands */
    write(error);
    return 1;
  }

  for( i = 0; i < sizeof( keys ); i++ ) {
    if ( cdata = pCommands[keys[i]] ) {
      file = cdata[M_C_FILE];
      s="Information on "+file+": ";
      if ( (ob = CheckCommand( file, "QueryVersion" ) ) ||
           (ob = CheckCommand( file, "QueryAuthor" ) ) ) {
       s+="\n\n"; 
       rc = call_other( ob, "QueryAuthor"); 
       if (rc) s+="  Author:   "+rc+"\n";
       rc = call_other( ob, "QueryVersion"); 
       if (rc) s+="  Version:  "+rc+"\n";
       rc=ctime(file_date(file+".c"));
       s+="  Filedate: "+rc+"\n";
       rc=to_string(file_size(file+".c"));
       s+="  Filesize: "+rc+ " byte\n";
       rc=file_size(file+".hlp");
       if (rc>0) s+="  Helpfile: available ("+to_string(rc)+" byte)\n";
       else s+="  Helpfile: not available\n";
       rc = call_other( ob, "QueryShort"); 
       if (rc)
       {
         rc2=explode(rc,"\n");
         for (j=0;j<sizeof(rc2);j++)
         {
           if (j==0) s+="  Info:     "+rc2[j]+"\n";
           else      s+="            "+rc2[j]+"\n";
         }
       }
       rc = call_other( ob, "QueryUsage"); 
       if (rc)
       {
         rc2=explode(rc,"\n");
         for (j=0;j<sizeof(rc2);j++)
         {
           if (j==0) s+="  Usage:    "+rc2[j]+"\n";
           else      s+="            "+rc2[j]+"\n";
         }
       }
       ob->remove();
       if (ob) destruct(ob);
      }
      else s+="No information available.\n";
      write(s);
    }

  }

  return 1;
}
/*
** The binherat command
*/
public int cmd_binheart( string str ) 
{
  int i;
  string s;
  string *tmp_heart;

  if (CheckHelp(str)) return 1;
  if ( (str) )
    return notify_fail( "Usage: binheart\n" ), 0;

  s="Heartbeat commands of the tool:\n";
  tmp_heart=filter(pHeartbeat,#'IsEnabled);
  for (i=0;i<sizeof(tmp_heart);i++)
  {
    s+=sprintf("  - %s\n",tmp_heart[i]);
  }
  L_STR->SMore(s);

  return 1;
}

/*
** The binpacke command to easy isntall deinstall packages
**
** Format of the packagefile
** [xtool]
** rootdir=/obj/tool/bin/xtool          # optional 
** options=-s                           # optional
** packagetitle=This is the xtool       # optional
** packageinfo=With this tool you get the basic x-commands # optional
** xload=xload.c
** xtest=test/xtest.c
** [<nextpackage>]
**
** Remark: to use packagetitle etc. makes sense (not block commands!)
** TODO maybe move install/remove/etc to binadd,binrem,...
*/
public int cmd_binpackage( string str ) 
{
string package;
string *pcks;
string *mods;
string s;
string tmp;
int i;
int idx;

  if (CheckHelp(str)) return 1;
  if ( (!str) )
    return notify_fail( "Usage: binpackage <mode> [package]\n" ), 0;

  if (str=="reset") // internal command only
  {
     UnloadIni(BINPACKAGES);
     write("Package reset done.\n");
  }
  else if (str=="list installed" || str=="li")
  {
    pcks=GetInstalledPackages();
    s="Installed packages:\n";
    for (i=0;i<sizeof(pcks);i++)
    {
      tmp=GetPackageKeyValue(pcks[i],"packagetitle");
      if (!tmp) s+="  - "+pcks[i]+"\n";
      else s+=sprintf("  - %-15.15s: %s\n",pcks[i],tmp);
    }
    L_STR->SMore(s);
  }
  else if (str=="list available" || str=="la")
  {
    pcks=GetAvailablePackages()-GetInstalledPackages();
    s="Available packages:\n";
    for (i=0;i<sizeof(pcks);i++)
    {
      tmp=GetPackageKeyValue(pcks[i],"packagetitle");
      if (!tmp) s+="  - "+pcks[i]+"\n";
      else s+=sprintf("  - %-15.15s: %s\n",pcks[i],tmp);
    }
    L_STR->SMore(s);
  }
  else if (1==sscanf(str,"info %s",package) )
  {
    pcks=GetInstalledPackages();
    idx=member(pcks,package);
    if (idx>=0)
    {
      s="Package '"+package+"' is installed:\n";
      mods=GetInstalledModules(package);
      for (i=0;i<sizeof(mods);i++)
      {
        s+="  - "+mods[i]+"\n";
      }
      tmp=GetPackageKeyValue(package,"packageinfo");
      if (tmp)
      {
        s+="Package information:\n  "+L_STR->wrap(tmp,75,2)+"\n";
      }
      L_STR->SMore(s);
    }
    else
    {
      pcks=GetAvailablePackages();
      idx=member(pcks,package);
      if (idx>=0)
      {
        s="Package '"+package+"' is available:\n";
        mods=GetAvailableModules(package);
        for (i=0;i<sizeof(mods);i++)
        {
          s+="  - "+mods[i]+"\n";
        }
        tmp=GetPackageKeyValue(package,"packageinfo");
        if (tmp)
        {
          s+="Package information:\n  "+L_STR->wrap(tmp,75,2)+"\n";
        }
        L_STR->SMore(s);
      }
      else
      {
        write("Package  '"+package+"' not existant.\n");
      }
    }
  }
  else if (1==sscanf(str,"install %s",package) )
  {
    if (IsInstalledPackage(package))
    {
      write("Package '"+package+"' is already installed.\n");
    }
    else if (!IsAvailablePackage(package))
    {
      write("Package '"+package+"' is not availble.\n");
    }
    else 
    {
      mods=GetAvailableModules(package);
      AddModules(package,mods);
      write("Installing package '"+package+"' done.\n");
    }
  }
  else if (1==sscanf(str,"update %s",package) )
  {
    if (!IsInstalledPackage(package))
    {
      write("Package '"+package+"' is not installed.\n");
    }
    else if (!IsAvailablePackage(package))
    {
      write("Package '"+package+"' is not availble.\n");
    }
    else 
    {
      // to be removed
      mods=GetInstalledModules(package)-GetAvailableModules(package);
      RemModules(package,mods);

      // not installed modules
      mods=GetAvailableModules(package)-GetInstalledModules(package);
      AddModules(package,mods);

      write("Updating package '"+package+"' done.\n");
    }
  }
  else if (1==sscanf(str,"uninstall %s",package) ||
           1==sscanf(str,"remove %s",package) )
  {
    if (!IsInstalledPackage(package))
    {
      write("Package '"+package+"' is not installed.\n");
    }
    else 
    {
      mods=GetInstalledModules(package);
      RemModules(package,mods);
      RemPackage(package);
      write("Removing package '"+package+"' done.\n");
    }
  }
  else if (1==sscanf(str,"disable %s",package) )
  {
    if (!IsInstalledPackage(package))
    {
      write("Package '"+package+"' is not installed.\n");
    }
    else 
    {
      mods=GetInstalledModules(package);

      for (i=0;i<sizeof(mods);i++)
      {
        cmd_bindisable(mods[i]);
      }
      write("Disabling package '"+package+"' done.\n");
    }
  }
  else if (1==sscanf(str,"enable %s",package) )
  {
    if (!IsInstalledPackage(package))
    {
      write("Package '"+package+"' is not installed.\n");
    }
    else 
    {
      mods=GetInstalledModules(package);

      for (i=0;i<sizeof(mods);i++)
      {
        cmd_binenable(mods[i]);
      }
      write("Enabling package '"+package+"' done.\n");
    }
  }
  else
  {
    return notify_fail( "Unkown mode for binpackage.\n" ), 0;
  }
  return 1;
}


/*
** The help checker
*/
public int CheckHelp(string str)
{
string verb;
  verb=query_verb();
  if (!str || !verb) return 0;
  if (-1!=member(({"?","help","/?","--help","-h","-?"}),str))
  {
    return cmd_binhelp(verb); 
  }
  return 0;
}
