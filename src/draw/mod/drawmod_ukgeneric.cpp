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

#include "draw/mod/drawmod_ukgeneric.h"
#include "draw/drawres.h"
#include "draw/drawengine.h"
#include "layout/layout.h"
#include "core/track.h"
#include "core/signal.h"
#include "core/routetypes.h"
#include "core/trackpiece.h"
#include "core/trackcircuit.h"
#include "core/trackreservation.h"
#include "core/points.h"
#include <tuple>

IMG_EXTERN(track_v_png, track_v)
IMG_EXTERN(track_h_png, track_h)
IMG_EXTERN(track_d_png, track_d)
IMG_EXTERN(track_dv_png, track_dv)
IMG_EXTERN(track_dh_png, track_dh)
IMG_EXTERN(sigpost_png, sigpost)
IMG_EXTERN(sigpost_bar_png, sigpost_bar)
IMG_EXTERN(circle_png, circle)
IMG_EXTERN(shunt_png, shunt)
IMG_EXTERN(blank_png, blank)

#define BERTHLEVEL 10
#define UNKNOWNLEVEL 20

#define APPROACHLOCKING_FLASHINTERVAL 500
#define POINTS_OOC_FLASHINTERVAL 500

using guilayout::LAYOUT_DIR;

namespace {
	enum sprite_ids {
		SID_typemask        = 0xFF,
		SID_trackseg        = 1,
		SID_pointsooc       = 2,
		SID_signalroute     = 8,
		SID_signalshunt     = 9,
		SID_signalpost      = 16,
		SID_signalpostbar   = 17,

		SID_dirmask         = 0xF00,
		SID_dirshift        = 8,

		SID_raw_img         = 0x1000,

		SID_has_tc          = 0x2000,
		SID_tc_occ          = 0x4000,
		SID_reserved        = 0x8000,

		SID_signal_red      = 0x2000,
		SID_signal_yellow   = 0x4000,
		SID_signal_green    = 0x8000,
		SID_signal_blank    = 0x10000,
		SID_signal_unknown  = 0x20000,

		SID_shunt_red       = 0x2000,
		SID_shunt_white     = 0x4000,
		SID_shunt_grey      = 0x8000,
		SID_shunt_blank     = 0x10000,
	};

	LAYOUT_DIR spriteid_to_layoutdir(draw::sprite_ref sid) {
		return static_cast<LAYOUT_DIR>((sid & SID_dirmask) >> SID_dirshift);
	}
	draw::sprite_ref layoutdir_to_spriteid(LAYOUT_DIR dir) {
		return static_cast<std::underlying_type<LAYOUT_DIR>::type>(dir) << SID_dirshift;
	}
}

namespace draw {
	draw::sprite_ref track_sprite_extras(const generictrack *gt) {
		draw::sprite_ref extras = 0;
		const track_circuit *tc = gt->GetTrackCircuit();
		if (tc && tc->Occupied()) {
			extras |= SID_tc_occ;
		}
		reservationcountset rcs;
		gt->ReservationTypeCount(rcs);
		if (rcs.routeset > rcs.routesetauto) {
			extras |= SID_reserved;
		}
		return extras;
	}

	sprite_ref change_spriteid_layoutdir(LAYOUT_DIR newdir, sprite_ref sr) {
		return (sr & ~SID_dirmask) | layoutdir_to_spriteid(newdir);
	}

	// This converts a folded direction into either U, UR or R.
	// Returns true if handled
	bool dir_relative_fold(LAYOUT_DIR dir, draw::sprite_obj &sp, draw::sprite_ref sr) {
		switch (dir) {
			case LAYOUT_DIR::U:
				return false;

			case LAYOUT_DIR::UR:
				return false;

			case LAYOUT_DIR::RU:
				sp.LoadFromSprite(change_spriteid_layoutdir(LAYOUT_DIR::RD, sr));
				sp.Mirror(MIRROR::VERT);
				return true;

			case LAYOUT_DIR::R:
				return false;

			case LAYOUT_DIR::RD:
				sp.LoadFromSprite(change_spriteid_layoutdir(LAYOUT_DIR::UR, sr));
				sp.Mirror(MIRROR::HORIZ);
				return true;

			case LAYOUT_DIR::DR:
				sp.LoadFromSprite(change_spriteid_layoutdir(LAYOUT_DIR::UR, sr));
				sp.Mirror(MIRROR::VERT);
				return true;

			default:
				return false;
		}
	}

	// This converts a diagonal direction into either UR or LD,
	// Non-diagonal directions are unchanged
	// Returns true if handled
	bool dir_relative_diag(LAYOUT_DIR dir, draw::sprite_obj &sp, draw::sprite_ref sr) {
		switch (dir) {
			// vertical in
			case LAYOUT_DIR::UR:
				return false;

			case LAYOUT_DIR::UL:
				sp.LoadFromSprite(change_spriteid_layoutdir(LAYOUT_DIR::UR, sr));
				sp.Mirror(MIRROR::HORIZ);
				return true;

			case LAYOUT_DIR::DR:
				sp.LoadFromSprite(change_spriteid_layoutdir(LAYOUT_DIR::UR, sr));
				sp.Mirror(MIRROR::VERT);
				return true;

			case LAYOUT_DIR::DL:
				sp.LoadFromSprite(change_spriteid_layoutdir(LAYOUT_DIR::UL, sr));
				sp.Mirror(MIRROR::VERT);
				return true;

			// horizontal in
			case LAYOUT_DIR::LD:
				return false;

			case LAYOUT_DIR::LU:
				sp.LoadFromSprite(change_spriteid_layoutdir(LAYOUT_DIR::LD, sr));
				sp.Mirror(MIRROR::VERT);
				return true;

			case LAYOUT_DIR::RD:
				sp.LoadFromSprite(change_spriteid_layoutdir(LAYOUT_DIR::LD, sr));
				sp.Mirror(MIRROR::HORIZ);
				return true;

			case LAYOUT_DIR::RU:
				sp.LoadFromSprite(change_spriteid_layoutdir(LAYOUT_DIR::LU, sr));
				sp.Mirror(MIRROR::HORIZ);
				return true;

			default:
				return false;
		}
	}

	struct points_sprites {
		draw::sprite_ref base_sprite_ooc;
		draw::sprite_ref base_sprite_normal;
		draw::sprite_ref base_sprite_reverse;
	};

	points_sprites MakePointsSprites(const generictrack *gt, const guilayout::layouttrack_obj::points_layout_info *layout_info) {
		points_sprites out;

		draw::sprite_ref base = 0;
		if (gt->GetTrackCircuit()) {
			base |= SID_has_tc;
		}

		LAYOUT_DIR turn_dir = (layout_info->facing == ReverseLayoutDirection(layout_info->normal))
				? layout_info->reverse : layout_info->normal;
		out.base_sprite_ooc = base | SID_pointsooc |
				layoutdir_to_spriteid(guilayout::EdgesToDirection(layout_info->facing, turn_dir));
		out.base_sprite_normal = base | SID_trackseg |
				layoutdir_to_spriteid(guilayout::FoldLayoutDirection(guilayout::EdgesToDirection(layout_info->facing, layout_info->normal)));
		out.base_sprite_reverse = base | SID_trackseg |
				layoutdir_to_spriteid(guilayout::FoldLayoutDirection(guilayout::EdgesToDirection(layout_info->facing, layout_info->reverse)));
		return out;
	}

	draw_func_type draw_module_ukgeneric::GetDrawTrack(const std::shared_ptr<guilayout::layouttrack_obj> &obj, error_collection &ec) {
		int x, y;
		std::tie(x, y) = obj->GetPosition();

		auto get_draw_error_func = [&](uint32_t foreground, uint32_t background) -> draw_func_type {
			return [x, y, obj, foreground, background](const draw_engine &eng, guilayout::world_layout &layout) {
				std::unique_ptr<draw::drawtextchar> drawtext(new draw::drawtextchar);
				drawtext->foregroundcolour = foreground;
				drawtext->backgroundcolour = background;
				drawtext->text = "?";
				layout.SetTextChar(x, y, std::move(drawtext), obj, UNKNOWNLEVEL);
			};
		};

		const generictrack *gt = obj->GetTrack();
		const genericsignal *gs = FastSignalCast(gt);
		if (gs) {
			enum {
				SDF_STEP_RIGHT        = 1<<0,
				SDF_HAVE_SHUNT        = 1<<1,
				SDF_HAVE_ROUTE        = 1<<2,
				SDF_CAN_DISPLAY_DY    = 1<<3,
			};
			unsigned int sigflags = 0;

			switch (obj->GetLayoutDirection()) {
				case LAYOUT_DIR::UR:
				case LAYOUT_DIR::DR:
					sigflags |= SDF_STEP_RIGHT;
					break;

				case LAYOUT_DIR::UL:
				case LAYOUT_DIR::DL:
					sigflags &= ~SDF_STEP_RIGHT;
					break;

				default: {
					// Bad signal direction
					return get_draw_error_func(0x7F7FFF, 0);
				}
			}

			route_class::set route_types = gs->GetAvailableRouteTypes(EDGE_FRONT).start;
			if (route_types & route_class::AllRoutes()) {
				sigflags |= SDF_HAVE_ROUTE;
			}
			if (route_types & route_class::AllShunts()) {
				sigflags |= SDF_HAVE_SHUNT;
			}

			draw::sprite_ref stick_sprite = layoutdir_to_spriteid(guilayout::FoldLayoutDirection(obj->GetLayoutDirection()));
			if (gs->GetSignalFlags() & GSF::AUTOSIGNAL) {
				stick_sprite |= SID_signalpostbar;
			} else {
				stick_sprite |= SID_signalpost;
			}

			if (is_higher_aspect_in_mask(gs->GetRouteDefaults().aspect_mask, 2)) {
				sigflags |= SDF_CAN_DISPLAY_DY;
			}

			return [x, y, gs, obj, stick_sprite, sigflags](const draw_engine &eng, guilayout::world_layout &layout) {
				int next_x = x;
				int next_y = y;
				int step_x = (sigflags & SDF_STEP_RIGHT) ? +1 : -1;

				//first draw stick
				draw::sprite_ref stick_sprite_extras = 0;
				if ((stick_sprite & SID_typemask) == SID_signalpost) {
					if (gs->GetCurrentForwardRoute()) {
						stick_sprite_extras |= SID_reserved;
					}
				}
				layout.SetSprite(x, y, stick_sprite | stick_sprite_extras, obj, 0);


				//draw head(s)
				std::shared_ptr<guilayout::pos_sprite_desc_opts> options;
				bool draw_blank = false;
				if (gs->GetSignalFlags() & GSF::APPROACHLOCKINGMODE) {
					unsigned int timebin = layout.GetWorld().GetGameTime() / APPROACHLOCKING_FLASHINTERVAL;
					if (timebin & 1) {
						draw_blank = true;
					}
					if (!options) {
						options = std::make_shared<guilayout::pos_sprite_desc_opts>();
					}
					options->refresh_interval_ms = APPROACHLOCKING_FLASHINTERVAL;
				}

				auto draw_route_head = [&](draw::sprite_ref extras) {
					next_x += step_x;
					if (draw_blank) {
						extras = SID_signal_blank;
					}
					layout.SetSprite(next_x, next_y, SID_signalroute | extras, obj, 0, options);
				};

				auto draw_single_route_head = [&](draw::sprite_ref extras) {
					draw_route_head(extras);
					if (sigflags & SDF_CAN_DISPLAY_DY) {
						draw_route_head(SID_signal_blank);
					}
				};

				auto draw_shunt_head = [&](draw::sprite_ref extras) {
					next_x += step_x;

					// Don't set options for grey shunt
					std::shared_ptr<guilayout::pos_sprite_desc_opts> shunt_options;
					if (!(extras & SID_shunt_grey)) {
						shunt_options = options;
						if (draw_blank) {
							extras = SID_shunt_blank;
						}
					}

					layout.SetSprite(next_x, next_y, SID_signalshunt | layoutdir_to_spriteid(obj->GetLayoutDirection()) | extras, obj, 0, std::move(shunt_options));
				};

				if (sigflags & SDF_HAVE_SHUNT) {
					if (gs->GetAspect() > 0 && route_class::IsShunt(gs->GetAspectType())) {
						draw_shunt_head(SID_shunt_white);
					} else if (sigflags & SDF_HAVE_ROUTE) {
						draw_shunt_head(SID_shunt_grey);
					} else {
						draw_shunt_head(SID_shunt_red);
					}
				}

				if (sigflags & SDF_HAVE_ROUTE) {
					if (gs->GetAspect() == 0) {
						draw_single_route_head(SID_signal_red);
					} else if (!is_higher_aspect_in_mask(gs->GetRouteDefaults().aspect_mask, gs->GetAspect())) {
						draw_single_route_head(SID_signal_green);
					} else if (gs->GetAspect() == 1) {
						draw_single_route_head(SID_signal_yellow);
					} else if (gs->GetAspect() == 2) {
						draw_route_head(SID_signal_yellow);
						draw_route_head(SID_signal_yellow);
					} else {
						draw_single_route_head(SID_signal_unknown);
					}
				}
			};
		}

		const trackseg *ts = dynamic_cast<const trackseg *>(gt);
		if (ts) {
			struct tracksegpointinfo {
				int x;
				int y;
				LAYOUT_DIR direction;
				draw::sprite_ref base_sprite;
			};
			auto points = std::make_shared<std::vector<tracksegpointinfo> >();

			guilayout::LayoutOffsetDirection(x, y, obj->GetLayoutDirection(), obj->GetLength(), [&](int sx, int sy, LAYOUT_DIR sdir) {
				draw::sprite_ref base_sprite = SID_trackseg | layoutdir_to_spriteid(guilayout::FoldLayoutDirection(sdir));
				if (ts->GetTrackCircuit()) {
					base_sprite |= SID_has_tc;
				}
				points->push_back({sx, sy, sdir, base_sprite});
			});

			return [points, ts, obj](const draw_engine &eng, guilayout::world_layout &layout) {
				draw::sprite_ref extras = track_sprite_extras(ts);
				for (auto &it : *points) {
					layout.SetSprite(it.x, it.y, it.base_sprite | extras, obj, 0);
				}
			};
		}

		const genericpoints *gp = dynamic_cast<const genericpoints *>(gt);
		if (gp) {
			const doubleslip *ds = dynamic_cast<const doubleslip *>(gp);
			if (ds) {
				// TODO
			} else {
				// All types of genericpoints except doubleslip can be dealt with together

				using points_layout_info = guilayout::layouttrack_obj::points_layout_info;

				auto layout_info = obj->GetPointsLayoutInfo();

				if (!layout_info) {
					return get_draw_error_func(0x7FFF7F, 0);
				}

				points_sprites ps = MakePointsSprites(gp, layout_info);

				if (layout_info->flags & points_layout_info::PLI_FLAGS::SHOW_MERGED) {
					auto base_sprite_ooc = ps.base_sprite_ooc;
					return [x, y, base_sprite_ooc, gp, obj](const draw_engine &eng, guilayout::world_layout &layout) {
						draw::sprite_ref sprite = track_sprite_extras(gp) | base_sprite_ooc;
						layout.SetSprite(x, y, sprite, obj, 0);
					};
				}

				return [x, y, ps, gp, obj](const draw_engine &eng, guilayout::world_layout &layout) {
					draw::sprite_ref sprite = track_sprite_extras(gp);
					genericpoints::PTF flags = gp->GetPointsFlags(0);

					if (flags & genericpoints::PTF::OOC) {
						sprite |= ps.base_sprite_ooc;
						unsigned int timebin = layout.GetWorld().GetGameTime() / POINTS_OOC_FLASHINTERVAL;
						if (timebin & 1) {
							sprite = change_spriteid_layoutdir(LAYOUT_DIR::NULLDIR, sprite);
						}
						auto options = std::make_shared<guilayout::pos_sprite_desc_opts>();
						options->refresh_interval_ms = APPROACHLOCKING_FLASHINTERVAL;
						layout.SetSprite(x, y, sprite, obj, 0, options);
						return;
					}

					if (flags & genericpoints::PTF::REV) {
						sprite |= ps.base_sprite_reverse;
					} else {
						sprite |= ps.base_sprite_normal;
					}
					layout.SetSprite(x, y, sprite, obj, 0);
				};
			}
		}

		const springpoints *sp = dynamic_cast<const springpoints *>(gt);
		if (sp) {
			using points_layout_info = guilayout::layouttrack_obj::points_layout_info;

			auto layout_info = obj->GetPointsLayoutInfo();
			if (!layout_info) {
				return get_draw_error_func(0x7FFF7F, 0);
			}

			points_sprites ps = MakePointsSprites(sp, layout_info);
			draw::sprite_ref base_sprite;
			if (layout_info->flags & points_layout_info::PLI_FLAGS::SHOW_MERGED) {
				base_sprite = ps.base_sprite_ooc;
			} else if (sp->GetSendReverseFlag()) {
				base_sprite = ps.base_sprite_reverse;
			} else {
				base_sprite = ps.base_sprite_normal;
			}

			return [x, y, base_sprite, sp, obj](const draw_engine &eng, guilayout::world_layout &layout) {
				draw::sprite_ref sprite = track_sprite_extras(sp) | base_sprite;
				layout.SetSprite(x, y, sprite, obj, 0);
			};
		}

		//default:
		return get_draw_error_func(0xFFFF00, 0x0000FF);
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
			if (!btext.empty()) {
				std::unique_ptr<draw::drawtextchar> drawtext(new draw::drawtextchar);
				drawtext->text = btext;
				drawtext->foregroundcolour = 0xFFFFFF;
				drawtext->backgroundcolour = 0;
				layout.SetTextString(berthdataptr->x, berthdataptr->y, std::move(drawtext), obj, BERTHLEVEL, berthdataptr->length, berthdataptr->length);
			} else {
				for (int i = 0; i < berthdataptr->length; i++) {
					layout.ClearSpriteLevel(berthdataptr->x + i, berthdataptr->y, BERTHLEVEL);
				}
			}
		};
	}

	draw_func_type draw_module_ukgeneric::GetDrawObj(const std::shared_ptr<guilayout::layoutgui_obj> &obj, error_collection &ec) {
		return [](const draw_engine &eng, guilayout::world_layout &layout) {

		};
	}

	void draw_module_ukgeneric::BuildSprite(draw::sprite_ref sr, draw::sprite_obj &sp, const draw_options &dopt) {  // must be re-entrant
		unsigned int type = sr & SID_typemask;
		LAYOUT_DIR dir = spriteid_to_layoutdir(sr);

		auto dir_relative_fold = [&](LAYOUT_DIR dir) -> bool {
			return draw::dir_relative_fold(dir, sp, sr);
		};

		auto dir_relative_diag = [&](LAYOUT_DIR dir) -> bool {
			return draw::dir_relative_diag(dir, sp, sr);
		};

		if (sr & SID_raw_img) {
			switch (type) {
				case SID_trackseg:
					if (dir_relative_fold(dir)) {
						return;
					}
					if (dir == LAYOUT_DIR::U) {
						sp.LoadFromFileDataFallback("track_v.png", GetImageData_track_v());
						return;
					}
					if (dir == LAYOUT_DIR::UR) {
						sp.LoadFromFileDataFallback("track_d.png", GetImageData_track_d());
						return;
					}
					if (dir == LAYOUT_DIR::R) {
						sp.LoadFromFileDataFallback("track_h.png", GetImageData_track_h());
						return;
					}
					break;

				case SID_pointsooc:
					if (dir_relative_diag(dir)) {
						return;
					}
					if (dir == LAYOUT_DIR::UR) {
						sp.LoadFromFileDataFallback("track_dv.png", GetImageData_track_dv());
						return;
					}
					if (dir == LAYOUT_DIR::LD) {
						sp.LoadFromFileDataFallback("track_dh.png", GetImageData_track_dh());
						return;
					}
					if (dir == LAYOUT_DIR::NULLDIR) {
						sp.LoadFromFileDataFallback("blank.png", GetImageData_blank());
						return;
					}
					break;

				case SID_signalpost:
					if (dir_relative_fold(dir)) {
						return;
					}
					if (dir == LAYOUT_DIR::UR) {
						sp.LoadFromFileDataFallback("sigpost.png", GetImageData_sigpost());
						return;
					}
					break;

				case SID_signalpostbar:
					if (dir_relative_fold(dir)) {
						return;
					}
					if (dir == LAYOUT_DIR::UR) {
						sp.LoadFromFileDataFallback("sigpost_bar.png", GetImageData_sigpost_bar());
						return;
					}
					break;

				case SID_signalroute:
					sp.LoadFromFileDataFallback("circle.png", GetImageData_circle());
					return;

				case SID_signalshunt:
					if (dir_relative_fold(dir)) {
						return;
					}
					if (dir == LAYOUT_DIR::UR) {
						sp.LoadFromFileDataFallback("shunt.png", GetImageData_shunt());
						return;
					}
					return;

				default:
					break;
			}
		}
		else {
			switch (type) {
				case SID_trackseg:
				case SID_pointsooc:
					sp.LoadFromSprite((sr & (SID_typemask | SID_dirmask)) | SID_raw_img);
					if (sr & SID_has_tc) {
						sp.ReplaceColour(0x0000FF, 0xFF0000);
					} else {
						sp.ReplaceColour(0x0000FF, 0x000000);
					}
					if (!(sr & SID_tc_occ)) {
						sp.ReplaceColour(0xFF0000, 0xAAAAAA);
					}
					if ((sr & SID_reserved) && !(sr & SID_tc_occ)) {
						sp.ReplaceColour(0xAAAAAA, 0xFFFFFF);
					}
					return;

				case SID_signalpost:
				case SID_signalpostbar:
					sp.LoadFromSprite((sr & (SID_typemask | SID_dirmask)) | SID_raw_img);
					if (!(sr & SID_reserved)) {
						sp.ReplaceColour(0xFFFFFF, 0xAAAAAA);
					}
					return;

				case SID_signalroute:
					sp.LoadFromSprite(SID_signalroute | SID_raw_img);
					if (sr & SID_signal_red) {
						sp.ReplaceColour(0xFFFFFF, 0xFF0000);
					} else if (sr & SID_signal_yellow) {
						sp.ReplaceColour(0xFFFFFF, 0xFFFF00);
					} else if (sr & SID_signal_green) {
						sp.ReplaceColour(0xFFFFFF, 0x00FF00);
					} else if (sr & SID_signal_blank) {
						sp.ReplaceColour(0xFFFFFF, 0x000000);
					} else {
						sp.ReplaceColour(0xFFFFFF, 0x7F7FFF);
					}
					return;

				case SID_signalshunt:
					sp.LoadFromSprite((sr & (SID_typemask | SID_dirmask)) | SID_raw_img);
					if (sr & SID_shunt_red) {
						sp.ReplaceColour(0xFFFFFF, 0xFF0000);
					} else if (sr & SID_shunt_white) {
						// do nothing
					} else if (sr & SID_shunt_grey) {
						sp.ReplaceColour(0xFFFFFF, 0xAAAAAA);
					} else if (sr & SID_signal_blank) {
						sp.ReplaceColour(0xFFFFFF, 0x000000);
					} else {
						sp.ReplaceColour(0xFFFFFF, 0x7F7FFF);
					}
					return;

				default:
					break;
			}
		}

		sp.DrawTextChar("?", 0xFF00FF, 0x00FF00);
	}

};
