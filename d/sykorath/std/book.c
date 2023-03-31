/*
 * A special book
 * introduced by Magictcs - Mar 98
 *
 */

#include <properties.h>
#include <lock.h>
#include <search.h>
#include <moving.h>

// some defines
#define TP this_player()
#define TO this_object()
#define NAME ({string})this_player()->QueryName()

// private variables
nosave int page;                      // actual page
nosave int maxpage;                   // number of pages of the book
nosave int last_read;                 // last read page
nosave string bookfile;               // ASCII-file that contains the book
nosave mixed content;                 // for usage in SetPageContent

inherit "/obj/chest";                 // useful for open/close

//******************
// Property handling
//******************
int QueryPage() { return page; }
int SetPage(int p) { return page=p; }
int QueryMaxPage() { return maxpage; }
int SetMaxPage(int m) { return maxpage=m; }
string QueryBookFile() { return bookfile; }
string SetBookFile(string f) { return bookfile=f; }

//******************************************************
// For setting the page of one page scroll/book directly
//******************************************************
public mixed SetPageContent(mixed c) {
  if (stringp(c)) content=c;
  if (closurep(c)) content=c;
  return c;
}

//*********************************************
// This function can be overloaded!
// for books or scrolls that have only one page
//*********************************************
string QueryPageContent() {
  string ret;
  string text;
  string *tmp;
  ret="";

  // SetPageContent is used?
  // then we return that content, instead of the file-content!
  if (content) return funcall(content);
                      // evaluate closure or return the string directly

  // read next part from file
  // file:
  // #number
  // text for page
  // #number
  // text for page
  // and so on....
  if (file_size(bookfile)<=0) {
    write(capitalize(QueryShort())+
      "has no content - please contact a wizard.\n");
    return 0;
  }
  text=read_file(bookfile);           // read the whole file

  if (last_read!=-1) {                // auto turn to the next page
    if (page<maxpage) {
      page++;
      ret+="You turn the "+QueryShort()+" to the next page.\n";
    } else {
      last_read=-1;
    }
  } else {
    last_read=page;
  }

  ret+="You read:\n";

  // text contains now the full file! - can make problems?!
  // we use regulaer expressions to find the needed page!

  // first we add an end marker
  text+="#";

  // now we search from "#<spaces>number  text #"
  tmp=regexplode(text,"#([ ]|page)*"+page+"[ ]*\n");  // find starting mark
  if (sizeof(tmp)!=3) {                // not found
    ret+="This page is missing!\n";
    return ret;
  }

  // tmp[2] contains the text after the starting mark up to the end
  // now we split at the '#' mark
  tmp=regexplode(tmp[2],"#");

  // build the return string
  ret+=tmp[0];
  ret+="------------------------------------------------------- "+
       page+" ---\n";
  return ret;                           // actually we don't use 'more'
  // TODO: enhance it for more usage
}

//************************
// Read a page of the book
//************************
// this function cannot be overloaded !!!
// necessary for the open/close handling
// if you want to overload the Content of the book - use overloading of
// QueryPageContent()
nomask string QueryReadMsg() {
  if (QueryLockState() != LOCK_OPEN) {
    return "You have to open "+QueryShort()+" first!\n";
  }
  return QueryPageContent();
}

//*****************************
// forbid usage of SetReagMsg()
//*****************************
nomask string SetReadMsg(string str) {
  // raise an (compile) error if this function is used in any file!!
  MASTER->log_error(
          object_name(),
          "Illegal usage of SetReadMsg() in object "+
          object_name(this_object())+"\n"+
          "Object will not work as expected!\n");
  return 0;
}

//************
// turn a page
//************
// turn to next page [in book]
// turn to last page [in book]
// turn to first page [in book]
// turn to page <number> [in book]
int cmd_turn(string str) {
  string *exp;
  int newpage;
  object book;

  // only one page?!
  if (QueryMaxPage()<=1) return 0;

  notify_fail("Use: turn to next page in "+strip_article(QueryShort())+"\n"
              "     turn to first page in "+strip_article(QueryShort())+"\n"
              "     turn to last page in "+strip_article(QueryShort())+"\n"
              "     turn to page <number> in "+
                    strip_article(QueryShort())+"\n");

  if (!str) return 0;
  if (QueryLockState() != LOCK_OPEN) {
    write("You have to open "+QueryShort()+" first!\n");
    return 1;
  }
  // next page or number?
  // str contains:
  // to (next page|first page|last page|page <number>) page[ in <id for book>]
  // we split it at "in":
  exp=explode(norm_id(str)," in ");        // don't remove the spaces!
  if (sizeof(exp)>2) return 0;             // more than 2 "in" strings?!
  if (sizeof(exp)==2) {                    // we have an "in" found ->
                                           // let's check for the right book

    book=({object})TP->Search(exp[1],SEARCH_INV|SM_OBJECT);
                                           // find the book inside the player
    if (!book) {
      notify_fail("Turn what?\n", NOTIFY_NOT_VALID);
      return 0;                            // no book found
    }
    if (book!=TO) {
      notify_fail("Turn what?\n", NOTIFY_NOT_OBJ);
      return 0;                            // another book ?
    }
  }

  // okay, we have the right book, now let's check which page is the meant one

  switch (exp[0]) {
    case "to next page": {
      write("You turn to the next page.\n");
      show(NAME+" turn a "+QueryShort()+" to the next page.\n");
      page++;
      if (page>maxpage) {
        page=maxpage;
        write("You arrived at the last page.\n");
      }
      last_read=-1;                          // no auto turn
      return 1;
    }
    case "to first page": {
      write("You turn to the first page.\n");
      show(NAME+" turn a "+QueryShort()+" to the first page.\n");
      page=1;
      last_read=-1;
      return 1;
    }
    case "to last page": {
      write("You turn to the last page.\n");
      show(NAME+" turn a "+QueryShort()+" to the last page.\n");
      page=maxpage;
      last_read=-1;
      return 1;
    }
  }

  // no first,last or next pagge
  // let'c check for "page <number>"
  sscanf(exp[0],"to page %d",newpage);      // parse newpage
  if (newpage!=0) {                      // we got a new page
    if (newpage>maxpage) {
      notify_fail("The new page number has to be less than the maximal "
        "number of pages.\n",NOTIFY_NOT_VALID);
      return 0;
    }
    if (newpage<=0) {
      notify_fail("The new page number has to greater than 1!\n",
        NOTIFY_NOT_VALID);
      return 0;
    }
    write("You turn the "+QueryShort()+" to page "+newpage+".\n");
    page=newpage;
    last_read=-1;                         // no auto turn
    return 1;
  }
  notify_fail("The range of the new page must be between 1 and "+
    maxpage+".\n", NOTIFY_NOT_VALID);
  return 0;
}

//***************************************
// help msg
// closure needed, because the QueryShort
//***************************************
string help_msg() {
  string ret;
  ret= "Use: open/close "+strip_article(QueryShort())+"\n";

  // is a turn page command available?!
  // if yes - add the help message :)
  if (QueryMaxPage()>1) {
  ret+="     turn to page <number> in "+strip_article(QueryShort())+"\n"
       "     turn to next page in "+strip_article(QueryShort())+"\n"
       "     turn to first page in "+strip_article(QueryShort())+"\n"
       "     turn to last page in "+strip_article(QueryShort())+"\n";
  }

  ret+="     read "+strip_article(QueryShort())+"\n";
  return ret;
}

//*******************************************************
// hmm it is a container, but noone should detect this :)
//*******************************************************
string QueryIntShort() { return QueryShort(); }

//*******
// create
//*******
public void create() {
  ::create();
  page=1;                             // default one page
  maxpage=1;                          // default one page
  SetShort("a book");
  SetLong("A simple book.\n");
  SetWeight(1000);                    // 1 kg

  SetTransparency(0);                 // no internal is visible

  SetHelpMsg(#'help_msg);             // can be overloaded

  AddId("book");                      // remove the 'chest' id !
  RemoveClassId(({"chest","container"}));
  RemoveId("chest");
}

//*****
// init
//*****
public void init() {
  ::init();
  add_action("cmd_turn","turn");
}

//****************************************
// reset the book when it is opened/closed
//****************************************
public varargs int fopen (string str,int where) {
  int ret;
  ret=::fopen(str,where);
  if (ret==1) { page=1; }          // reset page
  last_read=-1;                    // no auto-turn while reading first page
  return ret;
}

//***************************
// forbid entering of objects
//***************************
int allow_enter (int method,mixed extra) {
  return ME_NO_ENTER;
}

