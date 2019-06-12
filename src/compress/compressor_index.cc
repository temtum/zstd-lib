#include <napi.h>
#include <uv.h>
#include "stream_compressor.h"

namespace ZSTD_NODE {

  Napi::Object Init (Napi::Env env, Napi::Object exports) {
      StreamCompressor::Init(env, exports);
      return exports;
  }

  NODE_API_MODULE(compressor, Init)

}