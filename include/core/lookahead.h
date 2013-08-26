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

#ifndef INC_LOOKAHEAD_ALREADY
#define INC_LOOKAHEAD_ALREADY

#include <deque>
#include <functional>

#include "track.h"

class generictrack;
class routingpoint;
class train;

class lookahead  : public serialisable_obj {
	struct lookaheaditem {
		enum class LAI_FLAGS {
			ZERO					= 0,
			HALT_UNLESS_VISIBLE			= 1<<0,
			SCAN_BEYOND_IF_PASSABLE			= 1<<1,
			ALLOW_DIFFERENT_CONNECTION_INDEX	= 1<<2,
			TRACTION_UNSUITABLE			= 1<<3,
			NOT_ALWAYS_PASSABLE			= 1<<4,
		};
		uint64_t start_offset;
		uint64_t end_offset;
		uint64_t sighting_offset;
		unsigned int speed;
		track_target_ptr piece;
		unsigned int connection_index = 0;
		LAI_FLAGS flags = LAI_FLAGS::ZERO;
	};
	struct lookaheadroutingpoint {
		uint64_t offset;
		uint64_t sighting_offset;
		vartrack_target_ptr<routingpoint> gs;
		unsigned int last_aspect;
		std::deque<lookaheaditem> l2_list;
	};


	uint64_t current_offset = 0;
	std::deque<lookaheadroutingpoint> l1_list;

	public:
	enum class LA_ERROR {
		NONE,
		SIG_TARGET_CHANGE,
		SIG_ASPECT_LESS_THAN_EXPECTED,
		WAITING_AT_RED_SIG,
                TRACTION_UNSUITABLE,
	};

	void Init(const train *t  /* optional */, const track_location &pos, const route *rt = 0);
	void Advance(unsigned int distance);
	void CheckLookaheads(const train *t  /* optional */, const track_location &pos, std::function<void(unsigned int distance, unsigned int speed)> f, std::function<void(LA_ERROR err, const track_target_ptr &piece)> errfunc);
	void ScanAppend(const train *t  /* optional */, const track_location &pos, unsigned int blocklimit, const route *rt);

	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

template<> struct enum_traits< lookahead::lookaheaditem::LAI_FLAGS > {	static constexpr bool flags = true; };

#endif
