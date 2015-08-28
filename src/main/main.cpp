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

#include <cstdio>
#include "common.h"
#include "main/wx_common.h"
#include "main/main.h"
#include "main/wx_util.h"
#include "main/view.h"
#include "main/main_gui.h"
#include "core/world_serialisation.h"
#include "core/world.h"
#include "draw/wx/draw_engine_wx.h"
#include "draw/draw_module.h"
#include "draw/draw_options.h"
#include "layout/layout.h"
#include <wx/log.h>

#include "draw/mod/draw_mod_uk_generic.h"

IMPLEMENT_APP(grass_app)

bool grass_app::OnInit() {
	//wxApp::OnInit();	//don't call this, it just calls the default command line processor
	SetAppName(wxT("GRASS"));
	srand((unsigned int) time(0));
	return CmdLineProc(argv, argc);
}

int grass_app::OnExit() {
	return wxApp::OnExit();
}

bool grass_app::LoadGame(const wxString &base, const wxString &save) {
	int x, y;
	std::tie(x, y) = GetSpriteSizes();
	if (!eng || x != (int) eng->GetSpriteWidth() || y != (int) eng->GetSpriteHeight()) {
		eng = std::make_shared<draw::wx_draw_engine>(GetCurrentDrawModule(), x, y, GetDrawOptions());
	}

	error_collection ec;
	w = std::make_shared<world>();
	layout = std::make_shared<gui_layout::world_layout>(*w, GetCurrentDrawModule());
	layout->SetWorldSharedPtr(w);
	world_deserialisation ws(*w);
	layout->SetWorldSerialisationLayout(ws);
	ws.LoadGameFromFiles(stdstrwx(base), stdstrwx(save), ec);
	layout->ProcessLayoutObjSet(ec);
	if (ec.GetErrorCount()) {
		DisplayErrors(ec);
		return false;
	} else {
		InitialDrawAll();
		RunGameTimer();
		MakeNewViewWin();
		return true;
	}
}

std::pair<int, int> grass_app::GetSpriteSizes() const {
	//place-holder values
	return std::make_pair(8, 16);
}

std::shared_ptr<draw::draw_module> grass_app::GetCurrentDrawModule() {
	//TODO: make this user-controllable
	return std::shared_ptr<draw::draw_module>(new draw::draw_module_uk_generic);
}

std::shared_ptr<draw::draw_options> grass_app::GetDrawOptions() {
	if (!opts) {
		opts = std::make_shared<draw::draw_options>();
	}
	return opts;
}

void grass_app::DisplayErrors(error_collection &ec) {
	std::stringstream str;
	str << ec;
	wxLogError(wxT("One or more errors occurred, aborting:\n%s\n"), wxstrstd(str.str()).c_str());
}

grass_app::grass_app() {
	panelset.reset(new main_gui::gr_view_winlist);
}

grass_app::~grass_app() {

}

void grass_app::RunGameTimer() {
	if (!timer) {
		timer.reset(new game_timer(this));
	}
	if (ispaused || speedfactor == 0) {
		timer->Stop();
	} else {
		timer->Start(timestepms * 256 / speedfactor, wxTIMER_CONTINUOUS);
	}
}

void game_timer::Notify() {
	if (app->layout) {
		app->layout->ClearUpdateSet();
		app->layout->ClearRedrawMap();
	}
	world_time oldtime = app->w->GetGameTime();
	app->w->GameStep(app->timestepms);
	world_time newtime = app->w->GetGameTime();
	if (app->eng && app->layout) {
		app->layout->LayoutTimeStep(oldtime, newtime);
		app->layout->IterateUpdateSet([&](gui_layout::layout_obj *obj) {
			obj->draw_function(*(app->eng), *(app->layout));
		});
		app->layout->IterateRedrawMap([&](int x, int y) {
			for (auto &it : app->panelset->viewpanels) {
				it->RefreshSprites(x, y, 1, 1);
			}
		});
	}
}

void grass_app::MakeNewViewWin() {
	main_gui::gr_view_win *view = new main_gui::gr_view_win(layout, eng, panelset);
	view->Show(true);
}

void grass_app::InitialDrawAll() {
	if (eng && layout) {
		layout->IterateAllLayoutObjects([&](gui_layout::layout_obj *obj) {
			obj->draw_function(*eng, *layout);
		});
	}
}
