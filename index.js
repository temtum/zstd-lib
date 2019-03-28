'use strict';

exports.compress = compress;
exports.decompress = decompress;
exports.compressSync = compressSync;
exports.decompressSync = decompressSync;
exports.compressStream = compressStream;
exports.decompressStream = decompressStream;

const compressor = require('bindings')('compressor.node');
const decompressor = require('bindings')('decompressor.node');
const Transform = require('stream').Transform;
const util = require('util');

function compress(input, params, cb) {
  if (arguments.length === 2) {
    cb = params;
    params = {};
  }
  if (!Buffer.isBuffer(input)) {
    process.nextTick(cb, new Error('Input is not a buffer.'));
    return;
  }
  if (typeof cb !== 'function') {
    process.nextTick(cb, new Error('Second argument is not a function.'));
    return;
  }
  const stream = new TransformStreamCompressor(params);
  const chunks = [];
  let length = 0;

  stream.on('error', cb);
  stream.on('data', function(c) {
    chunks.push(c);
    length += c.length;
  });
  stream.on('end', function() {
    cb(null, Buffer.concat(chunks, length));
  });
  stream.end(input);
}

function decompress(input, params, cb) {
  if (arguments.length === 2) {
    cb = params;
    params = {};
  }
  if (!Buffer.isBuffer(input)) {
    process.nextTick(cb, new Error('Input is not a buffer.'));
    return;
  }
  if (typeof cb !== 'function') {
    process.nextTick(cb, new Error('Second argument is not a function.'));
    return;
  }

  const stream = new TransformStreamDecompressor(params);
  const chunks = [];
  let length = 0;
  stream.on('error', cb);
  stream.on('data', function(c) {
    chunks.push(c);
    length += c.length;
  });
  stream.on('end', function() {
    cb(null, Buffer.concat(chunks, length));
  });
  stream.end(input);
}

function compressSync(input, params) {
  if (!Buffer.isBuffer(input)) {
    throw new Error('Input is not a buffer.');
  }
  const stream = new TransformStreamCompressor(params || {}, true);
  const chunks = [];
  let length = 0;

  stream.on('error', function(e) {
    throw e;
  });
  stream.on('data', function(c) {
    chunks.push(c);
    length += c.length;
  });
  stream.end(input);

  return Buffer.concat(chunks, length);
}

function decompressSync(input, params) {
  if (!Buffer.isBuffer(input)) {
    throw new Error('Input is not a buffer.');
  }

  const stream = new TransformStreamDecompressor(params || {}, true);
  const chunks = [];
  let length = 0;

  stream.on('error', function(e) {
    throw e;
  });
  stream.on('data', function(c) {
    chunks.push(c);
    length += c.length;
  });
  stream.end(input);

  return Buffer.concat(chunks, length);
}

function TransformStreamCompressor(params, sync) {
  Transform.call(this, params);

  this.compressor = new compressor.StreamCompressor(params || {});
  this.sync = sync || false;

  const blockSize = this.compressor.getBlockSize();

  this.status = {
    blockSize: blockSize,
    remaining: blockSize
  };
}
util.inherits(TransformStreamCompressor, Transform);

TransformStreamCompressor.prototype._transform = function(chunk, encoding, next) {
  compressStreamChunk(this, chunk, this.compressor, this.status, this.sync, next);
};

TransformStreamCompressor.prototype._flush = function(done) {
  const that = this;

  this.compressor.compress(true, function(err, output) {
    if (err) {
      return done(err);
    }
    if (output) {
      for (let i = 0; i < output.length; i++) {
        that.push(output[i]);
      }
    }
    return done();
  }, !this.sync);
};

// We need to fill the blockSize for better compression results
function compressStreamChunk(stream, chunk, compressor, status, sync, done) {
  const length = chunk.length;

  if (length > status.remaining) {
    const slicedChunk = chunk.slice(0, status.remaining);

    chunk = chunk.slice(status.remaining);
    status.remaining = status.blockSize;

    compressor.copy(slicedChunk);
    compressor.compress(false, function(err, output) {
      if (err) {
        return done(err);
      }
      if (output) {
        for (let i = 0; i < output.length; i++) {
          stream.push(output[i]);
        }
      }
      return compressStreamChunk(stream, chunk, compressor, status, sync, done);
    }, !sync);
  } else if (length <= status.remaining) {
    status.remaining -= length;
    compressor.copy(chunk);
    return done();
  }
}

function compressStream(params) {
  return new TransformStreamCompressor(params);
}

function TransformStreamDecompressor(params, sync) {
  Transform.call(this, params);

  this.decompressor = new decompressor.StreamDecompressor(params || {});
  this.sync = sync || false;

  const blockSize = this.decompressor.getBlockSize();

  this.status = {
    blockSize: blockSize,
    remaining: blockSize
  };
}
util.inherits(TransformStreamDecompressor, Transform);

TransformStreamDecompressor.prototype._transform = function(chunk, encoding, next) {
  decompressStreamChunk(this, chunk, this.decompressor, this.status, this.sync, next);
};

TransformStreamDecompressor.prototype._flush = function(done) {
  const that = this;

  this.decompressor.decompress(function(err, output) {
    if (err) {
      return done(err);
    }
    if (output) {
      for (let i = 0; i < output.length; i++) {
        that.push(output[i]);
      }
    }
    return done();
  }, !this.sync);
};

// We need to fill the blockSize for better compression results
function decompressStreamChunk(stream, chunk, decompressor, status, sync, done) {
  const length = chunk.length;

  if (length > status.remaining) {
    const slicedChunk = chunk.slice(0, status.remaining);
    chunk = chunk.slice(status.remaining);
    status.remaining = status.blockSize;

    decompressor.copy(slicedChunk);
    decompressor.decompress(function(err, output) {
      if (err) {
        return done(err);
      }
      if (output) {
        for (let i = 0; i < output.length; i++) {
          stream.push(output[i]);
        }
      }
      return decompressStreamChunk(stream, chunk, decompressor, status, sync, done);
    }, !sync);
  } else if (length <= status.remaining) {
    status.remaining -= length;
    decompressor.copy(chunk);
    return done();
  }
}

function decompressStream(params) {
  return new TransformStreamDecompressor(params);
}
