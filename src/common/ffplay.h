#ifndef SDL_FFPLAY_H
#define SDL_FFPLAY_H

#include <inttypes.h>
#include <math.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>

#include "libavutil/avstring.h"
#include "libavutil/eval.h"
#include "libavutil/mathematics.h"
#include "libavutil/pixdesc.h"
#include "libavutil/imgutils.h"
#include "libavutil/dict.h"
#include "libavutil/parseutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/avassert.h"
#include "libavutil/time.h"
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#include "libavutil/opt.h"
#include "libavcodec/avfft.h"
#include "libswresample/swresample.h"

#if CONFIG_AVFILTER
# include "libavfilter/avfilter.h"
# include "libavfilter/buffersink.h"
# include "libavfilter/buffersrc.h"
#endif

#include <SDL.h>
#include <SDL_thread.h>

#include <assert.h>

extern SDL_Window* window;
extern SDL_Renderer* renderer;

extern int destWidth;
extern int destHeight;

int ffplay_main(int argc, char **argv);

typedef struct SpecifierOpt {
    char *specifier;    /**< stream/chapter/program/... specifier */
    union {
        uint8_t *str;
        int        i;
        int64_t  i64;
        uint64_t ui64;
        float      f;
        double   dbl;
    } u;
} SpecifierOpt;

typedef struct OptionDef {
    const char *name;
    int flags;
#define HAS_ARG    0x0001
#define OPT_BOOL   0x0002
#define OPT_EXPERT 0x0004
#define OPT_STRING 0x0008
#define OPT_VIDEO  0x0010
#define OPT_AUDIO  0x0020
#define OPT_INT    0x0080
#define OPT_FLOAT  0x0100
#define OPT_SUBTITLE 0x0200
#define OPT_INT64  0x0400
#define OPT_EXIT   0x0800
#define OPT_DATA   0x1000
#define OPT_PERFILE  0x2000     /* the option is per-file (currently ffmpeg-only).
                                   implied by OPT_OFFSET or OPT_SPEC */
#define OPT_OFFSET 0x4000       /* option is specified as an offset in a passed optctx */
#define OPT_SPEC   0x8000       /* option is to be stored in an array of SpecifierOpt.
                                   Implies OPT_OFFSET. Next element after the offset is
                                   an int containing element count in the array. */
#define OPT_TIME  0x10000
#define OPT_DOUBLE 0x20000
#define OPT_INPUT  0x40000
#define OPT_OUTPUT 0x80000
     union {
        void *dst_ptr;
        int (*func_arg)(void *, const char *, const char *);
        size_t off;
    } u;
    const char *help;
    const char *argname;
} OptionDef;

#if CONFIG_AVDEVICE
#define CMDUTILS_COMMON_OPTIONS_AVDEVICE                                                                                \
    { "sources"    , OPT_EXIT | HAS_ARG, { .func_arg = show_sources },                                                  \
      "list sources of the input device", "device" },                                                                   \
    { "sinks"      , OPT_EXIT | HAS_ARG, { .func_arg = show_sinks },                                                    \
      "list sinks of the output device", "device" },                                                                    \

#else
#define CMDUTILS_COMMON_OPTIONS_AVDEVICE
#endif

#define CMDUTILS_COMMON_OPTIONS                                                                                         \
/*    { "L",           OPT_EXIT,             { .func_arg = show_license },     "show license" },                          \
    { "h",           OPT_EXIT,             { .func_arg = show_help },        "show help", "topic" },                    \
    { "?",           OPT_EXIT,             { .func_arg = show_help },        "show help", "topic" },                    \
    { "help",        OPT_EXIT,             { .func_arg = show_help },        "show help", "topic" },                    \
    { "-help",       OPT_EXIT,             { .func_arg = show_help },        "show help", "topic" },                    \
    { "version",     OPT_EXIT,             { .func_arg = show_version },     "show version" },                          \
    { "buildconf",   OPT_EXIT,             { .func_arg = show_buildconf },   "show build configuration" },              \
    { "formats",     OPT_EXIT,             { .func_arg = show_formats },     "show available formats" },                \
    { "muxers",      OPT_EXIT,             { .func_arg = show_muxers },      "show available muxers" },                 \
    { "demuxers",    OPT_EXIT,             { .func_arg = show_demuxers },    "show available demuxers" },               \
    { "devices",     OPT_EXIT,             { .func_arg = show_devices },     "show available devices" },                \
    { "codecs",      OPT_EXIT,             { .func_arg = show_codecs },      "show available codecs" },                 \
    { "decoders",    OPT_EXIT,             { .func_arg = show_decoders },    "show available decoders" },               \
    { "encoders",    OPT_EXIT,             { .func_arg = show_encoders },    "show available encoders" },               \
    { "bsfs",        OPT_EXIT,             { .func_arg = show_bsfs },        "show available bit stream filters" },     \
    { "protocols",   OPT_EXIT,             { .func_arg = show_protocols },   "show available protocols" },              \
    { "filters",     OPT_EXIT,             { .func_arg = show_filters },     "show available filters" },                \
    { "pix_fmts",    OPT_EXIT,             { .func_arg = show_pix_fmts },    "show available pixel formats" },          \
    { "layouts",     OPT_EXIT,             { .func_arg = show_layouts },     "show standard channel layouts" },         \
    { "sample_fmts", OPT_EXIT,             { .func_arg = show_sample_fmts }, "show available audio sample formats" },   \
    { "colors",      OPT_EXIT,             { .func_arg = show_colors },      "show available color names" },            \
    { "loglevel",    HAS_ARG,              { .func_arg = opt_loglevel },     "set logging level", "loglevel" },         \
    { "v",           HAS_ARG,              { .func_arg = opt_loglevel },     "set logging level", "loglevel" },         \
    { "report",      0,                    { (void*)opt_report },            "generate a report" },                     \
    { "max_alloc",   HAS_ARG,              { .func_arg = opt_max_alloc },    "set maximum size of a single allocated block", "bytes" }, \
    { "cpuflags",    HAS_ARG | OPT_EXPERT, { .func_arg = opt_cpuflags },     "force specific cpu flags", "flags" },     \
    { "hide_banner", OPT_BOOL | OPT_EXPERT, {&hide_banner},     "do not show program banner", "hide_banner" },          \
    CMDUTILS_COMMON_OPTIONS_AVDEVICE                                                                                    \
*/
#endif /* SDL_FFPLAY_H */