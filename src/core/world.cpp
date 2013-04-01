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
#include "world.h"
#include "error.h"
#include "track.h"
#include "track_ops.h"
#include "trackcircuit.h"
#include "action.h"
#include "util.h"
#include "textpool.h"
#include "signal.h"
#include <iostream>

world::world() {
	InitFutureTypes();
}

world::~world() {
}

void world::GameStep(world_time delta) {
	gametime += delta;

	futures.ExecuteUpTo(gametime);

	for(auto it = tick_update_list.begin(); it != tick_update_list.end(); ++it) {
		(*it)->TrackTick();
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
	else if(name.compare(0, offset, track_circuit::GetTypeSerialisationClassNameStatic())) return FindOrMakeTrackCircuitByName(name.substr(offset+1));
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
track_circuit *world::FindOrMakeTrackCircuitByName(const std::string &name) {
	std::unique_ptr<track_circuit> &tc = all_track_circuits[name];
	if(! tc.get()) tc.reset(new track_circuit(*this, name));
	return tc.get();
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
