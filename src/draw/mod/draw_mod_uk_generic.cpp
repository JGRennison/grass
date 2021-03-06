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

#include "draw/mod/draw_mod_uk_generic.h"
#include "draw/draw_res.h"
#include "draw/draw_engine.h"
#include "draw/draw_options.h"
#include "layout/layout.h"
#include "core/track.h"
#include "core/signal.h"
#include "core/route_types.h"
#include "core/track_piece.h"
#include "core/track_circuit.h"
#include "core/track_reservation.h"
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
IMG_EXTERN(tc_gap_r_png, tc_gap_r)
IMG_EXTERN(tc_gap_d_png, tc_gap_d)
IMG_EXTERN(arrow_r_png, arrow_r)
IMG_EXTERN(arrow_u_png, arrow_u)

#define TEXTLEVEL 9
#define BERTHLEVEL 10
#define UNKNOWNLEVEL 20

#define APPROACHLOCKING_FLASHINTERVAL 500
#define POINTS_OOC_FLASHINTERVAL 500

using gui_layout::LAYOUT_DIR;

namespace {
	enum sprite_ids {
		SID_type_mask       = 0xFF,
		SID_track_seg       = 1,
		SID_points_ooc      = 2,
		SID_signal_route    = 8,
		SID_signal_shunt    = 9,
		SID_signal_post     = 16,
		SID_signal_post_bar = 17,
		SID_tc_gap_r        = 24,
		SID_tc_gap_d        = 25,
		SID_arrow           = 26,

		SID_dir_mask        = 0xF00,
		SID_dir_shift       = 8,

		SID_raw_img         = 0x1000,

		SID_has_tc          = 0x2000,
		SID_tc_occ          = 0x4000,
		SID_reserved        = 0x8000,
		SID_has_tc_gap_r    = 0x10000,
		SID_has_tc_gap_d    = 0x20000,

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
		return static_cast<LAYOUT_DIR>((sid & SID_dir_mask) >> SID_dir_shift);
	}
	draw::sprite_ref layoutdir_to_spriteid(LAYOUT_DIR dir) {
		return static_cast<std::underlying_type<LAYOUT_DIR>::type>(dir) << SID_dir_shift;
	}
}

namespace draw {
	draw::sprite_ref track_sprite_extras(const generic_track *gt) {
		draw::sprite_ref extras = 0;
		reservation_count_set rcs;
		gt->ReservationTypeCount(rcs);
		if (rcs.route_set > rcs.route_set_auto) {
			extras |= SID_reserved;
		}
		const track_circuit *tc = gt->GetTrackCircuit();
		if (tc && tc->Occupied()) {
			if ((rcs.route_set > 0) || !(tc->IsAnyPieceReserved())) {
				extras |= SID_tc_occ;
			}
		}
		return extras;
	}

	sprite_ref change_spriteid_layoutdir(LAYOUT_DIR newdir, sprite_ref sr) {
		return (sr & ~SID_dir_mask) | layoutdir_to_spriteid(newdir);
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

	// This converts non-diag direction into either U or R.
	// Returns true if handled
	bool dir_relative_non_diag(LAYOUT_DIR dir, draw::sprite_obj &sp, draw::sprite_ref sr) {
		switch (dir) {
			case LAYOUT_DIR::U:
				return false;

			case LAYOUT_DIR::D:
				sp.LoadFromSprite(change_spriteid_layoutdir(LAYOUT_DIR::U, sr));
				sp.Mirror(MIRROR::VERT);
				return true;

			case LAYOUT_DIR::R:
				return false;

			case LAYOUT_DIR::L:
				sp.LoadFromSprite(change_spriteid_layoutdir(LAYOUT_DIR::R, sr));
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
		draw::sprite_ref tc_gap_sprite;
	};

	draw::sprite_ref PointsLayoutTcGapSpriteRef(const generic_track *gt, const gui_layout::layout_track_obj::points_layout_info &info) {
		draw::sprite_ref sr = 0;

		auto exec_dir = [&](LAYOUT_DIR dir, EDGE edge) {
			if (dir == LAYOUT_DIR::R && gt->GetTrackCircuit() != gt->GetEdgeConnectingPiece(edge).track->GetTrackCircuit()) {
				sr |= SID_has_tc_gap_r;
			} else if (dir == LAYOUT_DIR::D && gt->GetTrackCircuit() != gt->GetEdgeConnectingPiece(edge).track->GetTrackCircuit()) {
				sr |= SID_has_tc_gap_d;
			}
		};
		exec_dir(info.facing, EDGE::PTS_FACE);
		exec_dir(info.normal, EDGE::PTS_NORMAL);
		exec_dir(info.reverse, EDGE::PTS_REVERSE);

		return sr;
	}

	points_sprites MakePointsSprites(const generic_track *gt, const gui_layout::layout_track_obj::points_layout_info *layout_info) {
		points_sprites out;

		draw::sprite_ref base = 0;
		if (gt->GetTrackCircuit()) {
			base |= SID_has_tc;
		}

		LAYOUT_DIR turn_dir = (layout_info->facing == ReverseLayoutDirection(layout_info->normal))
				? layout_info->reverse : layout_info->normal;
		out.base_sprite_ooc = base | SID_points_ooc |
				layoutdir_to_spriteid(gui_layout::EdgesToDirection(layout_info->facing, turn_dir));
		out.base_sprite_normal = base | SID_track_seg |
				layoutdir_to_spriteid(gui_layout::FoldLayoutDirection(gui_layout::EdgesToDirection(layout_info->facing, layout_info->normal)));
		out.base_sprite_reverse = base | SID_track_seg |
				layoutdir_to_spriteid(gui_layout::FoldLayoutDirection(gui_layout::EdgesToDirection(layout_info->facing, layout_info->reverse)));
		out.tc_gap_sprite = PointsLayoutTcGapSpriteRef(gt, *layout_info);
		return out;
	}

	draw::sprite_ref TcGapExtras(const draw_engine &eng, draw::sprite_ref tc_gap_sprite) {
		return eng.GetOptions()->tc_gaps ? tc_gap_sprite : 0;
	}

	draw_func_type draw_module_uk_generic::GetDrawTrack(const std::shared_ptr<gui_layout::layout_track_obj> &obj, error_collection &ec) {
		int x, y;
		std::tie(x, y) = obj->GetPosition();

		auto get_draw_error_func = [&](uint32_t foreground, uint32_t background) -> draw_func_type {
			return [x, y, obj, foreground, background](const draw_engine &eng, gui_layout::world_layout &layout) {
				std::unique_ptr<draw::draw_text_char> drawtext(new draw::draw_text_char);
				drawtext->foregroundcolour = foreground;
				drawtext->backgroundcolour = background;
				drawtext->text = "?";
				layout.SetTextChar(x, y, std::move(drawtext), obj, UNKNOWNLEVEL);
			};
		};

		const generic_track *gt = obj->GetTrack();
		const generic_signal *gs = FastSignalCast(gt);
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

			route_class::set route_types = gs->GetAvailableRouteTypes(EDGE::FRONT).start;
			if (route_types & route_class::AllRoutes()) {
				sigflags |= SDF_HAVE_ROUTE;
			}
			if (route_types & route_class::AllShunts()) {
				sigflags |= SDF_HAVE_SHUNT;
			}

			draw::sprite_ref stick_sprite = layoutdir_to_spriteid(gui_layout::FoldLayoutDirection(obj->GetLayoutDirection()));
			if (gs->GetSignalFlags() & GSF::AUTO_SIGNAL) {
				stick_sprite |= SID_signal_post_bar;
			} else {
				stick_sprite |= SID_signal_post;
			}

			if (is_higher_aspect_in_mask(gs->GetRouteDefaults().aspect_mask, 2)) {
				sigflags |= SDF_CAN_DISPLAY_DY;
			}

			return [x, y, gs, obj, stick_sprite, sigflags](const draw_engine &eng, gui_layout::world_layout &layout) {
				int next_x = x;
				int next_y = y;
				int step_x = (sigflags & SDF_STEP_RIGHT) ? +1 : -1;

				//first draw stick
				draw::sprite_ref stick_sprite_extras = 0;
				if ((stick_sprite & SID_type_mask) == SID_signal_post) {
					if (gs->GetCurrentForwardRoute()) {
						stick_sprite_extras |= SID_reserved;
					}
				}
				layout.SetSprite(x, y, stick_sprite | stick_sprite_extras, obj, 0);


				//draw head(s)
				std::shared_ptr<gui_layout::pos_sprite_desc_opts> options;
				bool draw_blank = false;
				if (gs->GetSignalFlags() & GSF::APPROACH_LOCKING_MODE) {
					unsigned int timebin = layout.GetWorld().GetGameTime() / APPROACHLOCKING_FLASHINTERVAL;
					if (timebin & 1) {
						draw_blank = true;
					}
					if (!options) {
						options = std::make_shared<gui_layout::pos_sprite_desc_opts>();
					}
					options->refresh_interval_ms = APPROACHLOCKING_FLASHINTERVAL;
				}

				auto draw_route_head = [&](draw::sprite_ref extras) {
					next_x += step_x;
					if (draw_blank) {
						extras = SID_signal_blank;
					}
					layout.SetSprite(next_x, next_y, SID_signal_route | extras, obj, 0, options);
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
					std::shared_ptr<gui_layout::pos_sprite_desc_opts> shunt_options;
					if (!(extras & SID_shunt_grey)) {
						shunt_options = options;
						if (draw_blank) {
							extras = SID_shunt_blank;
						}
					}

					layout.SetSprite(next_x, next_y, SID_signal_shunt | layoutdir_to_spriteid(obj->GetLayoutDirection()) | extras, obj, 0, std::move(shunt_options));
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

		const track_seg *ts = dynamic_cast<const track_seg *>(gt);
		if (ts) {
			struct track_segpointinfo {
				int x;
				int y;
				LAYOUT_DIR direction;
				draw::sprite_ref base_sprite;
				draw::sprite_ref tc_gap_extra;
			};
			auto points = std::make_shared<std::vector<track_segpointinfo> >();

			gui_layout::LayoutOffsetDirection(x, y, obj->GetLayoutDirection(), obj->GetLength(), [&](int sx, int sy, LAYOUT_DIR sdir) {
				draw::sprite_ref base_sprite = SID_track_seg | layoutdir_to_spriteid(gui_layout::FoldLayoutDirection(sdir));
				if (ts->GetTrackCircuit()) {
					base_sprite |= SID_has_tc;
				}
				points->push_back({sx, sy, sdir, base_sprite, 0});
			});

			if (!points->empty()) {
				auto check_tc_gap = [&](gui_layout::LAYOUT_DIR dir, EDGE out_edge) -> draw::sprite_ref {
					if (gt->GetEdgeConnectingPiece(out_edge).track->GetTrackCircuit() == gt->GetTrackCircuit()) return 0;

					gui_layout::LAYOUT_DIR out_edge_dir = DirectionToOutputEdge(dir);
					draw::sprite_ref out = 0;
					if (out_edge_dir == gui_layout::LAYOUT_DIR::R) {
						out |= SID_has_tc_gap_r;
					} else if (out_edge_dir == gui_layout::LAYOUT_DIR::R) {
						out |= SID_has_tc_gap_d;
					}
					return out;
				};
				points->front().tc_gap_extra |= check_tc_gap(ReverseLayoutDirection(points->front().direction), EDGE::FRONT);
				points->back().tc_gap_extra |= check_tc_gap(points->back().direction, EDGE::BACK);
			}

			return [points, ts, obj](const draw_engine &eng, gui_layout::world_layout &layout) {
				draw::sprite_ref extras = track_sprite_extras(ts);
				for (auto &it : *points) {
					layout.SetSprite(it.x, it.y, it.base_sprite | extras | TcGapExtras(eng, it.tc_gap_extra), obj, 0);
				}
			};
		}

		const generic_points *gp = dynamic_cast<const generic_points *>(gt);
		if (gp) {
			const double_slip *ds = dynamic_cast<const double_slip *>(gp);
			if (ds) {
				// TODO
			} else {
				// All types of generic_points except double_slip can be dealt with together

				using points_layout_info = gui_layout::layout_track_obj::points_layout_info;

				auto layout_info = obj->GetPointsLayoutInfo();

				if (!layout_info) {
					return get_draw_error_func(0x7FFF7F, 0);
				}

				points_sprites ps = MakePointsSprites(gp, layout_info);

				if (layout_info->flags & points_layout_info::PLI_FLAGS::SHOW_MERGED) {
					auto base_sprite_ooc = ps.base_sprite_ooc;
					auto tc_gap_sprite = ps.tc_gap_sprite;
					return [x, y, base_sprite_ooc, tc_gap_sprite, gp, obj](const draw_engine &eng, gui_layout::world_layout &layout) {
						draw::sprite_ref sprite = track_sprite_extras(gp) | TcGapExtras(eng, tc_gap_sprite) | base_sprite_ooc;
						layout.SetSprite(x, y, sprite, obj, 0);
					};
				}

				return [x, y, ps, gp, obj](const draw_engine &eng, gui_layout::world_layout &layout) {
					draw::sprite_ref sprite = track_sprite_extras(gp) | TcGapExtras(eng, ps.tc_gap_sprite);
					generic_points::PTF flags = gp->GetPointsFlags(0);

					if (flags & generic_points::PTF::OOC) {
						sprite |= ps.base_sprite_ooc;
						unsigned int timebin = layout.GetWorld().GetGameTime() / POINTS_OOC_FLASHINTERVAL;
						if (timebin & 1) {
							sprite = change_spriteid_layoutdir(LAYOUT_DIR::NULLDIR, sprite);
						}
						auto options = std::make_shared<gui_layout::pos_sprite_desc_opts>();
						options->refresh_interval_ms = POINTS_OOC_FLASHINTERVAL;
						layout.SetSprite(x, y, sprite, obj, 0, options);
						return;
					}

					if (flags & generic_points::PTF::REV) {
						sprite |= ps.base_sprite_reverse;
					} else {
						sprite |= ps.base_sprite_normal;
					}
					layout.SetSprite(x, y, sprite, obj, 0);
				};
			}
		}

		const spring_points *sp = dynamic_cast<const spring_points *>(gt);
		if (sp) {
			using points_layout_info = gui_layout::layout_track_obj::points_layout_info;

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
			auto tc_gap_sprite = ps.tc_gap_sprite;

			return [x, y, base_sprite, tc_gap_sprite, sp, obj](const draw_engine &eng, gui_layout::world_layout &layout) {
				draw::sprite_ref sprite = track_sprite_extras(sp) | TcGapExtras(eng, tc_gap_sprite) | base_sprite;
				layout.SetSprite(x, y, sprite, obj, 0);
			};
		}

		const start_of_line *sol = dynamic_cast<const start_of_line *>(gt);
		if (sol) {
			if (obj->GetLength() != 1) {
				return get_draw_error_func(0x7FFF7F, 0x800080);
			}
			bool reverse = dynamic_cast<const end_of_line *>(sol) != nullptr;
			LAYOUT_DIR dir = reverse ? obj->GetLayoutDirection() : ReverseLayoutDirection(obj->GetLayoutDirection());
			draw::sprite_ref sprite = change_spriteid_layoutdir(dir, SID_arrow | SID_shunt_grey);
			return [x, y, sprite, obj](const draw_engine &eng, gui_layout::world_layout &layout) {
				layout.SetSprite(x, y, sprite, obj, 0);
			};
		}

		//default:
		return get_draw_error_func(0xFFFF00, 0x0000FF);
	}

	draw_func_type draw_module_uk_generic::GetDrawBerth(const std::shared_ptr<gui_layout::layout_berth_obj> &obj, error_collection &ec) {
		struct draw_berth_data {
			int length;
			const track_berth *b;
			int x, y;
		};

		auto berth_data_ptr = std::make_shared<draw_berth_data>();
		berth_data_ptr->length = obj->GetLength();
		berth_data_ptr->b = obj->GetBerth();
		std::tie(berth_data_ptr->x, berth_data_ptr->y) = obj->GetPosition();

		return [berth_data_ptr, obj](const draw_engine &eng, gui_layout::world_layout &layout) {
			const std::string &btext = berth_data_ptr->b->contents;
			if (!btext.empty()) {
				draw::draw_text_char drawtext;
				drawtext.text = btext;
				drawtext.foregroundcolour = 0xFFFFFF;
				drawtext.backgroundcolour = 0;
				layout.SetTextString(berth_data_ptr->x, berth_data_ptr->y, drawtext, obj, BERTHLEVEL, berth_data_ptr->length, berth_data_ptr->length);
			} else {
				for (int i = 0; i < berth_data_ptr->length; i++) {
					layout.ClearSpriteLevel(berth_data_ptr->x + i, berth_data_ptr->y, BERTHLEVEL);
				}
			}
		};
	}

	draw_func_type draw_module_uk_generic::GetDrawObj(const std::shared_ptr<gui_layout::layout_gui_obj> &obj, error_collection &ec) {
		return [](const draw_engine &eng, gui_layout::world_layout &layout) {

		};
	}

	draw_func_type draw_module_uk_generic::GetDrawText(const std::shared_ptr<gui_layout::layout_text_obj> &obj, error_collection &ec) {
		return [obj](const draw_engine &eng, gui_layout::world_layout &layout) {
			int x, y;
			std::tie(x, y) = obj->GetPosition();
			layout.SetTextString(x, y, obj->GetDrawText(), obj, TEXTLEVEL);
		};
	}

	void draw_module_uk_generic::BuildSprite(draw::sprite_ref sr, draw::sprite_obj &sp, const draw_options &dopt) {  // must be re-entrant
		unsigned int type = sr & SID_type_mask;
		LAYOUT_DIR dir = spriteid_to_layoutdir(sr);

		auto dir_relative_fold = [&](LAYOUT_DIR dir) -> bool {
			return draw::dir_relative_fold(dir, sp, sr);
		};

		auto dir_relative_diag = [&](LAYOUT_DIR dir) -> bool {
			return draw::dir_relative_diag(dir, sp, sr);
		};

		auto dir_relative_non_diag = [&](LAYOUT_DIR dir) -> bool {
			return draw::dir_relative_non_diag(dir, sp, sr);
		};

		if (sr & SID_raw_img) {
			switch (type) {
				case SID_track_seg:
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

				case SID_points_ooc:
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

				case SID_signal_post:
					if (dir_relative_fold(dir)) {
						return;
					}
					if (dir == LAYOUT_DIR::UR) {
						sp.LoadFromFileDataFallback("sigpost.png", GetImageData_sigpost());
						return;
					}
					break;

				case SID_signal_post_bar:
					if (dir_relative_fold(dir)) {
						return;
					}
					if (dir == LAYOUT_DIR::UR) {
						sp.LoadFromFileDataFallback("sigpost_bar.png", GetImageData_sigpost_bar());
						return;
					}
					break;

				case SID_signal_route:
					sp.LoadFromFileDataFallback("circle.png", GetImageData_circle());
					return;

				case SID_signal_shunt:
					if (dir_relative_fold(dir)) {
						return;
					}
					if (dir == LAYOUT_DIR::UR) {
						sp.LoadFromFileDataFallback("shunt.png", GetImageData_shunt());
						return;
					}
					return;

				case SID_tc_gap_r:
					sp.LoadFromFileDataFallback("tc_gap_r.png", GetImageData_tc_gap_r());
					return;

				case SID_tc_gap_d:
					sp.LoadFromFileDataFallback("tc_gap_d.png", GetImageData_tc_gap_d());
					return;

				case SID_arrow:
					if (dir_relative_non_diag(dir)) {
						return;
					}
					if (dir == LAYOUT_DIR::U) {
						sp.LoadFromFileDataFallback("arrow_u.png", GetImageData_arrow_u());
						return;
					}
					if (dir == LAYOUT_DIR::R) {
						sp.LoadFromFileDataFallback("arrow_r.png", GetImageData_arrow_r());
						return;
					}
					return;

				default:
					break;
			}
		} else {
			switch (type) {
				case SID_track_seg:
				case SID_points_ooc:
					sp.LoadFromSprite((sr & (SID_type_mask | SID_dir_mask)) | SID_raw_img);
					if (sr & SID_has_tc_gap_r) {
						sp.OverlaySprite(SID_tc_gap_r | SID_raw_img);
					}
					if (sr & SID_has_tc_gap_d) {
						sp.OverlaySprite(SID_tc_gap_d | SID_raw_img);
					}
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

				case SID_signal_post:
				case SID_signal_post_bar:
					sp.LoadFromSprite((sr & (SID_type_mask | SID_dir_mask)) | SID_raw_img);
					if (!(sr & SID_reserved)) {
						sp.ReplaceColour(0xFFFFFF, 0xAAAAAA);
					}
					return;

				case SID_signal_route:
					sp.LoadFromSprite(SID_signal_route | SID_raw_img);
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

				case SID_signal_shunt:
				case SID_arrow:
					sp.LoadFromSprite((sr & (SID_type_mask | SID_dir_mask)) | SID_raw_img);
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
