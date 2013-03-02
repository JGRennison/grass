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

void future::Deserialise(const deserialiser_input &di, error_collection &ec) {

}

void future::Serialise(serialiser_output &so, error_collection &ec) const {
	SerialiseValueJson(trigger_time, so, "ftime");
	SerialiseValueJson(GetTypeSerialisationName(), so, "ftype");
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

void serialisable_futurable_obj::DeserialiseFutures(const deserialiser_input &di, error_collection &ec, const future_deserialisation_type_factory &dtf, future_set &fs) {
	deserialiser_input futuresdi(di.json["futures"], "futures", "futures", di);
	if(futuresdi.json.IsArray()) {
		for(rapidjson::SizeType i = 0; i < futuresdi.json.Size(); i++) {
			deserialiser_input subdi(futuresdi.json[i], "", MkArrayRefName(i), futuresdi);
			if(subdi.json.IsObject()) {
				subdi.seenprops.reserve(subdi.json.GetMemberCount());
				
				const rapidjson::Value &timeval = subdi.json["ftime"];
				if(IsType<future_time>(timeval)) {
					const rapidjson::Value &typeval = subdi.json["ftype"];
					if(typeval.IsString()) {
						subdi.type.assign(typeval.GetString(), typeval.GetStringLength());
						subdi.RegisterProp("ftype");
						if(!dtf.FindAndDeserialise(subdi.type, subdi, ec, fs, *this, GetType<future_time>(timeval))) {
							ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation(subdi, string_format("LoadGame: Unknown future type: %s", subdi.type.c_str()))));
						}
					}
					else {
						ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation(subdi, "Futures: Object has no type")));
					}
				}
				else {
					ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation(subdi, "Futures: Object has no time")));
				}
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
	if(!own_futures.empty()) {
		so.json_out.String("futures");
		so.json_out.StartArray();
		for(auto it = own_futures.begin(); it != own_futures.end(); ++it) {
			so.json_out.StartObject();
			(*it)->Serialise(so, ec);
			so.json_out.EndObject();
		}
		so.json_out.EndArray();
	}
}
