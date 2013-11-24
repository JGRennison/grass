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

#include "main/view.h"
#include "layout/layout.h"
#include <wx/event.h>
#include <wx/dcclient.h>
#include <wx/region.h>

BEGIN_EVENT_TABLE(maingui::grviewpanel, wxScrolledWindow)
END_EVENT_TABLE()

maingui::grviewpanel::grviewpanel(std::shared_ptr<guilayout::world_layout> layout_, std::shared_ptr<draw::wx_draw_engine> eng_)
		: layout(layout_), eng(eng_) {
	InitLayout();
}

void maingui::grviewpanel::OnDraw(wxDC& dc) {
	std::map<std::pair<int, int>, const guilayout::pos_sprite_desc *> redrawsprites;

	wxRegionIterator upd(GetUpdateRegion());
	while(upd) {
		wxRect rect(upd.GetRect());
		CalcUnscrolledPosition(rect.x, rect.y, &rect.x, &rect.y);
		int x1 = (rect.GetLeft() + layout_origin_x) / eng->GetSpriteWidth();
		int x2 = (rect.GetRight() + 1 +layout_origin_x) / eng->GetSpriteWidth();
		int y1 = (rect.GetTop() + layout_origin_y) / eng->GetSpriteHeight();
		int y2 = (rect.GetBottom() + 1 + layout_origin_y) / eng->GetSpriteHeight();
		layout->GetSpritesInRect(x1, x2, y1, y2, redrawsprites);
		upd++;
	}

	for(auto &obj : redrawsprites) {
		int x, y, wx, wy;
		std::tie(x, y) = obj.first;
		wx = (x - layout_origin_x) * eng->GetSpriteWidth();
		wy = (y - layout_origin_y) * eng->GetSpriteHeight();
		CalcScrolledPosition(wx, wy, &wx, &wy);
		if(obj.second->text) {
			draw::drawtextchar &dt = *(obj.second->text);
			draw::wx_sprite_obj txt(eng.get(), 0);
			txt.DrawTextChar(dt.text, dt.foregroundcolour, dt.backgroundcolour);
			dc.DrawBitmap(txt.GetSpriteBitmap(), wx, wy, false);
		}
		else {
			const wxBitmap &sprite = eng->GetSpriteBitmap(obj.second->sprite);
			dc.DrawBitmap(sprite, wx, wy, false);
		}
	}
}

void maingui::grviewpanel::InitLayout() {
	int x1, x2, y1, y2;
	layout->GetLayoutExtents(x1, x2, y1, y2, 2);
	layout_origin_x = x1;
	layout_origin_y = y1;
	SetVirtualSize((x2 - x1) * eng->GetSpriteWidth(), (y2 - y1) * eng->GetSpriteHeight());
	SetScrollbars(eng->GetSpriteWidth(), eng->GetSpriteHeight(), x2 - x1, y2 - y1);
}
