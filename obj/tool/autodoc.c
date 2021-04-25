//  The autodoc tool
//  made by Magictcs

/** @synopsis
    xclone /obj/tool/autodoc
    Usage: doc <filename>

    (filename with or without extension)
*/

/** @description
    The autodoc tool is made for you to generate documentations of
    your files automatically. The autodoc tool can detect special
    comments if you follow the guildelines for the comments in your
    file. The output is written into the same directory as the source
    file but with the extension .doc (without any warning if there is
    already such a file !!). But this technic will prevent you from
    overwriting an old doc-file (they are mostly moved to the doc
    directories). You have to move the doc-file manually to the right
    doc-directory (after checking the doc-file of course).

    The special comments start with a @ followed (without any space)
    by one of the next strings: (can be placed everywhere in the file)

    @synopsis - all following text until end of comment is added to
                the synopsis

    @description - all following text until end of comment is added to
                   the description

    @author - the following word is added to the list of authors

    @version - the following string (until linefeed) is used as
               versionstring

    @todo - all following text until end of comment is used as TODO text

    @bugs - all following text until end of comment describes know bugs

    @example - all text until next linefeed describes example files
    (points to example files)

    @see - links to other files (but not to examples)


    For each public function use the following special comments:
    (only functions which are preceeded by a @function comment appears
    in the auto-documentation)

    @function - starts a function definition for the following function

    @return - declares the return value of the function

    @param - defines the parameters of the function
*/

/**
  @author Magictcs
  @author a second author :)
  @version Version 0.1
*/

/**
  @todo solve the problem of long comments
*/

/**
  @bugs small problems with resolve_file
  problems with C++ comments (the double slash ones) if used in strings,
  not as comment!
  know problems with long strings in any special comment - 8 spaces will
  be preceeded and the output string will become too long
*/

/**
  @example There is actually no example file available
  but you may try 'doc autodoc' to autodoc itself
*/

/**
  @see no links needed
*/

#include <properties.h>

inherit "/std/thing";
inherit "/lib/string";

//**********
// variables
//**********
private string *author=({});     // contains the author/coauthors
private string version="";       // contains the version string
private string *synopsis=({});   // contains the synopsis
private string *desc=({});       // contains the description
private string *funcs=({});      // contains the function descriptions and
                                 // parameters and return values
private string *todo=({});       // contains all TODO
private string *bugs=({});       // contains all known bugs
private string *exams=({});      // contains examples
private string *inherits=({});   // contains the inherited items
private string *incl=({});       // contains all include lines
private string *links=({});      // contains links to other files

private string *function=({});   // function definition
private string *ret_val=({});    // ret_val of the functions
private string *param=({});      // parameters of the function
private int func_count=0;        // counter for function defs

private string *lines=({});      // contains most of the file itself
private string file="";          // contains the file name
private string out="";           // output string to file or to screen

// checks if filename exists or if it is a directory... and so on
// looks a little bit complicated - but it is needed for the check
// of directories
private int check_filename() {
  int size;
  notify_fail("Use: doc <filename>\n");
  file = resolve_file(file,this_player()->Query(P_CURRENTDIR));
  size=file_size(file);
  // first a check without .c for directories
  if (size==-2) {
    write("File "+file+" is a directory!\n");
    return 0;
  }
  if (size==-1) {
    if (file[<2..]!=".c")
      file+=".c";
    file = resolve_file(file, this_player()->Query(P_CURRENTDIR) );
    size=file_size(file);
    if (size<0) {
      write("File: "+file+" does not exists.\n");
      return 0;
    }
    if (size<1) {
      write("There is no such file.\n");
      return 0;
    }
  }
  return 1;                    // file is okay :)
}

// remove C++ comments ("//.....")
private string remove_cpp_comments (string in) {
  int i;
  string *tmp,*tmp2;
  string out;
  tmp=explode(in,"\n");
  for (i=0;i<sizeof(tmp);i++) {
    tmp2=regexplode(tmp[i],"//");           // hmm not very good, because
                                            // "//" inside strings match too
    if (sizeof(tmp2)>1) tmp[i]=tmp2[0];     // remove the c++ comment
  }
  out=implode(tmp,"\n");
  return out;
}

// remove the special comments and any @-comments
private void search_include(string file) {
  int i;
  string *tmp;
  // search for the include lines
  tmp=regexplode(file,
    "#include [ ]*[<\"][_a-zA-Z][_a-zA-Z0-9]*[\.]h[>\"]");
  if (sizeof(tmp)>1) {
     for (i=1;i<sizeof(tmp);i+=2) {
       incl+=({tmp[i]});
     }
  }
}

// remove the special comments and any @-comments
private void search_inherit(string file) {
  int i;
  string *tmp;
  // search for inherit lines
  tmp=regexplode(file,
    "inherit [ ]*[\"][_a-zA-Z/][_a-zA-Z0-9/]*[\"]");
  if (sizeof(tmp)>1) {
    for (i=1;i<sizeof(tmp);i+=2) {
      inherits+=({ tmp[i] });
    }
  }
}

// search for /** @function ... @param .... @return ... */
/**
  @function search_function
  @param string comment - a string of a special comment
  @param int pos - position of comment in whole file
  @param string file - a string contains the whole file
  @return none
*/
private void search_function(string comment,int pos,string file) {
  int i,j;
  string *tmp,*tmp2,tmp3,*tmp4;
  // search for the include lines
  // first remove the */ at the end
  comment=comment[0..<3];
  tmp=regexplode(comment,
    "@[fF][uU][nN][cC][tT][iI][oO][nN][ ]*");

  if (sizeof(tmp)>1) {
    tmp2=explode(lines[pos+1],"\n");
    if (strlen(tmp2[0])<=1) tmp3=tmp2[1];
    else tmp3=tmp2[0];
    // find the function deklaration:
    tmp4=explode(tmp3,"{");           // possible "{"
    function+=({ tmp4[0] });          // functions definition

    // search for @param or @return, but ignore lower/uppercase
    tmp2=regexplode(tmp[2],
      "(@[pP][aA][rR][aA][mM][ ]*|@[rR][eE][tT][uU][rR][nN])[ ]*");
                                      // extract function completely
    // build the new string with formated output
    tmp4=regexplode(tmp2[0],"[\n]")-({""})-({"\n"});
    for (j=0;j<sizeof(tmp4);j++) tmp4[j]=strip_blanks(tmp4[j]);
    tmp3=implode(tmp4,"\n        ");
    function[func_count]+=
        "\n        "+tmp3;            // insert function definition
    param+=({""});                    // mark the place in array
    ret_val+=({""});                  // mark the place in array
    for (i=2;i<sizeof(tmp2);i+=2) {
      tmp4=regexplode(tmp2[i],"[\n]")-({""})-({"\n"});
      for (j=0;j<sizeof(tmp4);j++) tmp4[j]=strip_blanks(tmp4[j]);
      tmp3=implode(tmp4,"\n          ");
      if (tmp2[i-1][1]=='p' || tmp2[i-1][1]=='P') {  // @param found!
        param[func_count]+=tmp3;
      } else {                        // @return found
        ret_val[func_count]+=tmp3;
      }
    }
    func_count++;
  }
}

// check for @author and store the author in the author-array
/**
  @function just another demonstration how the function extraction will work
  the description of the comment can be longer than one line!
  @param string comment - the actual special comment string
and this one too?
  @return none
*/
private void search_author(string comment) {
  int i;
  string *tmp;
  tmp=regexplode(comment,"@[aA][uU][tT][hH][oO][rR][ ]*");
  for (i=2;i<sizeof(tmp);i+=2) {
    author+=({ explode(tmp[i],"\n")[0] });
  }
}

// check for @version and store the version in the version string
/** @function and a 3rd function for checking
with more than 1 line too
and a 3rd line for func description
  @param string comment - special doc comment
  @return none
*/
private void search_version(string comment) {
  string *tmp;
  tmp=regexplode(comment,"@[vV][eE][rR][sS][iI][oO][nN][ ]*");
  if(sizeof(tmp)>1) {
    version=explode(tmp[2],"\n")[0];
  }
}

// remove the special comments and any @-comments
private void search_synopsis(string comment,int pos) {
  int i;
  string *tmp,*tmp2;
  // search for @synopsis
  tmp=regexplode(comment[4..(strlen(comment)-3)],
    "@[sS][yY][nN][oO][pP][sS][iI][sS]");
  if (sizeof(tmp)>1) {
    // remove this comment and the one before out of the lines array
    lines[pos]="";
//    lines-=({lines[pos-1]});
    // concat the rest of the string with "" and split them on "\n"
    tmp2=explode(implode(tmp[2..],""),"\n");
    for (i=0;i<sizeof(tmp2);i++) {
      synopsis+=({ strip_blanks(tmp2[i]) });
    }
  }
}

// remove the special comments and any @-comments
private void search_description(string comment,int pos) {
  int i,j;
  string *tmp,*tmp2;
  // search for @synopsis
  tmp=regexplode(comment[4..(strlen(comment)-3)],
    "@[dD][eE][sS][cC][rR][iI][pP][tT][iI][oO][nN]");
  if (sizeof(tmp)>1) {
    // remove this comment and the one before out of the lines array
    lines[pos]="";
    // concat the rest of the string with "" and split them on "\n"
    tmp2=explode(implode(tmp[2..],""),"\n");
    for (i=0;i<sizeof(tmp2);i++) {
      desc+=({ strip_blanks(tmp2[i]) });
    }
  }
}

// remove the special comments and any @-comments
private void search_bugs(string comment) {
  int i;
  string *tmp,*tmp2;
  // search for @bugs
  tmp=regexplode(comment[4..(strlen(comment)-3)],
    "@[bB][uU][gG][sS]");
  if (sizeof(tmp)>1) {
    tmp2=explode(tmp[2],"\n");
    for (i=0;i<sizeof(tmp2);i++) {
      bugs+=({ strip_blanks(tmp2[i]) });
    }
  }
}

// remove the special comments and any @-comments
private void search_todo(string comment) {
  int i;
  string *tmp,*tmp2;
  // search for @todo
  tmp=regexplode(comment[4..(strlen(comment)-3)],
    "@[tT][oO][dD][oO]");
  if (sizeof(tmp)>1) {
    tmp2=explode(tmp[2],"\n");
    for (i=0;i<sizeof(tmp2);i++) {
      todo+=({ strip_blanks(tmp2[i]) });
    }
  }
}

// remove the special comments and any @-comments
private void search_exams(string comment) {
  int i;
  string *tmp,*tmp2;
  // search for @example
  tmp=regexplode(comment[4..(strlen(comment)-3)],
    "@[eE][xX][aA][mM][pP][lL][eE]");
  if (sizeof(tmp)>1) {
    tmp2=explode(tmp[2],"\n");
    for (i=0;i<sizeof(tmp2);i++) {
      exams+=({ strip_blanks(tmp2[i]) });
    }
  }
}

// remove the special comments and any @-comments
private void search_see(string comment) {
  int i;
  string *tmp,*tmp2;
  // search for @see
  tmp=regexplode(comment[4..(strlen(comment)-3)],
    "@[sS][eE][eE]");
  if (sizeof(tmp)>1) {
    tmp2=explode(tmp[2],"\n");
    for (i=0;i<sizeof(tmp2);i++) {
      links+=({ strip_blanks(tmp2[i]) });
    }
  }
}

// parse_file - checks the file
// store all text found into the different arrays
private int parse_file() {
  int i,size;
  string tmp,*tmp2,tmp3;

  // tmp=linefeed+komplettes file einschliesslich der internen linefeeds
  size=file_size(file);
  if (size>45000) {               // special read for big files
    tmp="";
    for (i=0;i<(size/45000);i++) {
      tmp3=read_bytes(file,45000*i,45000);
      if (tmp3!=0) tmp+=tmp3;
    }
  } else {                        // normal file read (easier)
    tmp=read_file(file,0);        // including the \n !!!
    if (!tmp) {
      write("Error reading file!\nOperation canceled.\n"); return 0;
    }
  }

  tmp=remove_cpp_comments(tmp); // remove the "//" comments

  // first search the inherits and includes in the whole file:
  search_include(tmp);
  search_inherit(tmp);

  // string split at "/**....*/"
  // regulaer expressions looks a little bit complicated :)
  // but it is simple to understand:
  // search for "/**" followed by "*/" but don't ignore any "*" in the pattern
  // and then remove all not matching lines (0,2,4,6,8,.....);
  lines=regexplode(tmp,"/[*][*]([^*]|[*][^/])*[*]/");

  // do not merge the loops!!!! the lines array will change meanwhile!
//write("==Testausgabe==:\n"+lines[i]+"\n==Ende==\n");
  for (i=1;i<sizeof(lines);i+=2) {
    search_description(lines[i],i);  // find description
  }

  for (i=1;i<sizeof(lines);i+=2) {
    search_synopsis(lines[i],i);     // find synopsis
  }

  for (i=1;i<sizeof(lines);i+=2) {
    search_author(lines[i]);         // find author
    search_version(lines[i]);        // find version
    search_bugs(lines[i]);           // find known bugs
    search_todo(lines[i]);           // find todo lines
    search_exams(lines[i]);          // find example
    search_see(lines[i]);            // find see
    search_function(lines[i],i,tmp); // find all functions and return,params
  }
  return 1;
}

// write the inheritance tree from the parsed file
// maybe sort the output in future a little bit
private void write_inheritance() {
  int i;
  string *tmp;
  // inheritance if exists
  if (sizeof(inherits)) {
    out+="INHERITANCE\n";
    // cut the start of the filename and the end of the filename (.c)
    tmp=regexplode(file,"/");
    out+="        "+tmp[sizeof(tmp)-1][0..<3]+"\n";
    // first sort the inherit strings:
    for (i=0;i<sizeof(inherits);i++) {
      tmp=regexplode(inherits[i],"inherit \"[/]*");
      inherits[i]="        |-"+tmp[sizeof(tmp)-1][0..<2]+"\n";
    }
    // uuh sorry for the lambda closure :)), but sorting is easy in this way
    inherits=sort_array(inherits,lambda(({'a, 'b}), ({#'>, 'a, 'b})));
    // then put them to the inheritance tree:
    for (i=0;i<sizeof(inherits);i++) {
      // cut the words inherit and some other things
      out+=inherits[i];
    }
    out+="\n";
  }
}

// write know bugs
private void write_bugs() {
  int i;
  if (sizeof(bugs)) {
    out+="KNOWN BUGS\n";
    for (i=0;i<sizeof(bugs);i++)  {
      out+="        "+bugs[i]+"\n";
    }
    out+="\n";
  }
}

// write todo comments
private void write_todo() {
  int i;
  if (sizeof(todo)) {
    out+="TODO\n";
    for (i=0;i<sizeof(todo);i++)  {
      out+="        "+todo[i]+"\n";
    }
    out+="\n";
  }
}

// write the description
private void write_description() {
  int i;
  // @description if exists
  if (sizeof(desc)) {
    out+="DESCRIPTION:";
    for (i=0;i<sizeof(desc);i++) {
      out+="        "+desc[i]+"\n";
    }
    out+="\n";
  }
}

// write the example lines
private void write_exams() {
  int i;
  if (sizeof(exams)) {
    out+="EXAMPLES\n";
    for (i=0;i<sizeof(exams);i++)  {
      out+="        "+exams[i]+"\n";
    }
    out+="\n";
  }
}

// write the synopsis
private void write_synopsis() {
  int i;
  // @synopsis if exists
  if (sizeof(synopsis)) {
    out+="SYNOPSIS:";
    for (i=0;i<sizeof(synopsis);i++)
      out+="        "+synopsis[i]+"\n";
    out+="\n";
  }
}

// write the author
private void write_author() {
  int i;
  if (sizeof(author)>0) {
    out+="AUTHOR:\n";
    for (i=0;i<sizeof(author);i++)
      out+="        "+author[i]+"\n";
    out+="\n";
  }
}

// write links
private void write_links() {
  int i;
  if (sizeof(links)) {
    out+="SEE ALSO\n";
    for (i=0;i<sizeof(links);i++)  {
      out+="        "+links[i]+"\n";
    }
    out+="\n";
  }
}

// write function descriptions
private void write_funcs() {
  int i;
  if (sizeof(function)) {
    out+="PUBLIC FUNCTIONS\n";
    for (i=0;i<sizeof(function);i++) {
      out+="\n\n        "+function[i];
      if (param[i]!="") {
        out+="\n        Parameter:\n";
        out+="          "+param[i];
      }
      if (ret_val[i]!="") {
        out+="\n        Return Value:\n";
        out+="          "+ret_val[i];
      }
    }
    out+="\n";
  }
}

// generate the output file
// incl and inherits are not used actually!
private int write_docu() {
  int i;
  string *tmp;
  // first the object:
  out="OBJECT:\n        "+file[0..(strlen(file)-3)]+"\n\n";
  write_author();                          // write the authors
  if (strlen(version)) {                   // print versionstring if exists
    out+="VERSION:\n        "+version+"\n\n";
  }
  // time of last update
  out+="LAST UPDATE:\n";
  out+="        "+ctime(get_dir(file,0x04)[0])+"\n\n";
  write_synopsis();                        // write the synopsis
  write_description();                     // write the description
  write_funcs();                           // write description of public
                                           // functions
  write_todo();                            // write todo comments
  write_bugs();                            // write known bugs
  write_inheritance();                     // write the inheritance tree
  write_exams();                           // write the examples
  write_links();                           // write links
// actually to screen - in future to file
// to write it to a file -just use write_file().....
//  smore(out,this_player()->QueryPageSize());
  file=file[0..<3]+".doc";
  if (file_size(file)>0) {
    write("Overwrite old file.\n");
    rm(file);
  }
  if (write_file(file,out)==1) {
    write("Documentation is written to: "+file+"\n");
  }
  else {
    write("There was an error writing file: "+file+"\n");
  }
  return 1;
}

/* cmd_doc runs over the file, and extracts the special comments used
   for the autodoc functions
   @param string str the filename of the file to be parsed
   @return Returns 0 if failure (no filename given or file not found)
           Returns 1 if parsing was correct and the doc-file was made
*/
int cmd_doc(string f) {
  if (!f) { notify_fail("Use: doc <filename>\n"); return 0; }
  file=f;
  if (!check_filename()) return 0;
  // reset all internal variables first
  author=({});
  version="";
  synopsis=({});
  lines=({});
  desc=({});
  funcs=({});
  todo=({});
  bugs=({});
  exams=({});
  inherits=({});
  incl=({});
  links=({});
  function=({});
  ret_val=({});
  param=({});
  func_count=0;
  out="";
  write("Writing doc for the file: "+file+"\n");
  if (!parse_file()) return 1;          // parse the file and extract all
  write_docu();                         // write the documentaion file
  return 1;
}

//*******************************************
// private function - for internal usage only
//*******************************************
private string printhelp() {
  return
  "Usage: doc <name> - to create a file name.doc\n\n"
  "The document file will follow the guidelines for the document style.\n"
  "The tool is able to find special comments in your file.\n";
}

//*****************************************************
// create() initialize short,long and some other things
//*****************************************************
create() {
  ::create();
  Set(P_AUTOOBJECT,1);
  SetShort("the autodoc (tool)");
  SetLong(
    "A special tool to create document files very fast.\n"
    "To use the autodoc tool - see 'help autodoc'\n");
  SetHelpMsg(#'printhelp);
  SetIds(({"autodoc"}));
  SetNoDrop(1);
  SetNoSell(1);
}

//*****************************************************
// init() adds an action for the documentation function
//*****************************************************
init() {
  ::init();
  add_action("cmd_doc","doc");
}

