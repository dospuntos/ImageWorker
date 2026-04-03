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
static const uint32 kMsgNewFile = 'fnew';
static const uint32 kMsgOpenFile = 'fopn';
static const uint32 kMsgSaveFile = 'fsav';
static const uint32 kMsgFitToWindow = 'fitw';
static const uint32 kMsgActualSize = 'acts';
static const uint32 kMsgNextImage = 'next';
static const uint32 kMsgPrevImage = 'prev';
static const uint32 kMsgDeleteImage = 'delI';
static const uint32 kMsgRotate90CW = 'rocw';
static const uint32 kMsgRotate90CCW = 'rccw';

enum {
	// Text Transformations
	M_TRANSFORM_UPPERCASE              = 'upcs',
};


#endif // CONSTANTS_H
