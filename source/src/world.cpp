//  grass - Generic Rail And Signalling Simulator
//
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
#include "world.h"
#include "error.h"

void world::ConnectTrack(generictrack *track1, DIRTYPE dir1, std::string name2, DIRTYPE dir2, error_collection &ec) {
	auto target_it = all_pieces.find(name2);
	if(target_it == all_pieces.end()) {
		connection_forward_declarations.emplace_back(track1, dir1, name2, dir2);
	}
	else {
		track1->FullConnect(dir1, track_target_ptr(target_it->second.get(), dir2), ec);
	}
}

void world::ConnectAllPiecesInit(error_collection &ec) {
	for(auto it = connection_forward_declarations.begin(); it != connection_forward_declarations.end(); ++it) {
		auto target_it = all_pieces.find(it->name2);
		if(target_it == all_pieces.end()) {
			//error
		}
		else {
			it->track1->FullConnect(it->dir1, track_target_ptr(target_it->second.get(), it->dir2), ec);
		}
	}
}

void world::PostLayoutInit(error_collection &ec) {
	for(auto it = all_pieces.begin(); it != all_pieces.end(); ++it) {
		it->second->PostLayoutInit(ec);
	}
}
