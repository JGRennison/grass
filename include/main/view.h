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

#ifndef INC_MAIN_VIEW_ALREADY
#define INC_MAIN_VIEW_ALREADY

#include "layout/layout.h"
#include "draw/wx/draw_engine_wx.h"
#include "main/main_gui.h"
#include <memory>
#include <wx/scrolwin.h>
#include <wx/frame.h>
#include <set>

namespace main_gui {

	class gr_view_panel : public wxScrolledWindow {
		std::shared_ptr<gui_layout::world_layout> layout;
		std::shared_ptr<draw::wx_draw_engine> eng;
		int layout_origin_x = 0;
		int layout_origin_y = 0;

		public:
		gr_view_panel(wxWindow *parent, std::shared_ptr<gui_layout::world_layout> layout_, std::shared_ptr<draw::wx_draw_engine> eng_);
		void OnDraw(wxDC& dc);
		void InitLayout();
		void RefreshSprites(int x, int y, int w = 1, int h = 1);

		DECLARE_EVENT_TABLE()
	};

	class gr_view_win : public wxFrame {
		gr_view_panel *panel;
		std::shared_ptr<gr_view_winlist> winlist;

		public:
		gr_view_win(std::shared_ptr<gui_layout::world_layout> layout_, std::shared_ptr<draw::wx_draw_engine> eng_, std::shared_ptr<gr_view_winlist> winlist_);
		~gr_view_win();
	};

};
#endif
