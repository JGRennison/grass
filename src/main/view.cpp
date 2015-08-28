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

#include "main/view.h"
#include "layout/layout.h"
#include "util/util.h"
#include <wx/event.h>
#include <wx/dcclient.h>
#include <wx/region.h>

BEGIN_EVENT_TABLE(main_gui::gr_view_panel, wxScrolledWindow)
END_EVENT_TABLE()

main_gui::gr_view_panel::gr_view_panel(wxWindow *parent, std::shared_ptr<gui_layout::world_layout> layout_, std::shared_ptr<draw::wx_draw_engine> eng_)
		: wxScrolledWindow(parent), layout(layout_), eng(eng_) {
	SetBackgroundColour(*wxBLACK);
}

void main_gui::gr_view_panel::OnDraw(wxDC& dc) {
	std::map<std::pair<int, int>, const gui_layout::pos_sprite_desc *> redrawsprites;

	wxRegionIterator upd(GetUpdateRegion());
	while (upd) {
		wxRect rect(upd.GetRect());
		CalcUnscrolledPosition(rect.x, rect.y, &rect.x, &rect.y);
		int x1 = layout_origin_x + (rect.GetLeft() / (int) eng->GetSpriteWidth());
		int x2 = layout_origin_x + 1 + (rect.GetRight() / (int) eng->GetSpriteWidth());
		int y1 = layout_origin_y + (rect.GetTop() / (int) eng->GetSpriteHeight());
		int y2 = layout_origin_y + 1 + (rect.GetBottom() / (int) eng->GetSpriteHeight());
		layout->GetSpritesInRect(x1, x2, y1, y2, redrawsprites);
		upd++;
	}

	for (auto &obj : redrawsprites) {
		int x, y, wx, wy;
		std::tie(x, y) = obj.first;
		wx = (x - layout_origin_x) * eng->GetSpriteWidth();
		wy = (y - layout_origin_y) * eng->GetSpriteHeight();

		if (obj.second->text) {
			draw::draw_text_char &dt = *(obj.second->text);
			draw::wx_sprite_obj txt(eng.get(), 0);
			txt.DrawTextChar(dt.text, dt.foregroundcolour, dt.backgroundcolour);
			dc.DrawBitmap(txt.GetSpriteBitmap(), wx, wy, false);
		} else {
			const wxBitmap &sprite = eng->GetSpriteBitmap(obj.second->sprite);
			dc.DrawBitmap(sprite, wx, wy, false);
		}
	}
}

void main_gui::gr_view_panel::InitLayout() {
	int x1, x2, y1, y2;
	layout->GetLayoutExtents(x1, x2, y1, y2, 2);
	layout_origin_x = x1;
	layout_origin_y = y1;
	int w = std::max(x2 - x1, 1);
	int h = std::max(y2 - y1, 1);

	SetScrollbars(eng->GetSpriteWidth(), eng->GetSpriteHeight(), w, h);
	Refresh(true);
}

void main_gui::gr_view_panel::RefreshSprites(int x, int y, int w, int h) {
	int wx = (x - layout_origin_x) * eng->GetSpriteWidth();
	int wy = (y - layout_origin_y) * eng->GetSpriteHeight();
	int dx = w * eng->GetSpriteWidth();
	int dy = h * eng->GetSpriteHeight();
	RefreshRect(wxRect(wx, wy, dx, dy));
}

main_gui::gr_view_win::gr_view_win(std::shared_ptr<gui_layout::world_layout> layout_, std::shared_ptr<draw::wx_draw_engine> eng_, std::shared_ptr<main_gui::gr_view_winlist> winlist_)
	: wxFrame(0, wxID_ANY, wxT("GRASS")), winlist(std::move(winlist_)) {
	panel = new gr_view_panel(this, std::move(layout_), std::move(eng_));
	panel->InitLayout();
	winlist->viewpanels.emplace_back(panel);
	winlist->toplevelpanels.emplace_back(this);
}

main_gui::gr_view_win::~gr_view_win() {
	container_unordered_remove(winlist->viewpanels, panel);
	container_unordered_remove(winlist->toplevelpanels, this);
}
