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
#include "track_ops.h"
#include "serialisable_impl.h"
#include <memory>

void future_pointsaction::ExecuteAction() {
	genericpoints *gp = dynamic_cast<genericpoints *>(&GetTarget());
	if(gp) {
		gp->SetPointFlagsMasked(index, bits, mask);
	}
}

void future_pointsaction::Deserialise(const deserialiser_input &di, error_collection &ec) {
	future::Deserialise(di, ec);
	CheckTransJsonValueDef(index, di, "index", 0, ec);
	CheckTransJsonValueDef(bits, di, "bits", 0, ec);
	CheckTransJsonValueDef(mask, di, "mask", 0, ec);
}

void future_pointsaction::Serialise(serialiser_output &so, error_collection &ec) const {
	future::Serialise(so, ec);
	SerialiseValueJson(index, so, "index");
	SerialiseValueJson(bits, so, "bits");
	SerialiseValueJson(mask, so, "mask");
}

void action_pointsaction::ExecuteAction() {
	if(!target) return;

	unsigned int old_pflags = target->GetPointFlags(index);
	unsigned int change_flags = (old_pflags ^ bits) & mask;

	unsigned immediate_action_bits = 0;
	unsigned immediate_action_mask = 0;

	if(change_flags & genericpoints::PTF_REV) {
		if(old_pflags & genericpoints::PTF_LOCKED) {
			//report error that points are locked
		}
		//else if(reserved) {
			//report error that points are reserved
		//}
		else {
			CancelFutures(index, 0, genericpoints::PTF_OOC);
			immediate_action_bits |= (bits & genericpoints::PTF_REV) | genericpoints::PTF_OOC;
			immediate_action_mask |= genericpoints::PTF_REV | genericpoints::PTF_OOC;

			//code for random failures goes here

			ActionRegisterFuture(std::make_shared<future_pointsaction>(*target, GetPointsMovementCompletionTime(), index, 0, genericpoints::PTF_OOC));
		}
	}
	if(change_flags & genericpoints::PTF_LOCKED) {
		immediate_action_bits |= bits & genericpoints::PTF_LOCKED;
		immediate_action_mask |= genericpoints::PTF_LOCKED;
	}
	if(change_flags & genericpoints::PTF_REMINDER) {
		immediate_action_bits |= bits & genericpoints::PTF_REMINDER;
		immediate_action_mask |= genericpoints::PTF_REMINDER;
	}

	ActionRegisterFuture(std::make_shared<future_pointsaction>(*target, w.GetGameTime() + 1, index, immediate_action_bits, immediate_action_mask));
}

world_time action_pointsaction::GetPointsMovementCompletionTime() {
	return w.GetGameTime() + 5000;
}

void action_pointsaction::CancelFutures(unsigned int index, unsigned int setmask, unsigned int clearmask) {
	auto func = [&](future &f) {
		future_pointsaction *fp = dynamic_cast<future_pointsaction *>(&f);
		if(!fp) return;
		if(fp->index != index) return;
		unsigned int foundset = fp->mask & fp->bits & setmask;
		unsigned int foundunset = fp->mask & (~fp->bits) & clearmask;
		if(foundset || foundunset) {
			unsigned int newmask = fp->mask & ~(foundset | foundunset);
			if(newmask) {
				ActionRegisterFuture(std::make_shared<future_pointsaction>(*target, fp->GetTriggerTime(), index, fp->bits, newmask));
			}
			ActionCancelFuture(f);
		}
	};

	target->EnumerateFutures(func);
}

void action_pointsaction::Deserialise(const deserialiser_input &di, error_collection &ec) {
	CheckTransJsonValueDef(index, di, "index", 0, ec);
	CheckTransJsonValueDef(bits, di, "bits", 0, ec);
	CheckTransJsonValueDef(mask, di, "mask", 0, ec);
}

void action_pointsaction::Serialise(serialiser_output &so, error_collection &ec) const {
	SerialiseValueJson(index, so, "index");
	SerialiseValueJson(bits, so, "bits");
	SerialiseValueJson(mask, so, "mask");
}
