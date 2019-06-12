#ifndef STREAM_COMPRESSOR_H
#define STREAM_COMPRESSOR_H

#include <napi.h>
#include <uv.h>

#define ZSTD_STATIC_LINKING_ONLY
#include "zstd.h"
#include "stream_coder.h"

namespace ZSTD_NODE {

  using Napi::Function;
  using Napi::Object;
  
  class StreamCompressor : public StreamCoder {
  public:
    friend class StreamCompressWorker;
    static Napi::Object Init(Napi::Env env, Napi::Object exports);

  private:
    static Napi::FunctionReference constructor;
    explicit StreamCompressor(Napi::Object userParams);
    ~StreamCompressor();

    static Napi::Value New(const Napi::CallbackInfo& info);
    static Napi::Value GetBlockSize(const Napi::CallbackInfo& info);
    static Napi::Value Copy(const Napi::CallbackInfo& info);
    static Napi::Value Compress(const Napi::CallbackInfo& info);

    ZSTD_CStream *zcs;

    size_t inputSize;
    size_t dstSize;

    size_t inPos;
    size_t dstPos;
    
    void *input;
    void *dst;
    void *dict;
  };

}

#endif
