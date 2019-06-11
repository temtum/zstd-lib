#include "stream_coder.h"

namespace ZSTD_NODE {

  using Napi::NewBuffer;
  using Napi::Set;

  StreamCoder::StreamCoder() {};

  StreamCoder::~StreamCoder() {
    size_t nChunks = pending_output.size();
    for (size_t i = 0; i < nChunks; i++) {
      alloc.Free(pending_output[i]);
    }
    alloc.ReportMemoryToV8();
  }

  Napi::Array StreamCoder::PendingChunksAsArray() {
    size_t nChunks = pending_output.size();
    Napi::Array chunks = Napi::Array::New(env, nChunks);

    for (size_t i = 0; i < nChunks; i++) {
      char *cur = pending_output[i];
      Allocator::AllocatedBuffer *bufInfo = Allocator::GetBufferInfo(cur);
      Set(chunks, i, NewBuffer(reinterpret_cast<char*>(cur),
                               bufInfo->size - bufInfo->available,
                               Allocator::NodeFree,
                               NULL));
    }

    pending_output.clear();

    return chunks;
  }

}
