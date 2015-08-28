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

#ifndef INC_DRAW_TYPES_ALREADY
#define INC_DRAW_TYPES_ALREADY

#include <functional>
#include <cstdint>

namespace gui_layout {
	class world_layout;
};

namespace draw {
	struct draw_options;
	class draw_module;
	class draw_engine;

	typedef uint32_t sprite_ref;
	typedef std::function<void(const draw_engine &, gui_layout::world_layout &)> draw_func_type;

	struct draw_text_char {
		std::string text;
		uint32_t foregroundcolour;
		uint32_t backgroundcolour;
	};
};

#endif
