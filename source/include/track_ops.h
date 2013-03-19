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

#include "future.h"
#include "serialisable.h"
#include "action.h"
#include "world_ops.h"
#include "points.h"

class genericpoints;
class generictrack;
class route;
class routingpoint;

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
	genericpoints *target;
	unsigned int index;
	genericpoints::PTF bits;
	genericpoints::PTF mask;

	private:
	void CancelFutures(unsigned int index, genericpoints::PTF setmask, genericpoints::PTF clearmask) const;
	world_time GetPointsMovementCompletionTime() const;

	public:
	action_pointsaction(world &w_) : action(w_), target(0) { }
	action_pointsaction(world &w_, genericpoints &targ, unsigned int index_, genericpoints::PTF bits_, genericpoints::PTF mask_) : action(w_), target(&targ), index(index_), bits(bits_), mask(mask_) { }
	static std::string GetTypeSerialisationNameStatic() { return "action_pointsaction"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void ExecuteAction() const override;
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

class action_reservetrack_base;

class future_routeoperation_base : public future {
	protected:
	const route *reserved_route;

	public:
	future_routeoperation_base(futurable_obj &targ, world_time ft, future_id_type id, const route *reserved_route_) : future(targ, ft, id), reserved_route(reserved_route_) { }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

class future_reservetrack : public future_routeoperation_base {
	public:
	future_reservetrack(futurable_obj &targ, world_time ft, future_id_type id) : future_routeoperation_base(targ, ft, id, 0) { };
	future_reservetrack(futurable_obj &targ, world_time ft, const route *reserved_route_) : future_routeoperation_base(targ, ft, 0, reserved_route_)  { };
	static std::string GetTypeSerialisationNameStatic() { return "future_reservetrack"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void ExecuteAction() override;
};

class future_unreservetrack : public future_routeoperation_base {
	RRF extraflags = RRF::ZERO;

	public:
	future_unreservetrack(futurable_obj &targ, world_time ft, future_id_type id) : future_routeoperation_base(targ, ft, id, 0) { };
	future_unreservetrack(futurable_obj &targ, world_time ft, const route *reserved_route_, RRF extraflags_) : future_routeoperation_base(targ, ft, 0, reserved_route_), extraflags(extraflags_) { };
	static std::string GetTypeSerialisationNameStatic() { return "future_unreservetrack"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void ExecuteAction() override;
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

class action_reservetrack_base : public action {
	public:
	action_reservetrack_base(world &w_) : action(w_) { }
	bool TryReserveRoute(const route *rt, world_time action_time, std::function<void(const std::shared_ptr<future> &f)> error_handler) const;
	bool TryUnreserveRoute(routingpoint *startsig, world_time action_time, std::function<void(const std::shared_ptr<future> &f)> error_handler) const;
	bool GenericRouteUnreservation(const route *targrt, routingpoint *targsig, RRF extraflags) const;
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

class action_unreservetrackroute : public action_routereservetrackop {
	public:
	action_unreservetrackroute(world &w_) : action_routereservetrackop(w_, 0) { }
	action_unreservetrackroute(world &w_, const route &targ) : action_routereservetrackop(w_, &targ) { }
	static std::string GetTypeSerialisationNameStatic() { return "action_unreservetrackroute"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void ExecuteAction() const override;
};

class action_unreservetrack : public action_reservetrack_base {
	const routingpoint *target;

	public:
	action_unreservetrack(world &w_) : action_reservetrack_base(w_), target(0) { }
	action_unreservetrack(world &w_, const routingpoint &targ) : action_reservetrack_base(w_), target(&targ) { }
	static std::string GetTypeSerialisationNameStatic() { return "action_unreservetrack"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void ExecuteAction() const override;
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};
