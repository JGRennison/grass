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

namespace draw {

	class wx_draw_engine;

	class wx_sprite_obj : public sprite_obj {
		friend wx_draw_engine;
		sprite_ref this_sr = 0;
		wxImage img;
		wxBitmap bmp;
		wx_draw_engine *eng = 0;

		enum class GST {
			OBJ,
			IMG,
			BMP,
		};

		void CheckType(GST type);

		public:
		virtual bool LoadFromFile(const std::string &file) override;
		virtual bool LoadFromData(void *data, size_t length) override;
		virtual void LoadFromSprite(sprite_ref sr) override;
		virtual void FillColour(uint32_t rgb) override;
		virtual void ReplaceColour(uint32_t rgb_src, uint32_t rgb_dest) override;
		virtual void DrawTextChar(const std::string &text, uint32_t foregroundcolour, uint32_t backgroundcolour) override;

	};

	class wx_draw_engine : public draw_engine {
		friend wx_sprite_obj;
		std::unordered_map<sprite_ref, wx_sprite_obj> sprites;
		wx_sprite_obj &GetSpriteObj(sprite_ref sr, wx_sprite_obj::GST type);

		public:
		wx_draw_engine(std::shared_ptr<draw_module> dmod_, unsigned int sw, unsigned int sh, std::shared_ptr<draw_options> dopt_);
		std::string GetName() const { return "wx draw engine"; }
	};

};

#endif
