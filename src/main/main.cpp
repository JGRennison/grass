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

#include <cstdio>
#include "common.h"
#include "main/wxcommon.h"
#include "main/main.h"
#include "main/wxutil.h"
#include "main/view.h"
#include "core/world_serialisation.h"
#include "core/world.h"
#include "draw/wx/drawengine_wx.h"
#include "draw/drawmodule.h"
#include "draw/drawoptions.h"
#include "layout/layout.h"
#include <wx/log.h>


IMPLEMENT_APP(grassapp)

bool grassapp::OnInit() {
	//wxApp::OnInit();	//don't call this, it just calls the default command line processor
	SetAppName(wxT("GRASS"));
	srand((unsigned int) time(0));
	return cmdlineproc(argv, argc);
}

int grassapp::OnExit() {
	return wxApp::OnExit();
}

bool grassapp::LoadGame(const wxString &base, const wxString &save) {
	int x, y;
	std::tie(x, y) = GetSpriteSizes();
	if(!eng || x != (int) eng->GetSpriteWidth() || y != (int) eng->GetSpriteHeight()) {
		eng = std::make_shared<draw::wx_draw_engine>(GetCurrentDrawModule(), x, y, GetDrawOptions());
	}

	error_collection ec;
	w = std::make_shared<world>();
	layout = std::make_shared<guilayout::world_layout>(*w, GetCurrentDrawModule());
	layout->SetWorldSharedPtr(w);
	world_serialisation ws(*w);
	layout->SetWorldSerialisationLayout(ws);
	ws.LoadGameFromFiles(stdstrwx(base), stdstrwx(save), ec);
	layout->ProcessLayoutObjSet(ec);
	if(ec.GetErrorCount()) {
		DisplayErrors(ec);
		return false;
	}
	else {
		InitialDrawAll();
		RunGameTimer();
		MakeNewViewWin();
		return true;
	}
}

std::pair<int, int> grassapp::GetSpriteSizes() const {
	//place-holder values
	return std::make_pair(10, 20);
}

std::shared_ptr<draw::draw_module> grassapp::GetCurrentDrawModule() {
	//TODO: add code here to do this
	return std::shared_ptr<draw::draw_module>();
}

std::shared_ptr<draw::draw_options> grassapp::GetDrawOptions() {
	if(!opts) opts = std::make_shared<draw::draw_options>();
	return opts;
}

void grassapp::DisplayErrors(error_collection &ec) {
	std::stringstream str;
	str << ec;
	wxLogError(wxT("One or more errors occurred, aborting:\n%s\n"), wxstrstd(str.str()).c_str());
}

grassapp::grassapp() {
	panelset.reset(new maingui::grviewwinlist);
}

grassapp::~grassapp() {

}

void grassapp::RunGameTimer() {
	if(!timer) timer.reset(new gametimer(this));
	if(ispaused || speedfactor == 0) {
		timer->Stop();
	}
	else {
		timer->Start(timestepms * 256 / speedfactor, wxTIMER_CONTINUOUS);
	}
}

void gametimer::Notify() {
	if(app->layout) {
		app->layout->ClearUpdateSet();
		app->layout->ClearRedrawMap();
	}
	app->w->GameStep(app->timestepms);
	if(app->eng && app->layout) {
		app->layout->IterateUpdateSet([&](guilayout::layout_obj *obj) {
			obj->drawfunction(*(app->eng), *(app->layout));
		});
		app->layout->IterateRedrawMap([&](int x, int y) {
			for(auto &it : app->panelset->viewpanels) {
				it->RefreshSprites(x, y, 1, 1);
			}
		});
	}
}

void grassapp::MakeNewViewWin() {
	maingui::grviewwin *view = new maingui::grviewwin(layout, eng, panelset);
	view->Show(true);
}

void grassapp::InitialDrawAll() {
	if(eng && layout) {
		layout->IterateAllLayoutObjects([&](guilayout::layout_obj *obj) {
			obj->drawfunction(*eng, *layout);
		});
	}
}
