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

#ifndef INC_TRACKOPS_ALREADY
#define INC_TRACKOPS_ALREADY

#include <deque>
#include "core/future.h"
#include "core/serialisable.h"
#include "core/action.h"
#include "core/world_ops.h"
#include "core/points.h"
#include "core/routetypes.h"

class genericpoints;
class generictrack;
class genericsignal;
class route;
class routingpoint;
enum class GMRF : unsigned int;
enum class GSF : unsigned int;
typedef std::vector<routingpoint *> via_list;

class action_pointsaction;

class future_pointsaction : public future {
	friend action_pointsaction;

	unsigned int index;
	genericpoints::PTF bits;
	genericpoints::PTF mask;

	public:
	future_pointsaction(futurable_obj &targ, world_time ft, future_id_type id) : future(targ, ft, id) { };
	future_pointsaction(futurable_obj &targ, world_time ft, unsigned int index_, genericpoints::PTF bits_, genericpoints::PTF mask_) : future(targ, ft, 0), index(index_), bits(bits_), mask(mask_) { };
	static std::string GetTypeSerialisationNameStatic() { return "future_pointsaction"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void ExecuteAction() override;
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

class action_pointsaction : public action {
	public:
	enum class APAF {
		ZERO                     = 0,
		IGNORERESERVATION        = 1<<0,
		NOOVERLAPSWING           = 1<<1,
	};

	private:
	genericpoints *target;
	unsigned int index;
	genericpoints::PTF bits;
	genericpoints::PTF mask;
	APAF aflags = APAF::ZERO;

	private:
	void CancelFutures(unsigned int index, genericpoints::PTF setmask, genericpoints::PTF clearmask) const;
	world_time GetPointsMovementCompletionTime() const;
	bool TrySwingOverlap(std::function<void()> &overlap_callback, bool settingreverse, std::string *failreasonkey = 0) const;

	public:
	action_pointsaction(world &w_) : action(w_), target(0) { }
	action_pointsaction(world &w_, genericpoints &targ, unsigned int index_, genericpoints::PTF bits_, genericpoints::PTF mask_, APAF aflags_ = APAF::ZERO) : action(w_), target(&targ), index(index_), bits(bits_), mask(mask_), aflags(aflags_) { }
	static std::string GetTypeSerialisationNameStatic() { return "action_pointsaction"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void ExecuteAction() const override;
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
	inline APAF SetFlagsMasked(APAF flags, APAF mask);
};
template<> struct enum_traits< action_pointsaction::APAF > { static constexpr bool flags = true; };

inline action_pointsaction::APAF action_pointsaction::SetFlagsMasked(action_pointsaction::APAF flags, action_pointsaction::APAF mask) { aflags = (aflags & (~mask)) | flags; return aflags; }

class action_reservetrack_base;

class future_routeoperation_base : public future {
	protected:
	const route *reserved_route;

	public:
	future_routeoperation_base(futurable_obj &targ, world_time ft, future_id_type id, const route *reserved_route_) : future(targ, ft, id), reserved_route(reserved_route_) { }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

class future_reservetrack_base : public future_routeoperation_base {
	RRF rflags = RRF::ZERO;

	public:
	future_reservetrack_base(futurable_obj &targ, world_time ft, future_id_type id) : future_routeoperation_base(targ, ft, id, 0) { };
	future_reservetrack_base(futurable_obj &targ, world_time ft, const route *reserved_route_, RRF rflags_) : future_routeoperation_base(targ, ft, 0, reserved_route_), rflags(rflags_)  { };
	static std::string GetTypeSerialisationNameStatic() { return "future_reservetrack_base"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void ExecuteAction() override;
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

class future_reservetrack : public future_reservetrack_base {
	public:
	future_reservetrack(futurable_obj &targ, world_time ft, const route *reserved_route_, RRF rflags_ = RRF::ZERO) : future_reservetrack_base(targ, ft, reserved_route_, rflags_ | RRF::RESERVE) { };
};
class future_unreservetrack : public future_reservetrack_base {
	public:
	future_unreservetrack(futurable_obj &targ, world_time ft, const route *reserved_route_, RRF rflags_ = RRF::ZERO) : future_reservetrack_base(targ, ft, reserved_route_, rflags_ | RRF::UNRESERVE) { };
};

class action_reservetrack_base : public action {
	private:
	void CancelApproachLocking(genericsignal *sig) const;

	public:
	action_reservetrack_base(world &w_) : action(w_) { }
	bool TryReserveRoute(const route *rt, world_time action_time, std::function<void(const std::shared_ptr<future> &f)> error_handler) const;
	bool TryUnreserveRoute(routingpoint *startsig, world_time action_time, std::function<void(const std::shared_ptr<future> &f)> error_handler) const;
	bool GenericRouteUnreservation(const route *targrt, routingpoint *targsig, RRF extraflags) const;
	const route *TestSwingOverlapAndReserve(const route *target_route, std::string *failreasonkey = 0) const;
};

class action_reservetrack_routeop : public action_reservetrack_base {
	protected:
	const route *targetroute;

	public:
	action_reservetrack_routeop(world &w_, const route *targ) : action_reservetrack_base(w_), targetroute(targ) { }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

class action_reservetrack_sigop : public action_reservetrack_base {
	protected:
	genericsignal *target;

	public:
	action_reservetrack_sigop(world &w_, genericsignal *targ) : action_reservetrack_base(w_), target(targ) { }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

class action_reservetrack : public action_reservetrack_routeop {
	public:
	action_reservetrack(world &w_) : action_reservetrack_routeop(w_, 0) { }
	action_reservetrack(world &w_, const route &targ) : action_reservetrack_routeop(w_, &targ) { }
	static std::string GetTypeSerialisationNameStatic() { return "action_reservetrack"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void ExecuteAction() const override;
};

class action_reservepath : public action_reservetrack_base {
	const routingpoint *start;
	const routingpoint *end;
	route_class::set allowed_route_types;
	GMRF gmr_flags;
	RRF extraflags;
	via_list vias;


	public:
	action_reservepath(world &w_) : action_reservetrack_base(w_) { }
	action_reservepath(world &w_, const routingpoint *start_, const routingpoint *end_);
	action_reservepath &SetAllowedRouteTypes(route_class::set s);
	action_reservepath &SetGmrFlags(GMRF gmr_flags_);
	action_reservepath &SetExtraFlags(RRF extraflags_);
	action_reservepath &SetVias(via_list vias_);
	static std::string GetTypeSerialisationNameStatic() { return "action_reservepath"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void ExecuteAction() const override;
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

//NB: this operation does no approach locking or other safety checks
class action_unreservetrackroute : public action_reservetrack_routeop {
	public:
	action_unreservetrackroute(world &w_) : action_reservetrack_routeop(w_, 0) { }
	action_unreservetrackroute(world &w_, const route &targ) : action_reservetrack_routeop(w_, &targ) { }
	static std::string GetTypeSerialisationNameStatic() { return "action_unreservetrackroute"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void ExecuteAction() const override;
};

class action_unreservetrack : public action_reservetrack_sigop {
	public:
	action_unreservetrack(world &w_) : action_reservetrack_sigop(w_, 0) { }
	action_unreservetrack(world &w_, genericsignal &targ) : action_reservetrack_sigop(w_, &targ) { }
	static std::string GetTypeSerialisationNameStatic() { return "action_unreservetrack"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void ExecuteAction() const override;
};

class future_signalflags : public future {
	GSF bits;
	GSF mask;

	public:
	future_signalflags(futurable_obj &targ, world_time ft, future_id_type id) : future(targ, ft, id) { };
	future_signalflags(futurable_obj &targ, world_time ft, GSF bits_, GSF mask_) : future(targ, ft, 0), bits(bits_), mask(mask_) { };
	static std::string GetTypeSerialisationNameStatic() { return "future_signalflags"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void ExecuteAction() override;
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

class action_approachlockingtimeout : public action_reservetrack_sigop {
	public:
	action_approachlockingtimeout(world &w_) : action_reservetrack_sigop(w_, 0) { }
	action_approachlockingtimeout(world &w_, genericsignal &targ) : action_reservetrack_sigop(w_, &targ) { }
	static std::string GetTypeSerialisationNameStatic() { return "action_approachlockingtimeout"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void ExecuteAction() const override;
};

#endif
