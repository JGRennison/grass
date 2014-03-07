//  grass - Generic Rail And Signalling Simulator
//
//  WEBSITE: http://bitbucket.org/JGRennison/grass
//  WEBSITE: http://grss.sourceforge.net
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
#include "core/world.h"
#include "core/error.h"
#include "core/track.h"
#include "core/track_ops.h"
#include "core/trackcircuit.h"
#include "core/action.h"
#include "core/util.h"
#include "core/textpool.h"
#include "core/signal.h"
#include "core/train.h"
#include "core/serialisable_impl.h"
#include <iostream>

world::world() : track_circuits(*this), track_triggers(*this) {
	InitFutureTypes();
}

world::~world() {
}

void world::GameStep(world_time delta) {
	update_set.clear();

	gametime += delta;

	futures.ExecuteUpTo(gametime);

	for(auto it = tick_update_list.begin(); it != tick_update_list.end(); ++it) {
		(*it)->TrackTick();
	}
	for(auto it = all_trains.begin(); it != all_trains.end(); ) {
		auto current = it;
		++it;    //do this as *current may be modified/deleted
		current->TrainTimeStep(delta);
	}
	for(auto &it : update_set) {
		it->UpdateNotification(*this);
	}
}

void world::ConnectTrack(generictrack *track1, EDGETYPE dir1, std::string name2, EDGETYPE dir2, error_collection &ec) {
	auto target_it = all_pieces.find(name2);
	if(target_it == all_pieces.end()) {
		connection_forward_declarations.emplace_back(track1, dir1, name2, dir2);
	}
	else {
		track1->FullConnect(dir1, track_target_ptr(target_it->second.get(), dir2), ec);
	}
}

void world::LayoutInit(error_collection &ec) {
	for(auto it = connection_forward_declarations.begin(); it != connection_forward_declarations.end(); ++it) {
		auto target_it = all_pieces.find(it->name2);
		if(target_it == all_pieces.end()) {
			ec.RegisterNewError<error_trackconnection_notfound>(track_target_ptr(it->track1, it->dir1), it->name2);
		}
		else {
			it->track1->FullConnect(it->dir1, track_target_ptr(target_it->second.get(), it->dir2), ec);
		}
	}
	for(auto it = all_pieces.begin(); it != all_pieces.end(); ++it) {
		it->second->AutoConnections(ec);
	}
	layout_init_final_fixups.Execute(ec);
	for(auto it = all_pieces.begin(); it != all_pieces.end(); ++it) {
		it->second->CheckUnconnectedEdges(ec);
	}
}

void world::PostLayoutInit(error_collection &ec) {
	for(auto it = all_pieces.begin(); it != all_pieces.end(); ++it) {
		it->second->PostLayoutInit(ec);
	}
	post_layout_init_final_fixups.Execute(ec);
}

named_futurable_obj *world::FindFuturableByName(const std::string &name) {
	size_t offset = name.find('/');
	if(offset == std::string::npos) return 0;
	if(name.compare(0, offset, generictrack::GetTypeSerialisationClassNameStatic())) return FindTrackByName(name.substr(offset+1));
	else if(name.compare(0, offset, track_circuit::GetTypeSerialisationClassNameStatic())) return track_circuits.FindOrMakeByName(name.substr(offset+1));
	else if(name.compare(0, offset, track_train_counter_block::GetTypeSerialisationClassNameStatic())) return track_triggers.FindOrMakeByName(name.substr(offset+1));
	else if(name.compare(0, offset, train::GetTypeSerialisationClassNameStatic())) return FindTrainByName(name.substr(offset+1));
	else if(name == this->GetFullSerialisationName()) return this;
	return 0;
}

std::string world::FormatGameTime(world_time wt) const {
	unsigned int s = wt/1000;
	unsigned int m = s/60;
	unsigned int h = m/60;
	return string_format("%02d:%02d:%02d.%03d", h, m%60, s%60, wt%1000);
}

textpool &world::GetUserMessageTextpool() {
	static defaultusermessagepool umtp;
	return umtp;
}

void world::LogUserMessageLocal(LOGCATEGORY lc, const std::string &message) {
	std::cout << message << "\n";
}

void world::InitFutureTypes() {
	MakeFutureTypeWrapper<future_pointsaction>(future_types);
	MakeFutureTypeWrapper<future_genericusermessage_reason>(future_types);
	MakeFutureTypeWrapper<future_genericusermessage>(future_types);
	MakeFutureTypeWrapper<future_reservetrack_base>(future_types);
	MakeFutureTypeWrapper<future_signalflags>(future_types);
	MakeFutureTypeWrapper<future_action_wrapper>(future_types);
}

void world::SubmitAction(const action &request) {
	request.Execute();
}

void world::AddTractionType(std::string name, bool alwaysavailable) {
	traction_types[name].name=name;
	traction_types[name].alwaysavailable=alwaysavailable;
}

traction_type *world::GetTractionTypeByName(std::string name) const {
	auto tt = traction_types.find(name);
	if(tt != traction_types.end()) return const_cast<traction_type *>(&(tt->second));
	else return 0;
}

generictrack *world::FindTrackByName(const std::string &name) const {
	auto it = all_pieces.find(name);
	if(it != all_pieces.end()) {
		if(it->second) return it->second.get();
	}
	return 0;
}

track_train_counter_block *world::FindTrackTrainBlockOrTrackCircuitByName(const std::string &name) {
	track_train_counter_block *res = track_triggers.FindByName(name);
	if(res) return res;
	else return track_circuits.FindByName(name);
}

void world::RegisterTickUpdate(generictrack *targ) {
	tick_update_list.push_back(targ);
}

void world::UnregisterTickUpdate(generictrack *targ) {
	//this does not need to do anything, for now
}

void world::ExecuteIfActionScope(std::function<void()> func) {
	if(IsAuthoritative()) func();
}

void world::CapAllTrackPieceUnconnectedEdges() {
	layout_init_final_fixups.AddFixup([this](error_collection &ec) {
		std::deque<startofline *> newpieces;
		for(auto it = all_pieces.begin(); it != all_pieces.end(); ++it) {
			std::vector<generictrack::edgelistitem> edgelist;
			it->second->GetListOfEdges(edgelist);
			for(auto jt = edgelist.begin(); jt != edgelist.end(); ++jt) {
				if(! jt->target->IsValid()) {
					std::string name = string_format("#edge%d", this->auto_seq_item);
					this->auto_seq_item++;
					startofline *sol = new startofline(*this);
					sol->SetName(name);
					newpieces.push_back(sol);
					sol->FullConnect(EDGE_FRONT, track_target_ptr(it->second.get(), jt->edge), ec);
				}
			}
		}
		for(auto it : newpieces) {
			this->all_pieces[it->GetName()].reset(it);
		}
	});
}

train *world::CreateEmptyTrain() {
	all_trains.emplace_front(*this);
	return &all_trains.front();
}

void world::DeleteTrain(train *t) {
	all_trains.remove_if([&](const train &lt) { return &lt == t; });
}

train *world::FindTrainByName(const std::string &name) const {
	for(auto &it : all_trains) {
		if(it.GetName() == name) return const_cast<train*>(&it);
	}
	return 0;
}

unsigned int world::EnumerateTrains(std::function<void(const train &)> f) const {
	unsigned int count = 0;
	for(auto &it : all_trains) {
		f(it);
		count++;
	}
	return count;
}


vehicle_class *world::FindOrMakeVehicleClassByName(const std::string &name) {
	std::unique_ptr<vehicle_class> &vc = all_vehicle_classes[name];
	if(! vc.get()) vc.reset(new vehicle_class(name));
	return vc.get();
}

vehicle_class *world::FindVehicleClassByName(const std::string &name) {
	auto vcit = all_vehicle_classes.find(name);
	if(vcit == all_vehicle_classes.end()) return 0;
	else return vcit->second.get();
}

void world::MarkUpdated(updatable_obj *wo) {
	update_set.insert(wo);
}

void world::Deserialise(const deserialiser_input &di, error_collection &ec) {
	CheckTransJsonValue(gametime, di, "gametime", ec);
	CheckTransJsonValue(load_count, di, "load_count", ec);
	load_count++;
}

void world::Serialise(serialiser_output &so, error_collection &ec) const {
	SerialiseValueJson(GetTypeSerialisationClassName(), so, "type");
	SerialiseValueJson(gametime, so, "gametime");
	SerialiseValueJson(load_count, so, "load_count");

	serialisable_futurable_obj::Serialise(so, ec);
}
