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
//  You  should have received a copy of the GNU General Public License
//  along with this program. If not, see <http://www.gnu.org/licenses/>.
//
//  2013 - Jonathan Rennison <j.g.rennison@gmail.com>
//==========================================================================

#include "common.h"
#include "util/error.h"
#include "core/train.h"
#include "core/serialisable_impl.h"
#include "core/deserialisation_scalarconv.h"

void vehicle_class::Deserialise(const deserialiser_input &di, error_collection &ec) {
	CheckTransJsonValueDefProc(length, di, "length", 0, ec, dsconv::Length);
	CheckTransJsonValueDefProc(max_speed, di, "maxspeed", UINT_MAX, ec, dsconv::Speed);
	CheckTransJsonValueDefProc(tractive_force, di, "tractiveforce", 0, ec, dsconv::Force);
	CheckTransJsonValueDefProc(tractive_power, di, "tractivepower", 0, ec, dsconv::Power);
	CheckTransJsonValueDefProc(braking_force, di, "brakingforce", 0, ec, dsconv::Force);
	CheckTransJsonValueDefProc(nominal_rail_traction_limit, di, "tractivelimit", braking_force, ec, dsconv::Force);
	CheckTransJsonValueDefProc(cumul_drag_const, di, "dragc", 0, ec, dsconv::Force);
	CheckTransJsonValueDefProc(cumul_drag_v, di, "dragv", 0, ec, dsconv::ForcePerSpeedCoeff);
	CheckTransJsonValueDefProc(cumul_drag_v2, di, "dragv2", 0, ec, dsconv::ForcePerSpeedSqCoeff);
	CheckTransJsonValueDefProc(face_drag_v2, di, "facedragv2", 0, ec, dsconv::ForcePerSpeedSqCoeff);
	if (CheckTransJsonValueProc(full_mass, di, "mass", ec, dsconv::Mass)) {
		empty_mass = full_mass;
	} else {
		CheckTransJsonValueDefProc(full_mass, di, "full_mass", 0, ec, dsconv::Mass);
		CheckTransJsonValueDefProc(empty_mass, di, "empty_mass", 0, ec, dsconv::Mass);
		if (full_mass < empty_mass) {
			ec.RegisterNewError<error_deserialisation>(di, "Vehicle class: full mass < empty mass");
		}
	}
	if (!length || !full_mass || !empty_mass) {
		ec.RegisterNewError<error_deserialisation>(di, "Vehicle class: length and mass must be non-zero");
	}
	CheckTransJsonSubArray(traction_types, di, "traction_types", "traction_types", ec);
}

void vehicle_class::Serialise(serialiser_output &so, error_collection &ec) const {
}

void train::Deserialise(const deserialiser_input &di, error_collection &ec) {
	world_obj::Deserialise(di, ec);

	auto genericerror = [&](const deserialiser_input &edi, const std::string &message) {
		ec.RegisterNewError<error_deserialisation>(edi, "Invalid train definition: " + message);
	};

	if (!di.w) {
		genericerror(di, "No world");
		return;
	}

	auto add_train_segment = [&](const std::string &veh_typename, unsigned int multiplier, bool reversed, bool full, bool calc_seg_mass, unsigned int seg_mass) {
		if (multiplier == 0) {
			return;
		}
		vehicle_class *vc = di.w->FindVehicleClassByName(veh_typename);
		if (!vc) {
			genericerror(di, "No such vehicle class: " + veh_typename);
			return;
		}
		train_segments.emplace_back();
		train_unit &tu = train_segments.back();
		tu.veh_type = vc;
		if (reversed) {
			tu.stflags |= train_unit::STF::REV;
		}
		if (full) {
			tu.stflags |= train_unit::STF::FULL;
		}
		tu.veh_multiplier = multiplier;
		if (calc_seg_mass) {
			tu.CalculateSegmentMass();
		} else {
			if (seg_mass > vc->full_mass * multiplier || seg_mass < vc->empty_mass * multiplier) {
				genericerror(di, "Segment mass out of range");
			}
			tu.segment_total_mass = seg_mass;
		}
	};

	auto parse_train_segment_val = [&](const deserialiser_input &tsdi, error_collection &ec) {
		if (tsdi.json.IsString()) {
			add_train_segment(tsdi.json.GetString(), 1, false, false, true, 0);
		} else if (tsdi.json.IsObject()) {
			std::string vehclassname;
			if (!CheckTransJsonValue(vehclassname, tsdi, "classname", ec)) {
				genericerror(tsdi, "No class name");
				return;
			}

			unsigned int segment_mass = 0;
			bool got_segment_mass = CheckTransJsonValueProc(segment_mass, tsdi, "segmentmass", ec, dsconv::Mass);

			add_train_segment(vehclassname,
					 CheckGetJsonValueDef<unsigned int>(tsdi, "count", 1, ec),
					 CheckGetJsonValueDef<bool>(tsdi, "reversed", false, ec),
					 CheckGetJsonValueDef<bool>(tsdi, "full", false, ec),
					 !got_segment_mass,
					 segment_mass);

			tsdi.PostDeserialisePropCheck(ec);
		} else {
			genericerror(tsdi, "Invalid type");
		}
	};

	CheckIterateJsonArrayOrValue(di, "vehicleclasses", "vehicleclass", ec, parse_train_segment_val);
	CheckTransJsonValueProc(current_speed, di, "speed", ec, dsconv::Speed);
	CheckTransJsonValue(head_relative_height, di, "head_relative_height", ec);
	CheckTransJsonValue(veh_speed_class, di, "veh_speed_class", ec);
	CheckTransJsonSubArray(active_tractions, di, "activetractions", "traction_types", ec);
	CheckTransJsonValueFlag(tflags, TF::CONSIST_REV_DIR, di, "reverseconsist", ec);
	CheckTransJsonSubObj(la, di, "lookahead", "lookahead", ec, false);

	ValidateActiveTractionSet();

	//todo: timetables, headcode

	CalculateTrainMotionProperties(1 << 8);    // TODO: replace this with the proper value

	track_location newpos;
	newpos.Deserialise("position", di, ec);
	if (newpos.IsValid()) {
		//do this after calculating motion properties (ie. length))
		DropTrainIntoPosition(newpos, ec);
	}
}

void train::Serialise(serialiser_output &so, error_collection &ec) const {
	world_obj::Serialise(so, ec);

	SerialiseObjectArrayContainer(train_segments, so, "vehicleclasses", [&](serialiser_output &so, const train_unit &tu) {
		SerialiseValueJson(tu.veh_type->name, so, "classname");
		SerialiseValueJson(tu.veh_multiplier, so, "count");
		SerialiseFlagJson(tu.stflags, train_unit::STF::FULL, so, "full");
		SerialiseFlagJson(tu.stflags, train_unit::STF::REV, so, "reversed");
		SerialiseValueJson(tu.segment_total_mass, so, "segmentmass");
	});
	SerialiseValueJson(current_speed, so, "speed");
	SerialiseValueJson(head_relative_height - tail_relative_height, so, "head_relative_height");
	SerialiseValueJson(veh_speed_class, so, "veh_speed_class");
	SerialiseSubArrayJson(active_tractions, so, "activetractions", ec);
	SerialiseFlagJson(tflags, TF::CONSIST_REV_DIR, so, "reverseconsist");
	SerialiseSubObjJson(la, so, "lookahead", ec);

	//todo: timetables, headcode
}
