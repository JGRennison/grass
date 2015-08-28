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
//  2015 - Jonathan Rennison <j.g.rennison@gmail.com>
//==========================================================================

#include "test/catch.hpp"
#include "test/world_test.h"
#include "core/serialisable_impl.h"

void world_test::SubmitAction(const action &request) {
	if (round_trip_actions) {
		error_collection ec;

		std::string json;
		writestream wr(json);
		WriterHandler hndl(wr);
		serialiser_output so(hndl);
		so.json_out.StartObject();
		request.Serialise(so, ec);
		so.json_out.EndObject();

		rapidjson::Document dc;
		if (dc.Parse<0>(json.c_str()).HasParseError()) {
			ec.RegisterNewError<error_jsonparse>(json, dc.GetErrorOffset(), dc.GetParseError());
		} else {
			deserialiser_input di(dc, "", "[world_test::SubmitAction]", this, nullptr, nullptr);
			DeserialiseAndSubmitAction(di, ec);
		}

		if (ec.GetErrorCount()) {
			FAIL("Error Collection: " << ec);
		}
	} else {
		world::SubmitAction(request);
	}
}
