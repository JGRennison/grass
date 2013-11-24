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
#include <forward_list>
#include "core/edgetype.h"
#include "core/error.h"
#include "draw/drawtypes.h"

class world_serialisation;
class generictrack;
class trackberth;
class error_collection;
class deserialiser_input;
class world;

namespace guilayout {
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

	class layout_obj : public std::enable_shared_from_this<layout_obj> {
		protected:
		int x = 0;
		int y = 0;

		enum {
			LOSM_X      = 1<<0,
			LOSM_Y      = 1<<1,
		};
		unsigned int setmembers = 0;

		public:
		virtual ~layout_obj() { }
		std::pair<unsigned int, unsigned int> GetDimensions() const;
		inline std::pair<int, int> GetPosition() const { return std::make_pair(x, y); }
		virtual std::string GetFriendlyName() const = 0;
		virtual void Process(world_layout &wl, error_collection &ec) = 0;
		virtual void Deserialise(const deserialiser_input &di, error_collection &ec);
		draw::draw_func_type drawfunction;
	};

	class layouttrack_obj : public layout_obj {
		protected:
		const generictrack *gt;
		int length = 1;
		LAYOUT_DIR layoutdirection = LAYOUT_DIR::NULLDIR;
		std::string track_type;

		enum {
			LTOSM_LENGTH       = 1<<16,
			LTOSM_LAYOUTDIR    = 1<<17,
			LTOSM_TRACKTYPE    = 1<<18,
		};

		public:
		layouttrack_obj(const generictrack *gt_) : gt(gt_) { }
		inline int GetLength() const { return length; }
		inline const generictrack * GetTrack() const { return gt; }
		inline LAYOUT_DIR GetLayoutDirection() const { return layoutdirection; }
		virtual std::string GetFriendlyName() const override;
		virtual void Process(world_layout &wl, error_collection &ec) override;
		virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	};

	class layoutberth_obj : public layout_obj {
		protected:
		const generictrack *gt = 0;
		const trackberth *b = 0;
		int length = 4;

		enum {
			LBOSM_LENGTH      = 1<<16,
		};

		public:
		layoutberth_obj(const generictrack *gt_, const trackberth *b_) : gt(gt_), b(b_) { }
		inline int GetLength() const { return length; }
		inline const generictrack * GetTrack() const { return gt; }
		inline const trackberth * GetBerth() const { return b; }
		virtual std::string GetFriendlyName() const override;
		virtual void Process(world_layout &wl, error_collection &ec) override;
		virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	};

	class layoutgui_obj : public layout_obj {
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

	struct layoutoffsetdirectionresult {
		int endx;
		int endy;
		int nextx;
		int nexty;
		LAYOUT_DIR nextdir;
	};

	layoutoffsetdirectionresult LayoutOffsetDirection(int startx, int starty, LAYOUT_DIR ld, unsigned int length, std::function<void(int, int, LAYOUT_DIR)> stepfunc = std::function<void(int, int, LAYOUT_DIR)>());

	struct pos_sprite_desc {
		int level = std::numeric_limits<int>::lowest();
		draw::sprite_ref sprite = 0;
		std::weak_ptr<layout_obj> owner;
		std::unique_ptr<draw::drawtextchar> text;
	};

	class world_layout : public std::enable_shared_from_this<world_layout> {
		std::deque<std::shared_ptr<layout_obj> > objs;
		std::multimap<const generictrack *, std::shared_ptr<layouttrack_obj> > tracktolayoutmap;
		std::multimap<const generictrack *, std::shared_ptr<layoutberth_obj> > berthtolayoutmap;
		const world &w;
		std::shared_ptr<draw::draw_module> eng;

		std::map<std::pair<int, int>, std::forward_list<pos_sprite_desc> > location_map;
		std::set<std::pair<int, int>> redraw_map;

		//this is so that we can hold onto w if it may go out of scope
		std::shared_ptr<const world> w_ptr;

		std::set<layout_obj *> objs_updated;

		public:
		world_layout(const world &w_, std::shared_ptr<draw::draw_module> eng_ = std::shared_ptr<draw::draw_module>()) : w(w_), eng(std::move(eng_)) { }
		virtual ~world_layout() { }
		const world & GetWorld() const { return w; }
		void AddLayoutObj(const std::shared_ptr<layout_obj> &obj);
		void SetWorldSerialisationLayout(world_serialisation &ws);
		void ProcessLayoutObjSet(error_collection &ec);
		void GetTrackLayoutObjs(const layout_obj &src, const generictrack *targetgt, error_collection &ec, std::vector<std::shared_ptr<layouttrack_obj> > &output);
		void GetTrackBerthLayoutObjs(const layout_obj &src, const generictrack *targetgt, error_collection &ec, std::vector<std::shared_ptr<layoutberth_obj> > &output);
		inline std::shared_ptr<draw::draw_module> GetDrawModule() const { return eng; }
		inline void AddTrackLayoutObj(const generictrack *gt, std::shared_ptr<layouttrack_obj> &&obj) { tracktolayoutmap.emplace(gt, std::move(obj)); }
		void SetSprite(int x, int y, draw::sprite_ref sprite, const std::shared_ptr<layout_obj> &owner, int level = 0);
		void SetTextChar(int x, int y, std::unique_ptr<draw::drawtextchar> &&dt, const std::shared_ptr<layout_obj> &owner, int level = 0);
		int SetTextString(int startx, int y, std::unique_ptr<draw::drawtextchar> &&dt, const std::shared_ptr<layout_obj> &owner, int level = 0, int minlength = -1, int maxlength = -1);
		const pos_sprite_desc *GetSprite(int x, int y);
		pos_sprite_desc &GetLocationRef(int x, int y, int level);
		void ClearSpriteLevel(int x, int y, int level);

		//*1 are inclusive limits, *2 are exclusive limits
		void GetSpritesInRect(int x1, int x2, int y1, int y2, std::map<std::pair<int, int>, const pos_sprite_desc *> &sprites) const;
		void GetLayoutExtents(int &x1, int &x2, int &y1, int &y2, int margin = 0) const;

		//this is just for ref-counting purposes, not needed if world is static
		void SetWorldSharedPtr(std::shared_ptr<const world> wp) { w_ptr = std::move(wp); }
		std::shared_ptr<world_layout> GetSharedPtrThis() { return shared_from_this(); }
		std::shared_ptr<const world_layout> GetSharedPtrThis() const { return shared_from_this(); }

		void ClearUpdateSet() { objs_updated.clear(); }
		void IterateUpdateSet(std::function<void(layout_obj *)> func) {
			for(auto &it : objs_updated) { func(it); }
		}
		void IterateAllLayoutObjects(std::function<void(layout_obj *)> func) {
			for(auto &it : objs) { func(it.get()); }
		}
		void MarkUpdated(layout_obj *obj) {
			objs_updated.insert(obj);
		}
	};

};

#endif
