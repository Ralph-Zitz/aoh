/* This file contains the functions to parse a roomfile and to extract the
 * important variables
 * (c) Sique 12/92
 */

#include "/obj/tool/roommaker/roommaker.h"

string delete_remarks(string str)
{
  string return_string;
  string token;
  int string_mode;
  int comment_mode;
  int escape_mode;

  return_string = "";
  string_mode = 0;
  comment_mode = 0;
  escape_mode = 0;

  while(str && str != "") {
    if(comment_mode)
      if(str[0..1] != "*/") {
	str = str[1..];
	continue;
      } else {
	comment_mode = 0;
	write("End of comment.\n");
	str = str[2..];
	continue;
      }
    token = str[0..0];
    if(string_mode) {
      if(token == "\\") {
	escape_mode = 1-escape_mode;
	return_string += token;
	str = str[1..];
	continue;
      }
      if(str == "\"") 
	if(escape_mode) {
	  return_string += token;
	  str = str[1..];
	  continue;
	} else {
	  string_mode = 1-string_mode;
	  escape_mode = 0;
	  return_string += token;
	  str = str[1..];
	  continue;
	}
      escape_mode = 0;
      return_string += token;
      str = str[1..];
      continue;
    }
    if(token == "/" && str[1] == 42) {
      comment_mode = 1;
      write("Begin of comment\n");
      str = str[2..];
      continue;
    }
    return_string += token;
    str = str[1..];
  }
  return return_string;
}

/*
static int parse_room(string path)
{
  int    the_size;
  string filecontains;
  string instance;
  string to_do;

  if((the_size = file_size(path)) == -1) return 0; /* file doesn't exist */
/*  if(the_size == -2) {
    write("Error: " + path + " is a directory!\n");
    return -1;
  }
  filecontains = read_file(path);
  filecontains = delete_remarks(filecontains);
  if(!sscanf(filecontains,"inherit %s",instance)) {
    write("Error: " + path + " seems to be no room-file! (inherit missing)\n");
    return -1;
  }
  while(instance != "" &&
	instance[0] == 32 || instance[0] == 0 || instance[0] == 9)
    instance == instance[1..];
  if(instance == "") {
    write("Error: " + path + "... inherit without file\n");
    return -1;
  }
  
*/
