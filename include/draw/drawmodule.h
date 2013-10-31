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

#ifndef INC_DRAWMODULE_ALREADY
#define INC_DRAWMODULE_ALREADY

#include <memory>
#include "core/error.h"
#include "draw/drawtypes.h"

namespace guilayout {
	class layouttrack_obj;
	class layoutberth_obj;
	class layoutgui_obj;
};

namespace draw {
	class draw_engine;

	class draw_module : public std::enable_shared_from_this<draw_module> {
		std::shared_ptr<draw_engine> eng;

		public:
		draw_module(std::shared_ptr<draw_engine> eng_) : eng(eng_) { }
		virtual draw_func_type GetDrawTrack(const guilayout::layouttrack_obj &obj, error_collection &ec);
		virtual draw_func_type GetDrawBerth(const guilayout::layoutberth_obj &obj, error_collection &ec);
		virtual draw_func_type GetDrawObj(const guilayout::layoutgui_obj &obj, error_collection &ec);
	};

};

#endif
