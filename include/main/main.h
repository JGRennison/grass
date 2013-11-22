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

#include <wx/app.h>
#include <memory>

#ifndef INC_MAIN_MAIN_ALREADY
#define INC_MAIN_MAIN_ALREADY

class error_collection;
class world;
namespace guilayout {
	class world_layout;
}
namespace draw {
	class wx_draw_engine;
	class draw_options;
	class draw_module;
}

class grassapp: public wxApp {
    virtual bool OnInit();
    virtual int OnExit();
	virtual ~grassapp();

	std::shared_ptr<world> w;
	std::shared_ptr<guilayout::world_layout> layout;
	std::shared_ptr<draw::wx_draw_engine> eng;
	std::shared_ptr<draw::draw_options> opts;

	public:
	bool cmdlineproc(wxChar ** argv, int argc);
	bool LoadGame(const wxString &base, const wxString &save);
	std::pair<int, int> GetSpriteSizes() const;
	std::shared_ptr<draw::draw_module> GetCurrentDrawModule();
	std::shared_ptr<draw::draw_options> GetDrawOptions();
	void DisplayErrors(error_collection &ec);
};

#endif
