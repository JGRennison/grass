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
//  2016 - Jonathan Rennison <j.g.rennison@gmail.com>
//==========================================================================

#ifndef INC_MAIN_TEXT_CMD_MAIN_ALREADY
#define INC_MAIN_TEXT_CMD_MAIN_ALREADY

#include "text_cmd/text_cmd.h"

struct text_cmd_main : public text_command_handler {
	text_cmd_main(const std::string &cmd_, world &w_)
			: text_command_handler(cmd_, w_) { }
	virtual bool ProcessTokens(const std::vector<std::string> &tokens) override;
};

#endif
