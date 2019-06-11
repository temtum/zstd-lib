#ifndef STREAM_DECOMPRESSOR_WORKER_H
#define STREAM_DECOMPRESSOR_WORKER_H

#include <napi.h>
#include <uv.h>
#include "stream_decompressor.h"

namespace ZSTD_NODE {

  using Napi::AsyncWorker;
  using Napi::FunctionReference;

  class StreamDecompressWorker : public AsyncWorker {
  public:
    StreamDecompressWorker(Callback *callback, StreamDecompressor *sd);
    ~StreamDecompressWorker();

    void Execute();
    void OnOK();
    void OnError();

  private:
    void pushToPendingOutput();

    StreamDecompressor *sd;
    ZSTD_outBuffer zOutBuf;
    ZSTD_inBuffer zInBuf;

    size_t ret;
  };

}

#endif
