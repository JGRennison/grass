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

#ifndef INC_MAIN_MAIN_GUI_ALREADY
#define INC_MAIN_MAIN_GUI_ALREADY

#include <vector>

namespace main_gui {

	class gr_view_panel;
	class gr_view_win;
	class trainwin;
	class cmd_input_win;

	struct gr_view_winlist {
		std::vector<gr_view_panel *> viewpanels;
		std::vector<gr_view_win *> toplevelpanels;
		std::vector<trainwin *> trainwins;
		std::vector<cmd_input_win *> cmd_input_wins;
	};

};

#endif
