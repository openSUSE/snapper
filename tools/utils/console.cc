/*---------------------------------------------------------------------------*\
                          ____  _ _ __ _ __  ___ _ _
                         |_ / || | '_ \ '_ \/ -_) '_|
                         /__|\_, | .__/ .__/\___|_|
                             |__/|_|  |_|
\*---------------------------------------------------------------------------*/

/** \file console.cc
 * Miscellaneous console utilities.
 */

#include <unistd.h>
#include <string>
#include <fstream>
#include <iostream>
#include <cstdlib>

using namespace std;

unsigned get_screen_width()
{
  if (!::isatty(STDOUT_FILENO))
    return -1; // no clipping

  int width = 80;

  const char *cols_env = getenv("COLUMNS");
  if (cols_env)
    width  = ::atoi (cols_env);

  // safe default
  if (!width)
    width = 80;

  return width;
}
