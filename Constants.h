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
	M_CLEAR_IMAGE   = 'clri',

	M_SHOW_SETTINGS = 'stng',
	M_SHOW_HELP     = 'help',
	M_PASTE			= 'past',
	M_COPY			= 'copy',

};


#endif // CONSTANTS_H
