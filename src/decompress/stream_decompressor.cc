#include "stream_decompressor.h"
#include "stream_decompress_worker.h"

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

  Napi::Object StreamDecompressor::Init(Napi::Env env, Napi::Object exports) {
    Napi::FunctionReference tpl = Napi::Function::New(env, New);
    tpl->SetClassName(Napi::String::New(env, "StreamDecompressor"));


    SetPrototypeMethod(tpl, "getBlockSize", GetBlockSize);
    SetPrototypeMethod(tpl, "copy", Copy);
    SetPrototypeMethod(tpl, "decompress", Decompress);

    constructor().Reset(GetFunction(tpl));
    Set(target, Napi::String::New(env, "StreamDecompressor"),
        GetFunction(tpl));
  }

  StreamDecompressor::StreamDecompressor(Napi::Object userParams) : zds(NULL), dict(NULL) {
    HandleScope scope;

    size_t dictSize = 0;

    Napi::String key;
    key = Napi::String::New(env, "dict");
    if (Has(userParams, key)) {
      Napi::Object dictBuf = Get(userParams, key)->ToObject(Napi::GetCurrentContext()).FromMaybe(Napi::Object());
      dictSize = Length(dictBuf);
      dict = alloc.Alloc(dictSize);
      memcpy(dict, Data(dictBuf), dictSize);
    }

    inputSize = ZSTD_DStreamInSize();
    input = alloc.Alloc(inputSize);
    inPos = 0;

    dstSize = ZSTD_DStreamOutSize();
    dst = alloc.Alloc(dstSize);
    dstPos = 0;

    ZSTD_customMem zcm = {Allocator::Alloc, Allocator::Free, &alloc};
    zds = ZSTD_createDStream_advanced(zcm);

    if (dict != NULL && dictSize > 0) {
      ZSTD_initDStream_usingDict(zds, dict, dictSize);
    } else {
      ZSTD_initDStream(zds);
    }
  }

  StreamDecompressor::~StreamDecompressor() {
    if (dict != NULL) {
      alloc.Free(dict);
    }
    if (input != NULL) {
      alloc.Free(input);
    }
    if (dst != NULL) {
      alloc.Free(dst);
    }
    ZSTD_freeDStream(zds);
  }

  Napi::Value StreamDecompressor::New(const Napi::CallbackInfo& info) {
    if (!info.IsConstructCall()) {
      Napi::Error::New(env, "StreamDecompressor() must be called as a constructor").ThrowAsJavaScriptException();
      return env.Null();
    }
    StreamDecompressor *sd = new StreamDecompressor(info[0].ToObject(Napi::GetCurrentContext()).FromMaybe(Napi::Object()));
    sd->Wrap(info.This());
    return info.This();
  }

  Napi::Value StreamDecompressor::GetBlockSize(const Napi::CallbackInfo& info) {
    return Napi::Number::New(env, ZSTD_DStreamInSize());
  }

  Napi::Value StreamDecompressor::Copy(const Napi::CallbackInfo& info) {
    StreamDecompressor* sd = ObjectWrap::Unwrap<StreamDecompressor>(info.Holder());
    Napi::Object chunkBuf = info[0].ToObject(Napi::GetCurrentContext()).FromMaybe(Napi::Object());
    char *chunk = Data(chunkBuf);
    size_t chunkSize = Length(chunkBuf);
    if (chunkSize != 0) {
      if (sd->inPos == sd->inputSize) {
        sd->inPos = 0;
      }
      char *pos = static_cast<char*>(sd->input) + sd->inPos;
      memcpy(pos, chunk, chunkSize);
      sd->inPos += chunkSize;
    }
  }

  Napi::Value StreamDecompressor::Decompress(const Napi::CallbackInfo& info) {
    StreamDecompressor* sd = ObjectWrap::Unwrap<StreamDecompressor>(info.Holder());
    bool isAsync = info[1].BooleanValue(Napi::GetCurrentContext());
    Callback *callback = new Callback(info[0].As<Napi::Function>());
    StreamDecompressWorker *worker = new StreamDecompressWorker(callback, sd);
    if (isAsync) {
      AsyncQueueWorker(worker);
    } else {
      worker->Execute();
      worker->WorkComplete();
    }
  }

  inline Persistent<Function>& StreamDecompressor::constructor() {
    static Persistent<Function> ctor;
    return ctor;
  }

}
