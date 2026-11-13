#pragma once
#include "library.h"
#ifndef _WIN32
#include <SDL/SDL_thread.h>
#endif
namespace moss {
// Owns file I/O, never the PCM: Capture keeps its completed buffer immutable
// until App observes completion. Only this worker writes its source metadata.
struct CaptureStore {
  uint32_t state = 0; // idle / writing / complete / failed
  char path[128] = {};
  const int16_t *pcm = nullptr;
  unsigned frames = 0, rate = 0;
  WavSource source;
#ifdef _WIN32
  HANDLE thread = nullptr;
  static DWORD WINAPI entry(void *context) {
    ((CaptureStore *)context)->write();
    return 0;
  }
#else
  SDL_Thread *thread = nullptr;
  static int entry(void *context) {
    ((CaptureStore *)context)->write();
    return 0;
  }
#endif
  void write() {
    bool ok = writeWav(path, pcm, frames, rate, 2);
    if (ok && source.open(path))
      source.overview();
    else
      ok = false;
    __atomic_store_n(&state, ok ? 2u : 3u, __ATOMIC_RELEASE);
  }
  void start(const char *file, const int16_t *data, unsigned length,
             unsigned hz) {
    snprintf(path, sizeof(path), "%s", file);
    pcm = data;
    frames = length;
    rate = hz;
    __atomic_store_n(&state, 1u, __ATOMIC_RELEASE);
#ifdef _WIN32
    thread = CreateThread(nullptr, 0, entry, this, 0, nullptr);
#else
    thread = SDL_CreateThread(entry, this);
#endif
    if (!thread)
      __atomic_store_n(&state, 3u, __ATOMIC_RELEASE);
  }
  void join() {
    if (!thread)
      return;
#ifdef _WIN32
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
#else
    SDL_WaitThread(thread, nullptr);
#endif
    thread = nullptr;
  }
  unsigned poll() {
    unsigned status = __atomic_load_n(&state, __ATOMIC_ACQUIRE);
    if (status == 2 || status == 3) {
      join();
      __atomic_store_n(&state, 0u, __ATOMIC_RELEASE);
    }
    return status;
  }
};
} // namespace moss
