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
//  2014 - Jonathan Rennison <j.g.rennison@gmail.com>
//==========================================================================

#ifndef INC_TEXT_CMD_ALREADY
#define INC_TEXT_CMD_ALREADY

#include <string>
#include <vector>

class world;

struct text_command_handler {
	protected:
	const std::string &cmd;
	world &w;

	bool ProcessTokens(const std::vector<std::string> &tokens);
	void RegisterInputError() const;
	void RegisterCannotUseError(const std::string &item, const std::string &reasoncode) const;

	public:
	text_command_handler(const std::string &cmd_, world &w_)
			: cmd(cmd_), w(w_) { }
	bool Execute();
};

#endif
