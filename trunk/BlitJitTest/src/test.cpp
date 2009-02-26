#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <AsmJit/AsmJitCompiler.h>
#include <BlitJit/BlitJit.h>

#include <SDL.h>
#include <SDL_image.h>

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
    AsmJit::Compiler c;
    BlitJit::Generator gen(&c);

    gen.fillSpan(
      BlitJit::Api::pixelFormats[dId],
      BlitJit::Api::pixelFormats[sId], 
      BlitJit::Api::operations[oId]);
    fn = reinterpret_cast<BlitJit::FillSpanFn>(memmgr.submit(c));

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
    AsmJit::Compiler c;
    BlitJit::Generator gen(&c);

    gen.blitSpan(
      BlitJit::Api::pixelFormats[dId],
      BlitJit::Api::pixelFormats[sId], 
      BlitJit::Api::operations[oId]);
    fn = reinterpret_cast<BlitJit::BlitSpanFn>(memmgr.submit(c));

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

  void MyFillRect(SDL_Surface* surf, int x, int y, int w, int h, BlitJit::UInt32 rgba);
  void MyBlit(SDL_Surface* dst, int x, int y, SDL_Surface* src, BlitJit::UInt32 op);

  void onRender();
  void onPause();
  void onEvent(SDL_Event* ev);

  JitManager jitmgr;
  SDL_Surface* screen;
  SDL_Surface* img[4];
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

static inline BlitJit::UInt32 premultiply(BlitJit::UInt32 x)
{
  BlitJit::UInt32 a = x >> 24;
  BlitJit::UInt32 t = (x & 0xff00ff) * a;
  t = (t + ((t >> 8) & 0xff00ff) + 0x800080) >> 8;
  t &= 0xff00ff;

  x = ((x >> 8) & 0xff) * a;
  x = (x + ((x >> 8) & 0xff) + 0x80);
  x &= 0xff00;
  x |= t | (a << 24);
  return x;
}

static SDL_Surface* LoadIMG(const char* path)
{
  SDL_Surface* t;
  if ((t = IMG_Load(path)) == NULL)
  {
    fprintf(stderr, "Can't load image %s: %s\n", path, SDL_GetError());
    return NULL;
  }
  SDL_Surface* img = SDL_CreateRGBSurface(SDL_SWSURFACE, t->w, t->h, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

  BlitJit::SysInt i;
  BlitJit::UInt32 *dst = (BlitJit::UInt32*)img->pixels;
  BlitJit::UInt32 *src = (BlitJit::UInt32*)t->pixels;
  for (i = 0; i < t->w * t->h; i++)
  {
    dst[i] = premultiply(
      (((src[i] & t->format->Rmask) >> t->format->Rshift) << 16) |
      (((src[i] & t->format->Gmask) >> t->format->Gshift) <<  8) |
      (((src[i] & t->format->Bmask) >> t->format->Bshift) <<  0) |
      (((src[i] & t->format->Amask) >> t->format->Ashift) << 24) );
  }
  SDL_FreeSurface(t);
  return img;
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

  img[0] = LoadIMG("babelfish.png");
  img[1] = LoadIMG("blockdevice.png");
  img[2] = LoadIMG("drop.png");
  img[3] = LoadIMG("kweather.png");

  return 0;
}

void Application::destroy()
{
  SDL_FreeSurface(screen);
  //SDL_FreeSurface(img);
  SDL_Quit();

  screen = NULL;
}

void log(const char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);

  //FILE* f = fopen("./log.txt", "a");
  vfprintf(stderr, fmt, ap);
  //fclose(f);

  va_end(ap);
}

void Application::MyFillRect(SDL_Surface* surf, int x, int y, int w, int h, BlitJit::UInt32 rgba)
{
/*
  int x1 = x;
  int y1 = y;
  int x2 = x + w;
  int y2 = y + h;

  if (x1 < 0) x1 = 0;
  if (y2 < 0) y2 = 0;
  if (x2 >= surf->w) x2 = surf->w-1;
  if (y2 >= surf->h) y2 = surf->h-1;

  if (x1 >= x2 || y1 >= y2) return;

  w = x2 - x1;
  h = y2 - y1;

  BlitJit::FillSpanFn fillSpan = jitmgr.getFillSpan(
    BlitJit::PixelFormat::ARGB32,
    BlitJit::PixelFormat::ARGB32,
    BlitJit::Operation::CombineCopy);

  BlitJit::UInt8* pixels = (BlitJit::UInt8*)surf->pixels;
  BlitJit::SysInt stride = (BlitJit::SysInt)surf->pitch;

  pixels += y * stride + x * 4;

  BlitJit::SysUInt i;
  for (i = 0; i < (BlitJit::SysUInt)h; i++, pixels += stride)
  {
    fillSpan(pixels, rgba, (BlitJit::SysUInt)w);
  }
*/
}

void Application::MyBlit(SDL_Surface* dst, int x, int y, SDL_Surface* src, BlitJit::UInt32 op)
{
  int x1 = x;
  int y1 = y;
  int x2 = x + src->w;
  int y2 = y + src->h;

  if (x1 < 0) x1 = 0;
  if (y1 < 0) y1 = 0;
  if (x2 >= dst->w) x2 = dst->w-1;
  if (y2 >= dst->h) y2 = dst->h-1;

  if (x1 >= x2 || y1 >= y2) return;

  int xoff = x1 - x;
  int yoff = y1 - y;
  int w = x2 - x1;
  int h = y2 - y1;

  BlitJit::BlitSpanFn blitSpan = jitmgr.getBlitSpan(
    BlitJit::PixelFormat::ARGB32,
    BlitJit::PixelFormat::ARGB32,
    op);

  BlitJit::UInt8* dstPixels = (BlitJit::UInt8*)dst->pixels;
  BlitJit::UInt8* srcPixels = (BlitJit::UInt8*)src->pixels;
  BlitJit::SysInt dstStride = (BlitJit::SysInt)dst->pitch;
  BlitJit::SysInt srcStride = (BlitJit::SysInt)src->pitch;

  dstPixels += y1 * dstStride + x1 * 4;
  srcPixels += yoff * srcStride + xoff * 4;

  BlitJit::SysUInt i;
  for (i = 0; i < (BlitJit::SysUInt)h; i++, dstPixels += dstStride, srcPixels += srcStride)
  {
    blitSpan(dstPixels, srcPixels, (BlitJit::SysUInt)w);
  }
}

void Application::onRender()
{
  SDL_LockSurface(screen);
  SDL_LockSurface(img[0]);
  SDL_LockSurface(img[1]);
  SDL_LockSurface(img[2]);
  SDL_LockSurface(img[3]);

  //DWORD t = GetTickCount();

  int w = screen->w;
  int h = screen->h;

/*
  for (BlitJit::SysUInt i = 0; i < 100; i++)
  {
    MyFillRect(screen, rand() % (w - 200), rand() % (h - 200), 200, 200, (rand() << 16) | rand());
  }
*/

  //img->format->Amask = 0;
  //img->format->Ashift = 0;
  //img->format->Aloss = 0;

  {
    int x = 0, y = 0;
    memset(screen->pixels, 255, screen->h * screen->pitch);
    for (BlitJit::SysUInt i = 0; i < BlitJit::Operation::Count; i++)
    {
      SDL_Surface* s = SDL_ConvertSurface(img[1], img[0]->format, 0);
      MyBlit(s, 0, 0, img[0], i);
      MyBlit(screen, x * 150, y * 130, s, BlitJit::Operation::CompositeOver);
      SDL_FreeSurface(s);

      if (++x == 4) { x = 0; y++; }
    }

    //fprintf(stderr, "Used %d\n", (int)jitmgr.memmgr.used());
  }

  for (BlitJit::SysUInt i = 0; i < 0; i++)
  {
    SDL_Surface* s = img[rand() % 4];

    //SDL_Rect srcrect = { 0, 0, s->w, s->h };
    //SDL_Rect dstrect = { rand() % (w - s->w), rand() % (h - s->h), s->w, s->h };
    //SDL_LowerBlit(s, &srcrect, screen, &dstrect);

    MyBlit(screen, rand() % (w - s->w), rand() % (h - s->h), s, BlitJit::Operation::CompositeOver);
  }

  //t = GetTickCount() - t;
  //log("Ticks: %u\n", t);

  SDL_UnlockSurface(img[3]);
  SDL_UnlockSurface(img[2]);
  SDL_UnlockSurface(img[1]);
  SDL_UnlockSurface(img[0]);
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

int main(int argc, char* argv[])
//int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  Application app;
  return app.run(640, 540, 32);
}
