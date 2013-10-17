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
#include "main/cmdline.h"

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
