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

#ifndef INC_DRAWRES_ALREADY
#define INC_DRAWRES_ALREADY

#include <utility>

#define IMG_EXTERN(obj, name) \
	extern "C" unsigned char res_##obj##_png_start[] asm("_binary_src_draw_res_" #obj "_start"); \
	extern "C" unsigned char res_##obj##_png_end[] asm("_binary_src_draw_res_" #obj "_end"); \
	static constexpr std::pair<void *, size_t> GetImageData_##name() { \
		return std::make_pair<void *, size_t>(res_##obj##_png_start, res_##obj##_png_end-res_##obj##_png_start); \
	} \

#endif
