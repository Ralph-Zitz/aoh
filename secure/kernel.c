/*--------------------------------------------------------------------------
 * /secure/kernel.c -- The kernel of simul efuns and other niceties.
 *
 * Copyright (C) 1997 by One Step Beyond.
 * All Rights Reserved.
 * Permission granted for non-commercial use.
 *--------------------------------------------------------------------------
 * Most of the work is done in included(!) submodules. At the point of
 * include, the file checks its name to include the correct modules.
 * The submodules in use are:
 *   kernel/compat.ic  : Compat code for old efuns.
 *   kernel/terminal_colour.ic: Compat code for terminal_colour() (long,
 *                       therefore in a file on its own).
 *   kernel/override.ic: Redefinitions of existing efuns.
 *   kernel/string.ic  : String management sefuns.
 *   kernel/objects.ic : Object oriented sefuns.
 *   kernel/misc.ic    : Miscellaneous sefuns.
 *   kernel/copy.ic    : the copy() sefun.
 *   kernel/file.ic    : File related sefuns.
 *   kernel/comm.ic    : Communication sefuns.
 *   kernel/search.ic  : Search related sefuns.
 *--------------------------------------------------------------------------
 */

#pragma strict_types
#pragma pedantic
#pragma save_types
#if __VERSION__ > "3.5.0"
#pragma rtt_checks
#pragma range_check
#endif
#include <secure/config.h>
#include <secure/kernel.h>
#include <secure/wizlevels.h>
#include <driver/wizlist.h>

/* Some defines used in the modules.
 */
#define TO       efun::this_object()
#define TI       efun::this_interactive()
#define TP       efun::this_player()
#define PRE      efun::previous_object()
#define LEVEL(x) MASTER->query_user_level(x)
#define NAME(x)  capitalize(getuid(x))

/* set the current object to be the previous object so we can do clean
 * call_other()s.
 */
#define FAKE_OBJECT set_this_object(previous_object())

/*-------------------------------------------------------------------------*/
// Submodules. Inherit at this point could be harmful. Order matters.

#if __FILE__ == "/secure/kernel.c"
#include "/secure/kernel/compat.ic"
#include "/secure/kernel/terminal_colour.ic"
#include "/secure/kernel/query_snoop.ic"
#include "/secure/kernel/override.ic"
#include "/secure/kernel/string.ic"
#include "/secure/kernel/debug_info.ic"
#include "/secure/kernel/objects.ic"
#include "/secure/kernel/misc.ic"
#include "/secure/kernel/copy.ic"
#include "/secure/kernel/file.ic"
#include "/secure/kernel/comm.ic"
#include "/secure/kernel/search.ic"
#include "/secure/kernel/seteuid.ic"
#include "/secure/kernel/shadow.ic"
#include "/secure/kernel/set_prompt.ic"
#include "/secure/kernel/query_load_average.ic"
#include "/secure/kernel/query_once_interactive.ic"
#include "/secure/kernel/query_udp_port.ic"
#include "/secure/kernel/query_mud_port.ic"
#include "/secure/kernel/query_ip_name.ic"
#include "/secure/kernel/query_ip_number.ic"
#include "/secure/kernel/set_heart_beat.ic"
#include "/secure/kernel/enable_commands.ic"
#include "/secure/kernel/disable_commands.ic"
#include "/secure/kernel/query_editing.ic"
#include "/secure/kernel/query_input_pending.ic"
#include "/secure/kernel/query_idle.ic"
#include "/secure/kernel/process_mxp.ic"
#else
#include "/secure/sparekernel/compat.ic"
#include "/secure/sparekernel/terminal_colour.ic"
#include "/secure/sparekernel/query_snoop.ic"
#include "/secure/sparekernel/override.ic"
#include "/secure/sparekernel/string.ic"
#include "/secure/sparekernel/debug_info.ic"
#include "/secure/sparekernel/objects.ic"
#include "/secure/sparekernel/misc.ic"
#include "/secure/sparekernel/copy.ic"
#include "/secure/sparekernel/file.ic"
#include "/secure/sparekernel/comm.ic"
#include "/secure/sparekernel/search.ic"
#include "/secure/sparekernel/seteuid.ic"
#include "/secure/sparekernel/shadow.ic"
#include "/secure/sparekernel/set_prompt.ic"
#include "/secure/sparekernel/query_load_average.ic"
#include "/secure/sparekernel/query_once_interactive.ic"
#include "/secure/sparekernel/query_udp_port.ic"
#include "/secure/sparekernel/query_mud_port.ic"
#include "/secure/sparekernel/query_ip_name.ic"
#include "/secure/sparekernel/query_ip_number.ic"
#include "/secure/sparekernel/set_heart_beat.ic"
#include "/secure/sparekernel/enable_commands.ic"
#include "/secure/sparekernel/disable_commands.ic"
#include "/secure/sparekernel/query_editing.ic"
#include "/secure/sparekernel/query_input_pending.ic"
#include "/secure/sparekernel/query_idle.ic"
#include "/secure/sparekernel/process_mxp.ic"
#endif

/*-------------------------------------------------------------------------*/
void start_simul_efun()

/* This function is either called by the gamedriver at boot time, or
 * from create() upon an update.
 * It initializes the kernel.
 */

{
  mixed *info;

  if ( !(info = get_extra_wizinfo(0)) ) {
      set_extra_wizinfo(0, info = allocate(BACKBONE_WIZINFO_SIZE));
#ifdef TIME_DELTA
      info[WI_BOOTTIME] = time()+TIME_DELTA;
#else
      info[WI_BOOTTIME] = time();
#endif
  }
  if (!(living_name_m = info[WI_LIVING_NAME]))
    living_name_m = info[WI_LIVING_NAME] = m_allocate(0, 1);
  if (!(name_living_m = info[WI_NAME_LIVING]))
    name_living_m = info[WI_NAME_LIVING] = m_allocate(0, 1);
  if (find_call_out("clean_simul_efun") < 0)
    efun::call_out("clean_simul_efun", 1800);
}

/*-------------------------------------------------------------------------*/
void create()
{
  setup_string();
  start_simul_efun();
}

/*-------------------------------------------------------------------------*/
static void clean_simul_efun()

/* Do some cleanup.
 */

{
  /* There might be destructed objects as keys. */
  m_indices(living_name_m);
  remove_call_out("clean_simul_efun");
  if (find_call_out("clean_name_living_m") < 0)
  {
    efun::call_out( "clean_name_living_m"
            , 1
            , m_indices(name_living_m)
            , sizeof(name_living_m)
        );
    }
    efun::call_out("clean_simul_efun", 3600);
}

/***************************************************************************/
