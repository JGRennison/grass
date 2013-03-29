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

#ifndef INC_TRACKOPS_ALREADY
#define INC_TRACKOPS_ALREADY

#include <deque>
#include "future.h"
#include "serialisable.h"
#include "action.h"
#include "world_ops.h"
#include "points.h"

class genericpoints;
class generictrack;
class genericsignal;
class route;
class routingpoint;
enum class GMRF : unsigned int;
typedef std::deque<routingpoint *> via_list;

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
		ZERO			= 0,
		IGNORERESERVATION	= 1<<0,
		NOOVERLAPSWING		= 1<<1,
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
template<> struct enum_traits< action_pointsaction::APAF > {	static constexpr bool flags = true; };

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
	public:
	action_reservetrack_base(world &w_) : action(w_) { }
	bool TryReserveRoute(const route *rt, world_time action_time, std::function<void(const std::shared_ptr<future> &f)> error_handler) const;
	bool TryUnreserveRoute(routingpoint *startsig, world_time action_time, std::function<void(const std::shared_ptr<future> &f)> error_handler) const;
	bool GenericRouteUnreservation(const route *targrt, routingpoint *targsig, RRF extraflags) const;
	const route *TestSwingOverlapAndReserve(const route *target_route, std::string *failreasonkey = 0) const;
};

class action_routereservetrackop : public action_reservetrack_base {
	protected:
	const route *targetroute;

	public:
	action_routereservetrackop(world &w_, const route *targ) : action_reservetrack_base(w_), targetroute(targ) { }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

class action_reservetrack : public action_routereservetrackop {
	public:
	action_reservetrack(world &w_) : action_routereservetrackop(w_, 0) { }
	action_reservetrack(world &w_, const route &targ) : action_routereservetrackop(w_, &targ) { }
	static std::string GetTypeSerialisationNameStatic() { return "action_reservetrack"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void ExecuteAction() const override;
};

class action_reservepath : public action_reservetrack_base {
	const routingpoint *start;
	const routingpoint *end;
	GMRF gmr_flags;
	RRF extraflags;
	via_list vias;


	public:
	action_reservepath(world &w_) : action_reservetrack_base(w_) { }
	action_reservepath(world &w_, const routingpoint *start_, const routingpoint *end_);
	action_reservepath &SetGmrFlags(GMRF gmr_flags_);
	action_reservepath &SetExtraFlags(RRF extraflags_);
	action_reservepath &SetVias(via_list vias_);
	static std::string GetTypeSerialisationNameStatic() { return "action_reservepath"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void ExecuteAction() const override;
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

class action_unreservetrackroute : public action_routereservetrackop {
	public:
	action_unreservetrackroute(world &w_) : action_routereservetrackop(w_, 0) { }
	action_unreservetrackroute(world &w_, const route &targ) : action_routereservetrackop(w_, &targ) { }
	static std::string GetTypeSerialisationNameStatic() { return "action_unreservetrackroute"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void ExecuteAction() const override;
};

class action_unreservetrack : public action_reservetrack_base {
	const genericsignal *target;

	public:
	action_unreservetrack(world &w_) : action_reservetrack_base(w_), target(0) { }
	action_unreservetrack(world &w_, const genericsignal &targ) : action_reservetrack_base(w_), target(&targ) { }
	static std::string GetTypeSerialisationNameStatic() { return "action_unreservetrack"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void ExecuteAction() const override;
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

#endif