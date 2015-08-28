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

#ifndef INC_TRACKOPS_ALREADY
#define INC_TRACKOPS_ALREADY

#include <deque>
#include "core/future.h"
#include "core/serialisable.h"
#include "core/action.h"
#include "core/world_ops.h"
#include "core/points.h"
#include "core/routetypes.h"

class generic_points;
class generic_track;
class generic_signal;
class route;
class routing_point;
enum class GMRF : unsigned int;
enum class GSF : unsigned int;
typedef std::vector<routing_point *> via_list;

class action_points_action;

class future_points_action : public future {
	friend action_points_action;

	unsigned int index;
	generic_points::PTF bits;
	generic_points::PTF mask;

	public:
	future_points_action(futurable_obj &targ, world_time ft, future_id_type id) : future(targ, ft, id) { };
	future_points_action(generic_points &targ, world_time ft, unsigned int index_, generic_points::PTF bits_, generic_points::PTF mask_)
			: future(targ, ft, targ.GetWorld().MakeNewFutureID()), index(index_), bits(bits_), mask(mask_) { };
	static std::string GetTypeSerialisationNameStatic() { return "future_points_action"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void ExecuteAction() override;
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

class action_points_action : public action {
	public:
	enum class APAF {
		ZERO                     = 0,
		IGNORE_RESERVATION       = 1<<0,
		NO_OVERLAP_SWING         = 1<<1,
		NO_POINTS_NORMALISE      = 1<<2,
		IS_POINTS_NORMALISE      = 1<<3,
	};

	private:
	generic_points *target;
	unsigned int index;
	generic_points::PTF bits;
	generic_points::PTF mask;
	APAF aflags = APAF::ZERO;

	private:
	void CancelFutures(unsigned int index, generic_points::PTF setmask, generic_points::PTF clearmask) const;
	world_time GetPointsMovementCompletionTime() const;
	bool TrySwingOverlap(std::function<void()> &overlap_callback, bool setting_reverse, std::string *fail_reason_key = nullptr) const;

	public:
	action_points_action(world &w_) : action(w_), target(nullptr) { }
	action_points_action(world &w_, generic_points &targ, unsigned int index_, generic_points::PTF bits_, generic_points::PTF mask_, APAF aflags_ = APAF::ZERO)
			: action(w_), target(&targ), index(index_), bits(bits_), mask(mask_), aflags(aflags_) { }
	action_points_action(world &w_, generic_points &targ, unsigned int index_, bool reverse);
	static std::string GetTypeSerialisationNameStatic() { return "action_points_action"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void ExecuteAction() const override;
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
	inline APAF SetFlagsMasked(APAF flags, APAF mask);

	generic_points *GetTarget() const { return target; }
	unsigned int GetIndex() const { return index; }
	APAF GetFlags() const { return aflags; }
};
template<> struct enum_traits< action_points_action::APAF > { static constexpr bool flags = true; };

inline action_points_action::APAF action_points_action::SetFlagsMasked(action_points_action::APAF flags, action_points_action::APAF mask) {
	aflags = (aflags & (~mask)) | flags;
	return aflags;
}

class action_points_auto_normalise : public action {
	private:
	generic_points *target;
	unsigned int index;

	private:
	world_time GetNormalisationStartTime() const;

	public:
	action_points_auto_normalise(world &w_) : action(w_), target(nullptr) { }
	action_points_auto_normalise(world &w_, generic_points &targ, unsigned int index_)
			: action(w_), target(&targ), index(index_) { }
	static std::string GetTypeSerialisationNameStatic() { return "action_points_auto_normalise"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void ExecuteAction() const override;
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	static void CancelFutures(generic_points *target, unsigned int index);
	static bool HasFutures(generic_points *target, unsigned int index);

	private:
	static bool HandleFuturesGeneric(generic_points *target, unsigned int index, bool cancel);
};

class action_reserve_track_base;

class future_route_operation_base : public future {
	protected:
	const route *reserved_route;

	public:
	future_route_operation_base(futurable_obj &targ, world_time ft, future_id_type id, const route *reserved_route_)
			: future(targ, ft, id), reserved_route(reserved_route_) { }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

class future_reserve_track_base : public future_route_operation_base {
	RRF rflags = RRF::ZERO;

	public:
	future_reserve_track_base(futurable_obj &targ, world_time ft, future_id_type id) : future_route_operation_base(targ, ft, id, nullptr) { };
	future_reserve_track_base(generic_track &targ, world_time ft, const route *reserved_route_, RRF rflags_)
			: future_route_operation_base(targ, ft, targ.GetWorld().MakeNewFutureID(), reserved_route_), rflags(rflags_)  { };
	static std::string GetTypeSerialisationNameStatic() { return "future_reserve_track_base"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void ExecuteAction() override;
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

class future_reserve_track : public future_reserve_track_base {
	public:
	future_reserve_track(generic_track &targ, world_time ft, const route *reserved_route_, RRF rflags_ = RRF::ZERO)
			: future_reserve_track_base(targ, ft, reserved_route_, rflags_ | RRF::RESERVE) { };
};
class future_unreserve_track : public future_reserve_track_base {
	public:
	future_unreserve_track(generic_track &targ, world_time ft, const route *reserved_route_, RRF rflags_ = RRF::ZERO)
			: future_reserve_track_base(targ, ft, reserved_route_, rflags_ | RRF::UNRESERVE) { };
};

class action_reserve_track_base : public action {
	private:
	void CancelApproachLocking(generic_signal *sig) const;

	public:
	action_reserve_track_base(world &w_) : action(w_) { }
	bool TryReserveRoute(const route *rt, world_time action_time, std::function<void(const std::shared_ptr<future> &f)> error_handler) const;
	bool TryUnreserveRoute(routing_point *startsig, world_time action_time, std::function<void(const std::shared_ptr<future> &f)> error_handler) const;
	bool GenericRouteUnreservation(const route *targrt, routing_point *targsig, RRF extra_flags) const;
	const route *TestSwingOverlapAndReserve(const route *target_route, std::string *fail_reason_key = nullptr) const;
};

class action_reserve_track_routeop : public action_reserve_track_base {
	protected:
	const route *targetroute;

	public:
	action_reserve_track_routeop(world &w_, const route *targ) : action_reserve_track_base(w_), targetroute(targ) { }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

class action_reserve_track_sigop : public action_reserve_track_base {
	protected:
	generic_signal *target;

	public:
	action_reserve_track_sigop(world &w_, generic_signal *targ) : action_reserve_track_base(w_), target(targ) { }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

class action_reserve_track : public action_reserve_track_routeop {
	public:
	action_reserve_track(world &w_) : action_reserve_track_routeop(w_, 0) { }
	action_reserve_track(world &w_, const route &targ) : action_reserve_track_routeop(w_, &targ) { }
	static std::string GetTypeSerialisationNameStatic() { return "action_reserve_track"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void ExecuteAction() const override;
};

class action_reserve_path : public action_reserve_track_base {
	const routing_point *start;
	const routing_point *end;
	route_class::set allowed_route_types;
	GMRF gmr_flags;
	RRF extra_flags;
	via_list vias;


	public:
	action_reserve_path(world &w_) : action_reserve_track_base(w_) { }
	action_reserve_path(world &w_, const routing_point *start_, const routing_point *end_);
	action_reserve_path &SetAllowedRouteTypes(route_class::set s);
	action_reserve_path &SetGmrFlags(GMRF gmr_flags_);
	action_reserve_path &SetExtraFlags(RRF extra_flags_);
	action_reserve_path &SetVias(via_list vias_);
	static std::string GetTypeSerialisationNameStatic() { return "action_reserve_path"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void ExecuteAction() const override;
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

//NB: this operation does no approach locking or other safety checks
class action_unreserve_track_route : public action_reserve_track_routeop {
	public:
	action_unreserve_track_route(world &w_) : action_reserve_track_routeop(w_, nullptr) { }
	action_unreserve_track_route(world &w_, const route &targ) : action_reserve_track_routeop(w_, &targ) { }
	static std::string GetTypeSerialisationNameStatic() { return "action_unreserve_track_route"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void ExecuteAction() const override;
};

class action_unreserve_track : public action_reserve_track_sigop {
	public:
	action_unreserve_track(world &w_) : action_reserve_track_sigop(w_, nullptr) { }
	action_unreserve_track(world &w_, generic_signal &targ) : action_reserve_track_sigop(w_, &targ) { }
	static std::string GetTypeSerialisationNameStatic() { return "action_unreserve_track"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void ExecuteAction() const override;
};

class future_signal_flags : public future {
	GSF bits;
	GSF mask;

	public:
	future_signal_flags(futurable_obj &targ, world_time ft, future_id_type id) : future(targ, ft, id) { };
	future_signal_flags(generic_signal &targ, world_time ft, GSF bits_, GSF mask_);
	static std::string GetTypeSerialisationNameStatic() { return "future_signal_flags"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void ExecuteAction() override;
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

class action_approach_locking_timeout : public action_reserve_track_sigop {
	public:
	action_approach_locking_timeout(world &w_) : action_reserve_track_sigop(w_, nullptr) { }
	action_approach_locking_timeout(world &w_, generic_signal &targ) : action_reserve_track_sigop(w_, &targ) { }
	static std::string GetTypeSerialisationNameStatic() { return "action_approach_locking_timeout"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void ExecuteAction() const override;
};

#endif
