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
#include "serialisable_impl.h"

uint64_t future::lastid = 0;

future::future(futurable_obj &targ, world_time ft, future_id_type id) : target(targ), trigger_time(ft), future_id(id ? id : MakeNewID()) {

}

future::~future() {

}

void future::Execute() {
	ExecuteAction();
}

void future::Deserialise(const deserialiser_input &di, error_collection &ec) {

}

void future::Serialise(serialiser_output &so, error_collection &ec) const {
	SerialiseValueJson(trigger_time, so, "ftime");
	SerialiseValueJson(future_id, so, "fid");
	SerialiseValueJson(GetTypeSerialisationName(), so, "ftype");
}

void future_set::RegisterFuture(const std::shared_ptr<future> &f) {
	futures.insert(std::make_pair(f->GetTriggerTime(), f));
	f->GetTarget().RegisterFuture(f.get());
}

void future_set::RemoveFuture(future &f) {
	f.GetTarget().DeregisterFuture(&f);
	auto range = futures.equal_range(f.GetTriggerTime());
	for(auto it = range.first; it != range.second; ) {
		auto current = it;
		++it;
		//this is so that *current can be erased without invalidating *it

		if(current->second.get() == &f) futures.erase(current);
	}
}

void future_set::ExecuteUpTo(world_time ft) {
	auto past_end = futures.upper_bound(ft);
	for(auto it = futures.begin(); it != past_end; ++it) {
		it->second->Execute();
		it->second->GetTarget().DeregisterFuture(it->second.get());
	}
	futures.erase(futures.begin(), past_end);
}

void futurable_obj::RegisterFuture(future *f) {
	own_futures.emplace_front(f);
}

void futurable_obj::DeregisterFuture(future *f) {
	own_futures.remove_if([&](const future *upf){ return upf == f; });
}

void futurable_obj::ClearFutures() {
	own_futures.clear();
}

void futurable_obj::EnumerateFutures(std::function<void (future &)> f) {
	for(auto it = own_futures.begin(); it != own_futures.end();) {
		auto current = it;
		++it;
		f(**current);
	}
}

void futurable_obj::EnumerateFutures(std::function<void (const future &)> f) const {
	for(auto it = own_futures.begin(); it != own_futures.end();) {
		auto current = it;
		++it;
		f(**current);
	}
}

bool futurable_obj::HaveFutures() const {
	return !own_futures.empty();
}

void serialisable_futurable_obj::DeserialiseFutures(const deserialiser_input &di, error_collection &ec, const future_deserialisation_type_factory &dtf, future_container &fc) {
	deserialiser_input futuresdi(di.json["futures"], "futures", "futures", di);
	if(futuresdi.json.IsArray()) {
		for(rapidjson::SizeType i = 0; i < futuresdi.json.Size(); i++) {
			deserialiser_input subdi(futuresdi.json[i], "", MkArrayRefName(i), futuresdi);
			if(subdi.json.IsObject()) {
				subdi.seenprops.reserve(subdi.json.GetMemberCount());
				
				future_id_type fid;
				world_time ftime;
				if(CheckTransJsonValue(fid, subdi, "fid", ec, true) && CheckTransJsonValue(ftime, subdi, "ftime", ec, true) && CheckTransJsonValue(subdi.type, subdi, "ftype", ec, true)) {
					if(!dtf.FindAndDeserialise(subdi.type, subdi, ec, fc, *this, ftime, fid)) {
						ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation(subdi, string_format("Futures: Unknown future type: %s", subdi.type.c_str()))));
					}
				}
				subdi.PostDeserialisePropCheck(ec);
			}
			else {
				ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation(subdi, "Futures: Expected object")));
			}
		}
	}
	else if(!futuresdi.json.IsNull()) {
		ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation(di, "Futures: Expected an array")));
	}
}

void serialisable_futurable_obj::Serialise(serialiser_output &so, error_collection &ec) const {
	if(HaveFutures()) {
		so.json_out.String("futures");
		so.json_out.StartArray();
		EnumerateFutures([&](const future &f) {
			so.json_out.StartObject();
			f.Serialise(so, ec);
			so.json_out.EndObject();
		});
		so.json_out.EndArray();
	}
}
