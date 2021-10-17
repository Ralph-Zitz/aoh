#ifndef MXP_H
#define MXP_H

#define MXP_SUPPORT_ENABLED  "mxp_enabled_features"
#define MXP_SUPPORT_DISABLED "mxp_disabled_features"
#ifndef TPMXP
#define TPMXP ({int})this_player()->QueryMXP()
#endif
#ifndef TOMXP
#define TOMXP ({int})this_object()->QueryMXP()
#endif

/* strings */

#define MXP_BEG "\x03"
#define MXP_END "\x04"
#define MXP_AMP "\x05"

/* characters */

#define MXP_BEGc '\x03'
#define MXP_ENDc '\x04'
#define MXP_AMPc '\x05'

/* constructs an MXP tag with < and > around it */

#define MXPTAG(arg) sprintf("%s%s%s", MXP_BEG, arg, MXP_END)
#define MXPTAG2(arg) sprintf("<%s>", arg)

#define ESC "\x1B"  /* esc character */

#define MXPMODE(arg) sprintf("%s[%dz", ESC, arg)

#endif // MXP_H
