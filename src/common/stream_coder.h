#ifndef STREAM_CODER_H
#define STREAM_CODER_H

#include <napi.h>
#include <uv.h>
#include "allocator.h"

namespace ZSTD_NODE {

  using Napi::ObjectWrap;

  using std::vector;

  using Napi::Array;
  
  class StreamCoder : public ObjectWrap {
  public:
    Allocator alloc;

    vector<char*> pending_output;
    Napi::Array PendingChunksAsArray();

  protected:
    explicit StreamCoder();
    ~StreamCoder();
  };

}

#endif
