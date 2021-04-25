/* FNames-Lib
**
** A collection of routines performing functions on filenames.
**
**   31-Jul-92 [m] Extracted from ~mateese/obj/lpclib
**   12-Sep-92 [m] Now makes use of features of Amylaar's GD.
**
**  [m] Mateese, Biontic Magics & Darkness
*/

#pragma strict_types
#pragma save_types

#include "/obj/lib/fnames.h"

/*-------------------------------------------------------------------------
** string basepath (string fname, void|int what)
**
**  Split a normalized filename (beginning with "/") into path (with
**  trailing "/") and basename. Return either the path (what == 0) or
**  the basename (what != 0).
*/

varargs string basepath (string fname, int what) {
  int i, last;
  last = sizeof(fname)-1;
  for (i = last; i >= 0; i--) {
    if (fname[i] == '/') {
      if (what == 0) return fname[0..i];
      else return fname[(i+1)..last];
    }
  }
  if (what == 0) return "/"; else return "";
}

/*-------------------------------------------------------------------------
** mixed *parse_fname (string src, void|object pl)
** 
**  Expand <src> to a complete filename by calling pl->get_path() (if pl isn't
**  given, it defaults to this_player()) and return an array with following
**  information:
** 
**    [NAME] (str): the complete pathname of srcname.
**    [PATH] (str): the pure path of [0] with trailing "/" (may be just "/") .
**    [BASE] (str): the basename of [0] (may be "").
**    [SIZE] (int): file_size([0]) or FILE_WILD if the basename contains
**                  wildcards ('*'). If it's FILE_NA, the full name is invalid.
**    [PSIZE](int): file_size([1]); if it's FILE_NA, the full name is invalid.
**    [BSIZE](int): file_size([2]) if given and not wildcarded, else undefined.
** 
**  The special constants used for SIZE,PSIZE,BSIZE are:
** 
**    FILE_NA   == -1 : File doesn't exists or is not accessible.
**    FILE_DIR  == -2 : File is a directory.
**    FILE_WILD == -3 : Filename contains wildcards ('*').
** 
**  FILE_NA and FILE_DIR are the ones also returned by efun file_size().
*/

varargs mixed *parse_fname(string src, object pl) {
  mixed *ret;
  string foo, bar;
  int size;
  if (!pl) pl = this_player();
  ret = allocate(6);
  src = (string) pl->get_path(src,0,pl);
  if (sscanf(src, "%s*%s", foo, bar)) {
    ret[PATH] = basepath(src,0);
    ret[BASE] = basepath(src,1);
    ret[NAME] = src;
    if (sscanf(ret[PATH], "%s*%s", foo, bar)) size = FILE_NA;
    else size = file_size(ret[PATH]);
    ret[PSIZE] = size;
    ret[SIZE] = (size != FILE_NA) ? FILE_WILD : FILE_NA;
  } else {
    size = file_size(src);
    if (size >= 0) {
      ret[NAME] = src;
      ret[PATH] = basepath(src, 0);
      ret[BASE] = basepath(src, 1);
      ret[PSIZE] = FILE_DIR;
      ret[BSIZE] = size;
      ret[SIZE] = size;
    } else {
      if (size == FILE_DIR) ret[PATH] = src+(src!="/" ? "/" : "");
      else ret[PATH] = src;
      ret[BASE] = "";
      ret[NAME] = ret[PATH];
      ret[PSIZE] = size;
      ret[SIZE] = size;
    }
  }
  return ret;
}

/*-------------------------------------------------------------------------
** string _tackon (string name, string path)
**
**  Returns <path>+<name>.
**  This is for internal use of get_flist() only.
*/

string _tackon (string name, string path) { return path+name; }

/*-------------------------------------------------------------------------
** string *get_flist (mixed *fdsc, void|int mode) {
** 
**  get_flist() takes the return of parse_fname() and returns an array
**  of the (base)names of matching files (which may also be empty if no files
**  are found).
**  If parse_fname() got an illegal file-designation, get_flist() returns 0.
**  <mode> defines how the output of get_flist() looks like. The several
**  single modes may be or'ed (with |) as needed.
**    GF_EXT : The simul_efun get_dir_ext() is used instead of get_dir(),
**             so that the filetype is automagically flagged.
**    GF_LONG: Every file in the resulting array includes it's full path.
**    GF_SIZE: after each filename follows it's size.
**    GF_DATE: after each filename (and opt. size) follows it's date.
**  Note that get_flist("/dir/") lists the full dir, 
**  whereas get_flist("/dir/*") may stumble over one file "/dir/*".
*/

varargs string *get_flist (mixed *fdsc, int mode) {
  string *flist;
  string dir, pat;
  int gext, glong;
  if (fdsc[SIZE] == FILE_NA) return 0;
  if (fdsc[SIZE] >= 0) return ({ fdsc[BASE] });
  dir = fdsc[PATH]; pat = fdsc[BASE];
  if (mode & GF_EXT) gext = 1, mode ^= GF_EXT;
  if (mode & GF_LONG) glong = 1, mode ^= GF_LONG;
  if (!(mode & 1)) mode |= 1;
  if (fdsc[SIZE] == FILE_DIR) { /* Make sure, that complete dir is listed */
    dir = fdsc[PATH]; pat = "*";
    while (   (flist = (gext ? get_dir_ext(dir+pat) : get_dir(dir+pat,mode)))
           && sizeof(flist) == 1 && flist[0] == pat
          )
      pat += "*";
    /* ... since some folks create files with name '*', '**' etc. */
  } else flist = (gext ? get_dir_ext(dir+pat) : get_dir (dir+pat, mode));
  if (!flist) flist = ({ });
  else if (glong) flist = map (flist, "_tackon", fdsc[PATH]);
  return flist;
}

/*-------------------------------------------------------------------------
** string f_name (mixed arg, void|int pointc)
**
**   Find the real filename (opt. with trailing '.c') of an object.
**   arg may be the object itself, or already it's object_name().
**   The name is not checked for validity, though.
*/

varargs string f_name (mixed arg, int pointc) {
  string name, foo, bar;
  if (!arg) return 0;
  if (objectp(arg)) name = object_name(arg); else name = arg;
  name = explode(name, "#")[0];
  if (pointc && !sscanf(name, "%s.c", foo)) name += ".c";
  return name;
}

/*************************************************************************/
