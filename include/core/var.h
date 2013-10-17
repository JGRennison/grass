//  grass - Generic Rail And Signalling Simulator
//
//  WEBSITE: http://bitbucket.org/JGRennison/grass
//  WEBSITE: http://grss.sourceforge.net
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

#ifndef INC_VAR_ALREADY
#define INC_VAR_ALREADY

#include <string>
#include <functional>
#include <cctype>
#include <map>

struct message_formatter {
	typedef std::function<std::string(std::string)> mf_lambda;
	std::map<std::string, mf_lambda> registered_variables;

	public:
	std::string FormatMessage(const std::string &input) const;
	void RegisterVariable(const std::string &name, mf_lambda func);

	private:
	std::string ExpandVariable(std::string::const_iterator &begin, std::string::const_iterator end) const;

	inline bool IsVarChar(char c) const {
		return isalpha(c) || (c == '-');
	}
};

#endif
