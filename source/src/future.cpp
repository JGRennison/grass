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
#include "future.h"

future::future(future_set &fs_, futurable_obj &targ, future_time ft) : fs(fs_), target(targ), trigger_time(ft) {
	targ.RegisterFuture(std::unique_ptr<future>(this));
}

future::~future() {
	if(f_flags & FF_INFS) {
		auto range = fs.futures.equal_range(trigger_time);
		for(auto it = range.first; it != range.second; ) {
			auto current = it;
			++it;
			//this is so that *current can be erased without invalidating *it

			if(current->second == this) fs.futures.erase(current);
		}
		f_flags &= ~FF_INFS;
	}
}

void future_set::ExecuteUpTo(future_time ft) {
	auto past_end = futures.upper_bound(ft);
	for(auto it = futures.begin(); it != past_end; ++it) {
		it->second->Execute();
		it->second->f_flags &= ~future::FF_INFS;
		it->second->target.ExterminateFuture(it->second);
	}
	futures.erase(futures.begin(), past_end);
}

void futurable_obj::RegisterFuture(std::unique_ptr<future> &&f) {
	f->fs.futures.insert(std::make_pair(f->trigger_time, f.get()));
	f->f_flags |= future::FF_INFS;

	own_futures.emplace_front(std::move(f));
}

void futurable_obj::ExterminateFuture(future *f) {
	own_futures.remove_if([&](const std::unique_ptr<future> &upf){ return upf.get() == f; });
}
void futurable_obj::ClearFutures() {
	own_futures.clear();
}