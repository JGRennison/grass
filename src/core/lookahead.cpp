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
#include "core/lookahead.h"
#include "core/track.h"
#include "core/signal.h"
#include "core/train.h"
#include "core/serialisable_impl.h"
#include <climits>

void lookahead::Init(const train *t /* optional */, const track_location &pos, const route *rt) {
	l1_list.clear();
	current_offset = ((uint64_t) 1)<<32;
	ScanAppend(t, pos, 1, rt);
}

void lookahead::Advance(unsigned int distance) {
	current_offset += distance;
	for(auto it = l1_list.begin(); it != l1_list.end();) {
		if(current_offset > it->offset) {
			l1_list.pop_front();
			it = l1_list.begin();
		}
		else {
			for(auto jt = it->l2_list.begin(); jt != it->l2_list.end();) {
				if(current_offset > jt->end_offset) {
					it->l2_list.pop_front();
					jt = it->l2_list.begin();
				}
				else ++jt;
			}
			++it;
		}
	}
}

void lookahead::CheckLookaheads(const train *t /* optional */, const track_location &pos, std::function<void(unsigned int distance, unsigned int speed)> f, std::function<void(LA_ERROR err, const track_target_ptr &piece)> errfunc) {
	unsigned int last_distance = UINT_MAX;
	unsigned int last_speed = UINT_MAX;
	auto sendresponse = [&](unsigned int distance, unsigned int speed) {
		if(speed == UINT_MAX) return;
		if(distance < last_distance || speed < last_speed) {
			f(distance, speed);
			last_distance = distance;
			last_speed = speed;
		}
	};

	for(auto it = l1_list.begin(); it != l1_list.end(); ++it) {
		for(auto jt = it->l2_list.begin(); jt != it->l2_list.end(); ++jt) {
			if(current_offset >= jt->start_offset && current_offset <= jt->end_offset) {
				sendresponse(0, jt->speed);
				if(jt->flags & lookaheaditem::LAI_FLAGS::TRACTION_UNSUITABLE) {
					errfunc(LA_ERROR::TRACTION_UNSUITABLE, jt->piece);
				}
			}
			else if(current_offset < jt->start_offset) {
				if(current_offset < jt->sighting_offset && jt->flags & lookaheaditem::LAI_FLAGS::HALT_UNLESS_VISIBLE) sendresponse(jt->start_offset - current_offset, 0);
				else if(jt->flags & lookaheaditem::LAI_FLAGS::NOT_ALWAYS_PASSABLE && current_offset >= jt->sighting_offset) {
					if(jt->flags & lookaheaditem::LAI_FLAGS::ALLOW_DIFFERENT_CONNECTION_INDEX && jt->connection_index != jt->piece.track->GetCurrentNominalConnectionIndex(jt->piece.direction)) {
						//points have moved, rescan
						Init(t, pos, 0);
						CheckLookaheads(t, pos, f, errfunc);
						return;
					}
					if(jt->piece.track->IsTrackPassable(jt->piece.direction, jt->connection_index)) {
						if(jt->flags & lookaheaditem::LAI_FLAGS::SCAN_BEYOND_IF_PASSABLE) {
							ScanAppend(t, track_location(jt->piece.track->GetConnectingPiece(jt->piece.direction)), 1, 0);
							jt->flags &= ~lookaheaditem::LAI_FLAGS::SCAN_BEYOND_IF_PASSABLE;
						}
						sendresponse(jt->start_offset - current_offset, jt->speed);
					}
					else {
						//track not passable, stop
						sendresponse(jt->start_offset - current_offset, 0);
					}
				}
				else sendresponse(jt->start_offset - current_offset, jt->speed);
			}
		}
		if(current_offset > it->offset) {
			return;
		}
		if(current_offset >= it->sighting_offset && it->gs.IsValid()) {
			//can see signal
			bool reinit = false;
			unsigned int aspect = it->gs.track->GetAspect();
			if(it->last_aspect > 0 && it->gs.track->GetAspectNextTarget() != std::next(it)->gs.track) {
				//signal now points somewhere else
				//this is bad
				errfunc(LA_ERROR::SIG_TARGET_CHANGE, it->gs);
				reinit = true;
			}
			if(aspect < it->last_aspect) {
				//adverse change of aspect
				//this is also bad
				errfunc(LA_ERROR::SIG_ASPECT_LESS_THAN_EXPECTED, it->gs);
				reinit = true;
			}
			if(reinit) {
				while(std::next(it) != l1_list.end()) l1_list.pop_back();
				const route *rt = 0;
				genericsignal *gs = FastSignalCast(l1_list.back().gs.track, l1_list.back().gs.direction);
				if(gs) rt = gs->GetCurrentForwardRoute();
				ScanAppend(t, track_location(l1_list.back().gs), aspect, rt);
			}
			else if(aspect > it->last_aspect) {
				//need to extend lookahead
				for(auto jt = std::next(it); jt != l1_list.end(); ++jt) {
					if(jt->gs.IsValid()) jt->last_aspect = jt->gs.track->GetAspect();
				}
				const route *rt = 0;
				genericsignal *gs = FastSignalCast(l1_list.back().gs.track, l1_list.back().gs.direction);
				if(gs) rt = gs->GetCurrentForwardRoute();
				ScanAppend(t, track_location(l1_list.back().gs), aspect - it->last_aspect, rt);
				it->last_aspect = aspect;
			}
			if(aspect == 0) {
				unsigned int distance = it->offset - current_offset;
				sendresponse(distance, 0);
				if(distance == 0) errfunc(LA_ERROR::WAITING_AT_RED_SIG, it->gs);
			}
		}
		else if(it->gs.IsValid() && it->last_aspect == 0) sendresponse(it->offset - current_offset, 0);
	}
}

void lookahead::ScanAppend(const train *t /* optional */, const track_location &pos, unsigned int blocklimit, const route *rt) {
	uint64_t offset;
	if(! l1_list.empty()) offset = l1_list.back().offset;
	else offset = current_offset;

	l1_list.emplace_back();
	lookaheadroutingpoint *l = &l1_list.back();
	auto newsignal = [&](const vartrack_target_ptr<routingpoint> &sig) {
		l->offset = offset;
		l->gs = sig;
		l->sighting_offset = offset - sig.track->GetSightingDistance(sig.direction);
		blocklimit--;
		l->last_aspect = std::min(blocklimit, sig.track->GetAspect());
	};
	auto newpiece = [&](const track_target_ptr &piece, unsigned int connection_index) {
		l->l2_list.emplace_back();
		lookaheaditem &l2 = l->l2_list.back();
		l2.connection_index = connection_index;
		l2.start_offset = offset;
		l2.sighting_offset = offset - piece.track->GetSightingDistance(piece.direction);
		offset += piece.track->GetLength(piece.direction);
		l2.end_offset = offset;
		l2.piece = piece;

		if(t) {
			const tractionset *ts = piece.track->GetTractionTypes();
			if(ts && !ts->CanTrainPass(t)) {
				l2.speed = 0;
				l2.flags |= lookaheaditem::LAI_FLAGS::TRACTION_UNSUITABLE;
				return;
			}
		}

		const speedrestrictionset *srs = piece.track->GetSpeedRestrictions();
		if(srs) l2.speed = srs->GetTrainTrackSpeedLimit(t);
		else l2.speed = UINT_MAX;
		if(! piece.track->IsTrackAlwaysPassable()) l2.flags |= lookaheaditem::LAI_FLAGS::NOT_ALWAYS_PASSABLE;
	};
	if(rt) {
		if(t && !rt->IsRouteTractionSuitable(t)) {
			l->l2_list.emplace_back();
			lookaheaditem &l2 = l->l2_list.back();
			l2.start_offset = offset;
			l2.end_offset = offset;
			l2.sighting_offset = 0;
			l2.speed = 0;
			l2.flags |= lookaheaditem::LAI_FLAGS::TRACTION_UNSUITABLE;
			l2.piece = rt->start;
		}

		auto it = rt->pieces.begin();
		if(pos.GetTrack() != rt->start.track) {
			for(; it != rt->pieces.end(); ++it) {
				if(pos.GetTrack() == it->location.track) {
					offset -= pos.GetTrack()->GetLength(pos.GetDirection()) - pos.GetTrack()->GetRemainingLength(pos.GetDirection(), pos.GetOffset());
					break;
				}
			}
		}
		for(;it != rt->pieces.end(); ++it) {
			genericsignal *gs = FastSignalCast(it->location.track, it->location.direction);
			if(gs) {
				newsignal(vartrack_target_ptr<routingpoint>(gs, it->location.direction));
				if(blocklimit) {
					l1_list.emplace_back();
					l = &l1_list.back();
				}
				else return;
			}
			else newpiece(it->location, it->connection_index);
		}
		newsignal(rt->end);
		if(blocklimit) {
			const route *next_rt = 0;
			genericsignal *gs = FastSignalCast(rt->end.track, rt->end.direction);
			if(gs) next_rt = gs->GetCurrentForwardRoute();
			ScanAppend(t, track_location(rt->end), blocklimit, next_rt);
		}
	}
	else {
		offset -= pos.GetTrack()->GetLength(pos.GetDirection()) - pos.GetTrack()->GetRemainingLength(pos.GetDirection(), pos.GetOffset());
		track_target_ptr current(pos.GetTrack(), pos.GetDirection());
		while(true) {
			genericsignal *gs = FastSignalCast(current.track, current.direction);
			if(gs) {
				newsignal(vartrack_target_ptr<routingpoint>(gs, current.direction));
				l->last_aspect = 0;
				return;
			}
			if(!current.track->IsTrackAlwaysPassable()) {
				unsigned int connection_index = current.track->GetCurrentNominalConnectionIndex(current.direction);
				newpiece(current, connection_index);
				l->l2_list.back().flags |= lookaheaditem::LAI_FLAGS::ALLOW_DIFFERENT_CONNECTION_INDEX | lookaheaditem::LAI_FLAGS::HALT_UNLESS_VISIBLE;
				if(current_offset < l->l2_list.back().sighting_offset) {    //can't see beyond this
					l->l2_list.back().flags |= lookaheaditem::LAI_FLAGS::SCAN_BEYOND_IF_PASSABLE;
					break;
				}
			}
			else newpiece(current, 0);

			track_target_ptr next = current.track->GetConnectingPiece(current.direction);
			if(next.IsValid()) current = next;
			else {
				routingpoint *rp = FastRoutingpointCast(current.track, current.direction);
				if(rp) newsignal(vartrack_target_ptr<routingpoint>(rp, current.direction));
				break;
			}
		}
		if(!l->gs.IsValid()) {
			if(l->l2_list.empty()) l1_list.pop_back();
			else {
				l->offset = l->l2_list.back().end_offset;    //dummy signal
				l->gs.Reset();
				l->sighting_offset = l->offset;
				l->last_aspect = 0;
			}
		}
	}
}

void lookahead::Deserialise(const deserialiser_input &di, error_collection &ec) {
	l1_list.clear();
	CheckTransJsonValueDef(current_offset, di, "current_offset", 0, ec);
	CheckIterateJsonArrayOrType<json_object>(di, "l1", "l1", ec, [&](const deserialiser_input &edi, error_collection &ec) {
		l1_list.emplace_back();
		lookaheadroutingpoint &l1 = l1_list.back();
		CheckTransJsonValueDef(l1.offset, edi, "offset", 0, ec);
		CheckTransJsonValueDef(l1.sighting_offset, edi, "sighting_offset", 0, ec);
		track_target_ptr ttp;
		ttp.Deserialise("gs", edi, ec);
		l1.gs = vartrack_target_ptr<routingpoint>(FastRoutingpointCast(ttp.track, ttp.direction), ttp.direction);
		CheckTransJsonValueDef(l1.last_aspect, edi, "last_aspect", 0, ec);
		CheckIterateJsonArrayOrType<json_object>(edi, "l2", "l2", ec, [&](const deserialiser_input &fdi, error_collection &ec) {
			l1.l2_list.emplace_back();
			lookaheaditem &l2 = l1.l2_list.back();
			CheckTransJsonValueDef(l2.start_offset, fdi, "start_offset", 0, ec);
			CheckTransJsonValueDef(l2.end_offset, fdi, "end_offset", 0, ec);
			CheckTransJsonValueDef(l2.sighting_offset, fdi, "sighting_offset", 0, ec);
			l2.piece.Deserialise("piece", fdi, ec);
			CheckTransJsonValueDef(l2.connection_index, fdi, "connection_index", 0, ec);
			CheckTransJsonValueDef(l2.flags, fdi, "flags", lookaheaditem::LAI_FLAGS::ZERO, ec);
		}, true);
	}, true);
}

void lookahead::Serialise(serialiser_output &so, error_collection &ec) const {
	SerialiseValueJson(current_offset, so, "current_offset");
	SerialiseObjectArrayContainer(l1_list, so, "l1", [&](serialiser_output &so, const lookaheadroutingpoint &l1) {
		SerialiseValueJson(l1.offset, so, "offset");
		SerialiseValueJson(l1.sighting_offset, so, "sighting_offset");
		track_target_ptr ttp = l1.gs;
		ttp.Serialise("gs", so, ec);
		SerialiseValueJson(l1.last_aspect, so, "last_aspect");
		SerialiseObjectArrayContainer(l1.l2_list, so, "l2", [&](serialiser_output &so, const lookaheaditem &l2) {
			SerialiseValueJson(l2.start_offset, so, "start_offset");
			SerialiseValueJson(l2.end_offset, so, "end_offset");
			SerialiseValueJson(l2.sighting_offset, so, "sighting_offset");
			l2.piece.Serialise("piece", so, ec);
			SerialiseValueJson(l2.connection_index, so, "connection_index");
			SerialiseValueJson(l2.flags, so, "flags");
		});
	});
}

