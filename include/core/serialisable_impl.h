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

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <cinttypes>
#include "flags.h"
#include "serialisable.h"
#include "error.h"
#include "util.h"
#include "world_serialisation.h"
#include "edgetype.h"

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
	std::string type;
	std::string reference_name;
	const rapidjson::Value &json;
	world *w;
	world_serialisation *ws;
	const deserialiser_input *parent;
	mutable std::vector<const char *> seenprops;
	deserialiser_input *objpreparse = 0;
	deserialiser_input *objpostparse = 0;

	deserialiser_input(const std::string &t, const std::string &r, const rapidjson::Value &j, world *w_=0, world_serialisation *ws_=0, const deserialiser_input *base=0) : type(t), reference_name(r), json(j), w(w_), ws(ws_), parent(base) { }
	deserialiser_input(const std::string &t, const std::string &r, const rapidjson::Value &j, const deserialiser_input &base) : type(t), reference_name(r), json(j), w(base.w), ws(base.ws), parent(&base) { }
	deserialiser_input(const rapidjson::Value &j, const std::string &t, const std::string &r, const deserialiser_input &base) : type(t), reference_name(r), json(j), w(base.w), ws(base.ws), parent(&base) { }
	deserialiser_input(const rapidjson::Value &j, const std::string &r, const deserialiser_input &base) : reference_name(r), json(j), w(base.w), ws(base.ws), parent(&base) { }

	inline void RegisterProp(const char *prop) const {
		seenprops.push_back(prop);
	}
	void PostDeserialisePropCheck(error_collection &ec) const;

	class deserialiser_input_deep_clone {
		std::vector< std::unique_ptr<deserialiser_input> > items;
		deserialiser_input *top;
		public:
		deserialiser_input_deep_clone(deserialiser_input *top_, std::vector< std::unique_ptr<deserialiser_input> > &&items_) :
			items(std::move(items_)), top(top_) { }
		deserialiser_input *GetTop() const { return top; };
	};
	std::shared_ptr<deserialiser_input_deep_clone> DeepClone() const;
	deserialiser_input *Clone() const;
};

struct serialiser_output {
	Handler &json_out;

	serialiser_output(Handler &h) : json_out(h) { }
};

class error_deserialisation : public error_obj {
	public:
	error_deserialisation(const deserialiser_input &di, const std::string &str = "");
	error_deserialisation(const std::string &str = "");
};

class error_jsonparse : public error_obj {
	public:
	error_jsonparse(const std::string &json, size_t erroroffset, const char *parseerror);
};

struct json_object { };
struct json_array { };

template <typename C> inline bool IsType(const rapidjson::Value& val);
template <> inline bool IsType<bool>(const rapidjson::Value& val) { return val.IsBool(); }
template <> inline bool IsType<unsigned int>(const rapidjson::Value& val) { return val.IsUint(); }
template <> inline bool IsType<int>(const rapidjson::Value& val) { return val.IsInt(); }
template <> inline bool IsType<uint64_t>(const rapidjson::Value& val) { return val.IsUint64(); }
template <> inline bool IsType<const char*>(const rapidjson::Value& val) { return val.IsString(); }
template <> inline bool IsType<std::string>(const rapidjson::Value& val) { return val.IsString(); }
template <> inline bool IsType<json_object>(const rapidjson::Value& val) { return val.IsObject(); }
template <> inline bool IsType<json_array>(const rapidjson::Value& val) { return val.IsArray(); }
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

template <typename C> inline const char *GetTypeFriendlyName();
template <> inline const char *GetTypeFriendlyName<bool>() { return "boolean"; }
template <> inline const char *GetTypeFriendlyName<unsigned int>() { return "unsigned integer"; }
template <> inline const char *GetTypeFriendlyName<int>() { return "integer"; }
template <> inline const char *GetTypeFriendlyName<uint64_t>() { return "unsigned 64-bit integer"; }
template <> inline const char *GetTypeFriendlyName<const char*>() { return "string"; }
template <> inline const char *GetTypeFriendlyName<std::string>() { return "string"; }
template <> inline const char *GetTypeFriendlyName<EDGETYPE>() { return "direction"; }
template <> inline const char *GetTypeFriendlyName<json_object>() { return "object"; }
template <> inline const char *GetTypeFriendlyName<json_array>() { return "array"; }

template <typename C, class Enable = void> struct flagtyperemover;

template<typename C>
struct flagtyperemover<C, typename std::enable_if<std::is_integral<C>::value>::type> {
	typedef C type;
};

template<typename C>
struct flagtyperemover<C, typename std::enable_if<enum_traits<C>::flags>::type> {
	typedef typename std::underlying_type<C>::type type;
};

template<typename C>
struct flagtyperemover<C, typename std::enable_if<std::is_convertible<C, std::string>::value>::type> {
	typedef C type;
};

template<typename C>
struct flagtyperemover<C, typename std::enable_if<std::is_convertible<C, const char*>::value>::type> {
	typedef C type;
};

template<typename C>
struct flagtyperemover<C, typename std::enable_if<std::is_convertible<C, EDGETYPE>::value>::type> {
	typedef C type;
};

template <typename C> inline void CheckJsonTypeAndReportError(const deserialiser_input &di, const char *prop, const rapidjson::Value& subval, error_collection &ec, bool mandatory=false) {
	if(!subval.IsNull()) {
		ec.RegisterNewError<error_deserialisation>(di, string_format("JSON variable of wrong type: %s, expected: %s", prop, GetTypeFriendlyName<C>()));
	}
	else if(mandatory) {
		ec.RegisterNewError<error_deserialisation>(di, string_format("Mandatory JSON variable is missing: %s, expected: %s", prop, GetTypeFriendlyName<C>()));
	}
}

template <typename C> inline bool CheckTransJsonValue(C &var, const deserialiser_input &di, const char *prop, error_collection &ec, bool mandatory=false) {
	const rapidjson::Value &subval=di.json[prop];
	if(!subval.IsNull()) di.RegisterProp(prop);
	bool res=IsType<typename flagtyperemover<C>::type>(subval);
	if(res) var=static_cast<C>(GetType<typename flagtyperemover<C>::type>(subval));
	else CheckJsonTypeAndReportError<typename flagtyperemover<C>::type>(di, prop, subval, ec, mandatory);
	return res;
}

template <typename C, typename D> inline bool CheckTransJsonValueDef(C &var, const deserialiser_input &di, const char *prop, const D def, error_collection &ec) {
	bool res = CheckTransJsonValue(var, di, prop, ec, false);
	if(!res) var = def;
	return res;
}

template <typename C> inline bool TransJsonValueProc(const rapidjson::Value &subval, C &var, const deserialiser_input &di, const char *prop, error_collection &ec, std::function<bool(const std::string &, uint64_t &, error_collection &)> conv, bool mandatory=false) {
	if(subval.IsString()) {
		uint64_t value;
		std::string strvalue(subval.GetString(), subval.GetStringLength());
		bool res = conv(strvalue, value, ec);
		if(res) {
			var = static_cast<C>(value);
			if(static_cast<uint64_t>(var) != value) {
				ec.RegisterNewError<error_deserialisation>(di, string_format("Property: %s, with value: \"%s\" (%" PRIu64 ") overflows when casting to type: %s", prop, strvalue.c_str(), value, GetTypeFriendlyName<C>()));
				return false;
			}
		}
		else ec.RegisterNewError<error_deserialisation>(di, string_format("Property: %s, with value: \"%s\" is invalid", prop, strvalue.c_str()));
		return res;
	}
	else if(IsType<C>(subval)) {
		var=GetType<C>(subval);
		return true;
	}
	else {
		CheckJsonTypeAndReportError<typename flagtyperemover<C>::type>(di, prop, subval, ec, true);
		return false;
	}
}

template <typename C> inline bool CheckTransJsonValueProc(C &var, const deserialiser_input &di, const char *prop, error_collection &ec, std::function<bool(const std::string &, uint64_t &, error_collection &)> conv, bool mandatory=false) {
	const rapidjson::Value &subval=di.json[prop];
	if(!subval.IsNull()) di.RegisterProp(prop);
	else return false;

	return TransJsonValueProc(subval, var, di, prop, ec, conv, mandatory);
}

template <typename C, typename D> inline bool CheckTransJsonValueDefProc(C &var, const deserialiser_input &di, const char *prop, const D def, error_collection &ec, std::function<bool(const std::string &, uint64_t &, error_collection &)> conv) {
	bool res = CheckTransJsonValueProc(var, di, prop, ec, conv, false);
	if(!res) var = def;
	return res;
}

template <typename C> class flag_conflict_checker {
	C set_bits;
	C clear_bits;
	void CheckError(C flagmask, const deserialiser_input &di, const char *prop, error_collection &ec) {
		if(set_bits & clear_bits & flagmask) {
			ec.RegisterNewError<error_deserialisation>(di, string_format("Flag variable: %s, contradicts earlier declarations in same scope", prop));
		}
	}

	public:
	flag_conflict_checker() : set_bits(static_cast<C>(0)), clear_bits(static_cast<C>(0)) { }

	void RegisterFlags(bool set, C flagmask, const deserialiser_input &di, const char *prop, error_collection &ec) {
		if(set) set_bits |= flagmask;
		else clear_bits |= flagmask;
		CheckError(flagmask, di, prop, ec);
	}

	void RegisterAndProcessFlags(C &val, bool set, C flagmask, const deserialiser_input &di, const char *prop, error_collection &ec) {
		if(set) val |= flagmask;
		else val &= ~flagmask;
		RegisterFlags(set, flagmask, di, prop, ec);
	}

	void RegisterFlagsMasked(C input, C flagmask, const deserialiser_input &di, const char *prop, error_collection &ec) {
		set_bits |= input & flagmask;
		clear_bits |= (~input) & flagmask;
		CheckError(flagmask, di, prop, ec);
	}

	void RegisterFlagsDual(C setmask, C clearmask, const deserialiser_input &di, const char *prop, error_collection &ec) {
		set_bits |= setmask;
		clear_bits |= clearmask;
		CheckError(clearmask | setmask, di, prop, ec);
	}

	void Ban(C mask) {
		set_bits |= mask;
		clear_bits |= mask;
	}
};

template <typename C, typename D> inline bool CheckTransJsonValueFlag(C &var, D flagmask, const deserialiser_input &di, const char *prop, error_collection &ec, flag_conflict_checker<C> *conflictcheck = 0) {
	const rapidjson::Value &subval=di.json[prop];
	if(!subval.IsNull()) di.RegisterProp(prop);
	bool res=IsType<bool>(subval);
	if(res) {
		bool set = GetType<bool>(subval);
		if(set) var = var | flagmask;
		else var = var & ~flagmask;

		if(conflictcheck) conflictcheck->RegisterFlags(set, flagmask, di, prop, ec);
	}
	else CheckJsonTypeAndReportError<typename flagtyperemover<C>::type>(di, prop, subval, ec);
	return res;
}

template <typename C, typename D> inline bool CheckTransJsonValueDefFlag(C &var, D flagmask, const deserialiser_input &di, const char *prop, bool def, error_collection &ec) {
	const rapidjson::Value &subval=di.json[prop];
	if(!subval.IsNull()) di.RegisterProp(prop);
	bool res=IsType<bool>(subval);
	bool flagval=res?static_cast<C>(GetType<bool>(subval)):def;
	if(flagval) var = var | flagmask;
	else var = var & ~flagmask;
	if(!res) CheckJsonTypeAndReportError<typename flagtyperemover<C>::type>(di, prop, subval, ec);
	return res;
}

template <typename C, typename D> inline C CheckGetJsonValueDef(const deserialiser_input &di, const char *prop, const D def, error_collection &ec, bool *hadval=0) {
	const rapidjson::Value &subval=di.json[prop];
	if(!subval.IsNull()) di.RegisterProp(prop);
	bool res=IsType<typename flagtyperemover<C>::type>(subval);
	if(hadval) *hadval=res;
	if(!res) CheckJsonTypeAndReportError<typename flagtyperemover<C>::type>(di, prop, subval, ec);
	return res?static_cast<C>(GetType<typename flagtyperemover<C>::type>(subval)):def;
}

template <typename C> inline bool CheckTransJsonTypeFunc(const deserialiser_input &di, const char *prop, const std::string &type_name, error_collection &ec, std::function<void(const deserialiser_input &di, error_collection &ec)> func, bool mandatory=false) {
	const rapidjson::Value &subval=di.json[prop];
	if(!subval.IsNull()) di.RegisterProp(prop);
	bool res = IsType<C>(subval);
	if(res) func(deserialiser_input(type_name, prop, subval, di), ec);
	else CheckJsonTypeAndReportError<C>(di, prop, subval, ec, mandatory);
	return res;
}

template <typename C> inline bool CheckTransJsonSubObj(C &obj, const deserialiser_input &di, const char *prop, const std::string &type_name, error_collection &ec, bool mandatory=false) {
	return CheckTransJsonTypeFunc<json_object>(di, prop, type_name, ec, [&](const deserialiser_input &di, error_collection &ec) { obj.Deserialise(di, ec); }, mandatory);
}

template <typename C> inline bool CheckTransJsonSubArray(C &obj, const deserialiser_input &di, const char *prop, const std::string &type_name, error_collection &ec, bool mandatory=false) {
	return CheckTransJsonTypeFunc<json_array>(di, prop, type_name, ec, [&](const deserialiser_input &di, error_collection &ec) { obj.Deserialise(di, ec); }, mandatory);
}

bool CheckIterateJsonArrayOrValue(const deserialiser_input &di, const char *prop, const std::string &type_name, error_collection &ec, std::function<void(const deserialiser_input &, error_collection &)> func, bool arrayonly = false);

template <typename C> inline bool CheckIterateJsonArrayOrType(const deserialiser_input &di, const char *prop, const std::string &type_name, error_collection &ec, std::function<void(const deserialiser_input &, error_collection &)> func, bool arrayonly = false) {
	auto innerfunc = [&](const deserialiser_input &inner_di, error_collection &ec) {
		if(IsType<C>(inner_di.json)) {
			func(inner_di, ec);
		}
		else {
			CheckJsonTypeAndReportError<C>(inner_di, inner_di.reference_name.c_str(), inner_di.json, ec);
		}
	};

	return CheckIterateJsonArrayOrValue(di, prop, type_name, ec, innerfunc);
}

template <typename C> inline bool CheckFillTypeVectorFromJsonArrayOrType(const deserialiser_input &di, const char *prop, error_collection &ec, std::vector<C> &vec) {
	auto func = [&](const deserialiser_input &fdi, error_collection &ec) {
		vec.push_back(GetType<C>(fdi.json));
	};
	return CheckIterateJsonArrayOrType<C>(di, prop, "", ec, func);
}

template <typename C> inline bool CheckTransRapidjsonValue(const rapidjson::Value *&val, const deserialiser_input &di, const char *prop, error_collection &ec, bool mandatory=false) {
	const rapidjson::Value &subval=di.json[prop];
	if(!subval.IsNull()) di.RegisterProp(prop);
	bool res=IsType<C>(subval);
	if(res) val=&subval;
	else CheckJsonTypeAndReportError<C>(di, prop, subval, ec, mandatory);
	return res;
}

template <typename C> inline bool CheckTransRapidjsonValueDef(const rapidjson::Value *&val, const deserialiser_input &di, const char *prop, error_collection &ec, const rapidjson::Value *def = 0) {
	bool res = CheckTransRapidjsonValue<C>(val, di, prop, ec);
	if(!res) val = def;
	return res;
}

template <typename C> inline void SerialiseSubObjJson(const C &obj, serialiser_output &so, const char *prop, error_collection &ec) {
	so.json_out.String(prop);
	so.json_out.StartObject();
	obj.Serialise(so, ec);
	so.json_out.EndObject();
}

template <typename C> inline void SerialiseSubArrayJson(const C &obj, serialiser_output &so, const char *prop, error_collection &ec) {
	so.json_out.String(prop);
	so.json_out.StartArray();
	obj.Serialise(so, ec);
	so.json_out.EndArray();
}

template <typename C> inline void SerialiseValueJson(C value, serialiser_output &so, const char *prop) {
	so.json_out.String(prop);
	SetType(so.json_out, static_cast<typename flagtyperemover<C>::type>(value));
}

template <typename C> inline void SerialiseFlagJson(C value, C mask, serialiser_output &so, const char *prop) {
	so.json_out.String(prop);
	SetType<bool>(so.json_out, value & mask);
}

template <typename C, typename D> inline void SerialiseObjectArrayContainer(const C &container, serialiser_output &so, const char *prop, std::function<void(serialiser_output &so, const D &val)> func) {
	so.json_out.String(prop);
	so.json_out.StartArray();
	for(auto &it : container) {
		so.json_out.StartObject();
		func(so, it);
		so.json_out.EndObject();
	}
	so.json_out.EndArray();
}

#endif
