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

#include "common.h"
#include "layout/layout.h"
#include "util/utf8.h"
#include "core/world_serialisation.h"
#include "core/signal.h"
#include "core/points.h"
#include "core/serialisable_impl.h"
#include "draw/draw_module.h"

namespace gui_layout {

	error_layout::error_layout(const layout_obj &obj, const std::string &errmsg) {
		msg << "GUI Layout error for: '" << obj.GetFriendlyName() << "'\n" << errmsg;
	}

	std::array<std::pair<gui_layout::LAYOUT_DIR, std::string>, 13> layout_direction_names { {
		{ gui_layout::LAYOUT_DIR::NULLDIR, "null" },
		{ gui_layout::LAYOUT_DIR::U, "u" },
		{ gui_layout::LAYOUT_DIR::UR, "ur" },
		{ gui_layout::LAYOUT_DIR::RU, "ru" },
		{ gui_layout::LAYOUT_DIR::R, "r" },
		{ gui_layout::LAYOUT_DIR::RD, "rd" },
		{ gui_layout::LAYOUT_DIR::DR, "dr" },
		{ gui_layout::LAYOUT_DIR::D, "d" },
		{ gui_layout::LAYOUT_DIR::DL, "dl" },
		{ gui_layout::LAYOUT_DIR::LD, "ld" },
		{ gui_layout::LAYOUT_DIR::L, "l" },
		{ gui_layout::LAYOUT_DIR::LU, "lu" },
		{ gui_layout::LAYOUT_DIR::UL, "ul" },
	} };

	static bool DeserialiseLayoutDirectionName(gui_layout::LAYOUT_DIR &dir, std::string str) {
		for (auto &it : layout_direction_names) {
			if (it.second == str) {
				dir = it.first;
				return true;
			}
		}
		return false;
	}

	std::array<layout_dir_mapping, 4> layout_table { {
		{ LAYOUT_DIR::UR, LAYOUT_DIR::RU, LAYOUT_DIR::U, LAYOUT_DIR::R },
		{ LAYOUT_DIR::UL, LAYOUT_DIR::LU, LAYOUT_DIR::U, LAYOUT_DIR::L },
		{ LAYOUT_DIR::DR, LAYOUT_DIR::RD, LAYOUT_DIR::D, LAYOUT_DIR::R },
		{ LAYOUT_DIR::DL, LAYOUT_DIR::LD, LAYOUT_DIR::D, LAYOUT_DIR::L },
	} };

	LAYOUT_DIR EdgesToDirection(LAYOUT_DIR in, LAYOUT_DIR out) {
		if (!IsEdgeLayoutDirection(in) || !IsEdgeLayoutDirection(out)) {
			return LAYOUT_DIR::NULLDIR;
		}

		LAYOUT_DIR in_dir = ReverseLayoutDirection(in);

		if (in_dir == out) return in_dir;

		for (auto &it : layout_table) {
			if (it.step1 == in_dir && it.step2 == out) {
				return it.dir;
			} else if (it.step2 == in_dir && it.step1 == out) {
				return it.altstep;
			}
		}

		return LAYOUT_DIR::NULLDIR;
	}
};

template<typename C>
struct flagtyperemover<C, typename std::enable_if<std::is_convertible<C, gui_layout::LAYOUT_DIR>::value>::type> {
	typedef C type;
};

template <> inline bool IsType<gui_layout::LAYOUT_DIR>(const rapidjson::Value& val) {
	if (val.IsString()) {
		gui_layout::LAYOUT_DIR dir = gui_layout::LAYOUT_DIR::NULLDIR;
		return gui_layout::DeserialiseLayoutDirectionName(dir, val.GetString());
	} else {
		return false;
	}
}
template <> inline gui_layout::LAYOUT_DIR GetType<gui_layout::LAYOUT_DIR>(const rapidjson::Value& val) {
	gui_layout::LAYOUT_DIR dir = gui_layout::LAYOUT_DIR::NULLDIR;
	gui_layout::DeserialiseLayoutDirectionName(dir, val.GetString());
	return dir;
}
template <> inline void SetType<gui_layout::LAYOUT_DIR>(Handler &out, gui_layout::LAYOUT_DIR val) {
	for (auto &it : gui_layout::layout_direction_names) {
		if (it.first == val) {
			out.String(it.second);
			return;
		}
	}
	out.String("");
}
template <> inline const char *GetTypeFriendlyName<gui_layout::LAYOUT_DIR>() { return "layout direction"; }

std::string gui_layout::layout_track_obj::GetFriendlyName() const {
	return "Track: " + gt->GetFriendlyName();
};

std::string gui_layout::layout_berth_obj::GetFriendlyName() const {
	return "Track Berth for: " + gt->GetFriendlyName();
};

std::string gui_layout::layout_gui_obj::GetFriendlyName() const {
	return "Layout GUI Object";
};

void gui_layout::layout_obj::Deserialise(const deserialiser_input &di, error_collection &ec) {
	if (CheckTransJsonValue(x, di, "x", ec)) {
		set_members |= LOSM_X;
	}
	if (CheckTransJsonValue(y, di, "y", ec)) {
		set_members |= LOSM_Y;
	}
}

void gui_layout::layout_track_obj::Deserialise(const deserialiser_input &di, error_collection &ec) {
	gui_layout::layout_obj::Deserialise(di, ec);
	if (CheckTransJsonValue(length, di, "length", ec)) {
		set_members |= LTOSM_LENGTH;
	}
	if (CheckTransJsonValue(layout_direction, di, "direction", ec)) {
		set_members |= LTOSM_LAYOUTDIR;
	}
	if (CheckTransJsonValue(track_type, di, "tracktype", ec)) {
		set_members |= LTOSM_TRACKTYPE;
	}

	deserialiser_input pointsdi(di.json["pointslayout"], "pointslayout", "pointslayout", di);
	if (pointsdi.json.IsObject()) {
		points_layout.reset(new points_layout_info());
		auto parse_dir = [&](LAYOUT_DIR &out, const char *name) {
			CheckTransJsonValue(out, pointsdi, name, ec);
			switch (out) {
				case LAYOUT_DIR::U:
				case LAYOUT_DIR::D:
				case LAYOUT_DIR::L:
				case LAYOUT_DIR::R:
					// OK
					break;

				default:
					ec.RegisterNewError<error_layout>(*this, string_format("points layout: direction for edge: '%s' is invalid", name));
					break;
			}
		};
		parse_dir(points_layout->facing, "facing");
		parse_dir(points_layout->normal, "normal");
		parse_dir(points_layout->reverse, "reverse");
		if (points_layout->facing == points_layout->normal ||
				points_layout->facing == points_layout->reverse ||
				points_layout->normal == points_layout->reverse) {
			ec.RegisterNewError<error_layout>(*this, "points layout: duplicate edges are not allowed");
		}
		CheckTransJsonValueFlag(points_layout->flags, points_layout_info::PLI_FLAGS::SHOW_MERGED, pointsdi, "show_merged", ec);
	}
}

void gui_layout::layout_berth_obj::Deserialise(const deserialiser_input &di, error_collection &ec) {
	gui_layout::layout_obj::Deserialise(di, ec);
	if (CheckTransJsonValue(length, di, "length", ec)) {
		set_members |= LBOSM_LENGTH;
	}
}

void gui_layout::layout_gui_obj::Deserialise(const deserialiser_input &di, error_collection &ec) {
	gui_layout::layout_obj::Deserialise(di, ec);
	if (CheckTransJsonValue(dx, di, "dx", ec)) {
		set_members |= LGOSM_DX;
	}
	if (CheckTransJsonValue(dy, di, "dy", ec)) {
		set_members |= LGOSM_DY;
	}
	if (CheckTransJsonValue(graphics, di, "graphics", ec)) {
		set_members |= LGOSM_GRAPHICS;
	}
}

static void RegisterUpdateHook(gui_layout::world_layout &wl, const generic_track *gt, gui_layout::layout_obj *obj) {
	std::weak_ptr<gui_layout::world_layout> wlptr = wl.GetSharedPtrThis();

	//NB: casting away constness to insert update hook!
	const_cast<generic_track *>(gt)->AddUpdateHook([wlptr, obj](updatable_obj*, world &) {
		std::shared_ptr<gui_layout::world_layout> wl = wlptr.lock();
		if (wl) {
			wl->MarkUpdated(obj);
		}
	});
}

void gui_layout::layout_track_obj::Process(world_layout &wl, error_collection &ec) {
	if (!(set_members & LOSM_X) || !(set_members & LOSM_Y)) {
		ec.RegisterNewError<error_layout>(*this, "x and/or y coordinate missing: layout track pieces must be positioned absolutely");
		return;
	}

	bool expecting_layout_direction = true;
	bool expecting_points_layout = false;

	const generic_points *gp = dynamic_cast<const generic_points *>(gt);
	if (gp) {
		expecting_layout_direction = false;
		const double_slip *ds = dynamic_cast<const double_slip *>(gp);
		if (ds) {
			// TODO
		} else {
			expecting_points_layout = true;
		}
	}

	const spring_points *sp = dynamic_cast<const spring_points *>(gt);
	if (sp) {
		expecting_layout_direction = false;
		expecting_points_layout = true;
	}

	if (expecting_layout_direction && layout_direction == LAYOUT_DIR::NULLDIR) {
		ec.RegisterNewError<error_layout>(*this, "No layout direction given.");
		return;
	}
	if (!expecting_layout_direction && (set_members & LTOSM_LAYOUTDIR)) {
		ec.RegisterNewError<error_layout>(*this, "Unexpected layout direction.");
		return;
	}
	if (expecting_points_layout && !points_layout) {
		ec.RegisterNewError<error_layout>(*this, "No points layout given.");
		return;
	}
	if (!expecting_points_layout && points_layout) {
		ec.RegisterNewError<error_layout>(*this, "Unexpected points layout.");
		return;
	}

	wl.AddTrackLayoutObj(gt, std::static_pointer_cast<layout_track_obj>(shared_from_this()));

	std::shared_ptr<draw::draw_module> dmod = wl.GetDrawModule();
	if (dmod) {
		draw_function = std::move(dmod->GetDrawTrack(std::static_pointer_cast<layout_track_obj>(shared_from_this()), ec));
		RegisterUpdateHook(wl, gt, this);
	}
}

void gui_layout::layout_berth_obj::Process(world_layout &wl, error_collection &ec) {
	if (!(set_members & LOSM_X) || !(set_members & LOSM_Y)) {
		ec.RegisterNewError<error_layout>(*this, "x and/or y coordinate missing: layout berths must be positioned absolutely");
		return;
	}

	std::shared_ptr<draw::draw_module> dmod = wl.GetDrawModule();
	if (dmod) {
		draw_function = std::move(dmod->GetDrawBerth(std::static_pointer_cast<layout_berth_obj>(shared_from_this()), ec));
		RegisterUpdateHook(wl, gt, this);
	}
}

void gui_layout::layout_gui_obj::Process(world_layout &wl, error_collection &ec) {
	if (!(set_members & LOSM_X) || !(set_members & LOSM_Y)) {
		ec.RegisterNewError<error_layout>(*this, "x and/or y coordinate missing: gui layout objects must be positioned absolutely");
		return;
	}
	std::shared_ptr<draw::draw_module> dmod = wl.GetDrawModule();
	if (dmod) {
		draw_function = std::move(dmod->GetDrawObj(std::static_pointer_cast<layout_gui_obj>(shared_from_this()), ec));
	}
}

gui_layout::layout_offset_direction_result gui_layout::LayoutOffsetDirection(int startx, int starty, LAYOUT_DIR ld, unsigned int length,
		std::function<void(int, int, LAYOUT_DIR)> stepfunc) {
	if (ld == LAYOUT_DIR::NULLDIR || !length) {
		return layout_offset_direction_result{ startx, starty, startx, starty, ld };
	}

	LAYOUT_DIR step1 = ld;
	LAYOUT_DIR step2 = ld;
	LAYOUT_DIR stepend1 = ld;
	LAYOUT_DIR stepend2 = ld;

	for (auto &it : layout_table) {
		if (it.dir == ld) {
			step1 = it.step2;
			step2 = it.step1;
			stepend1 = it.altstep;
			break;
		} else if (it.altstep == ld) {
			step1 = it.step1;
			step2 = it.step2;
			stepend1 = it.dir;
			break;
		}
	}

	int end_x = startx;
	int end_y = starty;
	int next_x = startx;
	int next_y = starty;
	LAYOUT_DIR outld = ld;

	auto stepcarddir = [&](LAYOUT_DIR ldcard) {
		switch (ldcard) {
			case LAYOUT_DIR::U:
				next_y--;
				break;

			case LAYOUT_DIR::D:
				next_y++;
				break;

			case LAYOUT_DIR::L:
				next_x--;
				break;

			case LAYOUT_DIR::R:
				next_x++;
				break;

			default:
				break;
		}

		if (length == 2) {
			end_x = next_x;
			end_y = next_y;
		}
	};

	while (true) {
		if (stepfunc) {
			stepfunc(next_x, next_y, outld);
		}
		stepcarddir(step1);
		outld = stepend1;
		length--;
		if (!length) break;

		if (stepfunc) {
			stepfunc(next_x, next_y, outld);
		}
		stepcarddir(step2);
		outld = stepend2;
		length--;
		if (!length) break;
	}

	return layout_offset_direction_result { end_x, end_y, next_x, next_y, outld };
}

void gui_layout::world_layout::SetWorldSerialisationLayout(world_deserialisation &ws) {
	std::weak_ptr<world_layout> wl_weak = shared_from_this();
	ws.gui_layout_generic_track = [wl_weak](const generic_track *gt, const deserialiser_input &di, error_collection &ec) {
		std::shared_ptr<world_layout> wl = wl_weak.lock();
		if (!wl) return;

		std::shared_ptr<layout_track_obj> obj = std::make_shared<layout_track_obj>(gt);
		obj->Deserialise(di, ec);

		wl->AddLayoutObj(std::static_pointer_cast<layout_obj>(obj));
	};
	ws.gui_layout_track_berth = [wl_weak](const track_berth *b, const generic_track *gt, const deserialiser_input &di, error_collection &ec) {
		std::shared_ptr<world_layout> wl = wl_weak.lock();
		if (!wl) return;

		std::shared_ptr<layout_berth_obj> obj = std::make_shared<layout_berth_obj>(gt, b);
		obj->Deserialise(di, ec);

		wl->AddLayoutObj(std::static_pointer_cast<layout_obj>(obj));
	};
	ws.gui_layout_guiobject = [wl_weak](const deserialiser_input &di, error_collection &ec) {
		std::shared_ptr<world_layout> wl = wl_weak.lock();
		if (!wl) return;

		std::shared_ptr<layout_gui_obj> obj = std::make_shared<layout_gui_obj>();
		obj->Deserialise(di, ec);

		wl->AddLayoutObj(std::static_pointer_cast<layout_obj>(obj));
	};
}

void gui_layout::world_layout::AddLayoutObj(const std::shared_ptr<layout_obj> &obj) {
	objs.push_back(obj);
}

void gui_layout::world_layout::ProcessLayoutObjSet(error_collection &ec) {
	for (auto &it : objs) {
		it->Process(*this, ec);
	}
}

void gui_layout::world_layout::GetTrackLayoutObjs(const layout_obj &src, const generic_track *targetgt, error_collection &ec,
		std::vector<std::shared_ptr<gui_layout::layout_track_obj> > &output) {
	decltype(track_to_layout_map)::iterator start, end;
	std::tie(start, end) = track_to_layout_map.equal_range(targetgt);
	for (auto it = start; it != end; ++it) {
		output.emplace_back(it->second);
	}
}

void gui_layout::world_layout::GetTrackBerthLayoutObjs(const layout_obj &src, const generic_track *targetgt, error_collection &ec,
		std::vector<std::shared_ptr<gui_layout::layout_berth_obj> > &output) {
	decltype(berth_to_layout_map)::iterator start, end;
	std::tie(start, end) = berth_to_layout_map.equal_range(targetgt);
	for (auto it = start; it != end; ++it) {
		output.emplace_back(it->second);
	}
}

gui_layout::pos_sprite_desc &gui_layout::world_layout::GetLocationRef(int x, int y, int level) {
	std::forward_list<pos_sprite_desc> &psd_list = location_map[std::make_pair(x, y)];

	auto prev = psd_list.before_begin();
	auto cur = std::next(prev);
	for (; cur != psd_list.end(); prev = cur, ++cur) {
		if (level == cur->level) {
			return *cur;
		}
		if (level > cur->level) {
			break;
		}
	}

	//couldn't find existing, make a new one
	auto newpsd = psd_list.emplace_after(prev);
	newpsd->level = level;
	return *newpsd;
}

void gui_layout::world_layout::SetSprite(int x, int y, draw::sprite_ref sprite, const std::shared_ptr<gui_layout::layout_obj> &owner,
		int level, std::shared_ptr<const pos_sprite_desc_opts> options) {
	pos_sprite_desc &psd = GetLocationRef(x, y, level);
	psd.sprite = sprite;
	psd.owner = owner;
	psd.text.reset();
	ChangeSpriteLevelOptions(psd, x, y, level, options);
	redraw_map.insert(std::make_pair(x, y));
}

void gui_layout::world_layout::SetTextChar(int x, int y, std::unique_ptr<draw::draw_text_char> &&dt, const std::shared_ptr<gui_layout::layout_obj> &owner,
		int level, std::shared_ptr<const pos_sprite_desc_opts> options) {
	pos_sprite_desc &psd = GetLocationRef(x, y, level);
	psd.sprite = 0;
	psd.owner = owner;
	psd.text = std::move(dt);
	ChangeSpriteLevelOptions(psd, x, y, level, options);
	redraw_map.insert(std::make_pair(x, y));
}

void gui_layout::world_layout::ClearSpriteLevel(int x, int y, int level) {
	auto it = location_map.find(std::make_pair(x, y));
	if (it == location_map.end()) {
		return;
	}

	std::forward_list<pos_sprite_desc> &psd_list = it->second;

	psd_list.remove_if([&](pos_sprite_desc &p) {
		if (p.level == level) {
			redraw_map.insert(std::make_pair(x, y));
			RemoveSpriteLevelOptions(p, x, y, level);
			return true;
		} else {
			return false;
		}
	});
	if (psd_list.empty()) {
		location_map.erase(it);
	}

}

int gui_layout::world_layout::SetTextString(int startx, int y, std::unique_ptr<draw::draw_text_char> &&dt, const std::shared_ptr<gui_layout::layout_obj> &owner,
		int level, int minlength, int maxlength, std::shared_ptr<const pos_sprite_desc_opts> options) {
	std::unique_ptr<draw::draw_text_char> tdt = std::move(dt);

	int len = strlen_utf8(tdt->text);
	if (len > maxlength) {
		len = maxlength;
	}

	auto puttext = [&](int x, std::string text) {
		std::unique_ptr<draw::draw_text_char> cdt(new draw::draw_text_char);
		cdt->text = std::move(text);
		cdt->foregroundcolour = dt->foregroundcolour;
		cdt->backgroundcolour = dt->backgroundcolour;
		SetTextChar(x, y, std::move(cdt), owner, level, options);
	};

	const char *str = tdt->text.c_str();
	for (int i = 0; i < len; i++) {
		int bytes = utf8firsttonumbytes(*str);
		puttext(startx + i, std::string(str, bytes));
		str += bytes;
	}
	if (minlength >= 0 && len < minlength) {
		for (int i = len; i < minlength; i++) {
			puttext(startx + i, " ");
		}
		len = minlength;
	}
	return len;
}

const gui_layout::pos_sprite_desc *gui_layout::world_layout::GetSprite(int x, int y) {
	const auto &it = location_map.find(std::make_pair(x, y));
	if (it == location_map.end()) {
		return nullptr;
	}

	std::forward_list<pos_sprite_desc> &psd_list = it->second;
	if (!psd_list.empty()) {
		return &(psd_list.front());
	} else {
		return nullptr;
	}
}

//*1 are inclusive limits, *2 are exclusive limits
void gui_layout::world_layout::GetSpritesInRect(int x1, int x2, int y1, int y2, std::map<std::pair<int, int>, const gui_layout::pos_sprite_desc *> &sprites) const {
	for (const auto &it : location_map) {
		int x, y;
		std::tie(x, y) = it.first;
		if (x >= x1 && x < x2 && y >= y1 && y < y2) {
			const std::forward_list<pos_sprite_desc> &psd_list = it.second;
			if (!psd_list.empty()) {
				sprites[it.first] = &(psd_list.front());
			}
		}
	}
}

void gui_layout::world_layout::GetLayoutExtents(int &x1, int &x2, int &y1, int &y2, int margin) const {
	x1 = y1 = std::numeric_limits<int>::max();
	x2 = y2 = std::numeric_limits<int>::lowest();

	for (const auto &it : location_map) {
		int x, y;
		std::tie(x, y) = it.first;
		if (x < x1) x1 = x;
		if (x > x2) x2 = x;
		if (y < y1) y1 = y;
		if (y > y2) y2 = y;
	}

	if (x1 > x2 || y1 > y2) {
		x1 = y1 = 0;
		x2 = y2 = -1;
	}
	x1 -= margin;
	x2 += 1 + margin;
	y1 -= margin;
	y2 += 1 + margin;
}

//This should be used for all set/remove operations involving the options field of pos_sprite_desc objects
//This handles (un)registering refresh intervals
void gui_layout::world_layout::ChangeSpriteLevelOptions(pos_sprite_desc &psd, int x, int y, int level, std::shared_ptr<const pos_sprite_desc_opts> options) {
	if (psd.options) {
		if (psd.options->refresh_interval_ms) {
			refresh_items[psd.options->refresh_interval_ms].remove_if([&](const refresh_item &r) {
				return r.x == x && r.y == y && r.level == level;
			});
		}
	}
	psd.options = options;
	if (options) {
		if (options->refresh_interval_ms) {
			refresh_items[options->refresh_interval_ms].push_front(refresh_item({x, y, level, psd.owner}));
		}
	}
}

//This adds layout objects which have refresh intervals to the updated list, as necessary
void gui_layout::world_layout::LayoutTimeStep(world_time oldtime, world_time newtime) {
	for (auto it : refresh_items) {
		unsigned int time_ms = it.first;
		unsigned int div = oldtime / time_ms;
		if (newtime >= time_ms * (div + 1)) {
			//need to update these objects
			auto rl = it.second;
			for (auto &jt : rl) {
				std::shared_ptr<layout_obj> obj = jt.owner.lock();
				if (obj) {
					MarkUpdated(obj.get());
				}
			}
		}
	}
}
