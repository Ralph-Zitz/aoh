/* This file contains the functions for to get articles
 *
 * Part of RoomMaker V0.99.9 (c) Sique 9/1992
 *
 * Author: Sique 9/94
 *
 * Last Update: Sep 15 1994
 */

nosave mixed *article;         /* contains the article settings */
nosave string *refresh;     /* contains the refresh modi */

mixed Set(string label,mixed arg);

#include "/obj/tool/roommaker/roommaker.h"

static void enter_article()
{
  SetLabel("prompt","Article #" + (sizeof(article[0])+1) + ": ");
  SetLabel("nostring","");
  SetLabel("show_file","show_file_string");
  SetLabel("ready","gotomenu");
  SetLabel("undo","gotomenu");
  SetLabel("redisplay","article_redisplay");
  SetLabel("delete","delete_article");
  SetLabel("save_file","normal_save_string");
  SetLabel("insert_string","insert_article");
  SetLabel("delete_return","gotogetstring");
  write("Please enter the path of the article (help with \"~h\")!\n");
  gotogetstring();
}

static void article_redisplay()
{
  int i;
  if(!sizeof(article[0]))
  {
    write("No articles are given.\n");
    return gotogetstring();
  }
  if(sizeof(article[0]) == 1) write("This article is");
  else write("These articles are");
  write(" given:\n");
  for(i=0;i<sizeof(article[0]);i++)
    write(article[0][i] + " ... " + refresh[article[1][i]] + "\n");
  gotogetstring();
}

static void gotogetrefresh()
{
  write("Refresh: ");
  input_to("getrefresh");
}

static void insert_article(string str)
{
  article[0] += ({this_player()->get_path(str)});
  article[1] += ({0});
  gotogetrefresh();
}

static void getrefresh(string str)
{
  int i;
  if(str[0..1] == "~!")
  {
    if(str[2..-1] == "")
    {
      SetLabel("nextfun","gotogettext");
      write("You interrupt to make " + QueryFile() +
         "\nRestart with \"~m\".\n");
      say(this_player()->QueryName() + " interupts to write a file.\n");
      Set("making",1);
      return;
    }
    this_player()->command_me(str[2..-1]);
    write("Refresh: ");
    input_to("getrefresh");
    return;
  }
  switch(str) {
  case "":
    write("Refresh mode is now NONE.\n");
    return enter_article();
  case "~q": return abort();
  case "~h":
    write(help() +
"1 ....... no refresh                 2 ....... refresh on reset if article was\n"+
"3 ....... refresh on reset if article             destructed\n" +
"          was removed from room      4 ....... refresh every reset\nRefresh: ")
;
    input_to("getrefresh");
    return;
  case "~f":
    write(file_print() + "Refresh: ");
    input_to("getrefresh");
    return;
  case "~u":
    write("Remove " + article[0][sizeof(article[0])-1] + "\n");
    article[0] = article[0][0..sizeof(article[0])-2];
    article[1] = article[1][0..sizeof(article[1])-2];
    return enter_article();
  case  ".":
  case "**":
    write("Refresh mode is now NONE.\n");
    return enter_article();
  case "~r":
    for(i=0;i<sizeof(article[0]);i++)
      write(article[0][i] + " ... " + refresh[article[0][i]] + "\n");
    write("Refresh: ");
    input_to("getrefresh");
    return;
  case "~d":
    write("Remove " + article[0][sizeof(article[0])-1] + "\n");
    article[0] = article[0][0..sizeof(article[0])-2];
    article[1] = article[1][0..sizeof(article[1])-2];
    return enter_article();
  case "~s":
    write(file_print()
          + "Do you want to save this file " + QueryFile() + "? (y/n) ");
    SetLabel("nextfun","gotogetrefresh");
    input_to("filewrite");
    return;
  /* end switch */ }
  if(str == "none" || str == "0" || str == "1") return enter_article();
  if(str == "destruct" || str == "2")
  {
    article[1][sizeof(article[0])-1] = 1;
    return enter_article();
  }
  if(str == "remove" || str == "3")
  {
    article[1][sizeof(article[0])-1] = 2;
    return enter_article();
  }
  if(str == "always" || str == "4")
  {
    article[1][sizeof(article[0])-1] = 3;
    return enter_article();
  }
  write("ERROR: don't understand! Please enter again: (Help with \"~h\")\n" +
     "Refresh: ");
  input_to("getrefresh");
}

/* Shows all articles and asks which of then should be deleted */
static void delete_article()
{
  int i;
  if(!sizeof(article[0]))
  {

    write("No articles are given.\n");
    return call_other(this_object(),QueryLabel("delete_return"));
  }
  if(sizeof(article[0]) == 1) write("This article is");
  else write("These articles are");
  write(" given:\n");
  for(i=0;i<sizeof(article[0]);i++)
      write((i+1) + ". " + article[0][i] + " ... " + refresh[article[1][i]] + "\n");
  write("Which article should be deleted?\n");
  input_to("delete_article2");
}

/* This fun REALLY deletes the article #number */

static void delete_article_really(int number)
{
  if(number >= 0) {
    write("Deleting " + article[0][number] + " ...\n");
    SetLabel("deleted_article",({article[0][number],article[1][number]}));
    article[0] = article[0][0..number-1] +
      article[0][number+1..sizeof(article[0])-1];
    article[1] = article[1][0..number-1] +
      article[1][number+1..sizeof(article[1])-1];
  }
  else write("Nothing deleted.\n");
  if(QueryLabel("delete_return") == "gotogetstring")
    SetLabel("prompt","Article #" + (sizeof(article[0])+1) + ": ");
}

/* Tries to delete the given article */

static void delete_article2(string str)
{
  int number;                   /* number of the given article */
  if(sscanf(str,"%d",number) && number <= sizeof(article[0]))
  {
    delete_article_really(number-1);
    return call_other(this_object(),QueryLabel("delete_return"));
  }
  number = (member(article[0],str));
  if(number == -1)
  {
    write("There is no article named " + str + "!\n");
    return call_other(this_object(),QueryLabel("delete_return"));
  }
  delete_article_really(number);
  return call_other(this_object(),QueryLabel("delete_return"));
}

static void goto_delete_article()
{
  SetLabel("delete_return","gotomenu");
  delete_article();
}
