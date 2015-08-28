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

#ifndef INC_DRAW_MOD_UK_GENERIC_ALREADY
#define INC_DRAW_MOD_UK_GENERIC_ALREADY

#include "draw/draw_module.h"

namespace draw {

	class draw_module_uk_generic : public draw_module {
		virtual draw_func_type GetDrawTrack(const std::shared_ptr<gui_layout::layout_track_obj> &obj, error_collection &ec) override;
		virtual draw_func_type GetDrawBerth(const std::shared_ptr<gui_layout::layout_berth_obj> &obj, error_collection &ec) override;
		virtual draw_func_type GetDrawObj(const std::shared_ptr<gui_layout::layout_gui_obj> &obj, error_collection &ec) override;
		virtual void BuildSprite(sprite_ref sr, sprite_obj &sp, const draw_options &dopt) override;   // must be re-entrant
	};

};

#endif
