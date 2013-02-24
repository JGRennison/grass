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

struct deserialiser_input {
	const std::string name;
	const std::string type;
	const rapidjson::Value &json;
	world *w;

	deserialiser_input(const std::string &n, const std::string &t, const rapidjson::Value &j, world *w_=0) : name(n), type(t), json(j), w(w_) { }
	deserialiser_input(const std::string &n, const std::string &t, const rapidjson::Value &j, const deserialiser_input &base) : name(n), type(t), json(j), w(base.w) { }
};

struct serialiser_output {
	Handler &json_out;
};

class error_deserialisation : public error_obj {
	public:
	error_deserialisation(const std::string &str = "") {
		msg << "JSON deserialisation error: " << str;
	}
};

template <typename C> inline bool IsType(const rapidjson::Value& val);
template <> inline bool IsType<bool>(const rapidjson::Value& val) { return val.IsBool(); }
template <> inline bool IsType<unsigned int>(const rapidjson::Value& val) { return val.IsUint(); }
template <> inline bool IsType<int>(const rapidjson::Value& val) { return val.IsInt(); }
template <> inline bool IsType<uint64_t>(const rapidjson::Value& val) { return val.IsUint64(); }
template <> inline bool IsType<const char*>(const rapidjson::Value& val) { return val.IsString(); }
template <> inline bool IsType<std::string>(const rapidjson::Value& val) { return val.IsString(); }
template <> inline bool IsType<DIRTYPE>(const rapidjson::Value& val) {
	if(val.IsString()) {
		DIRTYPE dir;
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
template <> inline std::string GetType<std::string>(const rapidjson::Value& val) { return val.GetString(); }
template <> inline DIRTYPE GetType<DIRTYPE>(const rapidjson::Value& val) {
	DIRTYPE dir = TDIR_NULL;
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
template <> inline void SetType<DIRTYPE>(Handler &out, DIRTYPE val) {
	out.String(SerialiseDirectionName(val));
}

struct placeholder_subobject { };

template <typename C> inline const char *GetTypeFriendlyName();
template <> inline const char *GetTypeFriendlyName<bool>() { return "boolean"; }
template <> inline const char *GetTypeFriendlyName<unsigned int>() { return "unsigned integer"; }
template <> inline const char *GetTypeFriendlyName<int>() { return "integer"; }
template <> inline const char *GetTypeFriendlyName<uint64_t>() { return "unsigned 64-bit integer"; }
template <> inline const char *GetTypeFriendlyName<const char*>() { return "string"; }
template <> inline const char *GetTypeFriendlyName<std::string>() { return "string"; }
template <> inline const char *GetTypeFriendlyName<DIRTYPE>() { return "direction"; }
template <> inline const char *GetTypeFriendlyName<placeholder_subobject>() { return "object"; }

template <typename C> inline void CheckJsonTypeAndReportError(const char *prop, const rapidjson::Value& subval, error_collection &ec) {
	if(!subval.IsNull()) {
		ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation(string_format("Json variable of wrong type: %s, expected: %s", prop, GetTypeFriendlyName<C>()))));
	}
}

template <typename C> inline bool CheckTransJsonValue(C &var, const rapidjson::Value& val, const char *prop, error_collection &ec) {
	const rapidjson::Value &subval=val[prop];
	bool res=IsType<C>(subval);
	if(res) GetType<C>(subval);
	else CheckJsonTypeAndReportError<C>(prop, subval, ec);
	return res;
}

template <typename C, typename D> inline bool CheckTransJsonValueDef(C &var, const rapidjson::Value& val, const char *prop, const D def, error_collection &ec) {
	const rapidjson::Value &subval=val[prop];
	bool res=IsType<C>(subval);
	var=res?GetType<C>(subval):def;
	if(!res) CheckJsonTypeAndReportError<C>(prop, subval, ec);
	return res;
}

template <typename C> inline bool CheckTransJsonValueFlag(C &var, C flagmask, const rapidjson::Value& val, const char *prop, error_collection &ec) {
	const rapidjson::Value &subval=val[prop];
	bool res=IsType<bool>(subval);
	if(res) {
		if(GetType<bool>(subval)) var|=flagmask;
		else var&=~flagmask;
	}
	else CheckJsonTypeAndReportError<C>(prop, subval, ec);
	return res;
}

template <typename C> inline bool CheckTransJsonValueDefFlag(C &var, C flagmask, const rapidjson::Value& val, const char *prop, bool def, error_collection &ec) {
	const rapidjson::Value &subval=val[prop];
	bool res=IsType<bool>(subval);
	bool flagval=res?GetType<bool>(subval):def;
	if(flagval) var|=flagmask;
	else var&=~flagmask;
	if(!res) CheckJsonTypeAndReportError<C>(prop, subval, ec);
	return res;
}

template <typename C, typename D> inline C CheckGetJsonValueDef(const rapidjson::Value& val, const char *prop, const D def, error_collection &ec, bool *hadval=0) {
	const rapidjson::Value &subval=val[prop];
	bool res=IsType<C>(subval);
	if(hadval) *hadval=res;
	if(!res) CheckJsonTypeAndReportError<C>(prop, subval, ec);
	return res?GetType<C>(subval):def;
}

template <typename C> inline bool CheckTransJsonSubObj(C &obj, const rapidjson::Value& val, const char *prop, const std::string &type_name, error_collection &ec, world *w=0) {
	const rapidjson::Value &subval=val[prop];
	bool res=subval.IsObject();
	if(res) return obj.Deserialise(deserialiser_input("", type_name, subval, w), ec);
	else {
		CheckJsonTypeAndReportError<placeholder_subobject>(prop, subval, ec);
		return false;
	}
}

template <typename C> inline bool SerialiseSubObjJson(const C &obj, serialiser_output &so, const char *prop, error_collection &ec) {
	so.json_out.String(prop);
	so.json_out.StartObject();
	bool res = obj.Serialise(so, ec);
	so.json_out.EndObject();
	return res;
}

template <typename C> inline void SerialiseValueJson(const C &value, serialiser_output &so, const char *prop) {
	so.json_out.String(prop);
	SetType(so.json_out, value);
}

#endif
