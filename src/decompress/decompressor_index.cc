#include <napi.h>
#include <uv.h>
#include "stream_decompressor.h"

namespace ZSTD_NODE {

  Napi::Object Init(Napi::Env env, Napi::Object exports) {
    StreamDecompressor::Init(env, target, module);
  }

  NODE_API_MODULE(decompressor, Init)
}
