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
#include "serialisable_impl.h"
#include "error.h"
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
