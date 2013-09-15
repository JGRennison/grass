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

class world_serialisation;

class layout_obj : public std::enable_shared_from_this<layout_obj> {
	std::pair<unsigned int, unsigned int> dimensions;
	std::pair<int, int> position;

	public:
	virtual ~layout_obj();
	std::pair<unsigned int, unsigned int> GetDimensions() const;
	std::pair<int, int> GetPosition() const;
};

class world_layout : public std::enable_shared_from_this<layout_obj> {
	std::deque<std::shared_ptr<layout_obj> > objs;

	public:
	void SetWorldSerialisationLayout(world_serialisation &ws);
	virtual ~world_layout();
};

#endif
