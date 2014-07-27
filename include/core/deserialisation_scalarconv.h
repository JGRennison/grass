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

#ifndef INC_DESERIALISATION_SCALARCONV_ALREADY
#define INC_DESERIALISATION_SCALARCONV_ALREADY

#include <string>

class error_collection;

namespace dsconv {
	bool Length(const std::string &in, uint64_t &out, error_collection &ec);
	bool Speed(const std::string &in, uint64_t &out, error_collection &ec);
	bool Time(const std::string &in, uint64_t &out, error_collection &ec);
	bool Force(const std::string &in, uint64_t &out, error_collection &ec);
	bool Mass(const std::string &in, uint64_t &out, error_collection &ec);
	bool Power(const std::string &in, uint64_t &out, error_collection &ec);
	bool ForcePerSpeedCoeff(const std::string &in, uint64_t &out, error_collection &ec);
	bool ForcePerSpeedSqCoeff(const std::string &in, uint64_t &out, error_collection &ec);
}

#endif
