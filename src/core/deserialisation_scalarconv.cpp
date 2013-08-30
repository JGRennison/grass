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
#include "deserialisation_scalarconv.h"
#include "error.h"

#include <cctype>
#include <vector>

namespace dsconv {

	struct scalar_conv {
		std::string postfix;
		unsigned int multiplier;
		unsigned int postshift;
	};

	bool DoConv(const std::string &in, uint64_t &out, const std::string &type, const std::vector<scalar_conv> &postfixes, error_collection &ec) {
		uint64_t value = 0;
		bool ok = true;
		bool overflow = false;
		bool seenpoint = false;
		bool seennum = false;
		unsigned int pointshift = 0;
		auto it = in.cbegin();
		for(; it != in.cend(); ++it) {
			if(!isblank(*it)) break;
		}
		for(; it != in.cend(); ++it) {
			if(*it >= '0' && *it <= '9') {
				uint64_t newvalue = (value * 10) + (*it - '0');
				if(newvalue < value) overflow = true;
				value = newvalue;
				seennum = true;
				if(seenpoint) pointshift++;
			}
			else if (*it == '.' && !seenpoint) {
				seenpoint = true;
			}
			else break;
		}
		if(!seennum) return false;
		for(; it != in.cend(); ++it) {
			if(!isblank(*it)) break;
		}
		std::string postfix(it, in.cend());
		if(!postfix.empty()) {
			bool found = false;
			for(auto pf : postfixes) {
				if(pf.postfix == postfix) {
					uint64_t newvalue = value * ((uint64_t) pf.multiplier);
					if(newvalue / ((uint64_t) pf.multiplier) != value) overflow = true;	//inefficient overflow check
					value = newvalue >> pf.postshift;
					found = true;
				}
			}
			if(!found) {
				ec.RegisterNewError<generic_error_obj>("Could not find scalar postfix: " + postfix + ", for input: " + in + ", of type: " + type);
				return false;
			}
		}
		if(pointshift) {
			uint64_t pshift = 10;
			for(unsigned int i = 1; i < pointshift; i++) pshift *= 10;
			value /= pshift;
		}

		if(overflow) {
			ec.RegisterNewError<generic_error_obj>("Integer wrap-around detected for input: " + in + ", of type: " + type + ", use a smaller value/less significant figures");
			return false;
		}
		if(ok) out = value;
		return ok;
	}

	const std::vector<scalar_conv> lsc {
		{ "mm", 1, 0 },
		{ "m", 1000, 0 },
		{ "km", 1000000, 0 },
		{ "ft", 305, 0 },
		{ "yd", 914, 0 },
		{ "miles", 1609344, 0 },
	};
	bool Length(const std::string &in, uint64_t &out, error_collection &ec) {
		return DoConv(in, out, "length", lsc, ec);
	}

	const std::vector<scalar_conv> ssc {
		{ "mm/s", 1, 0 },
		{ "m/s", 1000, 0 },
		{ "km/h", 278, 0 },
		{ "mph", 447, 0 },
	};
	bool Speed(const std::string &in, uint64_t &out, error_collection &ec) {
		return DoConv(in, out, "speed", ssc, ec);
	}

	const std::vector<scalar_conv> tsc {
		{ "ms", 1, 0 },
		{ "s", 1000, 0 },
		{ "m", 60000, 0 },
		{ "min", 60000, 0 },
		{ "h", 3600000, 0 },
	};
	bool Time(const std::string &in, uint64_t &out, error_collection &ec) {
		return DoConv(in, out, "time", tsc, ec);
	}

	const std::vector<scalar_conv> fsc {
		{ "N", 1, 0 },
		{ "kN", 1000, 0 },
		{ "MN", 1000000, 0 },
		{ "lbf", 4555, 10 },
	};
	bool Force(const std::string &in, uint64_t &out, error_collection &ec) {
		return DoConv(in, out, "force", fsc, ec);
	}

	const std::vector<scalar_conv> msc {
		{ "kg", 1, 0 },
		{ "t", 1000, 0 },
		{ "st", 907, 0 },
		{ "lt", 1016, 0 },
	};
	bool Mass(const std::string &in, uint64_t &out, error_collection &ec) {
		return DoConv(in, out, "mass", msc, ec);
	}

	const std::vector<scalar_conv> psc {
		{ "W", 1, 0 },
		{ "kW", 1000, 0 },
		{ "MW", 1000000, 0 },
		{ "GW", 1000000000, 0 },
		{ "hp", 746, 0 },
		{ "ft lbf/s", 684, 9 },
	};
	bool Power(const std::string &in, uint64_t &out, error_collection &ec) {
		return DoConv(in, out, "power", psc, ec);
	}

	const std::vector<scalar_conv> dfssc {
		{ "N/(m/s)", 1, 0 },
	};
	bool ForcePerSpeedCoeff(const std::string &in, uint64_t &out, error_collection &ec) {
		return DoConv(in, out, "drag force per unit speed", dfssc, ec);
	}

	const std::vector<scalar_conv> dfs2sc {
		{ "N/(m/s)^2", 1, 0 },
	};
	bool ForcePerSpeedSqCoeff(const std::string &in, uint64_t &out, error_collection &ec) {
		return DoConv(in, out, "drag force per square unit speed", dfs2sc, ec);
	}
}
