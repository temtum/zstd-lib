#ifndef STREAM_DECOMPRESSOR_H
#define STREAM_DECOMPRESSOR_H

#include <napi.h>
#include <uv.h>

#define ZSTD_STATIC_LINKING_ONLY
#include "zstd.h"
#include "stream_coder.h"

namespace ZSTD_NODE {

  using Napi::Persistent;

  using Napi::Function;
  using Napi::Object;
  
  class StreamDecompressor : public StreamCoder {
  public:
    friend class StreamDecompressWorker;
    static Napi::Object Init(Napi::Env env, Napi::Object exports);

  private:
    explicit StreamDecompressor(Napi::Object userParams);
    ~StreamDecompressor();

    static Napi::Value New(const Napi::CallbackInfo& info);
    static Napi::Value GetBlockSize(const Napi::CallbackInfo& info);
    static Napi::Value Copy(const Napi::CallbackInfo& info);
    static Napi::Value Decompress(const Napi::CallbackInfo& info);

    static inline Persistent<Function>& constructor();

    ZSTD_DStream *zds;
    
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
