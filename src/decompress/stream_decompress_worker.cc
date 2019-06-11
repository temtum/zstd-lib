#include "stream_decompress_worker.h"

namespace ZSTD_NODE {

  using Napi::HandleScope;
  using Napi::FunctionReference;
  using Napi::Error;

  using Napi::String;
    using Napi::Value;

  StreamDecompressWorker::StreamDecompressWorker(Callback *callback, StreamDecompressor* sd)
    : AsyncWorker(callback), sd(sd) {
    zInBuf = {sd->input, sd->inPos, 0};
    zOutBuf = {sd->dst, sd->dstSize, 0};
  }

  StreamDecompressWorker::~StreamDecompressWorker() {}

  void StreamDecompressWorker::Execute() {
    while (zInBuf.pos < zInBuf.size) {
      zOutBuf.pos = 0;
      ret = ZSTD_decompressStream(sd->zds, &zOutBuf, &zInBuf);
      if (ZSTD_isError(ret)) {
        SetErrorMessage(ZSTD_getErrorName(ret));
        return;
      }
      pushToPendingOutput();
    }
  }

  void StreamDecompressWorker::pushToPendingOutput() {
    char *output = static_cast<char*>(sd->alloc.Alloc(zOutBuf.pos));
    if (output == NULL) {
      SetErrorMessage("ZSTD decompress failed, out of memory");
      return;
    }
    memcpy(output, zOutBuf.dst, zOutBuf.pos);
    Allocator::AllocatedBuffer* buf_info = Allocator::GetBufferInfo(output);
    buf_info->available = 0;
    sd->pending_output.push_back(output);
  }

  void StreamDecompressWorker::OnOK() {
    HandleScope scope;

    const int argc = 2;
    Napi::Value argv[argc] = {
      env.Null(),
      sd->PendingChunksAsArray()
    };
    callback->Call(argc, argv, async_resource);

    sd->alloc.ReportMemoryToV8();
  }

  void StreamDecompressWorker::OnError() {
    HandleScope scope;

    const int argc = 1;
    Napi::Value argv[argc] = {
      Error(Napi::String::New(env, ErrorMessage()))
    };
    callback->Call(argc, argv, async_resource);

    sd->alloc.ReportMemoryToV8();
  }

}
