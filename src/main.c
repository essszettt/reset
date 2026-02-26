/*-----------------------------------------------------------------------------+
|                                                                              |
| filename: main.c                                                             |
| project:  ZX Spectrum Next - RESET                                           |
| author:   Stefan Zell                                                        |
| date:     01/21/2026                                                         |
|                                                                              |
+------------------------------------------------------------------------------+
|                                                                              |
| description:                                                                 |
|                                                                              |
| Application to reset the ZX Spectrum Next                                    |
|                                                                              |
+------------------------------------------------------------------------------+
|                                                                              |
| Copyright (c) 01/21/2026 STZ Engineering                                     |
|                                                                              |
| This software is provided  "as is",  without warranty of any kind, express   |
| or implied. In no event shall STZ or its contributors be held liable for any |
| direct, indirect, incidental, special or consequential damages arising out   |
| of the use of or inability to use this software.                             |
|                                                                              |
| Permission is granted to anyone  to use this  software for any purpose,      |
| including commercial applications,  and to alter it and redistribute it      |
| freely, subject to the following restrictions:                               |
|                                                                              |
| 1. Redistributions of source code must retain the above copyright            |
|    notice, definition, disclaimer, and this list of conditions.              |
|                                                                              |
| 2. Redistributions in binary form must reproduce the above copyright         |
|    notice, definition, disclaimer, and this list of conditions in            |
|    documentation and/or other materials provided with the distribution.      |
|                                                                          ;-) |
+-----------------------------------------------------------------------------*/

/*============================================================================*/
/*                               Includes                                     */
/*============================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arch/zxn.h>
#include <arch/zxn/esxdos.h>

#include "libzxn.h"
#include "reset.h"
#include "version.h"

/*============================================================================*/
/*                               Defines                                      */
/*============================================================================*/
/*!
Default reset mode, used if no argument is given
*/
#define uiRESET_DEFAULT (0x01)

/*============================================================================*/
/*                               Namespaces                                   */
/*============================================================================*/

/*============================================================================*/
/*                               Strukturen                                   */
/*============================================================================*/

/*============================================================================*/
/*                               Typ-Definitionen                             */
/*============================================================================*/

/*============================================================================*/
/*                               Konstanten                                   */
/*============================================================================*/

/*============================================================================*/
/*                               Variablen                                    */
/*============================================================================*/
/*!
Global (static) data of the application.
*/
static struct _state
{
  /*!
  If this flag is set, then this structure is initialized
  */
  bool bInitialized;

  /*!
  Action to execute (help, version, sreenshot, ...)
  */
  action_t eAction;

  /*!
  Backup: Current speed of Z80N
  */
  uint8_t uiCpuSpeed;

  /*!
  Reset mode to execute via NREG 0x02: hard/soft/???
  */
  uint8_t uiMode;

  /*!
  Exitcode of the application, that is handovered to BASIC
  */
  int iExitCode;

} g_tState;

/*============================================================================*/
/*                               Prototypen                                   */
/*============================================================================*/

/*!
Diese Funktion wird einmalig beim Start der Anwendung zur Initialisierung aller
benoetigten Ressourcen aufgerufen.
*/
void _construct(void);

/*!
Diese Funktion wird einmalig beim Beenden der Anwendung zur Freigabe aller
belegten Ressourcen aufgerufen.
*/
void _destruct(void);

/*!
Diese Funktion interpretiert alle Argumente, die der Anwendung uebergeben
wurden.
*/
int parseArguments(int argc, char* argv[]);

/*!
Ausgabe der Hilfe dieser Anwendung.
*/
int showHelp(void);

/*!
Ausgabe der Versionsinformation dieser Anwendung.
*/
int showInfo(void);

/*!
This function resets the system.
*/
int reset(void);

/*============================================================================*/
/*                               Klassen                                      */
/*============================================================================*/

/*============================================================================*/
/*                               Implementierung                              */
/*============================================================================*/

/*----------------------------------------------------------------------------*/
/* _construct()                                                               */
/*----------------------------------------------------------------------------*/
void _construct(void)
{
  if (!g_tState.bInitialized)
  {
    g_tState.eAction    = ACTION_NONE;
    g_tState.iExitCode  = EOK;
    g_tState.uiMode     = uiRESET_DEFAULT;
    g_tState.uiCpuSpeed = zxn_getspeed();

    zxn_setspeed(RTM_28MHZ);
    g_tState.bInitialized  = true;
  }
}


/*----------------------------------------------------------------------------*/
/* _destruct()                                                                */
/*----------------------------------------------------------------------------*/
void _destruct(void)
{
  if (g_tState.bInitialized)
  {
    zxn_setspeed(g_tState.uiCpuSpeed);
    g_tState.bInitialized = false;
  }
}


/*----------------------------------------------------------------------------*/
/* main()                                                                     */
/*----------------------------------------------------------------------------*/
int main(int argc, char* argv[])
{
  _construct();
  atexit(_destruct);

  if (EOK == (g_tState.iExitCode = parseArguments(argc, argv)))
  {
    switch (g_tState.eAction)
    {
      case ACTION_NONE:
        g_tState.iExitCode = EOK;
        break;

      case ACTION_INFO:
        g_tState.iExitCode = showInfo();
        break;

      case ACTION_HELP:
        g_tState.iExitCode = showHelp();
        break;

      case ACTION_RESET:
        g_tState.iExitCode = reset();
        break;
    }
  }

  return (int) (EOK == g_tState.iExitCode ? 0 : zxn_strerror(g_tState.iExitCode));
}


/*----------------------------------------------------------------------------*/
/* parseArguments()                                                           */
/*----------------------------------------------------------------------------*/
int parseArguments(int argc, char* argv[])
{
  int iReturn = EOK;

  g_tState.eAction = ACTION_NONE;

  int i = 1;

  while (i < argc)
  {
    const char* acArg = argv[i];

    if ('-' == acArg[0])
    {
      if ((0 == strcmp(acArg, "-h")) || (0 == stricmp(acArg, "--help")))
      {
        g_tState.eAction = ACTION_HELP;
      }
      else if ((0 == strcmp(acArg, "-v")) || (0 == stricmp(acArg, "--version")))
      {
        g_tState.eAction = ACTION_INFO;
      }
      else if ((0 == strcmp(acArg, "-H")) || (0 == stricmp(acArg, "--hard")))
      {
        g_tState.uiMode = 0x02;
      }
      else if ((0 == strcmp(acArg, "-S")) || (0 == stricmp(acArg, "--soft")))
      {
        g_tState.uiMode = 0x01;
      }
      else if ((0 == strcmp(acArg, "-r")) || (0 == stricmp(acArg, "--reset")))
      {
        if ((i + 1) < argc)
        {
          g_tState.uiMode = ((uint8_t) strtoul(argv[++i], 0, 0));
        }
        else
        {
          fprintf(stderr, "option %s requires a value\n", acArg);
          iReturn = EINVAL;
          break;
        }
      }
      else
      {
        fprintf(stderr, "unknown option: %s\n", acArg);
        iReturn = EINVAL;
        break;
      }
    }
    else
    {
      fprintf(stderr, "unexpected argument: %s\n", acArg);
      iReturn = EINVAL;
      break;
    }

    ++i;
  }

  if (ACTION_NONE == g_tState.eAction)
  {
    g_tState.eAction = ACTION_RESET;
  }

  return iReturn;
}


/*----------------------------------------------------------------------------*/
/* showHelp()                                                                 */
/*----------------------------------------------------------------------------*/
int showHelp(void)
{
  char_t acBuffer[0x10];
  strncpy(acBuffer, VER_INTERNALNAME_STR, sizeof(acBuffer));
  strupr(acBuffer);

  printf("%s\n\n", VER_FILEDESCRIPTION_STR);

  printf("%s [-H][-S][-r x][-h|-v]\n\n", acBuffer);
  //      0.........1.........2.........3.
  printf(" -H[ard]     hardware reset\n");
  printf(" -S[oft]     software reset (*)\n");
  printf(" -r[eset]    special reset \"x\"\n");
  printf(" -h[elp]     print this help\n");
  printf(" -v[ersion]  print version info\n");

  return EOK;
}


/*----------------------------------------------------------------------------*/
/* showInfo()                                                                 */
/*----------------------------------------------------------------------------*/
int showInfo(void)
{
  uint16_t uiVersion;
  char_t   acBuffer[0x10];

  strncpy(acBuffer, VER_INTERNALNAME_STR, sizeof(acBuffer));
  strupr(acBuffer);

  printf("%s " VER_LEGALCOPYRIGHT_STR "\n", acBuffer);

  if (ESX_DOSVERSION_NEXTOS_48K != (uiVersion = esx_m_dosversion()))
  {
    snprintf(acBuffer, sizeof(acBuffer), "NextOS %u.%02u",
             ESX_DOSVERSION_NEXTOS_MAJOR(uiVersion),
             ESX_DOSVERSION_NEXTOS_MINOR(uiVersion));
  }
  else
  {
    strncpy(acBuffer, "48K mode", sizeof(acBuffer));
  }

  //      0.........1.........2.........3.
  printf(" Version %s (%s)\n", VER_FILEVERSION_STR, acBuffer);
  printf(" Version %s (%s)\n", ZXN_VERSION_STR, ZXN_PRODUCTNAME_STR);
  printf(" Stefan Zell (info@diezells.de)\n");

  return EOK;
}


/*----------------------------------------------------------------------------*/
/* reset()                                                                    */
/*----------------------------------------------------------------------------*/
int reset(void)
{
  zxn_reset(g_tState.uiMode);

  return ENOTSUP;
}


/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/
