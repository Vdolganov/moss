#include "app.h"
#include <direct.h>
#include <mmsystem.h>
using namespace moss;
static App app;
static Engine engine;
static Library library;
static lab::Canvas canvas;
static HANDLE audioEvent, audioThread;
static HWAVEOUT audioDevice;
static CRITICAL_SECTION engineLock;
static volatile LONG stopping = 0;
static HWND window;
static int16_t pcm[4][2048];
static WAVEHDR headers[4];
static uint64_t micros() {
  LARGE_INTEGER now, freq;
  QueryPerformanceCounter(&now);
  QueryPerformanceFrequency(&freq);
  return uint64_t(now.QuadPart) * 1000000 / uint64_t(freq.QuadPart);
}
static DWORD WINAPI audioWorker(void *) {
  while (!InterlockedCompareExchange(&stopping, 0, 0)) {
    for (int i = 0; i < 4; i++)
      if (!(headers[i].dwFlags & WHDR_INQUEUE)) {
        EnterCriticalSection(&engineLock);
        engine.render(pcm[i], 1024);
        if (waveOutWrite(audioDevice, &headers[i], sizeof(WAVEHDR)) !=
            MMSYSERR_NOERROR) {
          InterlockedExchange(&stopping, 1);
          PostMessage(window, WM_APP + 1, 0, 0);
          LeaveCriticalSection(&engineLock);
          break;
        }
        LeaveCriticalSection(&engineLock);
      }
    WaitForSingleObject(audioEvent, 30);
  }
  return 0;
}
static int mapped(WPARAM key) {
  switch (key) {
  case VK_UP:
    return lab::Up;
  case VK_DOWN:
    return lab::Down;
  case VK_LEFT:
    return lab::Left;
  case VK_RIGHT:
    return lab::Right;
  case 'Z':
    return lab::A;
  case 'X':
    return lab::B;
  case 'C':
    return lab::X;
  case 'V':
    return lab::Y;
  case VK_SPACE:
    return lab::Start;
  case VK_TAB:
    return lab::Select;
  case 'Q':
    return lab::L1;
  case 'E':
    return lab::R1;
  case VK_OEM_4:
    return lab::L2;
  case VK_OEM_6:
    return lab::R2;
  case VK_ESCAPE:
    return lab::Menu;
  default:
    return -1;
  }
}
static LRESULT CALLBACK proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
  case WM_KEYDOWN:
  case WM_KEYUP:
    app.button(mapped(wp), msg == WM_KEYDOWN, micros());
    return 0;
  case WM_KILLFOCUS:
    app.clearInput();
    return 0;
  case WM_TIMER:
    app.tick(micros());
    app.pumpPreview();
    app.flushCapture();
    if (app.requestedSlot >= 0 || app.reload || app.job != App::NoJob) {
      EnterCriticalSection(&engineLock);
      waveOutPause(audioDevice);
      waveOutReset(audioDevice);
      app.streamReader.close();
      if (app.job != App::NoJob)
        app.service();
      else if (app.requestedSlot >= 0) {
        app.open(app.requestedSlot);
        app.requestedSlot = -1;
      } else
        engine.init(app.project, library.pcm, 44100, Assets);
      app.reload = false;
      waveOutRestart(audioDevice);
      LeaveCriticalSection(&engineLock);
    }
    if (app.exit)
      PostMessage(hwnd, WM_CLOSE, 0, 0);
    draw(canvas, app);
    InvalidateRect(hwnd, nullptr, FALSE);
    return 0;
  case WM_PAINT: {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(hwnd, &ps);
    RECT area;
    GetClientRect(hwnd, &area);
    BITMAPINFO info = {};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = 640;
    info.bmiHeader.biHeight = -480;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    StretchDIBits(dc, 0, 0, area.right, area.bottom, 0, 0, 640, 480,
                  canvas.pixels, &info, DIB_RGB_COLORS, SRCCOPY);
    EndPaint(hwnd, &ps);
    return 0;
  }
  case WM_APP + 1:
    app.say("AUDIO OUTPUT ERROR / RESTART MOSS");
    return 0;
  case WM_CLOSE:
    if (app.captureBusy()) {
      app.send({Op::CaptureStop});
      app.say("SAVING CAPTURE / CLOSE AGAIN WHEN SAVED");
      return 0;
    }
    if (app.dirty && !app.save())
      return 0;
    DestroyWindow(hwnd);
    return 0;
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  default:
    return DefWindowProc(hwnd, msg, wp, lp);
  }
}
int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int) {
  char exe[MAX_PATH];
  GetModuleFileNameA(nullptr, exe, sizeof(exe));
  char *slash = strrchr(exe, '\\');
  if (slash)
    *slash = 0;
  SetCurrentDirectoryA(exe);
  _mkdir("projects");
  _mkdir("reports");
  library.init();
  app.library = &library;
  InitializeCriticalSection(&engineLock);
  engine.init(app.project, library.pcm, 44100, Assets);
  app.engine = &engine;
  app.seedProjects();
  WNDCLASSA wc = {};
  wc.lpfnWndProc = proc;
  wc.hInstance = instance;
  wc.lpszClassName = "MOSS040";
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  RegisterClassA(&wc);
  RECT rect = {0, 0, 640, 480};
  AdjustWindowRect(
      &rect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);
  window = CreateWindowA(
      wc.lpszClassName, "MOSS 0.5.0 / Z X C V + arrows / Q E / [ ]",
      WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT,
      CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, nullptr,
      nullptr, instance, nullptr);
  if (!window)
    return 3;
  audioEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  WAVEFORMATEX format = {};
  format.wFormatTag = WAVE_FORMAT_PCM;
  format.nChannels = 2;
  format.nSamplesPerSec = 44100;
  format.wBitsPerSample = 16;
  format.nBlockAlign = 4;
  format.nAvgBytesPerSec = 176400;
  if (waveOutOpen(&audioDevice, WAVE_MAPPER, &format, (DWORD_PTR)audioEvent, 0,
                  CALLBACK_EVENT) != MMSYSERR_NOERROR) {
    MessageBoxA(window, "Could not open audio output", "MOSS", MB_OK);
    return 4;
  }
  for (int i = 0; i < 4; i++) {
    headers[i].lpData = (LPSTR)pcm[i];
    headers[i].dwBufferLength = sizeof(pcm[i]);
    if (waveOutPrepareHeader(audioDevice, &headers[i], sizeof(WAVEHDR)) !=
        MMSYSERR_NOERROR)
      return 5;
  }
  audioThread = CreateThread(nullptr, 0, audioWorker, nullptr, 0, nullptr);
  if (!audioThread)
    return 6;
  ShowWindow(window, SW_SHOW);
  SetTimer(window, 1, 33, nullptr);
  MSG msg;
  while (GetMessage(&msg, nullptr, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  InterlockedExchange(&stopping, 1);
  SetEvent(audioEvent);
  WaitForSingleObject(audioThread, INFINITE);
  waveOutReset(audioDevice);
  for (auto &h : headers)
    waveOutUnprepareHeader(audioDevice, &h, sizeof(h));
  waveOutClose(audioDevice);
  CloseHandle(audioThread);
  CloseHandle(audioEvent);
  DeleteCriticalSection(&engineLock);
  return 0;
}
