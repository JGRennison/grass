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
	~future();
	virtual void Execute() = 0;
};

class future_set {
	friend class future;
	friend class futurable_obj;
	std::multimap<future_time, future *> futures;

	public:
	void ExecuteUpTo(future_time ft);
};

class futurable_obj {
	friend class future;
	std::forward_list<std::unique_ptr<future> > own_futures;

	void RegisterFuture(std::unique_ptr<future> &&f);

	public:
	void ExterminateFuture(future *f);
	void ClearFutures();
};

#endif