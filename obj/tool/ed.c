/*
 
TOOL:
  Hanarra's editor, Copyright 1991.
  OSB has express permission to use this editor. (Thanks Hanarra!)
 
VERSION:
  This is version 0.9
    It is important to realize that the editor is not fully tested
    in all aspects, especially in buffers over 1000 lines.  For files
    less then 1000 lines I have found no problems.
 
CHANGES:
  1992: Hyp.
     Fixed and modified original to work under native mode.
  1992: Killroy.
     Added various user-friendly features.
  Oct 13, 1992: Hanarra.
     added "move" and "copy" commands,
     added "insert file" command,
     split program into multiple #include files to make updates and
        enhancements easier to incorporate and debug.
 
*/


#if __FILE__ == "/obj/tool/ed.c"
#define ED_DIR   "/obj/tool/editor/"
#define TEXT_DIR "/obj/tool/editor/"
#define NO_LOG
#else
#define ED_DIR   "/players/hanarra/editor/"
#define TEXT_DIR "/players/hanarra/text/"
#endif

#include <properties.h>
#include <secure/wizlevels.h>

inherit "std/thing";
 
string buf,           /* the main storage buffer, 1000+ lines       */
       object_name,     /* stores the file name that is being edited  */
       insert_buf,    /* put what you want to insert in this global */
       find_string;   /* stores what string is being searched for   */
int    cur_line,      /* the current line                           */
       is_changed,    /* flag: whether buffer has been altered      */
       replace_start, /* start line # in the replace sequence       */
       replace_end,   /* end line # in the replace sequence         */
       replace_line,    /* current line we are checking             */
       replace_changed, /* flag: whether any line changed in replace seq. */
       replace_old,     /* stores what string we are looking for          */
       replace_new,     /* stores replacement string                      */
       replace_newline, /* stores what the whole line will be replaced    */
                        /* ...with if user verifies the change            */
       insert_where,  /* used by buf_insert(), what line to insert at     */
       chunk_size,    /* number of lines to display with print */ 
       disp_lines,    /* flag whether or not to show line #s   */
       is_loaded;     /* used to prevent outside reset calls   */
 
create() {
   if (is_loaded) {
      write("You can't call create() now!\n");
      return;
   }
   is_loaded = 1;
   ::create();
   SetLong("It's an editor, type 'help editor' for more info.\n");
   SetShort( capitalize( getuid( this_object() ) ) +"'s editor" );
   AddId("editor");
   Set(P_AUTOOBJECT , 1);
   SetNoDrop(1);
   SetNoGive(1);
   disp_lines = 1;
#ifndef NO_LOG
   ED_DIR "log_it"->do_log(getuid(this_object()));
#endif
   chunk_size = 10;
   buf_init();
   object_name = 0;
}
 
security() {
   if (getuid(this_object()) != getuid(this_player()) ) {
      write("This is not your editor!\n");
      return 1;
   }
}
 
holding() { if (environment() == this_player()) return 1; }
      
init() {
   if (query_user_level(this_player())>=LVL_WIZARD) {
      add_action("help","help");
      add_action("edit","file");       add_action("print","print");
      add_action("lines","lines");     add_action("chunk","chunk");
      add_action("insert","insert");   add_action("change","change");
      add_action("delete","delete");   add_action("find","find");
      add_action("next","next");       add_action("prev","prev");
      add_action("replace","replace");
      add_action("discard","discard"); add_action("info","info");
      add_action("move_lines","move"); add_action("copy_lines","copy");
      add_action("save","esave");
   }
}
 
#include ED_DIR "buffer.c"
#include ED_DIR "change.c"
#include ED_DIR "chunk.c"
#include ED_DIR "copy.c"
#include ED_DIR "delete.c"
#include ED_DIR "discard.c"
#include ED_DIR "files.c"
#include ED_DIR "insert.c"
#include ED_DIR "lines.c"
#include ED_DIR "misc.c"
#include ED_DIR "move.c"
#include ED_DIR "print.c"
#include ED_DIR "replace.c"
#include ED_DIR "searching.c"
