#include "stream_coder.h"

namespace ZSTD_NODE {

  StreamCoder::StreamCoder(const Napi::CallbackInfo &info) : Napi::ObjectWrap<StreamCoder>(info) {};

  StreamCoder::~StreamCoder() {
    size_t nChunks = pending_output.size();
    for (size_t i = 0; i < nChunks; i++) {
      alloc.Free(pending_output[i]);
    }
    //alloc.ReportMemoryToV8(Env());
  }

  Napi::Array StreamCoder::PendingChunksAsArray(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    size_t nChunks = pending_output.size();
    Napi::Array chunks = Napi::Array::New(env, nChunks);

    for (size_t i = 0; i < nChunks; i++) {
      char *cur = pending_output[i];
      Allocator::AllocatedBuffer *bufInfo = Allocator::GetBufferInfo(cur);
      Napi::Buffer<char> buf = Napi::Buffer<char>::New(env,
        reinterpret_cast<char*>(cur),
        bufInfo->size - bufInfo->available
      );
      (chunks).Set(i, buf);
    }

    pending_output.clear();

    return chunks;
  }

}