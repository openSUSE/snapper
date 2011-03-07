/* Y2CCSnapperAgent.cc
 *
 * Authors: Jiri Suchomel <jsuchome@suse.cz>
 *
 */

#include <scr/Y2AgentComponent.h>
#include <scr/Y2CCAgentComponent.h>

#include "SnapperAgent.h"

typedef Y2AgentComp <SnapperAgent> Y2SnapperAgentComp;

Y2CCAgentComp <Y2SnapperAgentComp> g_y2ccag_snapper ("ag_snapper");
