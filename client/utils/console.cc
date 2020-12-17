/*---------------------------------------------------------------------------*\
                          ____  _ _ __ _ __  ___ _ _
                         |_ / || | '_ \ '_ \/ -_) '_|
                         /__|\_, | .__/ .__/\___|_|
                             |__/|_|  |_|
\*---------------------------------------------------------------------------*/

/** \file console.cc
 * Miscellaneous console utilities.
 */


#include <stdlib.h>
#include <unistd.h>
#include <term.h>


namespace snapper
{


    unsigned
    get_screen_width_pure()
    {
	if (!isatty(STDOUT_FILENO))
	    return -1; // no clipping

	int width = 0;

	const char* cols_env = getenv("COLUMNS");
	if (cols_env)
	{
	    width = atoi(cols_env);
	}
	else
	{
	    // use terminfo from ncurses
	    setupterm(NULL, STDOUT_FILENO, NULL);
	    width = tigetnum("cols");

	    /*
	    // use readline
	    rl_initialize();
	    rl_get_screen_size(NULL, &width);
	    */
	}

	// safe default
	if (width <= 0)
	    width = 80;

	return width;
    }


    unsigned
    get_screen_width()
    {
	static unsigned width = get_screen_width_pure();

	return width;
    }

}
