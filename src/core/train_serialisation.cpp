//  grass - Generic Rail And Signalling Simulator
//
//  WEBSITE: http://bitbucket.org/JGRennison/grass
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
#include "train.h"
#include "serialisable_impl.h"
#include "error.h"
#include "deserialisation_scalarconv.h"

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
	if(CheckTransJsonValueProc(fullmass, di, "mass", ec, dsconv::Mass)) {
		emptymass = fullmass;
	}
	else {
		CheckTransJsonValueDefProc(fullmass, di, "fullmass", 0, ec, dsconv::Mass);
		CheckTransJsonValueDefProc(emptymass, di, "emptymass", 0, ec, dsconv::Mass);
		if(fullmass < emptymass) ec.RegisterNewError<error_deserialisation>(di, "Vehicle class: full mass < empty mass");
	}
	if(!length || !fullmass || !emptymass) {
		ec.RegisterNewError<error_deserialisation>(di, "Vehicle class: length and mass must be non-zero");
	}
	CheckTransJsonSubArray(tractiontypes, di, "tractiontypes", "tractiontypes", ec);
}

void vehicle_class::Serialise(serialiser_output &so, error_collection &ec) const {
}

void train::Deserialise(const deserialiser_input &di, error_collection &ec) {
	world_obj::Deserialise(di, ec);

	auto genericerror = [&](const deserialiser_input &edi, const std::string &message) {
		ec.RegisterNewError<error_deserialisation>(edi, "Invalid train definition: " + message);
	};

	if(!di.w) {
		genericerror(di, "No world");
		return;
	}

	auto add_train_segment = [&](const std::string &vehtypename, unsigned int multiplier, bool reversed, bool full, bool calc_seg_mass, unsigned int seg_mass) {
		if(multiplier == 0) return;
		vehicle_class *vc = di.w->FindVehicleClassByName(vehtypename);
		if(!vc) {
			genericerror(di, "No such vehicle class: " + vehtypename);
			return;
		}
		train_segments.emplace_back();
		train_unit &tu = train_segments.back();
		tu.vehtype = vc;
		if(reversed) tu.stflags |= train_unit::STF::REV;
		if(full) tu.stflags |= train_unit::STF::FULL;
		tu.veh_multiplier = multiplier;
		if(calc_seg_mass) tu.CalculateSegmentMass();
		else {
			if(seg_mass > vc->fullmass * multiplier || seg_mass < vc->emptymass * multiplier) {
				genericerror(di, "Segment mass out of range");
			}
			tu.segment_total_mass = seg_mass;
		}
	};

	auto parse_train_segment_val = [&](const deserialiser_input &tsdi, error_collection &ec) {
		if(tsdi.json.IsString()) {
			add_train_segment(tsdi.json.GetString(), 1, false, false, true, 0);
		}
		else if(tsdi.json.IsObject()) {
			std::string vehclassname;
			if(!CheckTransJsonValue(vehclassname, tsdi, "classname", ec)) {
				genericerror(tsdi, "No class name");
				return;
			}

			unsigned int segment_mass;
			bool got_segment_mass = CheckTransJsonValueProc(segment_mass, tsdi, "segmentmass", ec, dsconv::Mass);

			add_train_segment(vehclassname,
					 CheckGetJsonValueDef<unsigned int>(tsdi, "count", 1, ec),
					 CheckGetJsonValueDef<bool>(tsdi, "reversed", false, ec),
					 CheckGetJsonValueDef<bool>(tsdi, "full", false, ec),
					 !got_segment_mass,
					 segment_mass);

			tsdi.PostDeserialisePropCheck(ec);
		}
		else genericerror(tsdi, "Invalid type");
	};

	CheckIterateJsonArrayOrValue(di, "vehicleclasses", "vehicleclass", ec, parse_train_segment_val);
	CheckTransJsonValueProc(current_speed, di, "speed", ec, dsconv::Speed);
	CheckTransJsonValue(head_relative_height, di, "head_relative_height", ec);
	CheckTransJsonValue(vehspeedclass, di, "vehspeedclass", ec);
	CheckTransJsonSubArray(active_tractions, di, "activetractions", "tractiontypes", ec);
	CheckTransJsonValueFlag(tflags, TF::CONSISTREVDIR, di, "reverseconsist", ec);
	CheckTransJsonSubObj(la, di, "lookahead", "lookahead", ec, false);

	ValidateActiveTractionSet();

	track_location newpos;
	newpos.Deserialise("position", di, ec);
	if(newpos.IsValid()) {
		DropTrainIntoPosition(newpos);
	}

	//todo: timetables, headcode

	CalculateTrainMotionProperties(1<<8);	// TODO: replace this with the proper value
}

void train::Serialise(serialiser_output &so, error_collection &ec) const {
	world_obj::Serialise(so, ec);

	SerialiseObjectArrayContainer<std::list<train_unit>, train_unit>(train_segments, so, "vehicleclasses", [&](serialiser_output &so, const train_unit &tu) {
		SerialiseValueJson(tu.vehtype->name, so, "classname");
		SerialiseValueJson(tu.veh_multiplier, so, "count");
		SerialiseFlagJson(tu.stflags, train_unit::STF::FULL, so, "full");
		SerialiseFlagJson(tu.stflags, train_unit::STF::REV, so, "reversed");
		SerialiseValueJson(tu.segment_total_mass, so, "segmentmass");
	});
	SerialiseValueJson(current_speed, so, "speed");
	SerialiseValueJson(head_relative_height - tail_relative_height, so, "head_relative_height");
	SerialiseValueJson(vehspeedclass, so, "vehspeedclass");
	SerialiseSubArrayJson(active_tractions, so, "activetractions", ec);
	SerialiseFlagJson(tflags, TF::CONSISTREVDIR, so, "reverseconsist");
	SerialiseSubObjJson(la, so, "lookahead", ec);

	//todo: timetables, headcode
}
