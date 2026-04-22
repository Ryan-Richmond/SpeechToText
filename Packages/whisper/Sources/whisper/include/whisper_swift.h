// whisper_swift.h — Swift-facing shim for whisper.cpp
//
// This header is the ONLY header exported to the Swift module.
// It re-exports just the whisper.h API while hiding the internal
// ggml/gguf types that collide with the llama.swift xcframework.
//
// whisper.h itself includes ggml.h, so we need the compiler to see
// ggml.h during C compilation — but we prevent it from being part of
// the Swift Clang module by *not* listing it here.

#pragma once

// Forward-declare the opaque context type so Swift sees it without
// needing the full ggml.h definition.
#ifdef __cplusplus
extern "C" {
#endif

// Pull in the full whisper C API; the ggml types used in whisper.h
// are opaque from Swift's perspective (OpaquePointer).
#include "whisper.h"

#ifdef __cplusplus
}
#endif
