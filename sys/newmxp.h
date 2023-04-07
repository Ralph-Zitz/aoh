// --------------- NEWMXP ---------------
#define VT_ESC "\e"
#define VT_MXP_OPEN_CHAR 's'
#define VT_MXP_CLOSE_CHAR 't'
#define VT_MXP_OPEN(n) VT_ESC "[!"+(n)+"s"
#define VT_MXP_OPEN_ARG(n, arg) VT_ESC "[!"+(n)+";" + (arg) + "s"
#define VT_MXP_CLOSE(n) VT_ESC "[!"+(n)+"t"
#define VT_MXP_LINE_SECURE_MODE VT_ESC "[1z"
#define VT_MXP_TEMP_SECURE_MODE VT_ESC "[4z"
#define VT_MXP_LOCK_LOCKED_MODE VT_ESC "[7z"

#define VT_MXP_ROOM_NAME         1
#define VT_MXP_ROOM_DESC         2
#define VT_MXP_ROOM_EXITS        3
#define VT_MXP_ROOM_NUM          4
#define VT_MXP_ROOM_EXP          5
#define VT_MXP_PROMPT            6
#define VT_MXP_EX                7
#define VT_MXP_VERSION           8
#define VT_MXP_SUPPORT           9

#define VT_MXP_IROOMCONTENT     11
#define VT_MXP_LROOMCONTENT     12
#define VT_MXP_IINVENTORY       13

#define MSG_MXP(n, txt)             (VT_MXP_OPEN(n) + txt + VT_MXP_CLOSE(n))
#define MSG_MXP_ARG(n, arg, txt )   (VT_MXP_OPEN_ARG(n, arg) + txt + VT_MXP_CLOSE(n))

#define MSG_REXITS(dir)                 MSG_MXP(VT_MXP_ROOM_EXITS, dir)
#define MSG_RNAME(dir)                  MSG_MXP(VT_MXP_ROOM_NAME, dir)
#define MSG_RDESC(dir)                  MSG_MXP(VT_MXP_ROOM_DESC, dir)
#define MSG_ROOMN(dir)                  MSG_MXP(VT_MXP_ROOM_NUM, dir)
#define MSG_REXPIRE                     VT_MXP_OPEN(VT_MXP_ROOM_EXP)
#define MSG_PROMPT(dir)                 MSG_MXP(VT_MXP_PROMPT, dir)
#define MSG_EX(dir)                     MSG_MXP(VT_MXP_EX, dir)
#define MSG_IRCONTENT(num, desc)        MSG_MXP_ARG(VT_MXP_IROOMCONTENT, num, desc)
#define MSG_LRCONTENT(num, desc)        MSG_MXP_ARG(VT_MXP_LROOMCONTENT, num, desc)
#define MSG_IINVENTORY(num, desc)       MSG_MXP_ARG(VT_MXP_IINVENTORY, num, desc)
// --------------- End-Of-NEWMXP ---------------
