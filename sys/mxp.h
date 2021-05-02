#ifndef MXP_H
#define MXP_H

#define MXP_SUPPORT_ENABLED  "mxp_enabled_features"
#define MXP_SUPPORT_DISABLED "mxp_disabled_features"

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

#define ESC "\x1B"  /* esc character */

#define MXPMODE(arg) sprintf("%s[%dz", ESC, arg)

#endif // MXP_H
