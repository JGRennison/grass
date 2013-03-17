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

#ifndef INC_TRACKCIRCUIT_ALREADY
#define INC_TRACKCIRCUIT_ALREADY

#include <string>
#include "world_obj.h"

class train;

class track_circuit : public world_obj {
	unsigned int traincount = 0;
	unsigned int tc_flags = 0;

	public:
	enum {
		TCF_FORCEOCCUPIED	= 1<<0,
	};
	track_circuit(world &w_, const std::string &name_) : world_obj(w_) { SetName(name_); }
	track_circuit(world &w_) : world_obj(w_) { }
	void TrainEnter(train *t);
	void TrainLeave(train *t);
	bool Occupied() const { return traincount > 0 || tc_flags & TCF_FORCEOCCUPIED; }
	unsigned int GetTCFlags() const;
	unsigned int SetTCFlagsMasked(unsigned int bits, unsigned int mask);

	virtual std::string GetTypeName() const override { return "Track Circuit"; }
	static std::string GetTypeSerialisationClassNameStatic() { return "trackcircuit"; }
	virtual std::string GetTypeSerialisationClassName() const override { return GetTypeSerialisationClassNameStatic(); }

	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

#endif
