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
#include "lookahead.h"
#include "track.h"
#include "signal.h"
#include "train.h"
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
			}
			else if(current_offset < jt->start_offset) {
				if(current_offset < jt->sighting_offset && jt->flags & lookaheaditem::LAI_FLAGS::HALT_UNLESS_VISIBLE) sendresponse(jt->start_offset - current_offset, 0);
				else if(jt->piece.IsValid() && current_offset >= jt->sighting_offset) {
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
			if(aspect == 0) sendresponse(it->offset - current_offset, 0);
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
		const speedrestrictionset *srs = piece.track->GetSpeedRestrictions();
		if(srs) l2.speed = srs->GetTrainTrackSpeedLimit(t);
		else l2.speed = UINT_MAX;
		if(! piece.track->IsTrackAlwaysPassable()) l2.piece = piece;
	};
	if(rt) {
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
				if(current_offset < l->l2_list.back().sighting_offset) {	//can't see beyond this
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
				l->offset = l->l2_list.back().end_offset;	//dummy signal
				l->gs.Reset();
				l->sighting_offset = l->offset;
				l->last_aspect = 0;
			}
		}
	}
}