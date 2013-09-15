//  grass - Generic Rail And Signalling Simulator
//
//  WEBSITE: http://bitbucket.org/JGRennison/grass
//  WEBSITE: http://grss.sourceforge.net
//
//  NOTE: This software is licensed under the GPL. See: COPYING-GPL.txt
//
//  This program  is distributed in the  hope that it will  be useful, but
//  WITHOUT   ANY  WARRANTY;   without  even   the  implied   warranty  of
//  MERCHANTABILITY  or FITNESS  FOR A  PARTICULAR PURPOSE.   See  the GNU
//  General Public License for more details.
//
//  Jonathan Rennison (or anybody else) is in no way responsible, or liable
//  for this program or its use in relation to users, 3rd parties or to any
//  persons in any way whatsoever.
//
//  You  should have  received a  copy of  the GNU  General Public
//  License along  with this program; if  not, write to  the Free Software
//  Foundation, Inc.,  59 Temple Place,  Suite 330, Boston,  MA 02111-1307
//  USA
//
//  2013 - Jonathan Rennison <j.g.rennison@gmail.com>
//==========================================================================

#include "common.h"
#include "layout/layout.h"
#include "core/world_serialisation.h"

void world_layout::SetWorldSerialisationLayout(world_serialisation &ws) {
	std::weak_ptr<world_layout> wl_weak = shared_from_this();
	ws.gui_layout_generictrack = [wl_weak](const generictrack *gt, const deserialiser_input &di, error_collection &ec) {
		std::shared_ptr<world_layout> wl = wl_weak.lock();
		if(!wl) return;
	};
	ws.gui_layout_trackberth = [wl_weak](const trackberth *b, const generictrack *gt, const deserialiser_input &di, error_collection &ec) {
		std::shared_ptr<world_layout> wl = wl_weak.lock();
		if(!wl) return;
	};
	ws.gui_layout_guiobject = [wl_weak](const deserialiser_input &di, error_collection &ec) {
		std::shared_ptr<world_layout> wl = wl_weak.lock();
		if(!wl) return;
	};

}
