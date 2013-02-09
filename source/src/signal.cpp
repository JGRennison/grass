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

#include "../include/common.h"
#include "../include/signal.h"

#include <cassert>

bool genericsignal::HalfConnect(DIRTYPE this_entrance_direction, const track_target_ptr &target_entrance) {
	switch(this_entrance_direction) {
		case TDIR_FORWARD:
			return TryConnectPiece(prev, target_entrance);
		case TDIR_REVERSE:
			return TryConnectPiece(next, target_entrance);
		default:
			assert(false);
			return false;
	}
}

const track_target_ptr & genericsignal::GetConnectingPiece(DIRTYPE direction) const {
	switch(direction) {
		case TDIR_FORWARD:
			return next;
		case TDIR_REVERSE:
			return prev;
		default:
			assert(false);
			return empty_track_target;
	}
}

void genericsignal::TrainEnter(DIRTYPE direction, train *t) { }
void genericsignal::TrainLeave(DIRTYPE direction, train *t) { }

unsigned int genericsignal::GetMaxConnectingPieces(DIRTYPE direction) const {
	return 1;
}

const track_target_ptr & genericsignal::GetConnectingPieceByIndex(DIRTYPE direction, unsigned int index) const {
	return GetConnectingPiece(direction);
}

unsigned int genericsignal::GetSignalFlags() const {
	return sflags;
}

unsigned int genericsignal::SetSignalFlagsMasked(unsigned int set_flags, unsigned int mask_flags) {
	sflags = (sflags & (~mask_flags)) | set_flags;
	return sflags;
}