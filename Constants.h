/*
 * Copyright 2026, Johan Wagenheim <johan@dospuntos.no>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <SupportDefs.h>

extern const char* kApplicationSignature;
extern const char* kApplicationName;
extern const char* kSettingsFile;

enum {
    M_NEW_FILE      = 'fnew',
    M_OPEN_FILE     = 'fopn',
    M_SAVE_FILE     = 'fsav',
    M_FIT_TO_WINDOW = 'fitw',
    M_ACTUAL_SIZE   = 'acts',
    M_NEXT_IMAGE    = 'next',
    M_PREV_IMAGE    = 'prev',
    M_DELETE_IMAGE  = 'delI',
    M_ROTATE_90_CW  = 'rocw',
    M_ROTATE_90_CCW = 'rccw',
	M_FLIP_VERTICAL = 'flpv',
	M_FLIP_HORIZONTAL = 'flph',
	M_CONVERT_TO_GRAYSCALE = 'gray',
	M_SWAP_COLOR_RBG = 'srbg',
	M_SWAP_COLOR_GRB = 'sgrb',
	M_SWAP_COLOR_GBR = 'sgbr',
	M_SWAP_COLOR_BRG = 'sbrg',
	M_SWAP_COLOR_BGR = 'sbgr',
	M_CLEAR_IMAGE   = 'clri',
	M_SHOW_INFO		= 'info',

	M_SHOW_SETTINGS = 'stng',
	M_SHOW_HELP     = 'help',
	M_PASTE			= 'past',
	M_COPY			= 'copy',

	M_APPLY_SETTINGS = 'aply',
	M_CLOSE_SETTINGS = 'clst',

	M_MINIMIZE_WINDOW = 'mnmz',
	M_ALWAYS_ON_TOP = 'atop'

};


#endif // CONSTANTS_H
