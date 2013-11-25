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

#ifndef INC_DRAWMOD_UNITXT_ALREADY
#define INC_DRAWMOD_UNITXT_ALREADY

#include "draw/drawmodule.h"

namespace draw {

	class draw_module_ukgeneric : public draw_module {
		virtual draw_func_type GetDrawTrack(const std::shared_ptr<guilayout::layouttrack_obj> &obj, error_collection &ec) override;
		virtual draw_func_type GetDrawBerth(const std::shared_ptr<guilayout::layoutberth_obj> &obj, error_collection &ec) override;
		virtual draw_func_type GetDrawObj(const std::shared_ptr<guilayout::layoutgui_obj> &obj, error_collection &ec) override;
		virtual void BuildSprite(sprite_ref sr, sprite_obj &sp, const draw_options &dopt) override;   // must be re-entrant
	};

};

#endif
