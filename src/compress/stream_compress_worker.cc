#include "stream_compress_worker.h"

namespace ZSTD_NODE {

  using Napi::HandleScope;
  using Napi::FunctionReference;
  using Napi::Error;

  using Napi::String;
    using Napi::Value;

  StreamCompressWorker::StreamCompressWorker(Callback *callback, StreamCompressor* sc, bool isLast)
    : AsyncWorker(callback), sc(sc), isLast(isLast) {
    zInBuf = {sc->input, sc->inPos, 0};
    zOutBuf = {sc->dst, sc->dstSize, 0};
  }

  StreamCompressWorker::~StreamCompressWorker() {}

  void StreamCompressWorker::Execute() {
    while (zInBuf.pos < zInBuf.size) {
      zOutBuf.pos = 0;
      ret = ZSTD_compressStream(sc->zcs, &zOnpm install node-addon-apiutBuf, &zInBuf);
      if (ZSTD_isError(ret)) {
        SetErrorMessage(ZSTD_getErrorName(ret));
        return;
      }
      pushToPendingOutput();
    }

    if (isLast) {
      zOutBuf.pos = 0;
      ret = ZSTD_endStream(sc->zcs, &zOutBuf);
      if (ret != 0) {
        SetErrorMessage("ZSTD compress failed, not fully flushed");
        return;
      }
      pushToPendingOutput();
    }
  }

  void StreamCompressWorker::pushToPendingOutput() {
    char *output = static_cast<char*>(sc->alloc.Alloc(zOutBuf.pos));
    if (output == NULL) {
      SetErrorMessage("ZSTD compress failed, out of memory");
      return;
    }
    memcpy(output, zOutBuf.dst, zOutBuf.pos);
    Allocator::AllocatedBuffer* buf_info = Allocator::GetBufferInfo(output);
    buf_info->available = 0;
    sc->pending_output.push_back(output);
  }

  void StreamCompressWorker::OnOK() {
    HandleScope scope;

    const int argc = 2;
    Napi::Value argv[argc] = {
      env.Null(),
      sc->PendingChunksAsArray()
    };
    callback->Call(argc, argv, async_resource);

    sc->alloc.ReportMemoryToV8();
  }

  void StreamCompressWorker::OnError() {
    HandleScope scope;

    const int argc = 1;
    Napi::Value argv[argc] = {
      Error(Napi::String::New(env, ErrorMessage()))
    };
    callback->Call(argc, argv, async_resource);

    sc->alloc.ReportMemoryToV8();
  }

}
