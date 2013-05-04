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
