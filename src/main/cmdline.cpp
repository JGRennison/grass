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

#include "common.h"
#include "main/wxcommon.h"
#include "main/SimpleOpt.h"
#include "main/main.h"
#include <wx/string.h>
#include <wx/log.h>

typedef CSimpleOptTempl<wxChar> CSO;

enum {
	OPT_NEWGAME,
	OPT_LOAD,
};

CSO::SOption g_rgOptions[] =
{
	{ OPT_NEWGAME,  wxT("-n"),           SO_REQ_SHRT  },
	{ OPT_NEWGAME,  wxT("--new-game"),   SO_REQ_SHRT  },
	{ OPT_LOAD,     wxT("-l"),           SO_REQ_SHRT  },
	{ OPT_LOAD,     wxT("--load-game"),  SO_REQ_SHRT  },

	SO_END_OF_OPTIONS
};

static const wxChar* cmdlineargerrorstr(ESOError err) {
	switch (err) {
	case SO_OPT_INVALID:
		return wxT("Unrecognized option");
	case SO_OPT_MULTIPLE:
		return wxT("Option matched multiple strings");
	case SO_ARG_INVALID:
		return wxT("Option does not accept argument");
	case SO_ARG_INVALID_TYPE:
		return wxT("Invalid argument format");
	case SO_ARG_MISSING:
		return wxT("Required argument is missing");
	case SO_ARG_INVALID_DATA:
		return wxT("Option argument appears to be another option");
	default:
		return wxT("Unknown Error");
	}
}

bool grassapp::cmdlineproc(wxChar ** argv, int argc) {
	CSO args(argc, argv, g_rgOptions, SO_O_CLUMP|SO_O_EXACT|SO_O_SHORTARG|SO_O_FILEARG|SO_O_CLUMP_ARGD|SO_O_NOSLASH);
	auto argerror = [&](const wxChar* err) {
		wxLogError(wxT("Command line processing error: %s, arg: %s"), err, args.OptionText());
	};

	std::function<bool()> game_action;

	auto set_game_action = [&](std::function<bool()> func) -> bool {
		if(game_action) {
			argerror(wxT("More than one new game/load game specified"));
			return false;
		}
		else {
			game_action = std::move(func);
			return true;
		}
	};

	while (args.Next()) {
		if (args.LastError() != SO_SUCCESS) {
			argerror(cmdlineargerrorstr(args.LastError()));
			return false;
		}
		switch(args.OptionId()) {
			case OPT_NEWGAME: {
				wxString str1 = args.OptionArg();
				if(!set_game_action([str1, this]() { return LoadGame(str1, wxT("")); })) return false;
				break;
			}
			case OPT_LOAD: {
				if(args.m_nNextOption + 1 > args.m_nLastArg) {
					argerror(cmdlineargerrorstr(SO_ARG_MISSING));
					return false;
				}
				wxString str1 = args.OptionArg();
				wxString str2 = args.m_argv[args.m_nNextOption++];
				if(!set_game_action([str1, str2, this]() { return LoadGame(str1, str2); })) return false;
				break;
			}
		}
	}
	if(game_action) return game_action();
	else return false;
}
