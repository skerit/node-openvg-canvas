#include "freetype.h"

#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include <v8.h>
#include <node_buffer.h>
#include "nan.h"

using namespace v8;

/* FreeType */
NAN_METHOD(InitFreeType) {
  Nan::HandleScope scope;

  FT_Library *library = (FT_Library*) malloc(sizeof(FT_Library));
  Nan::AdjustExternalMemory(sizeof(FT_Library));

  if (FT_Init_FreeType(library)) {
    Nan::ThrowError("Error initializing freetype.");
  }

  info.GetReturnValue().Set(Nan::New<External>(library));
}

NAN_METHOD(DoneFreeType) {
  Nan::HandleScope scope;

  FT_Library* library = static_cast<FT_Library*>(External::Cast(*info[0])->Value());

  FT_Error error = FT_Done_FreeType(*library);
  free(library);
  Nan::AdjustExternalMemory(-sizeof(FT_Library));

  if (error) {
    Nan::ThrowError("Error unloading face.");
  }
}

/* Face */
NAN_METHOD(NewMemoryFace) {
  Nan::HandleScope scope;

  FT_Library* library = static_cast<FT_Library*>(External::Cast(*info[0])->Value());

  if (!node::Buffer::HasInstance(info[1])) {
    Nan::ThrowTypeError("Second argument must be a buffer");
  }

  Local<Object> bufferObj = info[1]->ToObject();
  char *bufferData = node::Buffer::Data(bufferObj);
  size_t bufferLength = node::Buffer::Length(bufferObj);

  FT_Face *face = (FT_Face*) malloc(sizeof(FT_Face));
  Nan::AdjustExternalMemory(sizeof(FT_Face));

  FT_Byte* file_base  = (FT_Byte*) bufferData;
  FT_Long  file_size  = (FT_Long) bufferLength;
  FT_Long  face_index = (FT_Long) (FT_Long) info[2]->Int32Value();

  // TODO: One or more of:
  //         * Enable/Disable this code with macros
  //         * Implement similar functionality in test code.

  FT_Error error = FT_New_Memory_Face(*library, file_base, file_size, face_index, face);

  if (error) {
    free(face);
    Nan::AdjustExternalMemory(-sizeof(FT_Face));
    Nan::ThrowError("Error loading face.");
  }

  Local<Object> faceObj = Nan::New<Object>();
  Nan::Set(faceObj, Nan::New("face").ToLocalChecked(), Nan::New<External>(face));
  Nan::Set(faceObj, Nan::New("glyph").ToLocalChecked(), Nan::Null());

  Nan::Set(faceObj, Nan::New("num_glyphs").ToLocalChecked(), Nan::New((int32_t) (*face)->num_glyphs));

  Nan::Set(faceObj, Nan::New("family_name").ToLocalChecked(), Nan::New((*face)->family_name).ToLocalChecked());
  Nan::Set(faceObj, Nan::New("style_name").ToLocalChecked(), Nan::New((*face)->style_name).ToLocalChecked());

  Nan::Set(faceObj, Nan::New("units_per_EM").ToLocalChecked(), Nan::New<Uint32>((*face)->units_per_EM));
  Nan::Set(faceObj, Nan::New("ascender").ToLocalChecked(), Nan::New<Int32>((*face)->ascender));
  Nan::Set(faceObj, Nan::New("descender").ToLocalChecked(), Nan::New<Int32>((*face)->descender));
  Nan::Set(faceObj, Nan::New("height").ToLocalChecked(), Nan::New<Int32>((*face)->height));

  info.GetReturnValue().Set(faceObj);
}

NAN_METHOD(DoneFace) {
  Nan::HandleScope scope;

  Local<Object> faceObj = info[0]->ToObject();
  FT_Face* face = static_cast<FT_Face*>(External::Cast(*Nan::Get(faceObj, Nan::New("face").ToLocalChecked()).ToLocalChecked())->Value());

  FT_Error error = FT_Done_Face(*face);
  free(face);
  Nan::AdjustExternalMemory(-sizeof(FT_Face));
  Nan::Set(faceObj, Nan::New("face").ToLocalChecked(), Nan::Null());

  if (error) {
    Nan::ThrowError("Error unloading face.");
  }
}

NAN_METHOD(SetCharSize) {
  Nan::HandleScope scope;

  Local<Object> faceObj = info[0]->ToObject();
  FT_Face* face = static_cast<FT_Face*>(External::Cast(*Nan::Get(faceObj, Nan::New("face").ToLocalChecked()).ToLocalChecked())->Value());

  FT_F26Dot6  char_width      = info[1]->Uint32Value();
  FT_F26Dot6  char_height     = info[2]->Uint32Value();
  FT_UInt     horz_resolution = info[3]->Uint32Value();
  FT_UInt     vert_resolution = info[4]->Uint32Value();

  FT_Error error = FT_Set_Char_Size(*face, char_width, char_height, horz_resolution, vert_resolution);

  if (error) {
    Nan::ThrowError("Error setting char size.");
  }
}

NAN_METHOD(GetCharIndex) {
  Nan::HandleScope scope;

  Local<Object> faceObj = info[0]->ToObject();
  FT_Face* face = static_cast<FT_Face*>(External::Cast(*Nan::Get(faceObj, Nan::New("face").ToLocalChecked()).ToLocalChecked())->Value());

  FT_ULong charcode = (FT_ULong) info[1]->Uint32Value();

  FT_UInt result = FT_Get_Char_Index(*face, charcode);

  info.GetReturnValue().Set(Nan::New<Uint32>(result));
}

NAN_METHOD(LoadGlyph) {
  Nan::HandleScope scope;

  Local<Object> faceObj = info[0]->ToObject();
  FT_Face* face = static_cast<FT_Face*>(External::Cast(*Nan::Get(faceObj, Nan::New("face").ToLocalChecked()).ToLocalChecked())->Value());

  FT_UInt  glyph_index = (FT_UInt) info[1]->Uint32Value();
  FT_Int32 load_flags = (FT_Int32) info[2]->Int32Value();

  FT_Error error = FT_Load_Glyph(*face, glyph_index, load_flags);

  if (error) {
    Nan::ThrowError("Error loading glyph.");
  }

  Local<Object> glyphObj = Nan::New<Object>();
  Nan::Set(faceObj, Nan::New("glyph").ToLocalChecked(), glyphObj);

  Local<Object> advanceObj = Nan::New<Object>();
  Nan::Set(glyphObj, Nan::New("advance").ToLocalChecked(), advanceObj);
  Nan::Set(advanceObj, Nan::New("x").ToLocalChecked(), Nan::New<Int32>((int32_t) (*face)->glyph->advance.x));
  Nan::Set(advanceObj, Nan::New("y").ToLocalChecked(), Nan::New<Int32>((int32_t) (*face)->glyph->advance.y));

  Local<Object> outlineObj = Nan::New<Object>();
  Nan::Set(glyphObj, Nan::New("outline").ToLocalChecked(), outlineObj);
  Nan::Set(outlineObj, Nan::New("nContours").ToLocalChecked(), Nan::New<Int32>((*face)->glyph->outline.n_contours));
  Nan::Set(outlineObj, Nan::New("nPoints").ToLocalChecked(), Nan::New<Int32>((*face)->glyph->outline.n_points));

  {
    size_t size;
    size = (*face)->glyph->outline.n_points * 2 * sizeof(int32_t);

    #if NODE_MAJOR_VERSION == 0 && NODE_MINOR_VERSION <= 10
      Local<Int32> sizeHandle = Nan::New(size);
      Local<Value> caargv[] = { sizeHandle };
      Local<Object> arr = global->Get(Nan::New("Int32Array").ToLocalChecked()).As<Function>()->NewInstance(1, caargv);
    #else
      Local<Int32Array> arr = Int32Array::New(ArrayBuffer::New(Isolate::GetCurrent(), size), 0, size);
    #endif

    Nan::TypedArrayContents<int32_t> arrContents(arr);
    int32_t* arrPtr = *arrContents;
    memcpy(arrPtr, (*face)->glyph->outline.points, size);

    Nan::Set(outlineObj, Nan::New("points").ToLocalChecked(), arr);
  }

  {
    size_t size = (*face)->glyph->outline.n_points * sizeof(int8_t);

    #if NODE_MAJOR_VERSION == 0 && NODE_MINOR_VERSION <= 10
      Local<Int32> sizeHandle = Nan::New(size);
      Local<Value> caargv[] = { sizeHandle };
      Local<Object> arr = global->Get(Nan::New("Int8Array").ToLocalChecked()).As<Function>()->NewInstance(1, caargv);
    #else
      Local<Int8Array> arr = Int8Array::New(ArrayBuffer::New(Isolate::GetCurrent(), size), 0, size);
    #endif

    Nan::TypedArrayContents<int8_t> arrContents(arr);
    int8_t* arrPtr = *arrContents;
    memcpy(arrPtr, (*face)->glyph->outline.tags, size);

    Nan::Set(outlineObj, Nan::New("tags").ToLocalChecked(), arr);
  }

  {
    size_t size = (*face)->glyph->outline.n_contours * sizeof(int16_t);

    #if NODE_MAJOR_VERSION == 0 && NODE_MINOR_VERSION <= 10
      Local<Int32> sizeHandle = Nan::New(size);
      Local<Value> caargv[] = { sizeHandle };
      Local<Object> arr = global->Get(Nan::New("Int16Array").ToLocalChecked()).As<Function>()->NewInstance(1, caargv);
    #else
      Local<Int16Array> arr = Int16Array::New(ArrayBuffer::New(Isolate::GetCurrent(), size), 0, size);
    #endif

    Nan::TypedArrayContents<int16_t> arrContents(arr);
    int16_t* arrPtr = *arrContents;
    memcpy(arrPtr, (*face)->glyph->outline.contours, size);

    Nan::Set(outlineObj, Nan::New("contours").ToLocalChecked(), arr);
  }

  Nan::Set(outlineObj, Nan::New("flags").ToLocalChecked(), Nan::New<Int32>((*face)->glyph->outline.flags));

  info.GetReturnValue().Set(glyphObj);
}
