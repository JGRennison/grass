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

#ifndef INC_SERIALISABLE_IMPL_ALREADY
#define INC_SERIALISABLE_IMPL_ALREADY

#include "serialisable.h"
#include "error.h"
#include "util.h"
#include "world_serialisation.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
#include "rapidjson/document.h"
#include "rapidjson/reader.h"
#include "rapidjson/writer.h"
#pragma GCC diagnostic pop

class world;

struct writestream {
	writestream(std::string &str_, size_t reshint=512 ) : str(str_) { str.clear(); str.reserve(reshint); }
	std::string &str;
	inline void Put(char ch) { str.push_back(ch); }
};

struct Handler : public rapidjson::Writer<writestream> {
	using rapidjson::Writer<writestream>::String;
	Handler& String(const std::string &str) {
		rapidjson::Writer<writestream>::String(str.c_str(), str.size());
		return *this;
	}
	Handler(writestream &wr) : rapidjson::Writer<writestream>::Writer(wr) { }
};

inline std::string MkArrayRefName(unsigned int i) {
	return "Array Element " + std::to_string(i);
}

struct deserialiser_input {
	std::string name;
	std::string type;
	std::string reference_name;
	const rapidjson::Value &json;
	world *w;
	world_serialisation *ws;
	const deserialiser_input *parent;
	mutable std::vector<const char *> seenprops;

	deserialiser_input(const std::string &n, const std::string &t, const std::string &r, const rapidjson::Value &j, world *w_=0, world_serialisation *ws_=0, const deserialiser_input *base=0) : name(n), type(t), reference_name(r), json(j), w(w_), ws(ws_), parent(base) { }
	deserialiser_input(const std::string &n, const std::string &t, const std::string &r, const rapidjson::Value &j, const deserialiser_input &base) : name(n), type(t), reference_name(r), json(j), w(base.w), ws(base.ws), parent(&base) { }
	deserialiser_input(const rapidjson::Value &j, const std::string &t, const std::string &r, const deserialiser_input &base) : type(t), reference_name(r), json(j), w(base.w), ws(base.ws), parent(&base) { }
	deserialiser_input(const rapidjson::Value &j, const std::string &r, const deserialiser_input &base) : reference_name(r), json(j), w(base.w), ws(base.ws), parent(&base) { }

	inline void RegisterProp(const char *prop) const {
		seenprops.push_back(prop);
	}
};

struct serialiser_output {
	Handler &json_out;
};

class error_deserialisation : public error_obj {
	public:
	error_deserialisation(const deserialiser_input &di, const std::string &str = "");
	error_deserialisation(const std::string &str = "");
};

template <typename C> inline bool IsType(const rapidjson::Value& val);
template <> inline bool IsType<bool>(const rapidjson::Value& val) { return val.IsBool(); }
template <> inline bool IsType<unsigned int>(const rapidjson::Value& val) { return val.IsUint(); }
template <> inline bool IsType<int>(const rapidjson::Value& val) { return val.IsInt(); }
template <> inline bool IsType<uint64_t>(const rapidjson::Value& val) { return val.IsUint64(); }
template <> inline bool IsType<const char*>(const rapidjson::Value& val) { return val.IsString(); }
template <> inline bool IsType<std::string>(const rapidjson::Value& val) { return val.IsString(); }
template <> inline bool IsType<EDGETYPE>(const rapidjson::Value& val) {
	if(val.IsString()) {
		EDGETYPE dir;
		return DeserialiseDirectionName(dir, val.GetString());
	}
	else return false;
}

template <typename C> inline C GetType(const rapidjson::Value& val);
template <> inline bool GetType<bool>(const rapidjson::Value& val) { return val.GetBool(); }
template <> inline unsigned int GetType<unsigned int>(const rapidjson::Value& val) { return val.GetUint(); }
template <> inline int GetType<int>(const rapidjson::Value& val) { return val.GetInt(); }
template <> inline uint64_t GetType<uint64_t>(const rapidjson::Value& val) { return val.GetUint64(); }
template <> inline const char* GetType<const char*>(const rapidjson::Value& val) { return val.GetString(); }
template <> inline std::string GetType<std::string>(const rapidjson::Value& val) { return std::string(val.GetString(), val.GetStringLength()); }
template <> inline EDGETYPE GetType<EDGETYPE>(const rapidjson::Value& val) {
	EDGETYPE dir = EDGE_NULL;
	DeserialiseDirectionName(dir, val.GetString());
	return dir;
}

template <typename C> inline void SetType(Handler &out, C val);
template <> inline void SetType<bool>(Handler &out, bool val) { out.Bool(val); }
template <> inline void SetType<unsigned int>(Handler &out, unsigned int val) { out.Uint(val); }
template <> inline void SetType<int>(Handler &out, int val) { out.Int(val); }
template <> inline void SetType<uint64_t>(Handler &out, uint64_t val) { out.Uint64(val); }
template <> inline void SetType<const char*>(Handler &out, const char* val) { out.String(val); }
template <> inline void SetType<std::string>(Handler &out, std::string val) { out.String(val); }
template <> inline void SetType<EDGETYPE>(Handler &out, EDGETYPE val) {
	out.String(SerialiseDirectionName(val));
}

struct placeholder_subobject { };
struct placeholder_array { };

template <typename C> inline const char *GetTypeFriendlyName();
template <> inline const char *GetTypeFriendlyName<bool>() { return "boolean"; }
template <> inline const char *GetTypeFriendlyName<unsigned int>() { return "unsigned integer"; }
template <> inline const char *GetTypeFriendlyName<int>() { return "integer"; }
template <> inline const char *GetTypeFriendlyName<uint64_t>() { return "unsigned 64-bit integer"; }
template <> inline const char *GetTypeFriendlyName<const char*>() { return "string"; }
template <> inline const char *GetTypeFriendlyName<std::string>() { return "string"; }
template <> inline const char *GetTypeFriendlyName<EDGETYPE>() { return "direction"; }
template <> inline const char *GetTypeFriendlyName<placeholder_subobject>() { return "object"; }
template <> inline const char *GetTypeFriendlyName<placeholder_array>() { return "array"; }

template <typename C> inline void CheckJsonTypeAndReportError(const deserialiser_input &di, const char *prop, const rapidjson::Value& subval, error_collection &ec) {
	if(!subval.IsNull()) {
		ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation(di, string_format("JSON variable of wrong type: %s, expected: %s", prop, GetTypeFriendlyName<C>()))));
	}
}

template <typename C> inline bool CheckTransJsonValue(C &var, const deserialiser_input &di, const char *prop, error_collection &ec) {
	const rapidjson::Value &subval=di.json[prop];
	di.RegisterProp(prop);
	bool res=IsType<C>(subval);
	if(res) var=GetType<C>(subval);
	else CheckJsonTypeAndReportError<C>(di, prop, subval, ec);
	return res;
}

template <typename C, typename D> inline bool CheckTransJsonValueDef(C &var, const deserialiser_input &di, const char *prop, const D def, error_collection &ec) {
	const rapidjson::Value &subval=di.json[prop];
	di.RegisterProp(prop);
	bool res=IsType<C>(subval);
	var=res?GetType<C>(subval):def;
	if(!res) CheckJsonTypeAndReportError<C>(di, prop, subval, ec);
	return res;
}

template <typename C> inline bool CheckTransJsonValueFlag(C &var, C flagmask, const deserialiser_input &di, const char *prop, error_collection &ec) {
	const rapidjson::Value &subval=di.json[prop];
	di.RegisterProp(prop);
	bool res=IsType<bool>(subval);
	if(res) {
		if(GetType<bool>(subval)) var|=flagmask;
		else var&=~flagmask;
	}
	else CheckJsonTypeAndReportError<C>(di, prop, subval, ec);
	return res;
}

template <typename C> inline bool CheckTransJsonValueDefFlag(C &var, C flagmask, const deserialiser_input &di, const char *prop, bool def, error_collection &ec) {
	const rapidjson::Value &subval=di.json[prop];
	di.RegisterProp(prop);
	bool res=IsType<bool>(subval);
	bool flagval=res?GetType<bool>(subval):def;
	if(flagval) var|=flagmask;
	else var&=~flagmask;
	if(!res) CheckJsonTypeAndReportError<C>(di, prop, subval, ec);
	return res;
}

template <typename C, typename D> inline C CheckGetJsonValueDef(const deserialiser_input &di, const char *prop, const D def, error_collection &ec, bool *hadval=0) {
	const rapidjson::Value &subval=di.json[prop];
	di.RegisterProp(prop);
	bool res=IsType<C>(subval);
	if(hadval) *hadval=res;
	if(!res) CheckJsonTypeAndReportError<C>(di, prop, subval, ec);
	return res?GetType<C>(subval):def;
}

template <typename C> inline void CheckTransJsonSubObj(C &obj, const deserialiser_input &di, const char *prop, const std::string &type_name, error_collection &ec) {
	const rapidjson::Value &subval=di.json[prop];
	di.RegisterProp(prop);
	if(subval.IsObject()) obj.DeserialiseObject(deserialiser_input("", type_name, prop, subval), ec);
	else {
		CheckJsonTypeAndReportError<placeholder_subobject>(di, prop, subval, ec);
	}
}

template <typename C> inline void CheckTransJsonSubArray(C &obj, const deserialiser_input &di, const char *prop, const std::string &type_name, error_collection &ec) {
	const rapidjson::Value &subval=di.json[prop];
	di.RegisterProp(prop);
	if(subval.IsArray()) obj.Deserialise(deserialiser_input("", type_name, prop, subval), ec);
	else {
		CheckJsonTypeAndReportError<placeholder_array>(di, prop, subval, ec);
	}
}

template <typename C> inline void CheckIterateJsonArrayOrType(const deserialiser_input &di, const char *prop, const std::string &type_name, error_collection &ec, std::function<void(const deserialiser_input &di, error_collection &ec)> func) {
	deserialiser_input subdi(di.json[prop], type_name, prop, di);
	di.RegisterProp(prop);

	auto innerfunc = [&](const deserialiser_input &inner_di) {
		if(IsType<C>(inner_di.json)) {
			func(inner_di, ec);
		}
		else {
			CheckJsonTypeAndReportError<C>(inner_di, inner_di.reference_name.c_str(), inner_di.json, ec);
		}
	};

	if(subdi.json.IsArray()) {
		for(rapidjson::SizeType i = 0; i < subdi.json.Size(); i++) {
			innerfunc(deserialiser_input(subdi.json[i], type_name, MkArrayRefName(i), subdi));
		}
	}
	else innerfunc(subdi);
}

template <typename C> inline void CheckFillTypeVectorFromJsonArrayOrType(const deserialiser_input &di, const char *prop, error_collection &ec, std::vector<C> &vec) {
	auto func = [&](const deserialiser_input &fdi, error_collection &ec) {
		vec.push_back(GetType<C>(fdi.json));
	};
	CheckIterateJsonArrayOrType<std::string>(di, prop, "", ec, func);
}

template <typename C> inline void SerialiseSubObjJson(const C &obj, serialiser_output &so, const char *prop, error_collection &ec) {
	so.json_out.String(prop);
	so.json_out.StartObject();
	obj.Serialise(so, ec);
	so.json_out.EndObject();
}

template <typename C> inline void SerialiseValueJson(C value, serialiser_output &so, const char *prop) {
	so.json_out.String(prop);
	SetType(so.json_out, value);
}

template <typename C> inline void SerialiseFlagJson(C value, C mask, serialiser_output &so, const char *prop) {
	so.json_out.String(prop);
	SetType<bool>(so.json_out, value & mask);
}

#endif
