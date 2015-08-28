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

#ifndef INC_LAYOUT_ALREADY
#define INC_LAYOUT_ALREADY

#include <memory>
#include <utility>
#include <deque>
#include <string>
#include <map>
#include <tuple>
#include <vector>
#include <functional>
#include <limits>
#include <set>
#include <array>
#include <forward_list>
#include "common.h"
#include "util/error.h"
#include "util/flags.h"
#include "core/edge_type.h"
#include "draw/draw_types.h"

class world_deserialisation;
class generic_track;
class track_berth;
class error_collection;
class deserialiser_input;
class world;

namespace gui_layout {
	struct layout_obj;
	class world_layout;

	class error_layout : public error_obj {
		public:
		error_layout(const layout_obj &obj, const std::string &msg);
	};

	enum class LAYOUT_DIR {
		NULLDIR = 0,
		U,
		UR,
		RU,
		R,
		RD,
		DR,
		D,
		DL,
		LD,
		L,
		LU,
		UL,
	};

	inline bool IsEdgeLayoutDirection(LAYOUT_DIR dir) {
		switch (dir) {
			case LAYOUT_DIR::U:       return true;
			case LAYOUT_DIR::R:       return true;
			case LAYOUT_DIR::D:       return true;
			case LAYOUT_DIR::L:       return true;
			default:                  return false;
		}
	}

	inline LAYOUT_DIR ReverseLayoutDirection(LAYOUT_DIR dir) {
		switch (dir) {
			case LAYOUT_DIR::NULLDIR: return LAYOUT_DIR::NULLDIR;
			case LAYOUT_DIR::U:       return LAYOUT_DIR::D;
			case LAYOUT_DIR::UR:      return LAYOUT_DIR::LD;
			case LAYOUT_DIR::RU:      return LAYOUT_DIR::DL;
			case LAYOUT_DIR::R:       return LAYOUT_DIR::L;
			case LAYOUT_DIR::RD:      return LAYOUT_DIR::UL;
			case LAYOUT_DIR::DR:      return LAYOUT_DIR::LU;
			case LAYOUT_DIR::D:       return LAYOUT_DIR::U;
			case LAYOUT_DIR::DL:      return LAYOUT_DIR::RU;
			case LAYOUT_DIR::LD:      return LAYOUT_DIR::UR;
			case LAYOUT_DIR::L:       return LAYOUT_DIR::R;
			case LAYOUT_DIR::LU:      return LAYOUT_DIR::DR;
			case LAYOUT_DIR::UL:      return LAYOUT_DIR::RD;
		}
		return LAYOUT_DIR::NULLDIR;
	}

	struct layout_dir_mapping {
		LAYOUT_DIR dir;
		LAYOUT_DIR altstep;
		LAYOUT_DIR step1;
		LAYOUT_DIR step2;
	};
	extern std::array<layout_dir_mapping, 4> layout_table;

	//This reduces the input set by half
	//In that if two directions are reverse, they will fold to the same value
	inline LAYOUT_DIR FoldLayoutDirection(LAYOUT_DIR dir) {
		if (static_cast<std::underlying_type<gui_layout::LAYOUT_DIR>::type>(dir) >=
				static_cast<std::underlying_type<gui_layout::LAYOUT_DIR>::type>(LAYOUT_DIR::D)) {
			return ReverseLayoutDirection(dir);
		} else {
			return dir;
		}
	}

	LAYOUT_DIR EdgesToDirection(LAYOUT_DIR in, LAYOUT_DIR out);

	class layout_obj : public std::enable_shared_from_this<layout_obj> {
		protected:
		int x = 0;
		int y = 0;

		enum {
			LOSM_X      = 1<<0,
			LOSM_Y      = 1<<1,
		};
		unsigned int set_members = 0;

		public:
		virtual ~layout_obj() { }
		std::pair<unsigned int, unsigned int> GetDimensions() const;

		inline std::pair<int, int> GetPosition() const {
			return std::make_pair(x, y);
		}

		virtual std::string GetFriendlyName() const = 0;
		virtual void Process(world_layout &wl, error_collection &ec) = 0;
		virtual void Deserialise(const deserialiser_input &di, error_collection &ec);
		draw::draw_func_type draw_function;
	};

	class layout_track_obj : public layout_obj {
		protected:
		const generic_track *gt;
		int length = 1;
		LAYOUT_DIR layout_direction = LAYOUT_DIR::NULLDIR;
		std::string track_type;

		enum {
			LTOSM_LENGTH       = 1<<16,
			LTOSM_LAYOUTDIR    = 1<<17,
			LTOSM_TRACKTYPE    = 1<<18,
		};

		public:
		struct points_layout_info {
			LAYOUT_DIR facing = LAYOUT_DIR::NULLDIR;
			LAYOUT_DIR normal = LAYOUT_DIR::NULLDIR;
			LAYOUT_DIR reverse = LAYOUT_DIR::NULLDIR;
			enum class PLI_FLAGS {
				SHOW_MERGED        = 1<<0,
			};
			PLI_FLAGS flags;
		};

		protected:
		std::unique_ptr<points_layout_info> points_layout;

		public:
		layout_track_obj(const generic_track *gt_) : gt(gt_) { }
		inline int GetLength() const { return length; }
		inline const generic_track * GetTrack() const { return gt; }
		inline LAYOUT_DIR GetLayoutDirection() const { return layout_direction; }
		inline const points_layout_info* GetPointsLayoutInfo() const { return points_layout.get(); }
		virtual std::string GetFriendlyName() const override;
		virtual void Process(world_layout &wl, error_collection &ec) override;
		virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	};

	class layout_berth_obj : public layout_obj {
		protected:
		const generic_track *gt = nullptr;
		const track_berth *b = nullptr;
		int length = 4;

		enum {
			LBOSM_LENGTH      = 1<<16,
		};

		public:
		layout_berth_obj(const generic_track *gt_, const track_berth *b_) : gt(gt_), b(b_) { }
		inline int GetLength() const { return length; }
		inline const generic_track * GetTrack() const { return gt; }
		inline const track_berth * GetBerth() const { return b; }
		virtual std::string GetFriendlyName() const override;
		virtual void Process(world_layout &wl, error_collection &ec) override;
		virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	};

	class layout_gui_obj : public layout_obj {
		protected:
		int dx = 0;
		int dy = 0;
		std::string graphics;

		enum {
			LGOSM_DX          = 1<<16,
			LGOSM_DY          = 1<<17,
			LGOSM_GRAPHICS    = 1<<18,
		};

		public:
		virtual std::string GetFriendlyName() const override;
		virtual void Process(world_layout &wl, error_collection &ec) override;
		virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	};

	struct layout_offset_direction_result {
		int endx;
		int endy;
		int nextx;
		int nexty;
		LAYOUT_DIR next_dir;
	};

	layout_offset_direction_result LayoutOffsetDirection(int startx, int starty, LAYOUT_DIR ld, unsigned int length,
			std::function<void(int, int, LAYOUT_DIR)> stepfunc = std::function<void(int, int, LAYOUT_DIR)>());

	struct pos_sprite_desc_opts {
		unsigned int refresh_interval_ms = 0;
	};

	struct pos_sprite_desc {
		int level = std::numeric_limits<int>::lowest();
		draw::sprite_ref sprite = 0;
		std::weak_ptr<layout_obj> owner;
		std::unique_ptr<draw::draw_text_char> text;
		std::shared_ptr<const pos_sprite_desc_opts> options;
	};

	class world_layout : public std::enable_shared_from_this<world_layout> {
		std::deque<std::shared_ptr<layout_obj> > objs;
		std::multimap<const generic_track *, std::shared_ptr<layout_track_obj> > track_to_layout_map;
		std::multimap<const generic_track *, std::shared_ptr<layout_berth_obj> > berth_to_layout_map;
		const world &w;
		std::shared_ptr<draw::draw_module> eng;

		std::map<std::pair<int, int>, std::forward_list<pos_sprite_desc> > location_map;
		std::set<std::pair<int, int>> redraw_map;

		struct refresh_item {
			int x;
			int y;
			int level;
			std::weak_ptr<layout_obj> owner;
		};
		std::map<unsigned int, std::forward_list<refresh_item> > refresh_items;

		//this is so that we can hold onto w if it may go out of scope
		std::shared_ptr<const world> w_ptr;

		std::set<layout_obj *> objs_updated;

		public:
		world_layout(const world &w_, std::shared_ptr<draw::draw_module> eng_ = std::shared_ptr<draw::draw_module>())
				: w(w_), eng(std::move(eng_)) { }

		virtual ~world_layout() { }
		const world & GetWorld() const { return w; }
		void AddLayoutObj(const std::shared_ptr<layout_obj> &obj);
		void SetWorldSerialisationLayout(world_deserialisation &ws);
		void ProcessLayoutObjSet(error_collection &ec);
		void GetTrackLayoutObjs(const layout_obj &src, const generic_track *targetgt, error_collection &ec,
				std::vector<std::shared_ptr<layout_track_obj> > &output);
		void GetTrackBerthLayoutObjs(const layout_obj &src, const generic_track *targetgt, error_collection &ec,
				std::vector<std::shared_ptr<layout_berth_obj> > &output);
		inline std::shared_ptr<draw::draw_module> GetDrawModule() const { return eng; }

		inline void AddTrackLayoutObj(const generic_track *gt, std::shared_ptr<layout_track_obj> &&obj) {
			track_to_layout_map.emplace(gt, std::move(obj));
		}

		void SetSprite(int x, int y, draw::sprite_ref sprite, const std::shared_ptr<layout_obj> &owner,
				int level = 0, std::shared_ptr<const pos_sprite_desc_opts> options = std::shared_ptr<const pos_sprite_desc_opts>());
		void SetTextChar(int x, int y, std::unique_ptr<draw::draw_text_char> &&dt, const std::shared_ptr<layout_obj> &owner,
				int level = 0, std::shared_ptr<const pos_sprite_desc_opts> options = std::shared_ptr<const pos_sprite_desc_opts>());
		int SetTextString(int startx, int y, std::unique_ptr<draw::draw_text_char> &&dt, const std::shared_ptr<layout_obj> &owner,
				int level = 0, int minlength = -1, int maxlength = -1,
				std::shared_ptr<const pos_sprite_desc_opts> options = std::shared_ptr<const pos_sprite_desc_opts>());
		const pos_sprite_desc *GetSprite(int x, int y);
		pos_sprite_desc &GetLocationRef(int x, int y, int level);
		void ClearSpriteLevel(int x, int y, int level);
		void ChangeSpriteLevelOptions(pos_sprite_desc &psd, int x, int y, int level, std::shared_ptr<const pos_sprite_desc_opts> options);
		void RemoveSpriteLevelOptions(pos_sprite_desc &psd, int x, int y, int level) {
			ChangeSpriteLevelOptions(psd, x, y, level, std::shared_ptr<const pos_sprite_desc_opts>());
		}

		//*1 are inclusive limits, *2 are exclusive limits
		void GetSpritesInRect(int x1, int x2, int y1, int y2, std::map<std::pair<int, int>, const pos_sprite_desc *> &sprites) const;
		void GetLayoutExtents(int &x1, int &x2, int &y1, int &y2, int margin = 0) const;

		//this is just for ref-counting purposes, not needed if world is static
		void SetWorldSharedPtr(std::shared_ptr<const world> wp) { w_ptr = std::move(wp); }
		std::shared_ptr<world_layout> GetSharedPtrThis() { return shared_from_this(); }
		std::shared_ptr<const world_layout> GetSharedPtrThis() const { return shared_from_this(); }

		void ClearUpdateSet() {
			objs_updated.clear();
		}

		void IterateUpdateSet(std::function<void(layout_obj *)> func) {
			for (auto &it : objs_updated) {
				func(it);
			}
		}

		void IterateAllLayoutObjects(std::function<void(layout_obj *)> func) {
			for (auto &it : objs) {
				func(it.get());
			}
		}

		void MarkUpdated(layout_obj *obj) {
			objs_updated.insert(obj);
		}

		void ClearRedrawMap() {
			redraw_map.clear();
		}

		void IterateRedrawMap(std::function<void(int x, int y)> func) {
			for (auto &it : redraw_map) {
				func(it.first, it.second);
			}
		}

		void LayoutTimeStep(world_time oldtime, world_time newtime);
	};
};
template<> struct enum_traits< gui_layout::layout_track_obj::points_layout_info::PLI_FLAGS > { static constexpr bool flags = true; };

#endif
