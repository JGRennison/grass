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
#include "draw/drawres.h"
#include "draw/drawengine.h"
#include "layout/layout.h"
#include "core/track.h"
#include "core/signal.h"
#include "core/routetypes.h"
#include "core/trackpiece.h"
#include "core/trackcircuit.h"
#include <tuple>

IMG_EXTERN(track_v_png, track_v)
IMG_EXTERN(track_h_png, track_h)
IMG_EXTERN(track_d_png, track_d)

#define BERTHLEVEL 10
#define UNKNOWNLEVEL 20

namespace {
	enum sprite_ids {
		SID_typemask        = 0xFF,
		SID_trackseg        = 1,

		SID_dirmask         = 0xF00,
		SID_dirshift        = 8,

		SID_raw_img         = 0x1000,

		SID_has_tc          = 0x2000,
		SID_tc_occ          = 0x4000,
	};

	guilayout::LAYOUT_DIR spriteid_to_layoutdir(draw::sprite_ref sid) {
		return static_cast<guilayout::LAYOUT_DIR>((sid & SID_dirmask) >> SID_dirshift);
	}
	draw::sprite_ref layoutdir_to_spriteid(guilayout::LAYOUT_DIR dir) {
		return static_cast<std::underlying_type<guilayout::LAYOUT_DIR>::type>(dir) << SID_dirshift;
	}
}

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
				draw::sprite_ref base_sprite;
			};
			auto points = std::make_shared<std::vector<tracksegpointinfo> >();

			guilayout::LayoutOffsetDirection(x, y, obj->GetLayoutDirection(), obj->GetLength(), [&](int sx, int sy, guilayout::LAYOUT_DIR sdir) {
				draw::sprite_ref base_sprite = SID_trackseg | layoutdir_to_spriteid(guilayout::FoldLayoutDirection(sdir));
				if(ts->GetTrackCircuit()) base_sprite |= SID_has_tc;
				points->push_back({sx, sy, sdir, base_sprite});
			});

			return [points, ts, obj](const draw_engine &eng, guilayout::world_layout &layout) {
				for(auto &it : *points) {
					//temporary drawing function
					draw::sprite_ref spid = it.base_sprite;
					track_circuit *tc = ts->GetTrackCircuit();
					if(tc && tc->Occupied()) spid |= SID_tc_occ;
					layout.SetSprite(it.x, it.y, spid, obj, 0);
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
		using guilayout::LAYOUT_DIR;

		auto getdirchangespid = [&](LAYOUT_DIR newdir) -> sprite_ref {
			return (sr & ~SID_dirmask) | layoutdir_to_spriteid(newdir);
		};

		enum {
			DIRF_H         = 1<<0,
		};

		//this converts a folded direction into either U, UR or:
		// if DIRF_H is set: R
		//returns true if handled
		auto dirrelative1 = [&](LAYOUT_DIR dir, unsigned int flags) -> bool {
			switch(dir) {
				case LAYOUT_DIR::U:
					return false;
				case LAYOUT_DIR::UR:
					return false;
				case LAYOUT_DIR::RU:
					sp.LoadFromSprite(getdirchangespid(LAYOUT_DIR::RD));
					sp.Mirror(false);
					return true;
				case LAYOUT_DIR::R:
					if(flags & DIRF_H) return false;
					sp.LoadFromSprite(getdirchangespid(LAYOUT_DIR::U));
					sp.Rotate90(true);
					return true;
				case LAYOUT_DIR::RD:
					sp.LoadFromSprite(getdirchangespid(LAYOUT_DIR::UR));
					sp.Mirror(true);
					return true;
				case LAYOUT_DIR::DR:
					sp.LoadFromSprite(getdirchangespid(LAYOUT_DIR::UR));
					sp.Mirror(false);
					return true;
				default:
					return false;
			}
		};

		unsigned int type = sr & SID_typemask;
		LAYOUT_DIR dir = spriteid_to_layoutdir(sr);

		if(sr & SID_raw_img) {
			switch(type) {
				case SID_trackseg:
					if(dirrelative1(dir, DIRF_H)) return;
					if(dir == LAYOUT_DIR::U) {
						sp.LoadFromFileDataFallback("track_v.png", GetImageData_track_v());
						return;
					}
					if(dir == LAYOUT_DIR::UR) {
						sp.LoadFromFileDataFallback("track_d.png", GetImageData_track_d());
						return;
					}
					if(dir == LAYOUT_DIR::R) {
						sp.LoadFromFileDataFallback("track_h.png", GetImageData_track_h());
						return;
					}
					break;

				default:
					break;
			}
		}
		else {
			switch(type) {
				case SID_trackseg:
					sp.LoadFromSprite((sr & (SID_typemask | SID_dirmask)) | SID_raw_img);
					if(sr & SID_has_tc) sp.ReplaceColour(0x0000FF, 0xFF0000);
					else sp.ReplaceColour(0x0000FF, 0x000000);
					if(!(sr & SID_tc_occ)) sp.ReplaceColour(0xFF0000, 0xAAAAAA);
					return;

				default:
					break;
			}
		}

		sp.DrawTextChar("?", 0xFF00FF, 0x00FF00);
	}

};
