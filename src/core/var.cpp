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

#include "core/var.h"
#include <algorithm>

std::string message_formatter::FormatMessage(const std::string &input) const {
	std::string output;
	for(auto it = input.begin(); it != input.end();) {
		if(*it == '\\') {
			++it;
			if(it != input.end()) output.push_back(*it);
			else break;
			++it;
		}
		else if(*it == '$') {
			++it;
			if(it == input.end()) break;
			output.append(ExpandVariable(it, input.end()));
		}
		else {
			output.push_back(*it);
			++it;
		}
	}
	return output;
}

void message_formatter::RegisterVariable(const std::string &name, mf_lambda func) {
	registered_variables[name] = func;
}

std::string message_formatter::ExpandVariable(std::string::const_iterator &begin, std::string::const_iterator end) const {
	std::string varname;
	std::string param;
	for(; begin != end; ++begin) {
		if(IsVarChar(*begin)) varname.push_back(*begin);
		else break;
	}

	if(varname.empty()) return "";

	if(begin != end && *begin == '(') {
		for(++begin; begin != end; ++begin) {
			if(*begin == '\\') {
				++begin;
				if(begin != end) param.push_back(*begin);
				else break;
			}
			else if(*begin == ')') {
				++begin;
				break;
			}
			else param.push_back(*begin);
		}
	}

	auto targetvar = registered_variables.find(varname);
	if(targetvar != registered_variables.end()) {
		return targetvar->second(param);
	}
	else return "[No Such Variable]";
}
