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

#ifndef INC_DRAWENGINE_ALREADY
#define INC_DRAWENGINE_ALREADY

#include "draw/drawtypes.h"
#include <string>
#include <memory>

namespace draw {

	class draw_module;

	class sprite_obj {
		public:
		virtual bool LoadFromFile(const std::string &file) = 0;
		virtual bool LoadFromData(void *data, size_t length) = 0;
		virtual void LoadFromSprite(sprite_ref sr) = 0;
		virtual void FillColour(uint32_t rgb) = 0;
		virtual void ReplaceColour(uint32_t rgb_src, uint32_t rgb_dest) = 0;
		virtual void DrawTextChar(const std::string &text, uint32_t foregroundcolour, uint32_t backgroundcolour) = 0;
		virtual void Mirror(bool horizontally) = 0;
	};

	class draw_engine {
		protected:
		std::shared_ptr<draw_module> dmod;
		unsigned int spritewidth;
		unsigned int spriteheight;
		std::shared_ptr<draw_options> dopt;

		public:
		draw_engine(std::shared_ptr<draw_module> dmod_, unsigned int sw, unsigned int sh, std::shared_ptr<draw_options> dopt_)
			: dmod(dmod_), spritewidth(sw), spriteheight(sh), dopt(dopt_) { }
		virtual std::string GetName() const = 0;
		unsigned int GetSpriteWidth() const { return spritewidth; }
		unsigned int GetSpriteHeight() const { return spriteheight; }
		const std::shared_ptr<draw_options> &GetOptions() const { return dopt; }
	};

};

#endif
