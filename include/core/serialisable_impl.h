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

#ifndef INC_SERIALISABLE_IMPL_ALREADY
#define INC_SERIALISABLE_IMPL_ALREADY

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <cinttypes>
#include <functional>
#include <memory>
#include "util/error.h"
#include "util/flags.h"
#include "util/util.h"
#include "core/serialisable.h"
#include "core/world_serialisation.h"
#include "core/edge_type.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
#include "rapidjson/document.h"
#include "rapidjson/reader.h"
#include "rapidjson/writer.h"
#pragma GCC diagnostic pop

class world;

struct writestream {
	writestream(std::string &str_, size_t reshint = 512 )
			: str(str_) {
		str.clear();
		str.reserve(reshint);
	}

	std::string &str;
	inline void Put(char ch) { str.push_back(ch); }
};

struct Handler {
	virtual Handler& Null() = 0;
	virtual Handler& Bool(bool b) = 0;
	virtual Handler& Int(int i) = 0;
	virtual Handler& Uint(unsigned u) = 0;
	virtual Handler& Int64(int64_t i64) = 0;
	virtual Handler& Uint64(uint64_t u64) = 0;
	virtual Handler& Double(double d) = 0;
	virtual Handler& String(const char* str, rapidjson::SizeType length, bool copy = false) = 0;
	virtual Handler& StartObject() = 0;
	virtual Handler& EndObject(rapidjson::SizeType memberCount = 0) = 0;
	virtual Handler& StartArray() = 0;
	virtual Handler& EndArray(rapidjson::SizeType elementCount = 0) = 0;
	virtual Handler& String(const char* str) = 0;
	Handler& String(const std::string &str) {
		String(str.c_str(), str.size());
		return *this;
	}
};

template<typename A, typename B> struct GenericWriterHandler : public Handler {
	private:
	A writer;

	public:
	GenericWriterHandler(B &wr) : writer(wr) { }

	GenericWriterHandler& Null() { writer.Null(); return *this; }
	GenericWriterHandler& Bool(bool b) { writer.Bool(b); return *this; }
	GenericWriterHandler& Int(int i) { writer.Int(i); return *this; }
	GenericWriterHandler& Uint(unsigned u) { writer.Uint(u); return *this; }
	GenericWriterHandler& Int64(int64_t i64) { writer.Int64(i64); return *this; }
	GenericWriterHandler& Uint64(uint64_t u64) { writer.Uint64(u64); return *this; }
	GenericWriterHandler& Double(double d) { writer.Double(d); return *this; }
	GenericWriterHandler& String(const char* str, rapidjson::SizeType length, bool copy = false) { writer.String(str, length, copy); return *this; }
	GenericWriterHandler& StartObject() { writer.StartObject(); return *this; }
	GenericWriterHandler& EndObject(rapidjson::SizeType memberCount = 0) { writer.EndObject(memberCount); return *this; }
	GenericWriterHandler& StartArray() { writer.StartArray(); return *this; }
	GenericWriterHandler& EndArray(rapidjson::SizeType elementCount = 0) { writer.EndArray(elementCount); return *this; }
	GenericWriterHandler& String(const char* str) { writer.String(str); return *this; }
};

using WriterHandler = GenericWriterHandler<rapidjson::Writer<writestream>, writestream>;

inline std::string MkArrayRefName(unsigned int i) {
	return "Array Element " + std::to_string(i);
}

struct deserialiser_input {
	std::string type;
	std::string reference_name;
	const rapidjson::Value &json;
	world *w;
	world_deserialisation *ws;
	const deserialiser_input *parent;
	mutable std::vector<const char *> seenprops;
	deserialiser_input *objpreparse = nullptr;
	deserialiser_input *objpostparse = nullptr;

	deserialiser_input(const rapidjson::Value &j, const std::string &t, const std::string &r, world *w_ = nullptr,
			world_deserialisation *ws_ = nullptr, const deserialiser_input *base = nullptr)
			: type(t), reference_name(r), json(j), w(w_), ws(ws_), parent(base) { }
	deserialiser_input(const rapidjson::Value &j, const std::string &t, const std::string &r, const deserialiser_input &base)
			: type(t), reference_name(r), json(j), w(base.w), ws(base.ws), parent(&base) { }

	inline void RegisterProp(const char *prop) const {
		seenprops.push_back(prop);
	}
	void PostDeserialisePropCheck(error_collection &ec) const;

	class deserialiser_input_clone_with_ancestors {
		std::vector< std::unique_ptr<deserialiser_input> > items;
		deserialiser_input *top;

		public:
		deserialiser_input_clone_with_ancestors(deserialiser_input *top_, std::vector< std::unique_ptr<deserialiser_input> > &&items_) :
			items(std::move(items_)), top(top_) { }
		deserialiser_input *GetTop() const { return top; };
	};
	std::shared_ptr<deserialiser_input_clone_with_ancestors> CloneWithAncestors() const;
	deserialiser_input *Clone() const;
};

enum class SOUTPUT_FLAGS {
	OUTPUT_STATIC         = 1<<0,
	OUTPUT_ALL_NAMES      = 1<<1,
	OUTPUT_NON_AUTO_NAMES = 1<<2,
};
template<> struct enum_traits< SOUTPUT_FLAGS > { static constexpr bool flags = true; };

struct serialiser_output_callbacks {
	virtual void StartStaticOutput() { }
	virtual void EndStaticOutput() { }
};

struct serialiser_output {
	Handler &json_out;
	flagwrapper<SOUTPUT_FLAGS> flags;
	std::unique_ptr<serialiser_output_callbacks> callbacks; //optional

	serialiser_output(Handler &h) : json_out(h) { }

	template <typename T> void OutputStatic(T func) {
		if (flags & SOUTPUT_FLAGS::OUTPUT_STATIC) {
			if (callbacks) {
				callbacks->StartStaticOutput();
			}
			func();
			if (callbacks) {
				callbacks->EndStaticOutput();
			}
		}
	}
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
template <> inline bool IsType<EDGE>(const rapidjson::Value& val) {
	if (val.IsString()) {
		EDGE dir;
		return DeserialiseDirectionName(dir, val.GetString());
	} else {
		return false;
	}
}

template <typename C> inline C GetType(const rapidjson::Value& val);
template <> inline bool GetType<bool>(const rapidjson::Value& val) { return val.GetBool(); }
template <> inline unsigned int GetType<unsigned int>(const rapidjson::Value& val) { return val.GetUint(); }
template <> inline int GetType<int>(const rapidjson::Value& val) { return val.GetInt(); }
template <> inline uint64_t GetType<uint64_t>(const rapidjson::Value& val) { return val.GetUint64(); }
template <> inline const char* GetType<const char*>(const rapidjson::Value& val) { return val.GetString(); }
template <> inline std::string GetType<std::string>(const rapidjson::Value& val) { return std::string(val.GetString(), val.GetStringLength()); }
template <> inline EDGE GetType<EDGE>(const rapidjson::Value& val) {
	EDGE dir = EDGE::INVALID;
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
template <> inline void SetType<EDGE>(Handler &out, EDGE val) {
	out.String(SerialiseDirectionName(val));
}

template <typename C> inline const char *GetTypeFriendlyName();
template <> inline const char *GetTypeFriendlyName<bool>() { return "boolean"; }
template <> inline const char *GetTypeFriendlyName<unsigned int>() { return "unsigned integer"; }
template <> inline const char *GetTypeFriendlyName<int>() { return "integer"; }
template <> inline const char *GetTypeFriendlyName<uint64_t>() { return "unsigned 64-bit integer"; }
template <> inline const char *GetTypeFriendlyName<const char*>() { return "string"; }
template <> inline const char *GetTypeFriendlyName<std::string>() { return "string"; }
template <> inline const char *GetTypeFriendlyName<EDGE>() { return "direction"; }
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
struct flagtyperemover<C, typename std::enable_if<std::is_convertible<C, EDGE>::value>::type> {
	typedef C type;
};

template <typename C> inline void CheckJsonTypeAndReportError(const deserialiser_input &di, const char *prop, const rapidjson::Value& subval,
		error_collection &ec, bool mandatory=false) {
	if (!subval.IsNull()) {
		ec.RegisterNewError<error_deserialisation>(di, string_format("JSON variable of wrong type: %s, expected: %s", prop, GetTypeFriendlyName<C>()));
	} else if (mandatory) {
		ec.RegisterNewError<error_deserialisation>(di, string_format("Mandatory JSON variable is missing: %s, expected: %s", prop, GetTypeFriendlyName<C>()));
	}
}

template <typename C> inline bool CheckTransJsonValue(C &var, const deserialiser_input &di, const char *prop, error_collection &ec, bool mandatory = false) {
	const rapidjson::Value &subval = di.json[prop];
	if (!subval.IsNull()) {
		di.RegisterProp(prop);
	}
	bool res = IsType<typename flagtyperemover<C>::type>(subval);
	if (res) {
		var = static_cast<C>(GetType<typename flagtyperemover<C>::type>(subval));
	} else {
		CheckJsonTypeAndReportError<typename flagtyperemover<C>::type>(di, prop, subval, ec, mandatory);
	}
	return res;
}

template <typename C, typename D> inline bool CheckTransJsonValueDef(C &var, const deserialiser_input &di, const char *prop,
		const D def, error_collection &ec) {
	bool res = CheckTransJsonValue(var, di, prop, ec, false);
	if (!res) {
		var = def;
	}
	return res;
}

//! Where F is a functor with the signature bool(const std::string &, uint64_t &, error_collection &)
template <typename C, typename F> inline bool TransJsonValueProc(const rapidjson::Value &subval, C &var, const deserialiser_input &di,
		const char *prop, error_collection &ec, F conv, bool mandatory = false) {
	if (subval.IsString()) {
		uint64_t value;
		std::string strvalue(subval.GetString(), subval.GetStringLength());
		bool res = conv(strvalue, value, ec);
		if (res) {
			var = static_cast<C>(value);
			if (static_cast<uint64_t>(var) != value) {
				ec.RegisterNewError<error_deserialisation>(di, string_format("Property: %s, with value: \"%s\" (%" PRIu64 ") overflows when casting to type: %s",
						prop, strvalue.c_str(), value, GetTypeFriendlyName<C>()));
				return false;
			}
		} else {
			ec.RegisterNewError<error_deserialisation>(di, string_format("Property: %s, with value: \"%s\" is invalid", prop, strvalue.c_str()));
		}
		return res;
	} else if (IsType<C>(subval)) {
		var = GetType<C>(subval);
		return true;
	} else {
		CheckJsonTypeAndReportError<typename flagtyperemover<C>::type>(di, prop, subval, ec, true);
		return false;
	}
}

//! Where F is a functor with the signature bool(const std::string &, uint64_t &, error_collection &)
template <typename C, typename F> inline bool CheckTransJsonValueProc(C &var, const deserialiser_input &di, const char *prop, error_collection &ec,
		F conv, bool mandatory = false) {
	const rapidjson::Value &subval = di.json[prop];
	if (!subval.IsNull()) {
		di.RegisterProp(prop);
	} else {
		return false;
	}

	return TransJsonValueProc(subval, var, di, prop, ec, conv, mandatory);
}

//! Where F is a functor with the signature bool(const std::string &, uint64_t &, error_collection &)
template <typename C, typename D, typename F> inline bool CheckTransJsonValueDefProc(C &var, const deserialiser_input &di, const char *prop,
		const D def, error_collection &ec, F conv) {
	bool res = CheckTransJsonValueProc(var, di, prop, ec, conv, false);
	if (!res) {
		var = def;
	}
	return res;
}

template <typename C> class flag_conflict_checker {
	C set_bits;
	C clear_bits;

	void CheckError(C flagmask, const deserialiser_input &di, const char *prop, error_collection &ec) {
		if (set_bits & clear_bits & flagmask) {
			ec.RegisterNewError<error_deserialisation>(di, string_format("Flag variable: %s, contradicts earlier declarations in same scope", prop));
		}
	}

	public:
	flag_conflict_checker() : set_bits(static_cast<C>(0)), clear_bits(static_cast<C>(0)) { }

	void RegisterFlags(bool set, C flagmask, const deserialiser_input &di, const char *prop, error_collection &ec) {
		if (set) {
			set_bits |= flagmask;
		} else {
			clear_bits |= flagmask;
		}
		CheckError(flagmask, di, prop, ec);
	}

	void RegisterAndProcessFlags(C &val, bool set, C flagmask, const deserialiser_input &di, const char *prop, error_collection &ec) {
		if (set) {
			val |= flagmask;
		} else {
			val &= ~flagmask;
		}
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

template <typename C, typename D> inline bool CheckTransJsonValueFlag(C &var, D flagmask, const deserialiser_input &di, const char *prop,
		error_collection &ec, flag_conflict_checker<C> *conflictcheck = 0) {
	const rapidjson::Value &subval = di.json[prop];
	if (!subval.IsNull()) {
		di.RegisterProp(prop);
	}
	bool res = IsType<bool>(subval);
	if (res) {
		bool set = GetType<bool>(subval);
		if (set) {
			var = var | flagmask;
		} else {
			var = var & ~flagmask;
		}

		if (conflictcheck) {
			conflictcheck->RegisterFlags(set, flagmask, di, prop, ec);
		}
	} else {
		CheckJsonTypeAndReportError<bool>(di, prop, subval, ec);
	}
	return res;
}

template <typename C, typename D> inline bool CheckTransJsonValueDefFlag(C &var, D flagmask, const deserialiser_input &di, const char *prop,
		bool def, error_collection &ec) {
	const rapidjson::Value &subval = di.json[prop];
	if (!subval.IsNull()) {
		di.RegisterProp(prop);
	}
	bool res = IsType<bool>(subval);
	bool flagval = res ? static_cast<C>(GetType<bool>(subval)) : def;
	if (flagval) {
		var = var | flagmask;
	} else {
		var = var & ~flagmask;
	}
	if (!res) {
		CheckJsonTypeAndReportError<bool>(di, prop, subval, ec);
	}
	return res;
}

template <typename C, typename D> inline C CheckGetJsonValueDef(const deserialiser_input &di, const char *prop, const D def,
		error_collection &ec, bool *hadval = nullptr) {
	const rapidjson::Value &subval = di.json[prop];
	if (!subval.IsNull()) {
		di.RegisterProp(prop);
	}
	bool res = IsType<typename flagtyperemover<C>::type>(subval);
	if (hadval) {
		*hadval = res;
	}
	if (!res) {
		CheckJsonTypeAndReportError<typename flagtyperemover<C>::type>(di, prop, subval, ec);
	}
	return res ? static_cast<C>(GetType<typename flagtyperemover<C>::type>(subval)) : def;
}

//! Where F is a functor with the signature void(const deserialiser_input &di, error_collection &ec)
template <typename C, typename F> inline bool CheckTransJsonTypeFunc(const deserialiser_input &di, const char *prop, const std::string &type_name,
		error_collection &ec, F func, bool mandatory = false) {
	const rapidjson::Value &subval = di.json[prop];
	if (!subval.IsNull()) {
		di.RegisterProp(prop);
	}
	bool res = IsType<C>(subval);
	if (res) {
		func(deserialiser_input(subval, type_name, prop, di), ec);
	} else {
		CheckJsonTypeAndReportError<C>(di, prop, subval, ec, mandatory);
	}
	return res;
}

template <typename C> inline bool CheckTransJsonSubObj(C &obj, const deserialiser_input &di, const char *prop, const std::string &type_name,
		error_collection &ec, bool mandatory = false) {
	return CheckTransJsonTypeFunc<json_object>(di, prop, type_name, ec,
			[&](const deserialiser_input &di, error_collection &ec) { obj.Deserialise(di, ec); }, mandatory);
}

template <typename C> inline bool CheckTransJsonSubArray(C &obj, const deserialiser_input &di, const char *prop, const std::string &type_name,
		error_collection &ec, bool mandatory = false) {
	return CheckTransJsonTypeFunc<json_array>(di, prop, type_name, ec,
			[&](const deserialiser_input &di, error_collection &ec) { obj.Deserialise(di, ec); }, mandatory);
}

bool CheckIterateJsonArrayOrValue(const deserialiser_input &di, const char *prop, const std::string &type_name, error_collection &ec,
		std::function<void(const deserialiser_input &, error_collection &)> func, bool arrayonly = false);

template <typename C> inline bool CheckIterateJsonArrayOrType(const deserialiser_input &di, const char *prop, const std::string &type_name,
		error_collection &ec, std::function<void(const deserialiser_input &, error_collection &)> func, bool arrayonly = false) {
	auto innerfunc = [&](const deserialiser_input &inner_di, error_collection &ec) {
		if (IsType<C>(inner_di.json)) {
			func(inner_di, ec);
		} else {
			CheckJsonTypeAndReportError<C>(inner_di, inner_di.reference_name.c_str(), inner_di.json, ec);
		}
	};

	return CheckIterateJsonArrayOrValue(di, prop, type_name, ec, innerfunc);
}

template <typename C> inline bool CheckFillTypeVectorFromJsonArrayOrType(const deserialiser_input &di, const char *prop,
		error_collection &ec, std::vector<C> &vec) {
	auto func = [&](const deserialiser_input &fdi, error_collection &ec) {
		vec.push_back(GetType<C>(fdi.json));
	};
	return CheckIterateJsonArrayOrType<C>(di, prop, "", ec, func);
}

template <typename C> inline bool CheckTransRapidjsonValue(const rapidjson::Value *&val, const deserialiser_input &di, const char *prop,
		error_collection &ec, bool mandatory = false) {
	const rapidjson::Value &subval = di.json[prop];
	if (!subval.IsNull()) {
		di.RegisterProp(prop);
	}
	bool res = IsType<C>(subval);
	if (res) {
		val = &subval;
	} else {
		CheckJsonTypeAndReportError<C>(di, prop, subval, ec, mandatory);
	}
	return res;
}

template <typename C> inline bool CheckTransRapidjsonValueDef(const rapidjson::Value *&val, const deserialiser_input &di, const char *prop,
		error_collection &ec, const rapidjson::Value *def = nullptr) {
	bool res = CheckTransRapidjsonValue<C>(val, di, prop, ec);
	if (!res) {
		val = def;
	}
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

//! Where F is a functor with the signature void(serialiser_output &so, const typename C::value_type &val)
template <typename C, typename F> inline void SerialiseArrayContainer(const C &container, serialiser_output &so, const char *prop, F func) {
	so.json_out.String(prop);
	so.json_out.StartArray();
	for (auto &it : container) {
		func(so, it);
	}
	so.json_out.EndArray();
}

//! Where F is a functor with the signature void(serialiser_output &so, const typename C::value_type &val)
template <typename C, typename F> inline void SerialiseObjectArrayContainer(const C &container, serialiser_output &so, const char *prop, F func) {
	SerialiseArrayContainer(container, so, prop, [&](serialiser_output &so, const typename C::value_type &val) {
		so.json_out.StartObject();
		func(so, val);
		so.json_out.EndObject();
	});
}

#endif
