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
	inline bool operator==(const draw_text_char &lhs, const draw_text_char &rhs){
		return std::tie(lhs.text, lhs.foregroundcolour, lhs.backgroundcolour) ==
				std::tie(rhs.text, rhs.foregroundcolour, rhs.backgroundcolour);
	}

};

namespace std {
	template<> struct hash<draw::draw_text_char> {
		typedef draw::draw_text_char argument_type;
		typedef std::size_t result_type;

		result_type operator()(argument_type const& s) const {
			result_type const h1 (std::hash<std::string>()(s.text));
			result_type const h2 (std::hash<uint32_t>()(s.foregroundcolour));
			result_type const h3 (std::hash<uint32_t>()(s.backgroundcolour));
			return h1 ^ h2 ^ h3;
		}
	};
}

#endif
