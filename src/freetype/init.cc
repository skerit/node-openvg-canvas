#include "nan.h"
#include "freetype.h"

NAN_MODULE_INIT(Init) {
  Nan::SetMethod(target, "initFreeType" , InitFreeType);
  Nan::SetMethod(target, "doneFreeType" , DoneFreeType);
  Nan::SetMethod(target, "newMemoryFace", NewMemoryFace);
  Nan::SetMethod(target, "doneFace"     , DoneFace);
  Nan::SetMethod(target, "setCharSize"  , SetCharSize);
  Nan::SetMethod(target, "getCharIndex" , GetCharIndex);
  Nan::SetMethod(target, "loadGlyph"    , LoadGlyph);
}
NODE_MODULE(freetype, Init)