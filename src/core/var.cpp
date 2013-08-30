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

#include "var.h"
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
