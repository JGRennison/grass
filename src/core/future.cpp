//  grass - Generic Rail And Signalling Simulator
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version. See: COPYING-GPL.txt
//
//  This program  is distributed in the  hope that it will  be useful, but
//  WITHOUT   ANY  WARRANTY;   without  even   the  implied   warranty  of
//  MERCHANTABILITY  or FITNESS  FOR A  PARTICULAR PURPOSE.   See  the GNU
//  General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program. If not, see <http://www.gnu.org/licenses/>.
//
//  2013 - Jonathan Rennison <j.g.rennison@gmail.com>
//==========================================================================

#include "common.h"
#include "core/future.h"
#include "core/serialisable_impl.h"

future::future(futurable_obj &targ, world_time ft, future_id_type id) : target(targ), trigger_time(ft), future_id(id) {
	assert(id > 0);
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

		if(current->second.get() == &f)
			futures.erase(current);
	}
}

void future_set::ExecuteUpTo(world_time ft) {
	auto past_end = futures.upper_bound(ft);
	std::vector<std::shared_ptr<future> > current_execs;
	for(auto it = futures.begin(); it != past_end; ++it) {
		current_execs.emplace_back(std::move(it->second));
	}
	futures.erase(futures.begin(), past_end);

	//do it this way as futures could themselves insert items into the future set
	for(auto &it : current_execs) {
		it->Execute();
		it->GetTarget().DeregisterFuture(it.get());
	}
}

void futurable_obj::RegisterFuture(future *f) {
	own_futures.emplace_back(f);
}

void futurable_obj::DeregisterFuture(future *f) {
	own_futures.remove_if([&](const future *upf) { return upf == f; });
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

void serialisable_futurable_obj::DeserialiseFutures(const deserialiser_input &di, error_collection &ec,
		const future_deserialisation_type_factory &dtf, future_container &fc) {
	deserialiser_input futuresdi(di.json["futures"], "futures", "futures", di);
	if(futuresdi.json.IsArray()) {
		di.RegisterProp("futures");
		for(rapidjson::SizeType i = 0; i < futuresdi.json.Size(); i++) {
			deserialiser_input subdi(futuresdi.json[i], "", MkArrayRefName(i), futuresdi);
			if(subdi.json.IsObject()) {
				subdi.seenprops.reserve(subdi.json.GetMemberCount());

				future_id_type fid;
				world_time ftime;
				if(CheckTransJsonValue(fid, subdi, "fid", ec, true) && CheckTransJsonValue(ftime, subdi, "ftime", ec, true)
						&& CheckTransJsonValue(subdi.type, subdi, "ftype", ec, true)) {
					if(!dtf.FindAndDeserialise(subdi.type, subdi, ec, fc, *this, ftime, fid)) {
						ec.RegisterNewError<error_deserialisation>(subdi, string_format("Futures: Unknown future type: %s", subdi.type.c_str()));
					}
				}
				subdi.PostDeserialisePropCheck(ec);
			}
			else {
				ec.RegisterNewError<error_deserialisation>(subdi, "Futures: Expected object");
			}
		}
	}
	else if(!futuresdi.json.IsNull()) {
		ec.RegisterNewError<error_deserialisation>(di, "Futures: Expected an array");
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
