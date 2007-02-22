/*
 * pommed - Apple laptops hotkeys handler daemon
 *
 * $Id$
 *
 * Copyright (C) 2006-2007 Julien BLACHE <jb@jblache.org>
 * Copyright (C) 2006 Yves-Alexis Perez <corsac@corsac.net>
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
#include <syslog.h>
#include <pci/pci.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "../pommed.h"
#include "../conffile.h"
#include "../lcd_backlight.h"
#include "../dbus.h"


#define SYSFS_DRIVER_NONE      0
#define SYSFS_DRIVER_RADEON    1


/* sysfs backlight driver in use */
static int bck_driver = SYSFS_DRIVER_NONE;

/* sysfs actual_brightness node path */
static char *actual_brightness[] =
  {
    "/dev/null",
    "/sys/class/backlight/radeonbl0/actual_brightness"
  };

/* sysfs brightness node path */
static char *brightness[] =
  {
    "/dev/null",
    "/sys/class/backlight/radeonbl0/brightness"
  };

/* sysfs max_brightness node path */
static char *max_brightness[] =
  {
    "/dev/null",
    "/sys/class/backlight/radeonbl0/max_brightness"
  };


struct _lcd_bck_info lcd_bck_info;


static int
sysfs_backlight_get(void)
{
  int fd;
  int n;
  char buffer[4];

  if (bck_driver == SYSFS_DRIVER_NONE)
    return 0;

  fd = open(actual_brightness[bck_driver], O_RDONLY);
  if (fd < 0)
    {
      logmsg(LOG_WARNING, "Could not open sysfs actual_brightness node: %s", strerror(errno));

      return 0;
    }

  n = read(fd, buffer, sizeof(buffer) -1);
  if (n < 1)
    {
      logmsg(LOG_WARNING, "Could not read sysfs actual_brightness node");

      close(fd);
      return 0;
    }
  close(fd);

  return atoi(buffer);
}

static int
sysfs_backlight_get_max(void)
{
  int fd;
  int n;
  char buffer[4];

  if (bck_driver == SYSFS_DRIVER_NONE)
    return 0;

  fd = open(max_brightness[bck_driver], O_RDONLY);
  if (fd < 0)
    {
      logmsg(LOG_WARNING, "Could not open sysfs max_brightness node: %s", strerror(errno));

      return 0;
    }

  n = read(fd, buffer, sizeof(buffer) -1);
  if (n < 1)
    {
      logmsg(LOG_WARNING, "Could not read sysfs max_brightness node");

      close(fd);
      return 0;
    }
  close(fd);

  return atoi(buffer);
}


static void
sysfs_backlight_set(int value)
{
  FILE *fp;

  fp = fopen(brightness[bck_driver], "a");
  if (fp == NULL)
    {
      logmsg(LOG_WARNING, "Could not open sysfs brightness node: %s", strerror(errno));

      return;
    }

  fprintf(fp, "%d", value);

  fclose(fp);
}

void
sysfs_backlight_step(int dir)
{
  int val;
  int newval;

  if (bck_driver == SYSFS_DRIVER_NONE)
    return;

  val = sysfs_backlight_get();

  if (dir == STEP_UP)
    {
      newval = val + lcd_sysfs_cfg.step;

      if (newval > lcd_bck_info.max)
	newval = lcd_bck_info.max;

      logdebug("LCD stepping +%d -> %d\n", lcd_sysfs_cfg.step, newval);
    }
  else if (dir == STEP_DOWN)
    {
      newval = val - lcd_sysfs_cfg.step;

      if (newval < SYSFS_BACKLIGHT_OFF)
	newval = SYSFS_BACKLIGHT_OFF;

      logdebug("LCD stepping -%d -> %d\n", lcd_sysfs_cfg.step, newval);
    }
  else
    return;

  sysfs_backlight_set(newval);

  mbpdbus_send_lcd_backlight(newval, val);

  lcd_bck_info.level = newval;
}


/* We can't fix the config until we know the max backlight value,
 * so, here, fix_config() is static and called at probe time
 */
static void
sysfs_backlight_fix_config(void)
{
  if (lcd_sysfs_cfg.init < 0)
    lcd_sysfs_cfg.init = -1;

  if (lcd_sysfs_cfg.init > lcd_bck_info.max)
    lcd_sysfs_cfg.init = lcd_bck_info.max;

  if (lcd_sysfs_cfg.step < 1)
    lcd_sysfs_cfg.step = 1;

  if (lcd_sysfs_cfg.step > (lcd_bck_info.max / 2))
    lcd_sysfs_cfg.step = lcd_bck_info.max / 2;
}


/* Look for the radeon backlight driver */
int
r9600_sysfs_backlight_probe(void)
{
  if (access(brightness[SYSFS_DRIVER_RADEON], W_OK) != 0)
    {
      logdebug("Failed to access brightness node: %s\n", strerror(errno));
      return -1;
    }

  if (access(actual_brightness[SYSFS_DRIVER_RADEON], R_OK) != 0)
    {
      logdebug("Failed to access actual_brightness node: %s\n", strerror(errno));
      return -1;
    }

  bck_driver = SYSFS_DRIVER_RADEON;

  lcd_bck_info.max = sysfs_backlight_get_max();

  /* Now we can fix the config */
  sysfs_backlight_fix_config();

  /*
   * Set the initial backlight level
   * The value has been sanity checked already
   */
  if (lcd_sysfs_cfg.init > -1)
    {
      sysfs_backlight_set(lcd_sysfs_cfg.init);
    }

  lcd_bck_info.level = sysfs_backlight_get();

    return 0;
}
