#include "stream_compressor.h"
#include "stream_compress_worker.h"

namespace ZSTD_NODE {

  using Napi::SetPrototypeMethod;
  using Napi::GetCurrentContext;
  using Napi::AsyncQueueWorker;
  using Napi::GetFunction;
  using Napi::HandleScope;
  using Napi::ObjectWrap;
  using Napi::NewBuffer;
  using Napi::FunctionReference;
  using Napi::Has;
  using Napi::Get;
  using Napi::Set;

  using node::Buffer::Length;
  using node::Buffer::Data;

  using Napi::FunctionReference;
  using Napi::Number;
  using Napi::String;
    using Napi::Value;

  Napi::Object StreamCompressor::Init(Napi::Env env, Napi::Object exports) {
    Napi::FunctionReference tpl = Napi::Function::New(env, New);
    tpl->SetClassName(Napi::String::New(env, "StreamCompressor"));


    SetPrototypeMethod(tpl, "getBlockSize", GetBlockSize);
    SetPrototypeMethod(tpl, "copy", Copy);
    SetPrototypeMethod(tpl, "compress", Compress);

    constructor().Reset(GetFunction(tpl));
    Set(target, Napi::String::New(env, "StreamCompressor"),
        GetFunction(tpl));
  }

  StreamCompressor::StreamCompressor(Napi::Object userParams) :  zcs(NULL), dict(NULL) {
    HandleScope scope;

    int level = 1;
    size_t dictSize = 0;

    Napi::String key;
    key = Napi::String::New(env, "level");
    if (Has(userParams, key)) {
      level = Get(userParams, key)->Int32Value(Napi::GetCurrentContext());
    }
    key = Napi::String::New(env, "dict");
    if (Has(userParams, key)) {
      Napi::Object dictBuf = Get(userParams, key)->ToObject(Napi::GetCurrentContext()).FromMaybe(Napi::Object());
      dictSize = Length(dictBuf);
      dict = alloc.Alloc(dictSize);
      memcpy(dict, Data(dictBuf), dictSize);
    }

    inputSize = ZSTD_CStreamInSize();
    input = alloc.Alloc(inputSize);
    inPos = 0;

    dstSize = ZSTD_CStreamOutSize();
    dst = alloc.Alloc(dstSize);
    dstPos = 0;

    ZSTD_customMem zcm = {Allocator::Alloc, Allocator::Free, &alloc};
    zcs = ZSTD_createCStream_advanced(zcm);

    if (dict != NULL && dictSize > 0) {
      ZSTD_initCStream_usingDict(zcs, dict, dictSize, level);
    } else {
      ZSTD_initCStream(zcs, level);
    }
  }

  StreamCompressor::~StreamCompressor() {
    if (dict != NULL) {
      alloc.Free(dict);
    }
    if (input != NULL) {
      alloc.Free(input);
    }
    if (dst != NULL) {
      alloc.Free(dst);
    }
    ZSTD_freeCStream(zcs);
  }

  Napi::Value StreamCompressor::New(const Napi::CallbackInfo& info) {
    if (!info.IsConstructCall()) {
      Napi::Error::New(env, "StreamCompressor() must be called as a constructor").ThrowAsJavaScriptException();
      return env.Null();
    }
    Napi::Object buf = info[0].ToObject(Napi::GetCurrentContext()).FromMaybe(Napi::Object());
    StreamCompressor *sc = new StreamCompressor(buf);
    sc->Wrap(info.This());
    return info.This();
  }

  Napi::Value StreamCompressor::GetBlockSize(const Napi::CallbackInfo& info) {
    return Napi::Number::New(env, ZSTD_CStreamInSize());
  }

  Napi::Value StreamCompressor::Copy(const Napi::CallbackInfo& info) {
    StreamCompressor* sc = ObjectWrap::Unwrap<StreamCompressor>(info.Holder());
    Napi::Object chunkBuf = info[0].ToObject(Napi::GetCurrentContext()).FromMaybe(Napi::Object());
    char *chunk = Data(chunkBuf);
    size_t chunkSize = Length(chunkBuf);
    if (chunkSize != 0) {
      if (sc->inPos == sc->inputSize) {
        sc->inPos = 0;
      }
      char *pos = static_cast<char*>(sc->input) + sc->inPos;
      memcpy(pos, chunk, chunkSize);
      sc->inPos += chunkSize;
    }
  }

  Napi::Value StreamCompressor::Compress(const Napi::CallbackInfo& info) {
    StreamCompressor* sc = ObjectWrap::Unwrap<StreamCompressor>(info.Holder());
    bool isLast = info[0].BooleanValue(Napi::GetCurrentContext());
    bool isAsync = info[2].BooleanValue(Napi::GetCurrentContext());
    Callback *callback = new Callback(info[1].As<Napi::Function>());
    StreamCompressWorker *worker = new StreamCompressWorker(callback, sc, isLast);
    if (isAsync) {
      AsyncQueueWorker(worker);
    } else {
      worker->Execute();
      worker->WorkComplete();
    }
  }

  inline Persistent<Function>& StreamCompressor::constructor() {
    static Persistent<Function> ctor;
    return ctor;
  }

}
