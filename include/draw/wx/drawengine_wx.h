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

#ifndef INC_DRAWENGINE_WX_ALREADY
#define INC_DRAWENGINE_WX_ALREADY

#include <unordered_map>
#include "draw/drawengine.h"
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/font.h>

namespace draw {

	class wx_draw_engine;

	class wx_sprite_obj : public sprite_obj {
		friend wx_draw_engine;
		sprite_ref this_sr;
		wxImage img;
		wxBitmap bmp;
		wx_draw_engine *eng;

		enum class GST {
			OBJ,
			IMG,
			BMP,
		};

		void CheckType(GST type);

		wx_sprite_obj()
			: this_sr(0), eng(0) { }

		public:
		wx_sprite_obj(wx_draw_engine *eng_, sprite_ref this_sr_)
			: this_sr(this_sr_), eng(eng_) { }
		virtual bool LoadFromFile(const std::string &file) override;
		virtual bool LoadFromData(void *data, size_t length) override;
		virtual void LoadFromSprite(sprite_ref sr) override;
		virtual void FillColour(uint32_t rgb) override;
		virtual void ReplaceColour(uint32_t rgb_src, uint32_t rgb_dest) override;
		virtual void DrawTextChar(const std::string &text, uint32_t foregroundcolour, uint32_t backgroundcolour) override;
		virtual void Mirror(bool horizontally) override;
		virtual void Rotate90(bool clockwise) override;
		const wxBitmap &GetSpriteBitmap() {
			CheckType(wx_sprite_obj::GST::BMP);
			return bmp;
		}
		const wxImage &GetSpriteImage() {
			CheckType(wx_sprite_obj::GST::IMG);
			return img;
		}

	};

	class wx_draw_engine : public draw_engine {
		friend wx_sprite_obj;
		std::unordered_map<sprite_ref, wx_sprite_obj> sprites;
		wx_sprite_obj &GetSpriteObj(sprite_ref sr, wx_sprite_obj::GST type);
		wxFont textfont;

		public:
		wx_draw_engine(std::shared_ptr<draw_module> dmod_, unsigned int sw, unsigned int sh, std::shared_ptr<draw_options> dopt_);
		std::string GetName() const override { return "wx draw engine"; }
		const wxBitmap &GetSpriteBitmap(sprite_ref sr) { return GetSpriteObj(sr, wx_sprite_obj::GST::BMP).bmp; }
		const wxImage &GetSpriteImage(sprite_ref sr) { return GetSpriteObj(sr, wx_sprite_obj::GST::IMG).img; }
		const wxFont &GetTextFont() const { return textfont; }
	};

};

#endif
