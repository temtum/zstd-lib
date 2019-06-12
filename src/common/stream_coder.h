#ifndef STREAM_CODER_H
#define STREAM_CODER_H

#include <napi.h>
#include <uv.h>
#include "allocator.h"

namespace ZSTD_NODE {

  using Napi::ObjectWrap;

  using std::vector;

  using Napi::Array;
  
  class StreamCoder : public Napi::ObjectWrap<StreamCoder> {

  public:
    static Napi::FunctionReference constructor;
    explicit StreamCoder(const Napi::CallbackInfo &info);
    ~StreamCoder();

  private:
    Allocator alloc;

    vector<char*> pending_output;
    Napi::Array PendingChunksAsArray(const Napi::CallbackInfo &info);
  };

}

#endif