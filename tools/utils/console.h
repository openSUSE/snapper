/*---------------------------------------------------------------------------*\
                          ____  _ _ __ _ __  ___ _ _
                         |_ / || | '_ \ '_ \/ -_) '_|
                         /__|\_, | .__/ .__/\___|_|
                             |__/|_|  |_|
\*---------------------------------------------------------------------------*/

/** \file console.h
 * Miscellaneous console utilities.
 */

#ifndef CONSOLE_H_
#define CONSOLE_H_

/**
 * Reads COLUMNS environment variable or gets the screen width from readline,
 * in that order. Falls back to 80 if all that fails.
 *
 * \NOTE In case stdout is not connected to a terminal max. unsigned
 * is returned. This should prevent clipping when output is redirected.
 */
unsigned get_screen_width();

#endif /* CONSOLE_H_ */
