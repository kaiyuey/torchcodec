// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <memory>
#include <stdexcept>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/avutil.h>
#include <libavutil/dict.h>
#include <libavutil/display.h>
#include <libavutil/file.h>
#include <libavutil/opt.h>
#include <libavutil/pixfmt.h>
#include <libavutil/version.h>
#include <libswresample/swresample.h>
}

namespace facebook::torchcodec {

// FFMPEG uses special delete functions for some structures. These template
// functions are used to pass into unique_ptr as custom deleters so we can
// wrap FFMPEG structs with unique_ptrs for ease of use.
template <typename T, typename R, R (*Fn)(T**)>
struct Deleterp {
  inline void operator()(T* p) const {
    if (p) {
      Fn(&p);
    }
  }
};

// Unique pointers for FFMPEG structures.
using UniqueAVFormatContext = std::unique_ptr<
    AVFormatContext,
    Deleterp<AVFormatContext, void, avformat_close_input>>;
using UniqueAVCodecContext = std::unique_ptr<
    AVCodecContext,
    Deleterp<AVCodecContext, void, avcodec_free_context>>;
using UniqueAVFrame =
    std::unique_ptr<AVFrame, Deleterp<AVFrame, void, av_frame_free>>;
using UniqueAVPacket =
    std::unique_ptr<AVPacket, Deleterp<AVPacket, void, av_packet_free>>;
using UniqueAVFilterGraph = std::unique_ptr<
    AVFilterGraph,
    Deleterp<AVFilterGraph, void, avfilter_graph_free>>;
using UniqueAVFilterInOut = std::unique_ptr<
    AVFilterInOut,
    Deleterp<AVFilterInOut, void, avfilter_inout_free>>;
using UniqueAVIOContext = std::
    unique_ptr<AVIOContext, Deleterp<AVIOContext, void, avio_context_free>>;
#ifdef FFMPEG_VERSION_4
using AVCodecPtr = AVCodec*;
#else
using AVCodecPtr = const AVCodec*;
#endif

// Success code from FFMPEG is just a 0. We define it to make the code more
// readable.
const int AVSUCCESS = 0;

// Returns the FFMPEG error as a string using the provided `errorCode`.
std::string getFFMPEGErrorStringFromErrorCode(int errorCode);

// A struct that holds state for reading bytes from an IO context.
// We give this to FFMPEG and it will pass it back to us when it needs to read
// or seek in the memory buffer.
struct AVIOBufferData {
  const uint8_t* data;
  size_t size;
  size_t current;
};

// A class that can be used as AVFormatContext's IO context. It reads from a
// memory buffer that is passed in.
class AVIOBytesContext {
 public:
  AVIOBytesContext(const void* data, size_t data_size, size_t tempBufferSize);
  ~AVIOBytesContext();

  // Returns the AVIOContext that can be passed to FFMPEG.
  AVIOContext* getAVIO();

  // The signature of this function is defined by FFMPEG.
  static int read(void* opaque, uint8_t* buf, int buf_size);

  // The signature of this function is defined by FFMPEG.
  static int64_t seek(void* opaque, int64_t offset, int whence);

 private:
  UniqueAVIOContext avioContext_;
  struct AVIOBufferData bufferData_;
};

} // namespace facebook::torchcodec