//  grass - Generic Rail And Signalling Simulator
//
//  WEBSITE: http://bitbucket.org/JGRennison/grass
//  WEBSITE: http://grss.sourceforge.net
//
//  NOTE: This software is licensed under the GPL. See: COPYING-GPL.txt
//
//  This program  is distributed in the  hope that it will  be useful, but
//  WITHOUT   ANY  WARRANTY;   without  even   the  implied   warranty  of
//  MERCHANTABILITY  or FITNESS  FOR A  PARTICULAR PURPOSE.   See  the GNU
//  General Public License for more details.
//
//  Jonathan Rennison (or anybody else) is in no way responsible, or liable
//  for this program or its use in relation to users, 3rd parties or to any
//  persons in any way whatsoever.
//
//  You  should have  received a  copy of  the GNU  General Public
//  License along  with this program; if  not, write to  the Free Software
//  Foundation, Inc.,  59 Temple Place,  Suite 330, Boston,  MA 02111-1307
//  USA
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
#include "core/edgetype.h"
#include "core/error.h"

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
		int rx = 0;
		int ry = 0;

		enum {
			LOSM_X      = 1<<0,
			LOSM_Y      = 1<<1,
			LOSM_RX     = 1<<2,
			LOSM_RY     = 1<<3,
		};
		unsigned int setmembers = 0;

		public:
		virtual ~layout_obj() { }
		std::pair<unsigned int, unsigned int> GetDimensions() const;
		std::pair<int, int> GetPosition() const;
		virtual std::string GetFriendlyName() const = 0;
		virtual void Process(world_layout &wl, error_collection &ec) = 0;
		virtual void Deserialise(const deserialiser_input &di, error_collection &ec);
	};

	class layouttrack_obj : public layout_obj {
		protected:
		const generictrack *gt;
		int length = 0;
		bool leftside = false;
		bool rightside = false;
		LAYOUT_DIR layoutdirection = LAYOUT_DIR::NULLDIR;
		std::string src_branch = "default";
		std::string dest_branch = "default";
		std::string prev;
		EDGETYPE prev_edge = EDGETYPE::EDGE_NULL;

		enum {
			LTOSM_LENGTH       = 1<<16,
			LTOSM_LEFTSIDE     = 1<<17,
			LTOSM_RIGHTSIDE    = 1<<18,
			LTOSM_LAYOUTDIR    = 1<<19,
			LTOSM_PREV         = 1<<20,
			LTOSM_PREVEDGE     = 1<<21,
		};

		struct edge_def {
			EDGETYPE edge;
			int x;
			int y;
		};
		std::vector<edge_def> edges;

		std::vector<std::function<void(layouttrack_obj &obj, error_collection &ec)> > fixups;

		public:
		layouttrack_obj(const generictrack *gt_) : gt(gt_) { }
		virtual std::string GetFriendlyName() const override;
		virtual void Process(world_layout &wl, error_collection &ec) override;
		virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
		void RelativeFixup(const layout_obj &src, std::function<void(layouttrack_obj &obj, error_collection &ec)> f, error_collection &ec);
	};

	class layoutberth_obj : public layout_obj {
		protected:
		const generictrack *gt = 0;
		const trackberth *b = 0;
		int length = 0;

		enum {
			LBOSM_LENGTH      = 1<<16,
		};

		public:
		layoutberth_obj(const generictrack *gt_, const trackberth *b_) : gt(gt_), b(b_) { }
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

	layoutoffsetdirectionresult LayoutOffsetDirection(int startx, int starty, LAYOUT_DIR ld, unsigned int length);

	class world_layout : public std::enable_shared_from_this<world_layout> {
		std::deque<std::shared_ptr<layout_obj> > objs;
		std::map<const generictrack *, std::shared_ptr<layouttrack_obj> > tracktolayoutmap;
		std::map<std::string, std::shared_ptr<layouttrack_obj> > layout_branches;
		const world &w;

		public:
		world_layout(const world &w_) : w(w_) { }
		virtual ~world_layout() { }
		const world & GetWorld() const { return w; }
		std::shared_ptr<layouttrack_obj> & GetLayoutBranchRef(std::string name) { return layout_branches[name]; }
		void AddLayoutObj(const std::shared_ptr<layout_obj> &obj);
		void SetWorldSerialisationLayout(world_serialisation &ws);
		void ProcessLayoutObjSet(error_collection &ec);
		std::shared_ptr<layouttrack_obj> GetTrackLayoutObj(const layout_obj &src, const generictrack *targetgt, error_collection &ec);
		void LayoutTrackRelativeFixup(const layout_obj &src, const generictrack *targetgt, std::function<void(layouttrack_obj &obj, error_collection &ec)> f, error_collection &ec);
	};

};

#endif
