 
static get_line(arg) {
   int line, matches;
 
   if (arg == "this")  return cur_line;
   if (arg == "end")   return last_line();
   matches = sscanf( arg, "%d", line );
   if (matches == 0) {
      write("Bad line number.\n");
      return (-1);
   }
   if (line > last_line()) {
      write("Not that many lines.\n");
      return (-1);
   }
   if (line < 1) {
      write("Bad line number\n");
      return (-1);
   }
   return line;
}
   
static get_range(arg) {
   int start,end,matches;
   if (arg=="this") arg = cur_line+"";
   if (arg=="all") arg = "1-end";
   matches = sscanf(arg,"%s-%s",start,end);
   if (matches == 0) {
      sscanf(arg,"%d",start);
      end = start;
   }
   if (matches == 2) {
      sscanf(start,"%d",start);
      if (end == "end")
         end = last_line();
      else
         if (sscanf(end,"%d",end) != 1)
            end = start;
   }
   if ( (start>end) || (start<1) ) {
      write("Bad range\n");
      return "error";
   }
   if (end > last_line()) {
      write("Not that many lines.\n");
      return "error";
   }
   return start+"-"+end;
}
 
static align_num(arg) {
  if (!disp_lines) return "";
  if (arg<10) 
      return "000"+arg+":";
   else if (arg<100)
      return "00"+arg+":";
   else if (arg<1000)
      return "0"+arg+":";
   else    
      return arg+":"; 
}   
 
info(arg) {
   if (!holding()) return 0;
   if (security()) return 1;
   write("Filename   : " + object_name   + "\n" );
   write("Last line  : " + last_line() + "\n" );
   write("is_changed : " + is_changed  + "\n" );
   write("high_buf   : " + (sizeof(buf)-1) + "\n" );
   write("low_buf    : " + (sizeof(buf[sizeof(buf)-1])-1) + "\n" );
   write("cur_line   : " + cur_line    + "\n" );
   return 1;
}
 
help(arg) {
   if (!holding()) return 0;
   if (security()) return 1;
   if (arg != "editor") return 0;
   this_player()->more(TEXT_DIR "editor_help");
   return 1;
}
 
