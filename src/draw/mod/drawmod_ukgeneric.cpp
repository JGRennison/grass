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

#include "draw/mod/drawmod_ukgeneric.h"
#include "layout/layout.h"
#include "core/track.h"
#include "core/signal.h"
#include "core/routetypes.h"
#include "core/trackpiece.h"
#include <tuple>

#define BERTHLEVEL 10
#define UNKNOWNLEVEL 20

namespace draw {

	draw_func_type draw_module_ukgeneric::GetDrawTrack(const std::shared_ptr<guilayout::layouttrack_obj> &obj, error_collection &ec) {
		int x, y;
		std::tie(x, y) = obj->GetPosition();

		const generictrack *gt = obj->GetTrack();
		const genericsignal *gs = FastSignalCast(gt);
		if(gs) {
			return [x, y, gs, obj](const draw_engine &eng, guilayout::world_layout &layout) {
				//temporary drawing function
				std::unique_ptr<draw::drawtextchar> drawtext(new draw::drawtextchar);
				drawtext->backgroundcolour = 0;
				if(gs->GetAspect() == 0) {
					drawtext->text = "R";
					drawtext->foregroundcolour = 0xFF0000;
				}
				else if(route_class::IsRoute(gs->GetAspectType())) {
					if(gs->GetAspect() == 1) {
						drawtext->text = "Y";
						drawtext->foregroundcolour = 0xFFFF00;
					}
					else if(gs->GetAspect() == 2) {
						drawtext->text = "D";
						drawtext->foregroundcolour = 0xFFFF00;
					}
					else if(gs->GetAspect() == 3) {
						drawtext->text = "G";
						drawtext->foregroundcolour = 0x00FF00;
					}
				}
				else if(route_class::IsShunt(gs->GetAspectType())) {
					drawtext->text = "S";
					drawtext->foregroundcolour = 0xFFFFFF;
				}
				layout.SetTextChar(x, y, std::move(drawtext), obj, 0);
			};
		}

		const trackseg *ts = dynamic_cast<const trackseg *>(gt);
		if(ts) {
			struct tracksegpointinfo {
				int x;
				int y;
				guilayout::LAYOUT_DIR direction;
			};
			auto points = std::make_shared<std::vector<tracksegpointinfo> >();

			guilayout::LayoutOffsetDirection(x, y, obj->GetLayoutDirection(), obj->GetLength(), [&](int sx, int sy, guilayout::LAYOUT_DIR sdir) {
				points->push_back({sx, sy, sdir});
			});

			return [points, ts, obj](const draw_engine &eng, guilayout::world_layout &layout) {
				for(auto &it : *points) {
					//temporary drawing function
					std::unique_ptr<draw::drawtextchar> drawtext(new draw::drawtextchar);
					drawtext->text = "T";
					drawtext->foregroundcolour = 0xFFFFFF;
					drawtext->backgroundcolour = 0;
					layout.SetTextChar(it.x, it.y, std::move(drawtext), obj, 0);
				}
			};
		}

		//default:
		return [x, y, obj](const draw_engine &eng, guilayout::world_layout &layout) {
			std::unique_ptr<draw::drawtextchar> drawtext(new draw::drawtextchar);
			drawtext->foregroundcolour = 0xFFFF00;
			drawtext->backgroundcolour = 0x0000FF;
			drawtext->text = "?";
			layout.SetTextChar(x, y, std::move(drawtext), obj, UNKNOWNLEVEL);
		};
	}

	draw_func_type draw_module_ukgeneric::GetDrawBerth(const std::shared_ptr<guilayout::layoutberth_obj> &obj, error_collection &ec) {
		struct drawberthdata {
			int length;
			const trackberth *b;
			int x, y;
		};

		auto berthdataptr = std::make_shared<drawberthdata>();
		berthdataptr->length = obj->GetLength();
		berthdataptr->b = obj->GetBerth();
		std::tie(berthdataptr->x, berthdataptr->y) = obj->GetPosition();

		return [berthdataptr, obj](const draw_engine &eng, guilayout::world_layout &layout) {
			const std::string &btext = berthdataptr->b->contents;
			if(!btext.empty()) {
				std::unique_ptr<draw::drawtextchar> drawtext(new draw::drawtextchar);
				drawtext->text = btext;
				drawtext->foregroundcolour = 0xFFFFFF;
				drawtext->backgroundcolour = 0;
				layout.SetTextString(berthdataptr->x, berthdataptr->y, std::move(drawtext), obj, BERTHLEVEL, berthdataptr->length, berthdataptr->length);
			}
			else {
				for(int i = 0; i < berthdataptr->length; i++) {
					layout.ClearSpriteLevel(berthdataptr->x + i, berthdataptr->y, BERTHLEVEL);
				}
			}
		};
	}

	draw_func_type draw_module_ukgeneric::GetDrawObj(const std::shared_ptr<guilayout::layoutgui_obj> &obj, error_collection &ec) {
		return [](const draw_engine &eng, guilayout::world_layout &layout) {

		};
	}

	void draw_module_ukgeneric::BuildSprite(sprite_ref sr, sprite_obj &sp, const draw_options &dopt) {  // must be re-entrant

	}

};
