/*--------------------------------------------------------------------------
 * /obj/lib/more.c -- multiuser/message aware more
 *
 * Copyright (C) 1997 by One Step Beyond.
 * All Rights Reserved.
 * Permission granted for non-commercial use.
 *
 * [Softbyte] 14jan98 New version (strings,files,HTML)
 * [Softbyte] 17jan98 Smore can handle end functions
 * [Softbyte] 21jan98 Renamed get_data/set_data for compatibility
 * [Softbyte] 06feb98 More waits on the last page if flag=16 or more fun given
 *
 *--------------------------------------------------------------------------

/**
  @author Softbyte
  @version V1.0
  @see messages(C), string(L), pagesize(prop)
*/
/** @synopsis
       include <daemons.h>
       MORE->...

            or

       inherit "/lib/more";
	   ( it is possible to inherit the file, but not recommended )

*/
/** @description
    Multiuser aware more that uses the message-system to more a
    file or a text. The more command can ignore HTML comments and
    show text parts.

    Interactive commands are:
    [CR], d, +  : next page
    u   , b, -  : last page
    f           : go to the first page
    l           : go to the last page
    <nr>        : go to page <nr>
    /<regexp>   : scan forward until line matching <regexp>
    /           : repeats last /<regexp> command
    a           : redisplay current page
    c<COLOR>    : mark regexp's with this color
    full        : Shows the full document if only a part is shown
    partial     : Shows only a specific part of a document
    ?           : this help
    q           : quits more
*/

/** @example
	More /std/player.c without notifying the surroundings
	  "/lib/more"->more("/std/player.c", 1 );

	More /std/player.c with notifying the surroundings if the player is
    not invisbile
	  "/lib/more"->More("/std/player.c");

    More a string
	  "/lib/more"->more("Hello this is a test...\n",2);
	  "/lib/more"->SMore("Hello this is a test...\n");

    More a file not ignoring HTML comments but notifying the environment
	  "/lib/more"->more("/std/player.c", 4 );


*/
/** @todo Replace BMore ,...
*/
/** @bugs Colour is not saved in the player
*/

#include <secure/wizlevels.h>
#include <properties.h>
#include <msgclass.h>
#include <mxp.h>

#define MORE_MODE       0
#define MORE_REGEXP     1
#define MORE_FILE       2
#define MORE_COLOR      3
#define MORE_CHUNK      4
#define MORE_FILESIZE   5
#define MORE_RLINES     6
#define MORE_BYTES      7
#define MORE_READPAGES  8
#define MORE_PAGE       9
#define MORE_ITRANS    10
#define MORE_STRINGS   11
#define MORE_PARTIAL   12
#define MORE_FUNCTION  13
#define MORE_OBJECT    14

#define MORE_MAX_PAGES_READ 30

// mode flags
#define MORE_QUIET         1 // do not write 'looks at a complete different reality'
#define MORE_STRING        2 // filename is a string
#define MORE_NOHTML        4 // HTML comments are suppressed
#define MORE_NOHEADER      8 // No Fileheader is shown
#define MORE_PROMPTLAST   16 // Prompt at the last page is shown

// mostly internal use
#define MORE_NOPARTIAL  1024 // ignore given partial segement and show full

mapping data;

public void even_more( string str, int showeof );

/*---------------------------------------------------------------------------
 * Some helpfuns to handle the data mapping
 *---------------------------------------------------------------------------
 */

/* just for debuggin purpose */
public mapping query_data() { return data; }

private mixed GetData( mixed type )
{
  if ( ! data ) data=([]);

  return data[this_player()->Query(P_REALNAME)][type];
}

private void SetData( mixed type, mixed v )
{
  mapping m;
  if ( ! data ) data=([]);

  m=data[this_player()->Query(P_REALNAME)];
  if (!m) m=([]);
  if (member(m,type)) m[type]=v;
  else m+=([type:v]);
  data[this_player()->Query(P_REALNAME)] = m;
}

private void remove_data()
{
  this_player()->Set(P_EDITING,0);
  if (! (GetData( MORE_MODE ) & MORE_QUIET) )
  {
    msg_say( CMSG_ROOM,
      add_gen(({string})this_player()->Query(P_NAME))+
      " view focuses again on this reality.\n" );
  }
  if ( ! data ) return;
  m_delete( data, ({string})this_player()->Query(P_REALNAME) );
}

// calls another object at the end of more
private void remote_call(string fun, object ob)
{
  if (fun && ob && objectp(ob) && stringp(fun))
    call_other(ob,fun);
}

// reads in the 'file' or the string array stored in MORESTRINGS
// from the 1 based line 'start' for 'number' lines
// if number==0 all remaining lines are read
string get_text(string file, int start, int number)
{
  int mode;
  string *strs;
  string tmp;
  int *partial;

  mode=GetData(MORE_MODE);
  partial=GetData(MORE_PARTIAL);
  if (mode & MORE_NOPARTIAL) partial=0;

  if (partial)
  {
    start+=partial[0]-1; // partial is 1 based
    if (start>=partial[1]) return 0; // EOF
    if (start+number>partial[1]) number=partial[1]-start;
  }

  if (mode & MORE_STRING)
  {
    start--;  // 0 based array indices
    strs=GetData(MORE_STRINGS);
    // sizeof is 2 bigger than possible indices (because of the final "")
    if (start<0 || start >sizeof(strs)-2) return 0; // EOF
    if (start+number>sizeof(strs)-1) number=sizeof(strs)-1-start;
    number--; // 3 lines is 0..(3-1)
    if (number<0) number=sizeof(strs)-2; // read til EOF
    tmp=implode(strs[start..start+number],"\n")+"\n";
  }
  else
  {
    tmp=read_file(file,start,number);
  }
  return tmp;
}

int flag_filter(string s,int mode)
{
  if (mode & MORE_NOHTML)  // filter out HTML comments
  {
    if (s[0..2]=="<!-") return 0;
    // if (s[0..5]=="<html>") return 0;
  }
  return 1;
}

/*--------------------------------------------------------------------------
 * Reads in a a given page which must be the NEXT not yet read page and
 * adjusts the internal data arrays
 *--------------------------------------------------------------------------
 */
private int ScanPage(int page)
{
  int *rlines;
  int *mbytes;
  int nlines,chunk,i,mode,blines;
  string tmp;
  string *strs;

  page--;   // 0 based count
  rlines=GetData(MORE_RLINES)||({});
  mbytes=GetData(MORE_BYTES)||({});
  chunk=GetData(MORE_CHUNK);
  mode=GetData(MORE_MODE);

  if (sizeof(rlines)!=page+1 || sizeof(mbytes)!=page)
  {
    msg_write( CMSG_ERROR,
	       "Internal error: Scanning page "+(page+1)+" failed.\n" );
    return 0; // error
  }
  tmp=get_text(GetData(MORE_FILE),rlines[page],chunk);
  if (tmp=="") tmp=0;

  if (!tmp)
  {
    rlines+=({-1});
    return 0;
  }

  strs=explode(tmp,"\n");
  nlines=sizeof(strs)-1;
  // nlines=sizeof(strs); // SB 230198
  blines=nlines;
  i=0;
  if (mode & MORE_NOHTML)  // filter out HTML comments
  {
    strs=filter(strs,#'flag_filter /*'*/,mode);
    nlines=sizeof(strs)-1;
    while(nlines<chunk && tmp!=0)
    {
      tmp=get_text(GetData(MORE_FILE),rlines[page]+chunk+i,1);
      if (tmp && flag_filter(tmp,mode))
      {
        strs+=({tmp});
        nlines++;
        i++;
      }
      else if (tmp) i++;
    }
    if (nlines<blines && tmp==0) nlines=blines; // TODO: EOF ok???
    tmp=implode(strs,"\n");

  }

  rlines+=({rlines[page]+nlines+i});
  if (page>0)
    mbytes+=({mbytes[page-1]+sizeof(tmp)});
  else
    mbytes+=({sizeof(tmp)});
  SetData(MORE_RLINES,rlines);
  SetData(MORE_BYTES,mbytes);

  SetData(MORE_READPAGES,page+1);

  return 1;
}

/*--------------------------------------------------------------------------
 * Returns the string content for a certin page as well as the number of
 * lines. Negative lines mean EOF
 *--------------------------------------------------------------------------
 */
private string GetPage (int page,int lines)
{
  int *rlines;
  string *strs;
  int act_page,mode;
  string tmp;

  mode=GetData(MORE_MODE);
  for (act_page=GetData(MORE_READPAGES)+1;act_page<=page;act_page++)
  {
    if (!ScanPage(act_page))
    {
      act_page--;
      break;
    }
  }

  if (act_page<page)  // page not available in file
  {
    lines=0;
    return "";
  }

  rlines=GetData(MORE_RLINES)||({});
  // The ScanPage did fill all rlines up to page+1 or up to page (EOF)
  if (rlines[page]<0)  // next page -1 => EOF
  {
    tmp=get_text(GetData(MORE_FILE), rlines[page-1],0);   // read til EOF
    strs=explode(tmp,"\n");
    if (mode & MORE_NOHTML)  // filter out HTML comments
    {
      strs=filter(strs,#'flag_filter /*'*/,mode);
      tmp=implode(strs,"\n");
    }
    lines=sizeof(strs)-1;
    lines=-lines;   // indicate EOF
  }
  else
  {
    lines=rlines[page]-rlines[page-1];
    tmp=get_text(GetData(MORE_FILE), rlines[page-1],lines);
    strs=explode(tmp,"\n");
    if (mode & MORE_NOHTML)  // filter out HTML comments
    {
      strs=filter(strs,#'flag_filter /*'*/,mode);
      tmp=implode(strs,"\n");
    }
    lines=sizeof(strs)-1;
    // lines=sizeof(strs); // SB 230198
    if (lines<GetData(MORE_CHUNK)) lines=-lines;  // last page (EOF)
  }
  return tmp;
}

/*--------------------------------------------------------------------------
 * write the header
 *--------------------------------------------------------------------------
 */
private string get_header()
{
  int mode;
  string tmp;

  mode=GetData(MORE_MODE);
  if (mode & MORE_NOHEADER) return "";

  if (IS_IMMORTAL( this_player() ) )
  {
    if (mode & MORE_STRING)
      tmp="%^"+GetData(MORE_COLOR)+"%^--- Text ("+GetData(MORE_FILESIZE)+
          " Bytes) ---%^RESET%^\n";
    else
      tmp="%^"+GetData(MORE_COLOR)+"%^--- "+GetData(MORE_FILE)+
         " ("+GetData(MORE_FILESIZE)+" Bytes) ---%^RESET%^\n";
  }
  else
  tmp="";
  return tmp;
}

/*--------------------------------------------------------------------------
 * write the more prompt
 *--------------------------------------------------------------------------
 */

public void more_prompt (int nlines)
{
  int percent,page,eline,sline,mode;
  int *rlines;
  int *mbytes;
  int *partial;


  rlines=GetData(MORE_RLINES)||({});
  mbytes=GetData(MORE_BYTES)||({});
  page=GetData(MORE_PAGE);
  mode=GetData(MORE_MODE);

  percent=100*mbytes[page-1]/GetData(MORE_FILESIZE);

  if (sizeof(rlines)>page && rlines[page]>=0) eline=rlines[page]-1;
  else eline=rlines[page-1];

  sline=rlines[page-1];

  partial=GetData(MORE_PARTIAL);
  if (mode & MORE_NOPARTIAL) partial=0;
  if (partial)
  {
    sline+=partial[0]-1;
    eline+=partial[0]-1;
  }

  msg_write( CMSG_GENERIC|MMSG_DIRECT|MMSG_NOWRAP,
	     "=== More:"+
         (partial?"":" ("+percent+"%)")+
         " Page "+page+ ","+
         " "+sline+".."+eline+
         (partial?" ["+partial[0]+".."+(partial[1]-1)+"]":"")+
       " [CR,u,f,l,q,/<regexp>,<page>,?] " );
  this_player()->print_eor();
}

/*--------------------------------------------------------------------------
 * start the more for this_player()
 *  fname : filename to more
 *  mode  : MORE_QUIET,MORE_STRING,MORE_NOHTML,...
 *  chunk : chunks to more the file, if not given, P_PAGESIZE is assumed
 *--------------------------------------------------------------------------
 */

/**
  @function
    Mores a text or file to this_player(). The only obligatory argument is
    fname.
  @param string|string * str
    The filename or the text which should be shown. It depends on the
    mode parameter how this argument is interpreted. If str is an array
    of strings the mode paramter is automatically set to MORE_STRING==2
  @param int mode
  Mode flags are:
     0: default
     1 : do not write 'looks at a complete different reality'
     2 : filename is a string
     4 : HTML comments are suppressed
     8 : No file heade is shown
    16 : The more prompt after the last page is shown
   @param int chunk
    If 'chunk' is given, it is used instead of P_PAGESIZE.
    Default is the pagesize of the player
   @param int itrans
    1: Should the wrapped text be indented
    0: no (default)
   @param int *partial
    If an integer array of size 2, i.e. ({from,to}) is given only
    the lines in this range are shown. Switcbhing to full mode is
    possible via an interactive command.
   @param string fun
     After the text was mored the function fun in object ob is called
     by call_other(ob, fun)
   @param object ob
     The object in which the above function will be called. Default is
     this_object()
*/
varargs int more ( mixed fname, int mode, int chunk, int itrans,
                  int *partial, string fun, object ob)
{
  int lines;
  int fsize;
  string *strs;

  string tmp;

  /* Softbyte 04oct98 ...
  if ( this_player()->Query(P_EDITING) ) {
    msg_write( CMSG_GENERIC|MMSG_DIRECT,
	       "Error: more cannot be used while editing.\n" );
    return 0;
  }
  */

  if (!stringp(fname) && !pointerp(fname)) {
    msg_write( CMSG_GENERIC,
	       "Error: Wrong argument 1 for more given.\n" );
    return 0;
  }

  if ( ! fname ) {
    msg_write( CMSG_GENERIC,
	       "Error: no filename for more given.\n" );
    return 0;
  }

  if (!chunk)
    chunk = ({int})this_player()->Query( P_PAGESIZE );

  if ( itrans )
    itrans = MMSG_IDENT_TRANS;

  if (!ob) ob=this_object();

  if (partial && sizeof(partial)!=2) partial=0;

  if (pointerp(fname))
  {
    strs=fname;
    mode|=MORE_STRING;
    fname=implode(fname,"\n");
  }
  else
  {
    strs=explode(fname,"\n");
  }
  if (sizeof(strs) && strs[<1]!="") strs+=({""}); // Add a final \n
  SetData(MORE_STRINGS,strs);

  // show a string not a file
  if (mode & MORE_STRING)
  {
    fsize=sizeof(fname);
  }
  else
  {
    fname = ({string})this_player()->get_path( fname, 0, this_player() );
    fsize=file_size(fname);
    if ( fsize < 0 )
    {
      msg_write( CMSG_ERROR, "Error: \""+fname+"\" isn't a file.\n" );
      return 0;
    }
  }
  if ( fsize == 0 )
  {
      msg_write( CMSG_ERROR, "Nothing to show.\n" );
      return 0;
  }

  SetData(MORE_MODE,mode);
  SetData(MORE_REGEXP,0);
  SetData(MORE_FILE,fname);
  SetData(MORE_CHUNK,chunk);
  SetData(MORE_FILESIZE,fsize);
  SetData(MORE_RLINES,({1}));
  SetData(MORE_BYTES,({}));
  SetData(MORE_READPAGES,0);
  SetData(MORE_PAGE,1);
  SetData(MORE_ITRANS,itrans);
  SetData(MORE_COLOR,"RED");
  SetData(MORE_PARTIAL,partial);
  SetData(MORE_FUNCTION,fun);
  SetData(MORE_OBJECT,ob);

  if ( ! (mode & MORE_QUIET) ) {
    msg_say( CMSG_ROOM,
      ({string})this_player()->Query(P_NAME)+
	     " looks at a completely different reality.\n" );
  }

  tmp=GetPage(1,&lines);
  tmp=get_header()+tmp;

  if ( tmp && tmp[<1] != '\n' ) tmp += "\n";
  msg_write( CMSG_GENERIC|MMSG_NOWRAP|MMSG_DIRECT|itrans, tmp);

  // if we got the flag to show the last eof do not exit right now
  if ( (!(mode & MORE_PROMPTLAST)) &&  (lines<=0) )
  {
      msg_write( CMSG_GENERIC|MMSG_DIRECT, "=== EOF.\n" );
      remove_data();
      remote_call(fun,ob);
      return 1;
  }

  this_player()->Set(P_EDITING,1);

  more_prompt(lines);
  input_to("even_more", 0, 0 );
  return 1;
}

/**
  @function
    Mores a file to this_player(). The only obligatory argument is
    fname. This is an easy to use version of more() for files.
  @param string fname
    The filename which should be shown.
   @param int chunk
    If 'chunk' is given, it is used instead of P_PAGESIZE.
    Default is the pagesize of the player
*/
varargs int More (mixed fname, int chunk)
{
  int flag;

  if (({int})this_player()->Query(P_INVIS)) flag=1;
  else flag=0;

  return more(fname,flag,chunk,0,0,0,0);
}

/**
  @function
    Mores a text to this_player(). The only obligatory argument is
    str. This is an easy to use version of more() for strings.
  @param string|string * str
    The text or text array which should be shown.
   @param int chunk
   If 'chunk' is given, it is used instead of P_PAGESIZE.
    Default is the pagesize of the player
   @param string fun
     After the text was mored the function fun in object ob is called
     by call_other(ob, fun). Setting a function will force the flag
     MORE_PROMPTLAST=16 to be set, i.e. wait after the last page for
     the more prompt
   @param object ob
     The object in which the above function will be called. Default is
     this_object()
   @param int end
     Unused
*/
varargs int smore (mixed text, int chunk, object ob, string fun, int end)
{
  int flag;

  if (({int})this_player()->Query(P_INVIS)) flag=1;
  else flag=0;
  flag|=MORE_STRING;
  flag|=MORE_NOHEADER;
  // if a more function is given we prompt the last page per default
  // if (fun) flag|=MORE_PROMPTLAST;
  // Mateese: No. Such assumptions usually make an ass out of u and me,
  //          and in the case of the ebook actually do.

  return more(text, flag,chunk,0,0,fun,ob);
}

/**
  @function
    Mores a text to this_player(). This is an very easy to use version of
    smore() for strings.
  @param string|string * str
    The text or text array which should be shown.
*/
int SMore (mixed text)
{
  return smore(text);
}



/*--------------------------------------------------------------------------
 * output the help text
 *--------------------------------------------------------------------------
 */
private void more_help ()
{
  msg_write( CMSG_GENERIC|MMSG_DIRECT,
    "Accepted commands are:\n"
    + "  [CR], d, +  : next page\n"
    + "  u   , b, -  : last page\n"
    + "  f           : go to the first page\n"
    + "  l           : go to the last page\n"
    + "  <nr>        : go to page <nr>\n"
    + "  /<regexp>   : scan forward until line matching <regexp>\n"
    + "  /           : repeats last /<regexp> command\n"
    + "  a           : redisplay current page\n"
    + "  c<COLOR>    : mark regexp's with this color\n"
    + "  full        : Shows the full document if only a part is shown\n"
    + "  partial     : Shows only a specific part of a document\n"
    + "  ?           : this help\n"
    + "  q           : quits more.\n"
    + "Each page is by "+GetData(MORE_CHUNK)+" lines long.\n"
    );
}

/*--------------------------------------------------------------------------
 * skip to the end of file
 *  we need call_out here to prevent too long evals for large files
 *--------------------------------------------------------------------------
 */

void skip_to_end_of_more ( )
{
  int page,step;
  int lines;

  page=GetData(MORE_READPAGES);

  for (step=0;step<MORE_MAX_PAGES_READ;step++)
  {
    GetPage(page+step+1,&lines);
    if (lines==0)
    {
      SetData(MORE_PAGE,page+step);
      even_more("a",1);  // redisplay last page..do not break on EOF
      return;
    }
    else if (lines<0) // EOF
    {
      SetData(MORE_PAGE,page+step+1);
      even_more("a",1);  // redisplay last page..do not break on EOF
      return;
    }
  }
  call_out("skip_to_end_of_more", 0 );
}

/*--------------------------------------------------------------------------
 * shows a page containing the regexp which will be marked RED
 *
 *--------------------------------------------------------------------------
 */
void show_regexp(string tmp,int page)
{
  int itrans;

  tmp=regreplace(tmp,GetData(MORE_REGEXP),
        "%^"+GetData(MORE_COLOR)+"%^\\0%^RESET%^",1);
  msg_write( CMSG_GENERIC|MMSG_DIRECT,
      "*** Pattern found on page "+page+".\n" );
  itrans=GetData(MORE_ITRANS);
  if ( tmp && tmp[<1] != '\n' ) tmp += "\n";
  msg_write( CMSG_GENERIC|MMSG_NOWRAP|MMSG_DIRECT|itrans, tmp );
  SetData(MORE_PAGE,page);
}

/*--------------------------------------------------------------------------
 * searches a regexp from page onwards
 *  we need call_out here to prevent too long evals for large files
 *--------------------------------------------------------------------------
 */
void search_regexp(int page)
{
  int step,lines;
  string *result;
  string tmp;

  if (!page) page=1;


  for (step=0;step<MORE_MAX_PAGES_READ;step++)
  {
    tmp=GetPage(page+step,&lines);
    if (lines==0)
    {
       msg_write( CMSG_GENERIC|MMSG_DIRECT,
         "*** Reached EOF ... Pattern not found!\n" );
      SetData(MORE_PAGE,page+step-1);
      even_more("XXX",1);  // goto last page..do not break on EOF
      return;
    }
    else if (lines<0) // EOF
    {
      result=regexp(({tmp}), GetData(MORE_REGEXP));
      if ( sizeof(result) )
      {
        show_regexp(tmp,page+step);
      }
      else
      {
        msg_write( CMSG_GENERIC|MMSG_DIRECT,
          "*** Reached EOF ... Pattern not found.\n" );
        SetData(MORE_PAGE,page+step);
      }
      even_more("XXX",1);  // redisplay last page..do not break on EOF
      return;
    }
    else
    {
      result=regexp(({tmp}), GetData(MORE_REGEXP));
      if ( sizeof(result) )
      {
        show_regexp(tmp,page+step);
        even_more("XXX",1);  // redisplay last page..do not break on EOF
        return;
      }
    }
  }
  call_out("search_regexp", 0, page+step );
}

/*--------------------------------------------------------------------------
 * output the next chunk of lines
 *--------------------------------------------------------------------------
 */

public void even_more ( string str, int showeof )
{
  int page,show;
  int lines;
  string tmp,fun;
  int itrans,npage,mode;
  object ob;


  show=0;
  page=GetData(MORE_PAGE);
  itrans=GetData(MORE_ITRANS);

  switch( str ) {
  case "XXX":  // do nothing
  break;
  case "a":
    show = 1; /* do nothing but redisplay */
    break;
  case "d":
  case "+":
    showeof=1;
    page++;
    show = 1;
    break;
  case "":
    showeof=0;
    page++;
    show = 1;
    break;
  case "q":
    msg_write( CMSG_GENERIC|MMSG_DIRECT, "== Ok.\n" );
    fun=GetData(MORE_FUNCTION);
    ob=GetData(MORE_OBJECT);
    remove_data();
    remote_call(fun,ob);
    return;
  case "u":
  case "b":
  case "-":
    if (page>1) page--;
    show = 1;
    break;
  case "f":
    page=1;
    show = 1;
    break;
  case "l":
    msg_write( CMSG_GENERIC|MMSG_DIRECT, "...wait...\n");
    skip_to_end_of_more( );
    return;
  case "?":
    more_help();
    break;
  case "full":
      mode=GetData(MORE_MODE);
      if (!GetData(MORE_PARTIAL))
      {
        msg_write( CMSG_GENERIC|MMSG_DIRECT,
          "*** The mode of this document cannot be changed.\n");
      }
      else if (mode & MORE_NOPARTIAL)
      {
        msg_write( CMSG_GENERIC|MMSG_DIRECT,
          "*** Full mode already enabled.\n");
      }
      else
      {
        mode|=MORE_NOPARTIAL;
        page=1;
        show=1;
        SetData(MORE_BYTES,({}));
        SetData(MORE_READPAGES,0);
        SetData(MORE_RLINES,({1}));
        SetData(MORE_MODE,mode);
      }
    break;
  case "partial":
      mode=GetData(MORE_MODE);
      if (!GetData(MORE_PARTIAL))
      {
        msg_write( CMSG_GENERIC|MMSG_DIRECT,
          "*** This document can only be viewed in full mode.\n");
      }
      else if (!(mode & MORE_NOPARTIAL))
      {
        msg_write( CMSG_GENERIC|MMSG_DIRECT,
          "*** Partial mode already enabled.\n");
      }
      else
      {
        mode&=~MORE_NOPARTIAL;
        page=1;
        show=1;
        SetData(MORE_BYTES,({}));
        SetData(MORE_READPAGES,0);
        SetData(MORE_RLINES,({1}));
        SetData(MORE_MODE,mode);
      }
    break;
  default:
    if (str[0] == '/') // regexp
    {
      npage=1;  // start from the next page on
      if (sizeof(str) < 2 && sizeof( GetData(MORE_REGEXP) || "") < 1)
      {
        msg_write( CMSG_GENERIC|MMSG_DIRECT,
          "*** No previous regular expression.\n" );
      }
      else
      {
        if (sizeof(str) > 1) // new regexp
        {
          SetData( MORE_REGEXP, str[1..] );
          npage=0;  // start from this page on
        }
        if (!regexp(({"dummy"}), GetData(MORE_REGEXP) ))
        {
          msg_write( CMSG_GENERIC|MMSG_DIRECT,
            "*** Bad regular expression.\n");
          SetData( MORE_REGEXP, 0 );
        }
        else
        {
          msg_write( CMSG_GENERIC|MMSG_DIRECT, "...searching...\n" );
          search_regexp(page+npage);
          return ;
        }// end searching
      }// end ok regexp
    } // end regexp
    else if (str[0]=='c' && sizeof(str)>1 )  // color change
    {
      show=0;
      str=str[1..];
      if (-1==member(({"BLACK","RED","GREEN","ORANGE","BLUE","MAGENTA",
                       "CYAN","WHITE"}),str))
     {
        msg_write( CMSG_GENERIC|MMSG_DIRECT,
          "*** Try (BLACK, RED, GREEN, ORANGE, BLUE, MAGENTA, CYAN, WHITE).\n");
     }
     else
     {
       SetData(MORE_COLOR,str);
        msg_write( CMSG_GENERIC|MMSG_DIRECT,
          "*** Colour set.\n");
     }

    }
    // ************* goto page ***************
    else if ( sscanf(str, "%d", npage) == 1 )
    {
      page=npage;
      showeof=1;
      show = 1;
    }
    break;
  }

  if (page<1) page=1;
  SetData(MORE_PAGE,page);

  tmp=GetPage(page,&lines);
  if (show && lines!=0 )
  {
    if ( page==1 ) tmp=get_header()+tmp;
    if ( tmp && tmp[<1] != '\n' ) tmp += "\n";
    msg_write( CMSG_GENERIC|MMSG_NOWRAP|MMSG_DIRECT|itrans, tmp );
  }

  // if we got the flag to show the last eof prompt we force showeof=1
  mode=GetData(MORE_MODE);
  if (mode & MORE_PROMPTLAST) showeof=1;

  if (lines==0)
  {
      msg_write( CMSG_GENERIC|MMSG_DIRECT, "=== EOF!\n" );
      fun=GetData(MORE_FUNCTION);
      ob=GetData(MORE_OBJECT);
      remove_data();
      remote_call(fun,ob);
      return ;
  }
  else if (lines<0 && !showeof)
  {
      msg_write( CMSG_GENERIC|MMSG_DIRECT, "=== EOF\n" );
      fun=GetData(MORE_FUNCTION);
      ob=GetData(MORE_OBJECT);
      remove_data();
      remote_call(fun,ob);
      return ;
  }

  more_prompt(lines);
  input_to("even_more", 0, showeof );

  return;
}
