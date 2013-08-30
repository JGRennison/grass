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
#include "serialisable_impl.h"
#include "error.h"
#include "utf8.h"
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
		if(strboundedlen_utf8(json, erroroffset, lineend) <= 36) {
			trail_start = true;
			linestart = stroffset_utf8(json, linestart, linelen - 71);
		}
		else if(strboundedlen_utf8(json, linestart, erroroffset) <= 37) {
			trail_end = true;
			lineend = stroffset_utf8(json, linestart, -(linelen - 71));
		}
		else {
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

void serialisable_obj::DeserialisePrePost(const char *name, const deserialiser_input &di, error_collection &ec) {
	const rapidjson::Value &subval=di.json[name];
	if(subval.IsNull()) return;
	else {
		di.RegisterProp(name);
		if(subval.IsString() && di.ws) di.ws->ExecuteTemplate(*this, subval.GetString(), di, ec);
		else if(subval.IsArray() && di.ws) {
			for(rapidjson::SizeType i = 0; i < subval.Size(); i++) {
				const rapidjson::Value &arrayval = subval[i];
				if(arrayval.IsString()) di.ws->ExecuteTemplate(*this, arrayval.GetString(), di, ec);
				else ec.RegisterNewError<error_deserialisation>(di, "Invalid template reference");
			}
		}
		else {
			ec.RegisterNewError<error_deserialisation>(di, "Invalid template reference");
		}
	}
}

void serialisable_obj::DeserialiseObject(const deserialiser_input &di, error_collection &ec) {
	di.seenprops.reserve(di.json.GetMemberCount());
	if(di.objpreparse) DeserialiseObject(*di.objpreparse, ec);
	DeserialisePrePost("preparse", di, ec);
	Deserialise(di, ec);
	DeserialisePrePost("postparse", di, ec);
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

std::shared_ptr<deserialiser_input::deserialiser_input_deep_clone> deserialiser_input::DeepClone() const {
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
	return std::make_shared<deserialiser_input::deserialiser_input_deep_clone>(top, std::move(items));
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
