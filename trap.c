/* trap.c -- Not the trap command, but useful functions for manipulating
   those objects.  The trap command is in builtins/trap.def. */

/* Copyright (C) 1987-2013 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Bash is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Bash.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "config.h"

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#include "bashtypes.h"
#include "bashansi.h"

#include <stdio.h>
#include <errno.h>

#include "bashintl.h"

#include <signal.h>

#include "trap.h"

#include "shell.h"
#include "flags.h"
#include "input.h"	/* for save_token_state, restore_token_state */
#include "jobs.h"
#include "signames.h"
#include "builtins.h"
#include "builtins/common.h"
#include "builtins/builtext.h"

#if defined (READLINE)
#  include <readline/readline.h>
#  include "bashline.h"
#endif

#ifndef errno
extern int errno;
#endif

/* Flags which describe the current handling state of a signal. */
#define SIG_INHERITED   0x0	/* Value inherited from parent. */
#define SIG_TRAPPED     0x1	/* Currently trapped. */
#define SIG_HARD_IGNORE 0x2	/* Signal was ignored on shell entry. */
#define SIG_SPECIAL     0x4	/* Treat this signal specially. */
#define SIG_NO_TRAP     0x8	/* Signal cannot be trapped. */
#define SIG_INPROGRESS	0x10	/* Signal handler currently executing. */
#define SIG_CHANGED	0x20	/* Trap value changed in trap handler. */
#define SIG_IGNORED	0x40	/* The signal is currently being ignored. */

#define SPECIAL_TRAP(s)	((s) == EXIT_TRAP || (s) == DEBUG_TRAP || (s) == ERROR_TRAP || (s) == RETURN_TRAP)

/* An array of such flags, one for each signal, describing what the
   shell will do with a signal.  DEBUG_TRAP == NSIG; some code below
   assumes this. */
static int sigmodes[BASH_NSIG];

static void free_trap_command __P((int));
static void change_signal __P((int, char *));

static int _run_trap_internal __P((int, char *));

static void free_trap_string __P((int));
static void reset_signal __P((int));
static void restore_signal __P((int));
static void reset_or_restore_signal_handlers __P((sh_resetsig_func_t *));

/* Variables used here but defined in other files. */
extern int last_command_exit_value;
extern int line_number;

extern int sigalrm_seen;
extern procenv_t alrmbuf;

extern char *this_command_name;
extern sh_builtin_func_t *this_shell_builtin;
extern procenv_t wait_intr_buf;
extern int return_catch_flag, return_catch_value;
extern int subshell_level;
extern WORD_LIST *subst_assign_varlist;

/* The list of things to do originally, before we started trapping. */
SigHandler *original_signals[NSIG];

/* For each signal, a slot for a string, which is a command to be
   executed when that signal is received.  The slot can also contain
   DEFAULT_SIG, which means do whatever you were going to do before
   you were so rudely interrupted, or IGNORE_SIG, which says ignore
   this signal. */
char *trap_list[BASH_NSIG];

/* A bitmap of signals received for which we have trap handlers. */
int pending_traps[NSIG];

/* Set to the number of the signal we're running the trap for + 1.
   Used in execute_cmd.c and builtins/common.c to clean up when
   parse_and_execute does not return normally after executing the
   trap command (e.g., when `return' is executed in the trap command). */
int running_trap;

/* Set to last_command_exit_value before running a trap. */
int trap_saved_exit_value;

/* The (trapped) signal received while executing in the `wait' builtin */
int wait_signal_received;

int trapped_signal_received;

#define GETORIGSIG(sig) \
  do { \
    original_signals[sig] = (SigHandler *)set_signal_handler (sig, SIG_DFL); \
    set_signal_handler (sig, original_signals[sig]); \
    if (original_signals[sig] == SIG_IGN) \
      sigmodes[sig] |= SIG_HARD_IGNORE; \
  } while (0)

#define SETORIGSIG(sig,handler) \
  do { \
    original_signals[sig] = handler; \
    if (original_signals[sig] == SIG_IGN) \
      sigmodes[sig] |= SIG_HARD_IGNORE; \
  } while (0)

#define GET_ORIGINAL_SIGNAL(sig) \
  if (sig && sig < NSIG && original_signals[sig] == IMPOSSIBLE_TRAP_HANDLER) \
    GETORIGSIG(sig)

void
initialize_traps ()
{
  register int i;

  initialize_signames();

  trap_list[EXIT_TRAP] = trap_list[DEBUG_TRAP] = trap_list[ERROR_TRAP] = trap_list[RETURN_TRAP] = (char *)NULL;
  sigmodes[EXIT_TRAP] = sigmodes[DEBUG_TRAP] = sigmodes[ERROR_TRAP] = sigmodes[RETURN_TRAP] = SIG_INHERITED;
  original_signals[EXIT_TRAP] = IMPOSSIBLE_TRAP_HANDLER;

  for (i = 1; i < NSIG; i++)
    {
      pending_traps[i] = 0;
      trap_list[i] = (char *)DEFAULT_SIG;
      sigmodes[i] = SIG_INHERITED;	/* XXX - only set, not used */
      original_signals[i] = IMPOSSIBLE_TRAP_HANDLER;
    }

  /* Show which signals are treated specially by the shell. */
#if defined (SIGCHLD)
  GETORIGSIG (SIGCHLD);
  sigmodes[SIGCHLD] |= (SIG_SPECIAL | SIG_NO_TRAP);
#endif /* SIGCHLD */

  GETORIGSIG (SIGINT);
  sigmodes[SIGINT] |= SIG_SPECIAL;

#if defined (__BEOS__)
  /* BeOS sets SIGINT to SIG_IGN! */
  original_signals[SIGINT] = SIG_DFL;
  sigmodes[SIGINT] &= ~SIG_HARD_IGNORE;
#endif

  GETORIGSIG (SIGQUIT);
  sigmodes[SIGQUIT] |= SIG_SPECIAL;

  if (interactive)
    {
      GETORIGSIG (SIGTERM);
      sigmodes[SIGTERM] |= SIG_SPECIAL;
    }
}

#ifdef DEBUG
/* Return a printable representation of the trap handler for SIG. */
static char *
trap_handler_string (sig)
     int sig;
{
  if (trap_list[sig] == (char *)DEFAULT_SIG)
    return "DEFAULT_SIG";
  else if (trap_list[sig] == (char *)IGNORE_SIG)
    return "IGNORE_SIG";
  else if (trap_list[sig] == (char *)IMPOSSIBLE_TRAP_HANDLER)
    return "IMPOSSIBLE_TRAP_HANDLER";
  else if (trap_list[sig])
    return trap_list[sig];
  else
    return "NULL";
}
#endif

/* Return the print name of this signal. */
char *
signal_name (sig)
     int sig;
{
  char *ret;

  /* on cygwin32, signal_names[sig] could be null */
  ret = (sig >= BASH_NSIG || sig < 0 || signal_names[sig] == NULL)
	? _("invalid signal number")
	: signal_names[sig];

  return ret;
}

/* Turn a string into a signal number, or a number into
   a signal number.  If STRING is "2", "SIGINT", or "INT",
   then (int)2 is returned.  Return NO_SIG if STRING doesn't
   contain a valid signal descriptor. */
int
decode_signal (string, flags)
     char *string;
     int flags;
{
  intmax_t sig;
  char *name;

  if (legal_number (string, &sig))
    return ((sig >= 0 && sig < NSIG) ? (int)sig : NO_SIG);

  /* A leading `SIG' may be omitted. */
  for (sig = 0; sig < BASH_NSIG; sig++)
    {
      name = signal_names[sig];
      if (name == 0 || name[0] == '\0')
	continue;

      /* Check name without the SIG prefix first case sensitively or
	 insensitively depending on whether flags includes DSIG_NOCASE */
      if (STREQN (name, "SIG", 3))
	{
	  name += 3;

	  if ((flags & DSIG_NOCASE) && strcasecmp (string, name) == 0)
	    return ((int)sig);
	  else if ((flags & DSIG_NOCASE) == 0 && strcmp (string, name) == 0)
	    return ((int)sig);
	  /* If we can't use the `SIG' prefix to match, punt on this
	     name now. */
	  else if ((flags & DSIG_SIGPREFIX) == 0)
	    continue;
	}

      /* Check name with SIG prefix case sensitively or insensitively
	 depending on whether flags includes DSIG_NOCASE */
      name = signal_names[sig];
      if ((flags & DSIG_NOCASE) && strcasecmp (string, name) == 0)
	return ((int)sig);
      else if ((flags & DSIG_NOCASE) == 0 && strcmp (string, name) == 0)
	return ((int)sig);
    }

  return (NO_SIG);
}

/* Non-zero when we catch a trapped signal. */
static int catch_flag;

void
run_pending_traps ()
{
  register int sig;
  int old_exit_value;
  WORD_LIST *save_subst_varlist;
  sh_parser_state_t pstate;
#if defined (ARRAY_VARS)
  ARRAY *ps;
#endif

  if (catch_flag == 0)		/* simple optimization */
    return;

  if (running_trap > 0)
    {
#if defined (DEBUG)
      internal_warning ("run_pending_traps: recursive invocation while running trap for signal %d", running_trap-1);
#endif
#if 0
      return;			/* no recursive trap invocations */
#else
      ;
#endif
    }

  catch_flag = trapped_signal_received = 0;

  /* Preserve $? when running trap. */
  old_exit_value = last_command_exit_value;
#if defined (ARRAY_VARS)
  ps = save_pipestatus_array ();
#endif

  for (sig = 1; sig < NSIG; sig++)
    {
      /* XXX this could be made into a counter by using
	 while (pending_traps[sig]--) instead of the if statement. */
      if (pending_traps[sig])
	{
	  if (running_trap == sig+1)
	    /*continue*/;

	  running_trap = sig + 1;

	  if (sig == SIGINT)
	    {
	      pending_traps[sig] = 0;	/* XXX */
	      run_interrupt_trap ();
	      CLRINTERRUPT;
	    }
#if defined (JOB_CONTROL) && defined (SIGCHLD)
	  else if (sig == SIGCHLD &&
		   trap_list[SIGCHLD] != (char *)IMPOSSIBLE_TRAP_HANDLER &&
		   (sigmodes[SIGCHLD] & SIG_INPROGRESS) == 0)
	    {
	      sigmodes[SIGCHLD] |= SIG_INPROGRESS;
	      run_sigchld_trap (pending_traps[sig]);	/* use as counter */
	      sigmodes[SIGCHLD] &= ~SIG_INPROGRESS;
	    }
	  else if (sig == SIGCHLD &&
		   trap_list[SIGCHLD] == (char *)IMPOSSIBLE_TRAP_HANDLER &&
		   (sigmodes[SIGCHLD] & SIG_INPROGRESS) != 0)
	    {
	      /* This can happen when run_pending_traps is called while
		 running a SIGCHLD trap handler. */
	      running_trap = 0;
	      /* want to leave pending_traps[SIGCHLD] alone here */
	      continue;					/* XXX */
	    }
	  else if (sig == SIGCHLD && (sigmodes[SIGCHLD] & SIG_INPROGRESS))
	    {
	      /* whoops -- print warning? */
	      running_trap = 0;		/* XXX */
	      /* want to leave pending_traps[SIGCHLD] alone here */
	      continue;
	    }
#endif
	  else if (trap_list[sig] == (char *)DEFAULT_SIG ||
		   trap_list[sig] == (char *)IGNORE_SIG ||
		   trap_list[sig] == (char *)IMPOSSIBLE_TRAP_HANDLER)
	    {
	      /* This is possible due to a race condition.  Say a bash
		 process has SIGTERM trapped.  A subshell is spawned
		 using { list; } & and the parent does something and kills
		 the subshell with SIGTERM.  It's possible for the subshell
		 to set pending_traps[SIGTERM] to 1 before the code in
		 execute_cmd.c eventually calls restore_original_signals
		 to reset the SIGTERM signal handler in the subshell.  The
		 next time run_pending_traps is called, pending_traps[SIGTERM]
		 will be 1, but the trap handler in trap_list[SIGTERM] will
		 be invalid (probably DEFAULT_SIG, but it could be IGNORE_SIG).
		 Unless we catch this, the subshell will dump core when
		 trap_list[SIGTERM] == DEFAULT_SIG, because DEFAULT_SIG is
		 usually 0x0. */
	      internal_warning (_("run_pending_traps: bad value in trap_list[%d]: %p"),
				sig, trap_list[sig]);
	      if (trap_list[sig] == (char *)DEFAULT_SIG)
		{
		  internal_warning (_("run_pending_traps: signal handler is SIG_DFL, resending %d (%s) to myself"), sig, signal_name (sig));
		  kill (getpid (), sig);
		}
	    }
	  else
	    {
	      /* XXX - should we use save_parser_state/restore_parser_state? */
	      save_parser_state (&pstate);
	      save_subst_varlist = subst_assign_varlist;
	      subst_assign_varlist = 0;

#if defined (JOB_CONTROL)
	      save_pipeline (1);	/* XXX only provides one save level */
#endif
	      /* XXX - set pending_traps[sig] = 0 here? */
	      pending_traps[sig] = 0;
	      evalstring (savestring (trap_list[sig]), "trap", SEVAL_NONINT|SEVAL_NOHIST|SEVAL_RESETLINE);
#if defined (JOB_CONTROL)
	      restore_pipeline (1);
#endif

	      subst_assign_varlist = save_subst_varlist;
	      restore_parser_state (&pstate);
	    }

	  pending_traps[sig] = 0;	/* XXX - move before evalstring? */
	  running_trap = 0;
	}
    }

#if defined (ARRAY_VARS)
  restore_pipestatus_array (ps);
#endif
  last_command_exit_value = old_exit_value;
}

sighandler
trap_handler (sig)
     int sig;
{
  int oerrno;

  if ((sigmodes[sig] & SIG_TRAPPED) == 0)
    {
#if defined (DEBUG)
      internal_warning ("trap_handler: signal %d: signal not trapped", sig);
#endif
      SIGRETURN (0);
    }

  if ((sig >= NSIG) ||
      (trap_list[sig] == (char *)DEFAULT_SIG) ||
      (trap_list[sig] == (char *)IGNORE_SIG))
    programming_error (_("trap_handler: bad signal %d"), sig);
  else
    {
      oerrno = errno;
#if defined (MUST_REINSTALL_SIGHANDLERS)
#  if defined (JOB_CONTROL) && defined (SIGCHLD)
      if (sig != SIGCHLD)
#  endif /* JOB_CONTROL && SIGCHLD */
      set_signal_handler (sig, trap_handler);
#endif /* MUST_REINSTALL_SIGHANDLERS */

      catch_flag = 1;
      pending_traps[sig]++;

      trapped_signal_received = sig;

      if (this_shell_builtin && (this_shell_builtin == wait_builtin))
	{
	  wait_signal_received = sig;
	  if (interrupt_immediately)
	    longjmp (wait_intr_buf, 1);
	}

#if defined (READLINE)
      /* Set the event hook so readline will call it after the signal handlers
	 finish executing, so if this interrupted character input we can get
	 quick response. */
      if (RL_ISSTATE (RL_STATE_SIGHANDLER) && interrupt_immediately == 0)
        bashline_set_event_hook ();
#endif

      if (interrupt_immediately)
	run_pending_traps ();

      errno = oerrno;
    }

  SIGRETURN (0);
}

int
first_pending_trap ()
{
  register int i;

  for (i = 1; i < NSIG; i++)
    if (pending_traps[i])
      return i;
  return -1;
}

int
any_signals_trapped ()
{
  register int i;

  for (i = 1; i < NSIG; i++)
    if (sigmodes[i] & SIG_TRAPPED)
      return i;
  return -1;
}

void
check_signals ()
{
  CHECK_ALRM;		/* set by the read builtin */
  QUIT;
}

/* Convenience functions the rest of the shell can use */
void
check_signals_and_traps ()
{
  check_signals ();

  run_pending_traps ();
}

#if defined (JOB_CONTROL) && defined (SIGCHLD)

#ifdef INCLUDE_UNUSED
/* Make COMMAND_STRING be executed when SIGCHLD is caught. */
void
set_sigchld_trap (command_string)
     char *command_string;
{
  set_signal (SIGCHLD, command_string);
}
#endif

/* Make COMMAND_STRING be executed when SIGCHLD is caught iff SIGCHLD
   is not already trapped.  IMPOSSIBLE_TRAP_HANDLER is used as a sentinel
   to make sure that a SIGCHLD trap handler run via run_sigchld_trap can
   reset the disposition to the default and not have the original signal
   accidentally restored, undoing the user's command. */
void
maybe_set_sigchld_trap (command_string)
     char *command_string;
{
  if ((sigmodes[SIGCHLD] & SIG_TRAPPED) == 0 && trap_list[SIGCHLD] == (char *)IMPOSSIBLE_TRAP_HANDLER)
    set_signal (SIGCHLD, command_string);
}

/* Temporarily set the SIGCHLD trap string to IMPOSSIBLE_TRAP_HANDLER.  Used
   as a sentinel in run_sigchld_trap and maybe_set_sigchld_trap to see whether
   or not a SIGCHLD trap handler reset SIGCHLD disposition to the default. */
void
set_impossible_sigchld_trap ()
{
  restore_default_signal (SIGCHLD);
  change_signal (SIGCHLD, (char *)IMPOSSIBLE_TRAP_HANDLER);
  sigmodes[SIGCHLD] &= ~SIG_TRAPPED;	/* maybe_set_sigchld_trap checks this */
}

/* Act as if we received SIGCHLD NCHILD times and increment
   pending_traps[SIGCHLD] by that amount.  This allows us to still run the
   SIGCHLD trap once for each exited child. */
void
queue_sigchld_trap (nchild)
     int nchild;
{
  if (nchild > 0)
    {
      catch_flag = 1;
      pending_traps[SIGCHLD] += nchild;
      trapped_signal_received = SIGCHLD;
    }
}
#endif /* JOB_CONTROL && SIGCHLD */

void
set_debug_trap (command)
     char *command;
{
  set_signal (DEBUG_TRAP, command);
}

void
set_error_trap (command)
     char *command;
{
  set_signal (ERROR_TRAP, command);
}

void
set_return_trap (command)
     char *command;
{
  set_signal (RETURN_TRAP, command);
}

#ifdef INCLUDE_UNUSED
void
set_sigint_trap (command)
     char *command;
{
  set_signal (SIGINT, command);
}
#endif

/* Reset the SIGINT handler so that subshells that are doing `shellsy'
   things, like waiting for command substitution or executing commands
   in explicit subshells ( ( cmd ) ), can catch interrupts properly. */
SigHandler *
set_sigint_handler ()
{
  if (sigmodes[SIGINT] & SIG_HARD_IGNORE)
    return ((SigHandler *)SIG_IGN);

  else if (sigmodes[SIGINT] & SIG_IGNORED)
    return ((SigHandler *)set_signal_handler (SIGINT, SIG_IGN)); /* XXX */

  else if (sigmodes[SIGINT] & SIG_TRAPPED)
    return ((SigHandler *)set_signal_handler (SIGINT, trap_handler));

  /* The signal is not trapped, so set the handler to the shell's special
     interrupt handler. */
  else if (interactive)	/* XXX - was interactive_shell */
    return (set_signal_handler (SIGINT, sigint_sighandler));
  else
    return (set_signal_handler (SIGINT, termsig_sighandler));
}

/* Return the correct handler for signal SIG according to the values in
   sigmodes[SIG]. */
SigHandler *
trap_to_sighandler (sig)
     int sig;
{
  if (sigmodes[sig] & (SIG_IGNORED|SIG_HARD_IGNORE))
    return (SIG_IGN);
  else if (sigmodes[sig] & SIG_TRAPPED)
    return (trap_handler);
  else
    return (SIG_DFL);
}

/* Set SIG to call STRING as a command. */
void
set_signal (sig, string)
     int sig;
     char *string;
{
  sigset_t set, oset;

  if (SPECIAL_TRAP (sig))
    {
      change_signal (sig, savestring (string));
      if (sig == EXIT_TRAP && interactive == 0)
	initialize_terminating_signals ();
      return;
    }

  /* A signal ignored on entry to the shell cannot be trapped or reset, but
     no error is reported when attempting to do so.  -- Posix.2 */
  if (sigmodes[sig] & SIG_HARD_IGNORE)
    return;

  /* Make sure we have original_signals[sig] if the signal has not yet
     been trapped. */
  if ((sigmodes[sig] & SIG_TRAPPED) == 0)
    {
      /* If we aren't sure of the original value, check it. */
      if (original_signals[sig] == IMPOSSIBLE_TRAP_HANDLER)
        GETORIGSIG (sig);
      if (original_signals[sig] == SIG_IGN)
	return;
    }

  /* Only change the system signal handler if SIG_NO_TRAP is not set.
     The trap command string is changed in either case.  The shell signal
     handlers for SIGINT and SIGCHLD run the user specified traps in an
     environment in which it is safe to do so. */
  if ((sigmodes[sig] & SIG_NO_TRAP) == 0)
    {
      BLOCK_SIGNAL (sig, set, oset);
      change_signal (sig, savestring (string));
      set_signal_handler (sig, trap_handler);
      UNBLOCK_SIGNAL (oset);
    }
  else
    change_signal (sig, savestring (string));
}

static void
free_trap_command (sig)
     int sig;
{
  if ((sigmodes[sig] & SIG_TRAPPED) && trap_list[sig] &&
      (trap_list[sig] != (char *)IGNORE_SIG) &&
      (trap_list[sig] != (char *)DEFAULT_SIG) &&
      (trap_list[sig] != (char *)IMPOSSIBLE_TRAP_HANDLER))
    free (trap_list[sig]);
}

/* If SIG has a string assigned to it, get rid of it.  Then give it
   VALUE. */
static void
change_signal (sig, value)
     int sig;
     char *value;
{
  if ((sigmodes[sig] & SIG_INPROGRESS) == 0)
    free_trap_command (sig);
  trap_list[sig] = value;

  sigmodes[sig] |= SIG_TRAPPED;
  if (value == (char *)IGNORE_SIG)
    sigmodes[sig] |= SIG_IGNORED;
  else
    sigmodes[sig] &= ~SIG_IGNORED;
  if (sigmodes[sig] & SIG_INPROGRESS)
    sigmodes[sig] |= SIG_CHANGED;
}

void
get_original_signal (sig)
     int sig;
{
  /* If we aren't sure the of the original value, then get it. */
  if (sig > 0 && sig < NSIG && original_signals[sig] == (SigHandler *)IMPOSSIBLE_TRAP_HANDLER)
    GETORIGSIG (sig);
}

void
get_all_original_signals ()
{
  register int i;

  for (i = 1; i < NSIG; i++)
    GET_ORIGINAL_SIGNAL (i);
}

void
set_original_signal (sig, handler)
     int sig;
     SigHandler *handler;
{
  if (sig > 0 && sig < NSIG && original_signals[sig] == (SigHandler *)IMPOSSIBLE_TRAP_HANDLER)
    SETORIGSIG (sig, handler);
}

/* Restore the default action for SIG; i.e., the action the shell
   would have taken before you used the trap command.  This is called
   from trap_builtin (), which takes care to restore the handlers for
   the signals the shell treats specially. */
void
restore_default_signal (sig)
     int sig;
{
  if (SPECIAL_TRAP (sig))
    {
      if ((sig != DEBUG_TRAP && sig != ERROR_TRAP && sig != RETURN_TRAP) ||
	  (sigmodes[sig] & SIG_INPROGRESS) == 0)
	free_trap_command (sig);
      trap_list[sig] = (char *)NULL;
      sigmodes[sig] &= ~SIG_TRAPPED;
      if (sigmodes[sig] & SIG_INPROGRESS)
	sigmodes[sig] |= SIG_CHANGED;
      return;
    }

  GET_ORIGINAL_SIGNAL (sig);

  /* A signal ignored on entry to the shell cannot be trapped or reset, but
     no error is reported when attempting to do so.  Thanks Posix.2. */
  if (sigmodes[sig] & SIG_HARD_IGNORE)
    return;

  /* If we aren't trapping this signal, don't bother doing anything else. */
  /* We special-case SIGCHLD and IMPOSSIBLE_TRAP_HANDLER (see above) as a
     sentinel to determine whether or not disposition is reset to the default
     while the trap handler is executing. */
  if (((sigmodes[sig] & SIG_TRAPPED) == 0) &&
      (sig != SIGCHLD || (sigmodes[sig] & SIG_INPROGRESS) == 0 || trap_list[sig] != (char *)IMPOSSIBLE_TRAP_HANDLER))
    return;

  /* Only change the signal handler for SIG if it allows it. */
  if ((sigmodes[sig] & SIG_NO_TRAP) == 0)
    set_signal_handler (sig, original_signals[sig]);

  /* Change the trap command in either case. */
  change_signal (sig, (char *)DEFAULT_SIG);

  /* Mark the signal as no longer trapped. */
  sigmodes[sig] &= ~SIG_TRAPPED;
}

/* Make this signal be ignored. */
void
ignore_signal (sig)
     int sig;
{
  if (SPECIAL_TRAP (sig) && ((sigmodes[sig] & SIG_IGNORED) == 0))
    {
      change_signal (sig, (char *)IGNORE_SIG);
      return;
    }

  GET_ORIGINAL_SIGNAL (sig);

  /* A signal ignored on entry to the shell cannot be trapped or reset.
     No error is reported when the user attempts to do so. */
  if (sigmodes[sig] & SIG_HARD_IGNORE)
    return;

  /* If already trapped and ignored, no change necessary. */
  if (sigmodes[sig] & SIG_IGNORED)
    return;

  /* Only change the signal handler for SIG if it allows it. */
  if ((sigmodes[sig] & SIG_NO_TRAP) == 0)
    set_signal_handler (sig, SIG_IGN);

  /* Change the trap command in either case. */
  change_signal (sig, (char *)IGNORE_SIG);
}

/* Handle the calling of "trap 0".  The only sticky situation is when
   the command to be executed includes an "exit".  This is why we have
   to provide our own place for top_level to jump to. */
int
run_exit_trap ()
{
  char *trap_command;
  int code, function_code, retval;
#if defined (ARRAY_VARS)
  ARRAY *ps;
#endif

  trap_saved_exit_value = last_command_exit_value;
#if defined (ARRAY_VARS)
  ps = save_pipestatus_array ();
#endif
  function_code = 0;

  /* Run the trap only if signal 0 is trapped and not ignored, and we are not
     currently running in the trap handler (call to exit in the list of
     commands given to trap 0). */
  if ((sigmodes[EXIT_TRAP] & SIG_TRAPPED) &&
      (sigmodes[EXIT_TRAP] & (SIG_IGNORED|SIG_INPROGRESS)) == 0)
    {
      trap_command = savestring (trap_list[EXIT_TRAP]);
      sigmodes[EXIT_TRAP] &= ~SIG_TRAPPED;
      sigmodes[EXIT_TRAP] |= SIG_INPROGRESS;

      retval = trap_saved_exit_value;
      running_trap = 1;

      code = setjmp_nosigs (top_level);

      /* If we're in a function, make sure return longjmps come here, too. */
      if (return_catch_flag)
	function_code = setjmp_nosigs (return_catch);

      if (code == 0 && function_code == 0)
	{
	  reset_parser ();
	  parse_and_execute (trap_command, "exit trap", SEVAL_NONINT|SEVAL_NOHIST|SEVAL_RESETLINE);
	}
      else if (code == ERREXIT)
	retval = last_command_exit_value;
      else if (code == EXITPROG)
	retval = last_command_exit_value;
      else if (function_code != 0)
        retval = return_catch_value;
      else
	retval = trap_saved_exit_value;

      running_trap = 0;
      return retval;
    }

#if defined (ARRAY_VARS)
  restore_pipestatus_array (ps);
#endif
  return (trap_saved_exit_value);
}

void
run_trap_cleanup (sig)
     int sig;
{
  sigmodes[sig] &= ~(SIG_INPROGRESS|SIG_CHANGED);
}

#define RECURSIVE_SIG(s) (SPECIAL_TRAP(s) == 0)

/* Run a trap command for SIG.  SIG is one of the signals the shell treats
   specially.  Returns the exit status of the executed trap command list. */
static int
_run_trap_internal (sig, tag)
     int sig;
     char *tag;
{
  char *trap_command, *old_trap;
  int trap_exit_value, *token_state;
  volatile int save_return_catch_flag, function_code;
  int flags;
  procenv_t save_return_catch;
  WORD_LIST *save_subst_varlist;
  sh_parser_state_t pstate;
#if defined (ARRAY_VARS)
  ARRAY *ps;
#endif

  trap_exit_value = function_code = 0;
  /* Run the trap only if SIG is trapped and not ignored, and we are not
     currently executing in the trap handler. */
  if ((sigmodes[sig] & SIG_TRAPPED) && ((sigmodes[sig] & SIG_IGNORED) == 0) &&
      (trap_list[sig] != (char *)IMPOSSIBLE_TRAP_HANDLER) &&
#if 0
      /* Uncomment this to allow some special signals to recursively execute
	 trap handlers. */
      (RECURSIVE_SIG (sig) || (sigmodes[sig] & SIG_INPROGRESS) == 0))
#else
      ((sigmodes[sig] & SIG_INPROGRESS) == 0))
#endif
    {
      old_trap = trap_list[sig];
      sigmodes[sig] |= SIG_INPROGRESS;
      sigmodes[sig] &= ~SIG_CHANGED;		/* just to be sure */
      trap_command =  savestring (old_trap);

      running_trap = sig + 1;

#if defined (ARRAY_VARS)
      ps = save_pipestatus_array ();
#endif

      save_parser_state (&pstate);
      save_subst_varlist = subst_assign_varlist;
      subst_assign_varlist = 0;

#if defined (JOB_CONTROL)
      if (sig != DEBUG_TRAP)	/* run_debug_trap does this */
	save_pipeline (1);	/* XXX only provides one save level */
#endif

      /* If we're in a function, make sure return longjmps come here, too. */
      save_return_catch_flag = return_catch_flag;
      if (return_catch_flag)
	{
	  COPY_PROCENV (return_catch, save_return_catch);
	  function_code = setjmp_nosigs (return_catch);
	}

      flags = SEVAL_NONINT|SEVAL_NOHIST;
      if (sig != DEBUG_TRAP && sig != RETURN_TRAP && sig != ERROR_TRAP)
	flags |= SEVAL_RESETLINE;
      if (function_code == 0)
	parse_and_execute (trap_command, tag, flags);

      trap_exit_value = last_command_exit_value;

#if defined (JOB_CONTROL)
      if (sig != DEBUG_TRAP)	/* run_debug_trap does this */
	restore_pipeline (1);
#endif

      subst_assign_varlist = save_subst_varlist;
      restore_parser_state (&pstate);

#if defined (ARRAY_VARS)
      restore_pipestatus_array (ps);
#endif
      running_trap = 0;

      sigmodes[sig] &= ~SIG_INPROGRESS;

      if (sigmodes[sig] & SIG_CHANGED)
	{
#if 0
	  /* Special traps like EXIT, DEBUG, RETURN are handled explicitly in
	     the places where they can be changed using unwind-protects.  For
	     example, look at execute_cmd.c:execute_function(). */
	  if (SPECIAL_TRAP (sig) == 0)
#endif
	    free (old_trap);
	  sigmodes[sig] &= ~SIG_CHANGED;
	}

      if (save_return_catch_flag)
	{
	  return_catch_flag = save_return_catch_flag;
	  return_catch_value = trap_exit_value;
	  COPY_PROCENV (save_return_catch, return_catch);
	  if (function_code)
	    longjmp (return_catch, 1);
	}
    }

  return trap_exit_value;
}

int
run_debug_trap ()
{
  int trap_exit_value;
  pid_t save_pgrp;
  int save_pipe[2];

  /* XXX - question:  should the DEBUG trap inherit the RETURN trap? */
  trap_exit_value = 0;
  if ((sigmodes[DEBUG_TRAP] & SIG_TRAPPED) && ((sigmodes[DEBUG_TRAP] & SIG_IGNORED) == 0) && ((sigmodes[DEBUG_TRAP] & SIG_INPROGRESS) == 0))
    {
#if defined (JOB_CONTROL)
      save_pgrp = pipeline_pgrp;
      pipeline_pgrp = 0;
      save_pipeline (1);
#  if defined (PGRP_PIPE)
      save_pgrp_pipe (save_pipe, 1);
#  endif
      stop_making_children ();
#endif

      trap_exit_value = _run_trap_internal (DEBUG_TRAP, "debug trap");

#if defined (JOB_CONTROL)
      pipeline_pgrp = save_pgrp;
      restore_pipeline (1);
#  if defined (PGRP_PIPE)
      close_pgrp_pipe ();
      restore_pgrp_pipe (save_pipe);
#  endif
      if (pipeline_pgrp > 0)
	give_terminal_to (pipeline_pgrp, 1);
      notify_and_cleanup ();
#endif
      
#if defined (DEBUGGER)
      /* If we're in the debugger and the DEBUG trap returns 2 while we're in
	 a function or sourced script, we force a `return'. */
      if (debugging_mode && trap_exit_value == 2 && return_catch_flag)
	{
	  return_catch_value = trap_exit_value;
	  longjmp (return_catch, 1);
	}
#endif
    }
  return trap_exit_value;
}

void
run_error_trap ()
{
  if ((sigmodes[ERROR_TRAP] & SIG_TRAPPED) && ((sigmodes[ERROR_TRAP] & SIG_IGNORED) == 0) && (sigmodes[ERROR_TRAP] & SIG_INPROGRESS) == 0)
    _run_trap_internal (ERROR_TRAP, "error trap");
}

void
run_return_trap ()
{
  int old_exit_value;

#if 0
  if ((sigmodes[DEBUG_TRAP] & SIG_TRAPPED) && (sigmodes[DEBUG_TRAP] & SIG_INPROGRESS))
    return;
#endif

  if ((sigmodes[RETURN_TRAP] & SIG_TRAPPED) && ((sigmodes[RETURN_TRAP] & SIG_IGNORED) == 0) && (sigmodes[RETURN_TRAP] & SIG_INPROGRESS) == 0)
    {
      old_exit_value = last_command_exit_value;
      _run_trap_internal (RETURN_TRAP, "return trap");
      last_command_exit_value = old_exit_value;
    }
}

/* Run a trap set on SIGINT.  This is called from throw_to_top_level (), and
   declared here to localize the trap functions. */
void
run_interrupt_trap ()
{
  _run_trap_internal (SIGINT, "interrupt trap");
}

/* Free all the allocated strings in the list of traps and reset the trap
   values to the default.  Intended to be called from subshells that want
   to complete work done by reset_signal_handlers upon execution of a
   subsequent `trap' command that changes a signal's disposition.  We need
   to make sure that we duplicate the behavior of
   reset_or_restore_signal_handlers and not change the disposition of signals
   that are set to be ignored. */
void
free_trap_strings ()
{
  register int i;

  for (i = 0; i < BASH_NSIG; i++)
    {
      if (trap_list[i] != (char *)IGNORE_SIG)
	free_trap_string (i);
    }
  trap_list[DEBUG_TRAP] = trap_list[EXIT_TRAP] = trap_list[ERROR_TRAP] = trap_list[RETURN_TRAP] = (char *)NULL;
}

/* Free a trap command string associated with SIG without changing signal
   disposition.  Intended to be called from free_trap_strings()  */
static void
free_trap_string (sig)
     int sig;
{
  change_signal (sig, (char *)DEFAULT_SIG);
  sigmodes[sig] &= ~SIG_TRAPPED;
}

/* Reset the handler for SIG to the original value but leave the trap string
   in place. */
static void
reset_signal (sig)
     int sig;
{
  set_signal_handler (sig, original_signals[sig]);
  sigmodes[sig] &= ~SIG_TRAPPED;
}

/* Set the handler signal SIG to the original and free any trap
   command associated with it. */
static void
restore_signal (sig)
     int sig;
{
  set_signal_handler (sig, original_signals[sig]);
  change_signal (sig, (char *)DEFAULT_SIG);
  sigmodes[sig] &= ~SIG_TRAPPED;
}

static void
reset_or_restore_signal_handlers (reset)
     sh_resetsig_func_t *reset;
{
  register int i;

  /* Take care of the exit trap first */
  if (sigmodes[EXIT_TRAP] & SIG_TRAPPED)
    {
      sigmodes[EXIT_TRAP] &= ~SIG_TRAPPED;
      if (reset != reset_signal)
	{
	  free_trap_command (EXIT_TRAP);
	  trap_list[EXIT_TRAP] = (char *)NULL;
	}
    }

  for (i = 1; i < NSIG; i++)
    {
      if (sigmodes[i] & SIG_TRAPPED)
	{
	  if (trap_list[i] == (char *)IGNORE_SIG)
	    set_signal_handler (i, SIG_IGN);
	  else
	    (*reset) (i);
	}
      else if (sigmodes[i] & SIG_SPECIAL)
	(*reset) (i);
    }

  /* Command substitution and other child processes don't inherit the
     debug, error, or return traps.  If we're in the debugger, and the
     `functrace' or `errtrace' options have been set, then let command
     substitutions inherit them.  Let command substitution inherit the
     RETURN trap if we're in the debugger and tracing functions. */
  if (function_trace_mode == 0)
    {
      sigmodes[DEBUG_TRAP] &= ~SIG_TRAPPED;
      sigmodes[RETURN_TRAP] &= ~SIG_TRAPPED;
    }
  if (error_trace_mode == 0)
    sigmodes[ERROR_TRAP] &= ~SIG_TRAPPED;
}

/* Reset trapped signals to their original values, but don't free the
   trap strings.  Called by the command substitution code and other places
   that create a "subshell environment". */
void
reset_signal_handlers ()
{
  reset_or_restore_signal_handlers (reset_signal);
}

/* Reset all trapped signals to their original values.  Signals set to be
   ignored with trap '' SIGNAL should be ignored, so we make sure that they
   are.  Called by child processes after they are forked. */
void
restore_original_signals ()
{
  reset_or_restore_signal_handlers (restore_signal);
}

/* If a trap handler exists for signal SIG, then call it; otherwise just
   return failure.  Returns 1 if it called the trap handler. */
int
maybe_call_trap_handler (sig)
     int sig;
{
  /* Call the trap handler for SIG if the signal is trapped and not ignored. */
  if ((sigmodes[sig] & SIG_TRAPPED) && ((sigmodes[sig] & SIG_IGNORED) == 0))
    {
      switch (sig)
	{
	case SIGINT:
	  run_interrupt_trap ();
	  break;
	case EXIT_TRAP:
	  run_exit_trap ();
	  break;
	case DEBUG_TRAP:
	  run_debug_trap ();
	  break;
	case ERROR_TRAP:
	  run_error_trap ();
	  break;
	default:
	  trap_handler (sig);
	  break;
	}
      return (1);
    }
  else
    return (0);
}

int
signal_is_trapped (sig)
     int sig;
{
  return (sigmodes[sig] & SIG_TRAPPED);
}

int
signal_is_pending (sig)
     int sig;
{
  return (pending_traps[sig]);
}

int
signal_is_special (sig)
     int sig;
{
  return (sigmodes[sig] & SIG_SPECIAL);
}

int
signal_is_ignored (sig)
     int sig;
{
  return (sigmodes[sig] & SIG_IGNORED);
}

int
signal_is_hard_ignored (sig)
     int sig;
{
  return (sigmodes[sig] & SIG_HARD_IGNORE);
}

void
set_signal_hard_ignored (sig)
     int sig;
{
  sigmodes[sig] |= SIG_HARD_IGNORE;
  original_signals[sig] = SIG_IGN;
}

void
set_signal_ignored (sig)
     int sig;
{
  original_signals[sig] = SIG_IGN;
}

int
signal_in_progress (sig)
     int sig;
{
  return (sigmodes[sig] & SIG_INPROGRESS);
}
