/*
 * mbpeventd - MacBook Pro hotkey handler daemon
 *
 * $Id$
 *
 * Copyright (C) 2006 Julien BLACHE <jb@jblache.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <poll.h>
#include <signal.h>

#include <syslog.h>
#include <stdarg.h>

#include <errno.h>

#include <smbios/SystemInfo.h>

#include <getopt.h>

#include "mbpeventd.h"
#include "kbd_backlight.h"
#include "lcd_backlight.h"
#include "cd_eject.h"
#include "evdev.h"
#include "conffile.h"
#include "audio.h"


/* Machine-specific operations */
struct machine_ops *mops;

/* MacBook Pro machines */

/* MacBookPro1,1 / MacBookPro1,2 (Core Duo) */
struct machine_ops mbp1_ops = {
  .type = MACHINE_MACBOOKPRO_1,
  .lcd_backlight_probe = x1600_backlight_probe,
  .lcd_backlight_step = x1600_backlight_step,
  .evdev_identify = evdev_is_geyser3,
};

/* MacBookPro2,1 / MacBookPro2,2 (Core2 Duo) */
struct machine_ops mbp2_ops = {
  .type = MACHINE_MACBOOKPRO_2,
  .lcd_backlight_probe = x1600_backlight_probe,
  .lcd_backlight_step = x1600_backlight_step,
  .evdev_identify = evdev_is_geyser4,
};


/* MacBook machines */

/* MacBook1,1 (Core Duo) */
struct machine_ops mb1_ops = {
  .type = MACHINE_MACBOOK_1,
  .lcd_backlight_probe = gma950_backlight_probe,
  .lcd_backlight_step = gma950_backlight_step,
  .evdev_identify = evdev_is_geyser3,
};

/* MacBook2,1 (Core2 Duo) */
struct machine_ops mb2_ops = {
  .type = MACHINE_MACBOOK_2,
  .lcd_backlight_probe = gma950_backlight_probe,
  .lcd_backlight_step = gma950_backlight_step,
  .evdev_identify = evdev_is_geyser4,
};


/* debug mode */
#ifndef DEBUG
int debug = 0;
#endif


/* Used by signal handlers */
static int running;


void
logmsg(int level, char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);

  if (debug)
    {
      switch (level)
	{
	  case LOG_INFO:
	    fprintf(stderr, "I: ");
	    break;

	  case LOG_WARNING:
	    fprintf(stderr, "W: ");
	    break;

	  case LOG_ERR:
	    fprintf(stderr, "E: ");
	    break;

	  default:
	    break;
	}
      vfprintf(stderr, fmt, ap);
      fprintf(stderr, "\n");
    }
  else
    {
      vsyslog(level | LOG_DAEMON, fmt, ap);
    }

  va_end(ap);
}


static machine_type
check_machine_smbios(void)
{
  int ret = MACHINE_UNKNOWN;

  const char *prop;

  if (geteuid() != 0)
    {
      logmsg(LOG_ERR, "root privileges needed for SMBIOS machine detection");

      return MACHINE_ERROR;
    }

  /* Check vendor name */
  prop = SMBIOSGetVendorName();
  logdebug("SMBIOS vendor name: [%s]\n", prop);

  if (strcmp(prop, "Apple Computer, Inc.") == 0)
    ret = MACHINE_MAC_UNKNOWN;

  SMBIOSFreeMemory(prop);

  if (ret != MACHINE_MAC_UNKNOWN)
    return ret;

  /* Check system name */
  prop = SMBIOSGetSystemName();
  logdebug("SMBIOS system name: [%s]\n", prop);

  /* Core Duo MacBook Pro 15" (January 2006) & 17" (April 2006) */
  if ((strcmp(prop, "MacBookPro1,1") == 0) || (strcmp(prop, "MacBookPro1,2") == 0))
    ret = MACHINE_MACBOOKPRO_1;
  /* Core2 Duo MacBook Pro 17" & 15" (October 2006) */
  else if ((strcmp(prop, "MacBookPro2,1") == 0) || (strcmp(prop, "MacBookPro2,2") == 0))
    ret = MACHINE_MACBOOKPRO_2;
  /* Core Duo MacBook (May 2006) */
  else if (strcmp(prop, "MacBook1,1") == 0)
    ret = MACHINE_MACBOOK_1;
  /* Core2 Duo MacBook (November 2006) */
  else if (strcmp(prop, "MacBook2,1") == 0)
    ret = MACHINE_MACBOOK_2;
  else
    logmsg(LOG_ERR, "Unknown Apple machine: %s", prop);

  if (ret != MACHINE_MAC_UNKNOWN)
    logmsg(LOG_INFO, "SMBIOS machine check: running on a %s", prop);

  SMBIOSFreeMemory(prop);

  return ret;
}


static void
usage(void)
{
  printf("mbpeventd v" M_VERSION " ($Rev$) MacBook Pro & MacBook hotkeys handler\n");
  printf("Copyright (C) 2006 Julien BLACHE <jb@jblache.org>\n");

  printf("Usage:\n");
  printf("\tmpbeventd\t-- start mbpeventd as a daemon\n");
  printf("\tmbpeventd -v\t-- print version and exit\n");
  printf("\tmbpeventd -f\t-- run in the foreground with debug messages\n");
}


void
sig_int_term_handler(int signal)
{
  running = 0;
}

int
main (int argc, char **argv)
{
  int ret;
  int c;
  int i;

  FILE *pidfile;

  struct pollfd *fds;
  int nfds;

  int reopen;
  machine_type machine;

  struct timeval tv_now;
  struct timeval tv_als;
  struct timeval tv_diff;

  while ((c = getopt(argc, argv, "fv")) != -1)
    {
      switch (c)
	{
	  case 'f':
#ifndef DEBUG
	    debug = 1;
#endif
	    break;

	  case 'v':
	    printf("mbpeventd v" M_VERSION " ($Rev$) MacBook Pro & MacBook hotkeys handler\n");
	    printf("Copyright (C) 2006 Julien BLACHE <jb@jblache.org>\n");

	    exit(0);
	    break;

	  default:
	    usage();

	    exit(-1);
	    break;
	}
    }

  if (!debug)
    {
      openlog("mbpeventd", LOG_PID, LOG_DAEMON);
    }

  logmsg(LOG_INFO, "mbpeventd v" M_VERSION " ($Rev$) MacBook Pro & MacBook hotkeys handler");
  logmsg(LOG_INFO, "Copyright (C) 2006 Julien BLACHE <jb@jblache.org>");

  /* Load our configuration */
  ret = config_load();
  if (ret < 0)
    {
      exit(-1);
    }

  /* Identify the machine we're running on */
  machine = check_machine_smbios();
  switch (machine)
    {
      case MACHINE_MACBOOKPRO_1:
	mops = &mbp1_ops;
	break;

      case MACHINE_MACBOOKPRO_2:
	mops = &mbp2_ops;
	break;

      case MACHINE_MACBOOK_1:
	mops = &mb1_ops;
	break;

      case MACHINE_MACBOOK_2:
	mops = &mb2_ops;
	break;

      case MACHINE_MAC_UNKNOWN:
	logmsg(LOG_ERR, "Unknown Apple machine");

	exit(1);
	break;

      case MACHINE_UNKNOWN:
	logmsg(LOG_ERR, "Unknown non-Apple machine");

	exit(1);
	break;

      default:
	exit(1);
	break;
    }

  ret = mops->lcd_backlight_probe();
  if (ret < 0)
    {
      logmsg(LOG_ERR, "No LCD backlight found");

      exit(1);
    }

  nfds = evdev_open(&fds);
  if (nfds < 1)
    {
      logmsg(LOG_ERR, "No suitable event devices found");

      exit(1);
    }

  if (has_kbd_backlight())
    kbd_backlight_status_init();

  ret = audio_init();
  if (ret < 0)
    {
      logmsg(LOG_WARNING, "Audio initialization failed, audio support disabled");
    }

  if (!debug)
    {
      /*
       * Detach from the console
       */
      if (daemon(0, 0) != 0)
	{
	  logmsg(LOG_ERR, "daemon() failed: %s", strerror(errno));

	  evdev_close(&fds, nfds);

	  exit(-1);
	}
    }

  pidfile = fopen(PIDFILE, "w");
  if (pidfile == NULL)
    {
      logmsg(LOG_WARNING, "Could not open pidfile %s: %s", PIDFILE, strerror(errno));

      evdev_close(&fds, nfds);

      exit(-1);
    }
  fprintf(pidfile, "%d\n", getpid());
  fclose(pidfile);

  gettimeofday(&tv_als, NULL);

  running = 1;
  signal(SIGINT, sig_int_term_handler);
  signal(SIGTERM, sig_int_term_handler);

  reopen = 0;

  while (running)
    {
      /* Attempt to reopen event devices, typically after resuming */
      if (reopen)
	{
	  nfds = evdev_reopen(&fds, nfds);

	  if (nfds < 1)
	    {
	      logmsg(LOG_ERR, "No suitable event devices found (reopen)");

	      break;
	    }

	  reopen = 0;
	}

      ret = poll(fds, nfds, LOOP_TIMEOUT);

      if (ret < 0) /* error */
	{
	  if (errno != EINTR)
	    {
	      logmsg(LOG_ERR, "poll() error: %s", strerror(errno));

	      break;
	    }
	}
      else if (ret != 0)
	{
	  for (i = 0; i < nfds; i++)
	    {
	      /* the event devices cease to exist when suspending */
	      if ((fds[i].revents & POLLERR)
		  || (fds[i].revents & POLLHUP)
		  || (fds[i].revents & POLLNVAL))
		{
		  logmsg(LOG_WARNING, "Error condition signaled on evdev, reopening");
		  reopen = 1;
		  break;
		}

	      if (fds[i].revents & POLLIN)
		evdev_process_events(fds[i].fd);
	    }

	  if (kbd_cfg.auto_on && has_kbd_backlight())
	    {
	      /* is it time to chek the ambient light sensors ? */
	      gettimeofday(&tv_now, NULL);
	      tv_diff.tv_sec = tv_now.tv_sec - tv_als.tv_sec;
	      if (tv_diff.tv_sec < 0)
		tv_diff.tv_sec = 0;

	      if (tv_diff.tv_sec == 0)
		{
		  tv_diff.tv_usec = tv_now.tv_usec - tv_als.tv_usec;
		}
	      else
		{
		  tv_diff.tv_sec--;
		  tv_diff.tv_usec = 1000000 - tv_als.tv_usec + tv_now.tv_usec;
		  tv_diff.tv_usec += tv_diff.tv_sec * 1000000;
		}

	      if (tv_diff.tv_usec >= (1000 * LOOP_TIMEOUT))
		{
		  kbd_backlight_ambient_check();
		  tv_als = tv_now;
		}
	    }
	}
      else if (kbd_cfg.auto_on && has_kbd_backlight())
	{
	  /* poll() timed out, check ambient light sensors */
	  kbd_backlight_ambient_check();
	  gettimeofday(&tv_als, NULL);
	}
    }

  evdev_close(&fds, nfds);
  unlink(PIDFILE);

  config_cleanup();

  logmsg(LOG_INFO, "Exiting");

  if (!debug)
    closelog();

  return 0;
}
