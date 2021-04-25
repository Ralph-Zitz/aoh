 
replace(arg) {
   int matches;
   string str1, str2, range, verify;
   if (!holding()) return 0;
   if (security()) return 1;
   if (!object_name) {write("No file name selected.\n"); return 1; }
   matches = sscanf(arg,"%s |%s|%s|%s",
                    range, replace_old, replace_new, verify );
   if (matches < 3) {
      write("Error in syntax.\n");
      write("USAGE: replace <range> |oldtext|newtext|<on/off>\n");
      return 1;
   }
   range = get_range(range);
   if (range == "error") return 1;
   sscanf(range,"%d-%d",replace_start,replace_end);
   if (replace_old == "") {
      write("You must give a string to replace.\n");
      return 1;
   }
   if ( verify == "off" )
      write("Verify is OFF\n");
   else
      write("Verify is ON\n");
   replace_changed = 0;
   replace_line = replace_start;
   while (replace_line <= replace_end) {
      if (sscanf(buf_read(replace_line),
                 "%s"+replace_old+"%s",str1,str2) == 2) {
         write(align_num(replace_line) + buf_read(replace_line) + "\n");
         replace_newline = str1 + replace_new + str2;
         if (verify != "off") {
            write( "Change this line (y/n/edit) ?");
            input_to("replace_verify");
            return 1;
         }
         buf_set( replace_line, replace_newline);
         write("Line changed...\n");
         replace_changed = 1;
         cur_line = replace_line;
      }
      else
         replace_line++;
   }
   if (!replace_changed)
      write( "String not found, no lines changed.\n");
   else
      write( "Search and replace done.\n");
   return 1;
}
 
static replace_verify(arg) {
   string str1, str2;
   if (arg==".") {
      write("This line not changed, search and replace aborted.\n");
      return 1;
   }
   else if (arg=="edit") {
      write("OLD\n");
      write( align_num(replace_line) + buf_read(replace_line) + "\n");
      write("NEW\n");
      write( align_num(replace_line) );
      input_to("replace_edit");
      return 1;
   }
   else if (arg=="y") {
      write("Line changed...\n");
      replace_changed = 1;
      buf_set( replace_line, replace_newline );
   }
   else if (arg=="n") {
      replace_line++;
   }
   while (replace_line<=replace_end) { 
     if (sscanf(buf_read(replace_line),"%s"+replace_old+"%s",str1,str2)==2) {
       write( align_num(replace_line) + buf_read(replace_line) + "\n" );
       replace_newline = str1 + replace_new + str2;
       write("Change this line (y/n/edit) ?");
       input_to("replace_verify");
       return 1;
     } 
     else
       replace_line++;
   }
   if (!replace_changed)
      write("String not found, no lines changed.\n");
   else
      write("Search and replace done.\n");
   return 1;
}
         
static replace_edit(arg) {
   if (arg==".") {
      write("Line NOT changed.\n");
   }
   else {
      write("Line changed.\n");
      buf_set(replace_line,arg);
      replace_changed = 1;
      cur_line = replace_line;
      replace_newline = arg;
   }
   write( align_num(cur_line) + buf_read(cur_line) + "\n" );
   write( "Change this line (y/n/edit) ?");
   input_to("replace_verify");
   return 1; 
}
 
