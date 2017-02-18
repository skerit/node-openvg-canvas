#pragma once

#include "nan.h"

NAN_METHOD(InitFreeType);
NAN_METHOD(DoneFreeType);
NAN_METHOD(NewMemoryFace);
NAN_METHOD(DoneFace);
NAN_METHOD(SetCharSize);
NAN_METHOD(GetCharIndex);
NAN_METHOD(LoadGlyph);