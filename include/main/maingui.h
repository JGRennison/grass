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
//  2014 - Jonathan Rennison <j.g.rennison@gmail.com>
//==========================================================================

#ifndef INC_MAIN_MAINGUI_ALREADY
#define INC_MAIN_MAINGUI_ALREADY

#include <vector>

namespace maingui {

	class grviewpanel;
	class grviewwin;
	class trainwin;

	struct grviewwinlist {
		std::vector<grviewpanel *> viewpanels;
		std::vector<grviewwin *> toplevelpanels;
		std::vector<trainwin *> trainwins;
	};

};

#endif
