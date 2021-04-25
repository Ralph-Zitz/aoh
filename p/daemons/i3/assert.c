//
// Wunderland Mudlib
//
// global/daemon/i3/assert.c  --  Type Checker
//
// $Log: assert.c,v $
// Revision 1.2  2002/08/07 05:22:22  Fiona
// changed output format
//

#pragma strong_types
#pragma no_clone
#pragma no_shadow

#include <i3.h>

private int check(mixed data, mixed ass);

// Return NULL if data matches assertion ass.
// If not an error string is returned.
static string assert(mixed* data, mixed* ass) {
  int i, nr, p, ok;
  nr = sizeof(ass);
  if (sizeof(data) != nr) return "invalid data size";

  for (i = 0; i < nr; ++i) {
    ok = 0;
    for (p = sizeof(ass[i]); --p;) {
      ok = check(data[i], ass[i][p]);
      if (ok) break;
    }
    if (!ok) return sprintf("assertion for %O failed with %O", ass[i][0], data[i]);
  }
  return 0;
}

private int check(mixed data, mixed ass) {
  // Assert NULL
  if (ass == 0) {
    if (intp(data) && data == 0) return 1;
    return 0;
  }

  // Assert type
  if (intp(ass)) return typeof(data) == ass;

  // Assert array (unused yet)
  if (pointerp(ass)) {
    if (!pointerp(data)) return 0;
    return assert(data, ass) == 0;
  }

  raise_error("assertion definition error\n");
}
