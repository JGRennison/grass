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

#ifndef INC_FUTURE_ALREADY
#define INC_FUTURE_ALREADY

#include <forward_list>
#include <memory>
#include <map>
#include "serialisable.h"

typedef unsigned int future_time;

class future_set;
class futurable_obj;
class serialisable_futurable_obj;

typedef deserialisation_type_factory<future_set &, serialisable_futurable_obj &, future_time> future_deserialisation_type_factory;

//all futures must be allocated with new
class future : public serialisable_obj {
	friend class future_set;
	friend class futurable_obj;
	future_set &fs;
	futurable_obj &target;
	unsigned int f_flags;
	future_time trigger_time;

	enum {
		FF_INFS		= 1<<0,
	};

	public:
	future(future_set &fs_, futurable_obj &targ, future_time ft);
	virtual ~future();
	virtual void Execute() = 0;
	virtual std::string GetTypeSerialisationName() const = 0;
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec);
	virtual void Serialise(serialiser_output &so, error_collection &ec) const;
	futurable_obj &GetTarget() { return target; } 
	future_set &GetFutureSet() { return fs; }
	future_time GetTriggerTime() { return trigger_time; }
};

class future_set {
	friend class future;
	friend class futurable_obj;
	std::multimap<future_time, future *> futures;

	public:
	void ExecuteUpTo(future_time ft);
};

class serialisable_futurable_obj;

class futurable_obj {
	friend class future;
	friend class serialisable_futurable_obj;

	std::forward_list<std::unique_ptr<future> > own_futures;

	void RegisterFuture(std::unique_ptr<future> &&f);

	public:
	virtual ~futurable_obj() { }
	void ExterminateFuture(future *f);
	void ClearFutures();
};

class serialisable_futurable_obj : public serialisable_obj, public futurable_obj {

	public:
	void DeserialiseFutures(const deserialiser_input &di, error_collection &ec, const future_deserialisation_type_factory &dtf, future_set &fs);
	virtual void Serialise(serialiser_output &so, error_collection &ec) const;
};

#endif