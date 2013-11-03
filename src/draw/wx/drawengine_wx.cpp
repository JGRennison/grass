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

#include "draw/wx/drawengine_wx.h"
#include "draw/drawmodule.h"
#include <wx/mstream.h>

namespace draw {

	bool wx_sprite_obj::LoadFromFile(const std::string &file) {
		wxString str;
		str.FromUTF8(file.c_str(), file.length());
		return img.LoadFile(str);
	}

	bool wx_sprite_obj::LoadFromData(void *data, size_t length) {
		wxMemoryInputStream memstream(data, length);
		return img.LoadFile(memstream);
	}

	void wx_sprite_obj::LoadFromSprite(sprite_ref sr) {
		wx_sprite_obj &src = eng->GetSpriteObj(sr, GST::IMG);
		img = src.img;
	}

	void wx_sprite_obj::FillColour(uint32_t rgb) {
		img.Create(eng->GetSpriteWidth(), eng->GetSpriteHeight(), true);
		if(rgb) {
			img.Replace(0, 0, 0,
				(rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
		}
	}

	void wx_sprite_obj::ReplaceColour(uint32_t rgb_src, uint32_t rgb_dest) {
		img.Replace((rgb_src >> 16) & 0xFF, (rgb_src >> 8) & 0xFF, rgb_src & 0xFF,
			(rgb_dest >> 16) & 0xFF, (rgb_dest >> 8) & 0xFF, rgb_dest & 0xFF);
	}

	void wx_sprite_obj::CheckType(GST type) {
		if(type == GST::IMG || type == GST::BMP) {
			if(!img.IsOk()) {
				eng->dmod->BuildSprite(this_sr, *this, *(eng->dopt));
			}
			if(!img.IsOk()) {
				// image is failed, use a nice pink block instead to make it obvious
				img.Create(eng->GetSpriteWidth(), eng->GetSpriteHeight(), true);
				img.Replace(0, 0, 0, 0xFF, 0x7F, 0x7F);
			}
		}
		if(type == GST::BMP) {
			if(!bmp.IsOk()) {
				if(eng->GetSpriteWidth() != (unsigned int) img.GetWidth() || eng->GetSpriteHeight() != (unsigned int) img.GetHeight()) {
					wxImage simg = img.Scale(eng->GetSpriteWidth(), eng->GetSpriteHeight());
					bmp = wxBitmap(simg);
				}
			}
		}
	}

	wx_draw_engine::wx_draw_engine(std::shared_ptr<draw_module> dmod_, unsigned int sw, unsigned int sh, std::shared_ptr<draw_options> dopt_)
		: draw_engine(dmod_, sw, sh, dopt_) {
		wxInitAllImageHandlers();
	}

	wx_sprite_obj &wx_draw_engine::GetSpriteObj(sprite_ref sr, wx_sprite_obj::GST type) {
		auto sp = sprites.insert(std::make_pair(sr, wx_sprite_obj()));
		wx_sprite_obj &s = sp.first->second;
		if(sp.second) {
			// this is a new sprite
			s.this_sr = sr;
			s.eng = this;
		}
		s.CheckType(type);
		return s;
	}

};
