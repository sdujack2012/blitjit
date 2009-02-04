#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <AsmJit/AsmJit.h>
#include <BlitJit/BlitJit.h>

#include <SDL.h>

struct JitManager
{
  JitManager();
  ~JitManager();

  BlitJit::FillSpanFn getFillSpan(
    BlitJit::UInt32 dId,
    BlitJit::UInt32 sId,
    BlitJit::UInt32 oId);

  BlitJit::BlitSpanFn getBlitSpan(
    BlitJit::UInt32 dId,
    BlitJit::UInt32 sId,
    BlitJit::UInt32 oId);

  // Memory manager
  BlitJit::MemoryManager memmgr;

  // Functions
  enum
  {
    FillSpanFnCount = 
      BlitJit::PixelFormat::Count * 
      BlitJit::PixelFormat::Count * 
      BlitJit::Operation::Count,

    BlitSpanFnCount = 
      BlitJit::PixelFormat::Count * 
      BlitJit::PixelFormat::Count * 
      BlitJit::Operation::Count 
  };

  BlitJit::FillSpanFn fillSpan[BlitSpanFnCount];
  BlitJit::BlitSpanFn blitSpan[BlitSpanFnCount];
};

JitManager::JitManager()
{
  memset(fillSpan, 0, sizeof(fillSpan));
  memset(blitSpan, 0, sizeof(blitSpan));
}

JitManager::~JitManager()
{
}

BlitJit::FillSpanFn JitManager::getFillSpan(
  BlitJit::UInt32 dId,
  BlitJit::UInt32 sId,
  BlitJit::UInt32 oId)
{
  BlitJit::SysUInt pos = (
    dId * BlitJit::Operation::Count * BlitJit::PixelFormat::Count +
    sId * BlitJit::Operation::Count +
    oId
  );

  // TODO: Threads
  BlitJit::FillSpanFn fn = fillSpan[pos];
  if (fn) return fn;

  {
    AsmJit::X86 a;
    BlitJit::Generator gen(a);

    gen.fillSpan(
      BlitJit::Api::pixelFormats[dId],
      BlitJit::Api::pixelFormats[sId], 
      BlitJit::Api::operations[oId]);
    fn = reinterpret_cast<BlitJit::FillSpanFn>
      (memmgr.submit(a.pData, a.codeSize()));

    fillSpan[pos] = fn;
    return fn;
  }
}

BlitJit::BlitSpanFn JitManager::getBlitSpan(
  BlitJit::UInt32 dId,
  BlitJit::UInt32 sId,
  BlitJit::UInt32 oId)
{
  BlitJit::SysUInt pos = (
    dId * BlitJit::Operation::Count * BlitJit::PixelFormat::Count +
    sId * BlitJit::Operation::Count +
    oId
  );

  // TODO: Threads
  BlitJit::BlitSpanFn fn = blitSpan[pos];
  if (fn) return fn;

  {
    AsmJit::X86 a;
    BlitJit::Generator gen(a);

    gen.blitSpan(
      BlitJit::Api::pixelFormats[dId],
      BlitJit::Api::pixelFormats[sId], 
      BlitJit::Api::operations[oId]);
    fn = reinterpret_cast<BlitJit::BlitSpanFn>
      (memmgr.submit(a.pData, a.codeSize()));

    blitSpan[pos] = fn;
    return fn;
  }
}

struct Application
{
  Application();
  ~Application();

  int run(int width, int height, int depth);
  int init(int width, int height, int depth);
  void destroy();

  void onRender();
  void onPause();
  void onEvent(SDL_Event* ev);

  JitManager jitmgr;
  SDL_Surface* screen;
  bool running;
};

Application::Application() :
  screen(NULL),
  running(false)
{
}

Application::~Application()
{
}

int Application::run(int width, int height, int depth)
{
  int i = init(width, height, depth);
  if (i) return i;

  running = true;

  while(running)
  {
    SDL_Event ev;
    while(SDL_PollEvent(&ev)) onEvent(&ev);

    onRender();
    onPause();
  }

  destroy();
  return 0;
}

int Application::init(int width, int height, int depth)
{
  if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
  {
    fprintf(stderr, "SDL_Init() failed: %s\n", SDL_GetError());
    return 1;
  }

  if ((screen = SDL_SetVideoMode(width, height, depth, SDL_HWSURFACE | SDL_DOUBLEBUF)) == NULL)
  {
    fprintf(stderr, "SDL_SetVideoMode() failed: %s\n", SDL_GetError());
    return 1;
  }

  return 0;
}

void Application::destroy()
{
  SDL_FreeSurface(screen);
  SDL_Quit();

  screen = NULL;
}

void log(const char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);

  FILE* f = fopen("./log.txt", "a");
  vfprintf(f, fmt, ap);
  fclose(f);

  va_end(ap);
}

void Application::onRender()
{
  BlitJit::FillSpanFn fillSpan = jitmgr.getFillSpan(
    BlitJit::PixelFormat::ARGB32,
    BlitJit::PixelFormat::ARGB32,
    BlitJit::Operation::CombineCopy);

  BlitJit::UInt8* pixels = (BlitJit::UInt8*)screen->pixels;
  BlitJit::SysInt stride = (BlitJit::SysInt)screen->pitch;
  BlitJit::SysUInt width = (BlitJit::SysUInt)screen->w;
  BlitJit::SysUInt height = (BlitJit::SysUInt)screen->h;

  static int j = 0;
  j = (j + 1) % 512;

  BlitJit::UInt32 rgba;
  if (j < 256)
    rgba = 0xFF000000 | ((j) << 16) | ((j) << 8) | (j);
  else
    rgba = 0xFF000000 | ((511 - j) << 16) | ((511 - j) << 8) | (511 - j);

  DWORD t = GetTickCount();

  BlitJit::SysUInt i;
  BlitJit::SysUInt z; 

  // For benchmarking
  for (z = 0; z < 100; z++)
  {
    for (i = 0; i < height; i++)
    {
      fillSpan(pixels + i * stride, rgba, width);
    }
  }

  t = GetTickCount() - t;
  log("Ticks: %u\n", t);

  SDL_LockSurface(screen);
  SDL_UnlockSurface(screen);
  SDL_Flip(screen);
}

void Application::onPause()
{
  SDL_Delay(10);
}

void Application::onEvent(SDL_Event* ev)
{
  switch (ev->type)
  {
    case SDL_QUIT:
      running = false;
      break;
  }
}

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  Application app;
  return app.run(640, 480, 32);
}
