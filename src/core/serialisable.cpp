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

#include "common.h"
#include "util/error.h"
#include "util/utf8.h"
#include "core/serialisable_impl.h"
#include <algorithm>

error_deserialisation::error_deserialisation(const deserialiser_input &di, const std::string &str) {
	std::function<unsigned int (const deserialiser_input *, unsigned int)> f= [&](const deserialiser_input *des, unsigned int counter) {
		if(des) {
			counter = f(des->parent, counter);
			msg << "\n\t" << counter << ": " << des->reference_name;
			if(!des->type.empty()) msg << ", Type: " << des->type;
			if(des->json.IsObject()) {
				const rapidjson::Value &nameval=des->json["name"];
				if(IsType<std::string>(nameval)) msg << ", Name: " << GetType<std::string>(nameval);
			}
			counter++;
		}
		return counter;
	};

	msg << "JSON deserialisation error: " << str;
	f(&di, 0);
}
error_deserialisation::error_deserialisation(const std::string &str) {
	msg << "JSON deserialisation error: " << str;
}

error_jsonparse::error_jsonparse(const std::string &json, size_t erroroffset, const char *parseerror) {
	size_t linestart = 0;
	size_t lineend = 0;
	size_t lineno = GetLineNumberOfStringOffset(json, erroroffset, &linestart, &lineend);
	size_t reallinestart = linestart;

	bool trail_start = false;
	bool trail_end = false;
	size_t linelen = strboundedlen_utf8(json, linestart, lineend);
	if(linelen > 77) {
		if(strboundedlen_utf8(json, linestart, erroroffset) <= 37) {
			//Trim end of string
			trail_end = true;
			lineend = stroffset_utf8(json, linestart, 71);
		}
		else if(strboundedlen_utf8(json, erroroffset, lineend) <= 36) {
			//Trim start of string
			trail_start = true;
			linestart = stroffset_utf8(json, linestart, linelen - 71);
		}
		else {
			//Trim both ends of string
			lineend = stroffset_utf8(json, erroroffset, 36);
			linestart = stroffset_utf8(json, erroroffset, -37);
			trail_start = trail_end = true;
		}
	}

	msg << string_format("JSON Parsing error at offset: %d (line: %d, column: %d), Error: %s\n", erroroffset, lineno, strboundedlen_utf8(json, reallinestart, erroroffset) + 1, parseerror);
	if(trail_start) msg << "...";
	msg << "\"";
	msg << json.substr(linestart, lineend-linestart);
	msg << "\"";
	if(trail_end) msg << "...";
	msg << "\n";
	msg << std::string(1 + strboundedlen_utf8(json, linestart, erroroffset) + (trail_start?3:0), '.');
	msg << "^\n";
}

void serialisable_obj::DeserialiseObject(const deserialiser_input &di, error_collection &ec) {
	di.seenprops.reserve(di.json.GetMemberCount());
	if(di.objpreparse) DeserialiseObject(*di.objpreparse, ec);
	Deserialise(di, ec);
	if(di.objpostparse) DeserialiseObject(*di.objpostparse, ec);
	di.PostDeserialisePropCheck(ec);
}

void deserialiser_input::PostDeserialisePropCheck(error_collection &ec) const {
	if(seenprops.size() == json.GetMemberCount()) return;
	else {
		for(auto it = json.MemberBegin(); it != json.MemberEnd(); ++it) {
			if(std::find_if(seenprops.begin(), seenprops.end(), [&](const char *& s) { return strcmp(it->name.GetString(), s) == 0; }) == seenprops.end()) {
				ec.RegisterNewError<error_deserialisation>(*this, string_format("Unknown object property: \"%s\"", it->name.GetString()));
			}
		}
	}
}

deserialiser_input *deserialiser_input::Clone() const {
	deserialiser_input *out = new deserialiser_input(type, reference_name, json, w, ws, parent);
	out->seenprops = seenprops;
	out->objpreparse = objpreparse;
	out->objpostparse = objpostparse;
	return out;
}

//The purpose of this function is *not* to clone the parse tree *downwards*, but to clone the *deserialiser_input ancestry* from the given point *upwards*
//ie. to enable a copy of *this to be made, which his still usable once this->parent, etc. go out of scope, whilst still maintaining a trace stack.
std::shared_ptr<deserialiser_input::deserialiser_input_clone_with_ancestors> deserialiser_input::CloneWithAncestors() const {
	std::vector< std::unique_ptr<deserialiser_input> > items;
	std::function<deserialiser_input *(const deserialiser_input *)> clone = [&](const deserialiser_input *in) -> deserialiser_input * {
		deserialiser_input *out =  in->Clone();
		items.emplace_back(out);
		if(out->parent) out->parent = clone(out->parent);
		if(out->objpreparse) out->objpreparse = clone(out->objpreparse);
		if(out->objpostparse) out->objpostparse = clone(out->objpostparse);
		return out;
	};
	deserialiser_input *top = clone(this);
	return std::make_shared<deserialiser_input::deserialiser_input_clone_with_ancestors>(top, std::move(items));
}

bool CheckIterateJsonArrayOrValue(const deserialiser_input &di, const char *prop, const std::string &type_name, error_collection &ec, std::function<void(const deserialiser_input &di, error_collection &ec)> func, bool arrayonly) {
	deserialiser_input subdi(di.json[prop], type_name, prop, di);
	if(!subdi.json.IsNull()) di.RegisterProp(prop);
	else return false;

	if(subdi.json.IsArray()) {
		subdi.type += "_array";
		for(rapidjson::SizeType i = 0; i < subdi.json.Size(); i++) {
			func(deserialiser_input(subdi.json[i], type_name, MkArrayRefName(i), subdi), ec);
		}
		return true;
	}
	else if(!arrayonly) {
		func(subdi, ec);
		return true;
	}
	else {
		CheckJsonTypeAndReportError<json_array>(subdi, subdi.reference_name.c_str(), subdi.json, ec);
		return false;
	}

}
