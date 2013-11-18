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

#ifndef INC_MAIN_VIEW_ALREADY
#define INC_MAIN_VIEW_ALREADY

#include "layout/layout.h"
#include "draw/wx/drawengine_wx.h"
#include <memory>
#include <wx/scrolwin.h>
#include <set>

namespace maingui {

	class grviewpanel : public wxScrolledWindow {
		std::shared_ptr<guilayout::world_layout> layout;
		std::shared_ptr<draw::wx_draw_engine> eng;
		int layout_origin_x = 0;
		int layout_origin_y = 0;

		public:
		grviewpanel(std::shared_ptr<guilayout::world_layout> layout_, std::shared_ptr<draw::wx_draw_engine> eng_);
		void OnDraw(wxDC& dc);

		DECLARE_EVENT_TABLE()
	};
};
#endif
