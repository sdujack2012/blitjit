// Config
// #define USE_CAIRO 1

// [Includes]
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <BlitJit/BlitJit.h>

#include <SDL.h>
#include <SDL_image.h>

#if defined(USE_CAIRO)
#include <cairo.h>
#endif // USE_CAIRO

#if defined(ASMJIT_WINDOWS)
#include <windows.h>
#endif // ASMJIT_WINDOWS

#if defined(ASMJIT_POSIX)
#include <sys/time.h>
#endif // ASMJIT_POSIX

// [Typedefs]
typedef unsigned char DATA8;
typedef unsigned short DATA16;
typedef unsigned int DATA32;











struct BLITJIT_API CodeManager
{
  CodeManager();
  ~CodeManager();

  BlitJit::FillSpanFn getFillSpan(DATA32 dId, DATA32 sId, DATA32 oId);
  BlitJit::FillRectFn getFillRect(DATA32 dId, DATA32 sId, DATA32 oId);

  BlitJit::BlitSpanFn getBlitSpan(DATA32 dId, DATA32 sId, DATA32 oId);
  BlitJit::BlitRectFn getBlitRect(DATA32 dId, DATA32 sId, DATA32 oId);

  // Functions
  enum
  {
    _Cnt = BlitJit::PixelFormat::Count * 
           BlitJit::PixelFormat::Count * 
           BlitJit::Operator::Count,

    FillSpanFnCount = _Cnt,
    FillRectFnCount = _Cnt,

    BlitSpanFnCount = _Cnt,
    BlitRectFnCount = _Cnt,
  };

  // Cache
  BlitJit::FillSpanFn _fillSpan[FillSpanFnCount];
  BlitJit::FillRectFn _fillRect[FillSpanFnCount];

  BlitJit::BlitSpanFn _blitSpan[BlitSpanFnCount];
  BlitJit::BlitRectFn _blitRect[BlitRectFnCount];
};

static inline DATA32 calcPfPfOp(DATA32 dId, DATA32 sId, DATA32 oId)
{
  return (dId * BlitJit::Operator::Count * BlitJit::PixelFormat::Count +
          sId * BlitJit::Operator::Count +
          oId);
}

CodeManager::CodeManager()
{
  memset(_fillSpan, 0, sizeof(_fillSpan));
  memset(_blitSpan, 0, sizeof(_blitSpan));
  memset(_fillRect, 0, sizeof(_fillRect));
  memset(_blitRect, 0, sizeof(_blitRect));
}

CodeManager::~CodeManager()
{
}

BlitJit::FillSpanFn CodeManager::getFillSpan(DATA32 dId, DATA32 sId, DATA32 oId)
{
  DATA32 pos = calcPfPfOp(dId, sId, oId);
  BlitJit::FillSpanFn fn;

  if ((fn = _fillSpan[pos]) != NULL)
  {
    return fn;
  }
  else
  {
    _fillSpan[pos] = fn = BlitJit::Api::genFillSpan(
      &BlitJit::Api::pixelFormats[dId],
      &BlitJit::Api::pixelFormats[sId],
      &BlitJit::Api::operators[oId]);
    return fn;
  }
}

BlitJit::FillRectFn CodeManager::getFillRect(DATA32 dId, DATA32 sId, DATA32 oId)
{
  DATA32 pos = calcPfPfOp(dId, sId, oId);
  BlitJit::FillRectFn fn;
  
  if ((fn = _fillRect[pos]) != NULL)
  {
    return fn;
  }
  else
  {
    _fillRect[pos] = fn = BlitJit::Api::genFillRect(
      &BlitJit::Api::pixelFormats[dId],
      &BlitJit::Api::pixelFormats[sId],
      &BlitJit::Api::operators[oId]);
    return fn;
  }
}

BlitJit::BlitSpanFn CodeManager::getBlitSpan(DATA32 dId, DATA32 sId, DATA32 oId)
{
  DATA32 pos = calcPfPfOp(dId, sId, oId);
  BlitJit::BlitSpanFn fn;

  if ((fn = _blitSpan[pos]) != NULL)
  {
    return fn;
  }
  else
  {
    _blitSpan[pos] = fn = BlitJit::Api::genBlitSpan(
      &BlitJit::Api::pixelFormats[dId],
      &BlitJit::Api::pixelFormats[sId],
      &BlitJit::Api::operators[oId]);
    return fn;
  }
}

BlitJit::BlitRectFn CodeManager::getBlitRect(DATA32 dId, DATA32 sId, DATA32 oId)
{
  DATA32 pos = calcPfPfOp(dId, sId, oId);
  BlitJit::BlitRectFn fn;
  
  if ((fn = _blitRect[pos]) != NULL)
  {
    return fn;
  }
  else
  {
    _blitRect[pos] = fn = BlitJit::Api::genBlitRect(
      &BlitJit::Api::pixelFormats[dId],
      &BlitJit::Api::pixelFormats[sId],
      &BlitJit::Api::operators[oId]);
    return fn;
  }
}
























static CodeManager codemgr;

static inline DATA32 premultiply(DATA32 x)
{
  DATA32 a = x >> 24;
  DATA32 t = (x & 0x00FF00FF) * a;
  t = (t + ((t >> 8) & 0x00FF00FF) + 0x00800080) >> 8;
  t &= 0x00FF00FF;

  x = ((x >> 8) & 0x000000FF) * a;
  x = (x + ((x >> 8) & 0x000000FF) + 0x00000080);
  x &= 0x0000FF00;
  x |= t | (a << 24);
  return x;
}

static inline DATA32 demultiply(DATA32 x)
{
  DATA32 a = x >> 24;

  return (a) 
    ? (((((x & 0x00FF0000) >> 16) * 255) / a) << 16) |
      (((((x & 0x0000FF00) >>  8) * 255) / a) <<  8) |
      (((((x & 0x000000FF)      ) * 255) / a)      ) |
      (a << 24)
    : 0;
}

struct AbstractImage
{
  AbstractImage();
  virtual ~AbstractImage() = 0;

  virtual bool create(int w, int h) = 0;
  virtual void destroy() = 0;
  virtual bool resize(int w, int h) = 0;

  inline int w() const { return _w; }
  inline int h() const { return _h; }
  inline BlitJit::SysInt stride() const { return _stride; }
  inline int format() const { return _format; }

  inline bool valid() const { return _pixels != NULL; }

  inline DATA8* pixels() { return (DATA8 *)_pixels; }
  inline DATA16* pixels16() { return (DATA16 *)_pixels; }
  inline DATA32* pixels32() { return (DATA32 *)_pixels; }

  inline DATA8* scanline() { return (DATA8 *)_scanline0; }
  inline DATA8* scanline(int y) { return (DATA8 *)_scanline0 + y * _stride; }

  inline DATA16* scanline16() { return (DATA16 *)scanline(); }
  inline DATA16* scanline16(int y) { return (DATA16 *)scanline(y); }

  inline DATA32* scanline32() { return (DATA32 *)scanline(); }
  inline DATA32* scanline32(int y) { return (DATA32 *)scanline(y); }

  void clear(DATA32 c);
  void fill(int x, int y, int w, int h, DATA32 rgba, DATA32 op);
  void blit(int x, int y, AbstractImage* img, DATA32 op);

#if defined(USE_CAIRO)
  cairo_surface_t* as_cairo()
  {
    return cairo_image_surface_create_for_data(
      (unsigned char*)_pixels,
      CAIRO_FORMAT_ARGB32,
      _w,
      _h,
      _stride);
  }
#endif // USE_CAIRO

protected:
  int _w;
  int _h;
  BlitJit::SysInt _stride;
  int _format;
  DATA8* _pixels;
  DATA8* _scanline0;
};

AbstractImage::AbstractImage() : 
  _w(0), _h(0),
  _stride(0),
  _format(0),
  _pixels(NULL),
  _scanline0(NULL) 
{
}

AbstractImage::~AbstractImage()
{
}

void AbstractImage::clear(DATA32 c)
{
  fill(0, 0, _w, _h, c, BlitJit::Operator::CompositeSrc);
}

void AbstractImage::fill(int x, int y, int w, int h, DATA32 rgba, DATA32 op)
{
  int x1 = x;
  int y1 = y;
  int x2 = x + w;
  int y2 = y + h;

  if (x1 < 0) x1 = 0;
  if (y2 < 0) y2 = 0;
  if (x2 >= _w) x2 = _w-1;
  if (y2 >= _h) y2 = _h-1;

  if (x1 >= x2 || y1 >= y2) return;

  int fillw = x2 - x1;
  int fillh = y2 - y1;

  BlitJit::UInt8* dstPixels = (BlitJit::UInt8*)scanline();
  BlitJit::SysInt dstStride = (BlitJit::SysInt)stride();

  dstPixels += y * dstStride + x * 4;

  BlitJit::FillRectFn fillRect = codemgr.getFillRect(
    BlitJit::PixelFormat::ARGB32,
    BlitJit::PixelFormat::ARGB32,
    op);
  fillRect(dstPixels, &rgba, dstStride, (BlitJit::SysUInt)fillw, (BlitJit::SysUInt)fillh);

/*
  BlitJit::FillSpanFn fillSpan = codemgr.getFillSpan(
    BlitJit::PixelFormat::ARGB32,
    BlitJit::PixelFormat::ARGB32,
    op);

  BlitJit::SysUInt i;
  for (i = 0; i < (BlitJit::SysUInt)fillh; i++, dstPixels += dstStride)
  {
    fillSpan(dstPixels, &rgba, (BlitJit::SysUInt)fillw);
  }
*/
}

void AbstractImage::blit(int x, int y, AbstractImage* img, DATA32 op)
{
  int x1 = x;
  int y1 = y;
  int x2 = x + img->w();
  int y2 = y + img->h();

  if (x1 < 0) x1 = 0;
  if (y1 < 0) y1 = 0;
  if (x2 > _w) x2 = _w;
  if (y2 > _h) y2 = _h;

  if (x1 >= x2 || y1 >= y2) return;

  int bltx = x1 - x;
  int blty = y1 - y;
  int bltw = x2 - x1;
  int blth = y2 - y1;

  DATA8* dstPixels = scanline();
  DATA8* srcPixels = img->scanline();

  BlitJit::SysInt dstStride = (BlitJit::SysInt)stride();
  BlitJit::SysInt srcStride = (BlitJit::SysInt)img->stride();

  dstPixels += y1 * dstStride + x1 * 4;
  srcPixels += blty * srcStride + bltx * 4;

  BlitJit::BlitRectFn blitRect = codemgr.getBlitRect(
    BlitJit::PixelFormat::ARGB32,
    BlitJit::PixelFormat::ARGB32,
    op);
  blitRect(dstPixels, srcPixels, dstStride, srcStride, (BlitJit::SysUInt)bltw, (BlitJit::SysUInt)blth);

/*
  BlitJit::BlitSpanFn blitSpan = codemgr.getBlitSpan(
    BlitJit::PixelFormat::ARGB32,
    BlitJit::PixelFormat::ARGB32,
    op);

  BlitJit::SysUInt i;
  for (i = 0; i < (BlitJit::SysUInt)h; i++, dstPixels += dstStride, srcPixels += srcStride)
  {
    blitSpan(dstPixels, srcPixels, (BlitJit::SysUInt)w);
  }
*/
}

#if defined(ASMJIT_WINDOWS)

struct DibImage : AbstractImage
{
  DibImage();
  DibImage(int w, int h);
  virtual ~DibImage();

  virtual bool create(int w, int h);
  virtual void destroy();
  virtual bool resize(int w, int h);

  inline HBITMAP hbitmap() const { return _hbitmap; }

  HDC getDc();
  void releaseDc();

protected:
  HBITMAP _hbitmap;
  HDC _hdc;
};

DibImage::DibImage() : 
  AbstractImage(),
  _hbitmap(NULL),
  _hdc(NULL)
{
}

DibImage::DibImage(int w, int h) :
  AbstractImage(),
  _hbitmap(NULL),
  _hdc(NULL)
{
  create(w, h);
}

DibImage::~DibImage()
{
  destroy();
}

bool DibImage::create(int w, int h)
{
  if (valid()) destroy();
  if (w <= 0 || h <= 0) return false;

  HDC hdc = CreateCompatibleDC(NULL);
  HGDIOBJ hOldObject;
  if (hdc == NULL) return false;

  // create bitmap information
  BITMAPINFO bmi;
  ZeroMemory(&bmi, sizeof(bmi));
  bmi.bmiHeader.biSize        = sizeof(bmi.bmiHeader);
  bmi.bmiHeader.biWidth       = w;
  // negative means from top to bottom
  bmi.bmiHeader.biHeight      = -h;
  bmi.bmiHeader.biPlanes      = 1;
  bmi.bmiHeader.biBitCount    = 32;
  bmi.bmiHeader.biCompression = BI_RGB;

  _hbitmap = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, (void**)&_pixels, NULL, 0);
  if (_hbitmap == NULL)
  {
    DeleteDC(hdc);
    return false;
  }

  // select bitmap to DC
  hOldObject = SelectObject(hdc, _hbitmap);
  if (hOldObject == 0)
  {
    DeleteObject((HGDIOBJ)_hbitmap);
    DeleteDC(hdc);

    _hbitmap = 0;
    return false;
  }

  // we need stride, windows should use 32 bit alignment, but be safe
  DIBSECTION info;
  GetObject(_hbitmap, sizeof(DIBSECTION), &info);

  _w = w;
  _h = h;
  _stride = info.dsBm.bmWidthBytes;
  _format = 0;
  _scanline0 = _pixels;

  _hdc = hdc;
  return true;
}

void DibImage::destroy()
{
  if (valid())
  {
    releaseDc();
    DeleteObject(_hbitmap);
    _w = 0;
    _h = 0;
    _stride = 0;
    _format = 0;
    _pixels = NULL; 
    _scanline0 = NULL;
    _hbitmap = NULL;
  }
}

bool DibImage::resize(int w, int h)
{
  if (_w != w || _h != h)
    return create(w, h);
  else
    return _w >= 0 && _h >= 0;
}

HDC DibImage::getDc()
{
  if (!_hdc) 
  {
    _hdc = CreateCompatibleDC(NULL);
    SelectObject(_hdc, _hbitmap);
  }

  return _hdc;
}

void DibImage::releaseDc()
{
  if (_hdc)
  {
    DeleteDC(_hdc);
    _hdc = NULL;
  }
}

#endif // ASMJIT_WINDOWS

struct SdlImage : AbstractImage
{
  SdlImage();
  SdlImage(int w, int h);
  virtual ~SdlImage();

  bool adopt(SDL_Surface* surface);

  virtual bool create(int w, int h);
  virtual void destroy();
  virtual bool resize(int w, int h);

  inline SDL_Surface* surface() const { return _surface; }

  inline void setAdopted(bool adopted) { _adopted = adopted; }

  void lock();
  void unlock();

protected:
  SDL_Surface* _surface;
  bool _adopted;
};

SdlImage::SdlImage() : 
  AbstractImage(),
  _surface(NULL)
{
}

SdlImage::SdlImage(int w, int h) :
  AbstractImage(),
  _surface(NULL)
{
  create(w, h);
}

SdlImage::~SdlImage()
{
  destroy();
}

bool SdlImage::adopt(SDL_Surface* surface)
{
  if (_surface) destroy();

  _w = surface->w;
  _h = surface->h;
  _stride = surface->pitch;
  _format = 0;
  _pixels = (DATA8 *)surface->pixels;
  _scanline0 = _pixels;

  _surface = surface;
  _adopted = true;

  return true;
}

bool SdlImage::create(int w, int h)
{
  if (_surface) destroy();
  if (w <= 0 || h <= 0) return false;

  _surface = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
  if (!_surface) return false;

  _w = _surface->w;
  _h = _surface->h;
  _stride = _surface->pitch;
  _format = 0;
  _pixels = (DATA8 *)_surface->pixels;
  _scanline0 = _pixels;

  _adopted = false;

  return true;
}

void SdlImage::destroy()
{
  if (_surface)
  {
    if (!_adopted) SDL_FreeSurface(_surface);

    _w = 0;
    _h = 0;
    _stride = 0;
    _format = 0;
    _pixels = NULL; 
    _scanline0 = NULL;

    _surface = NULL;
    _adopted = false;
  }
}

bool SdlImage::resize(int w, int h)
{
  if (_w != w || _h != h)
    return create(w, h);
  else
    return _w >= 0 && _h >= 0;
}

void SdlImage::lock()
{
  SDL_LockSurface(_surface);

  _stride = _surface->pitch;
  _pixels = (DATA8*)_surface->pixels;
  _scanline0 = _pixels;
}

void SdlImage::unlock()
{
  SDL_UnlockSurface(_surface);

  _stride = _surface->pitch;
  _pixels = (DATA8*)_surface->pixels;
  _scanline0 = _pixels;
}



static SdlImage* LoadIMG(const char* path)
{
  SDL_Surface* tmp;
  {
    char buffer[1024];
    if ((tmp = IMG_Load(path)) != NULL) goto ok;

    sprintf(buffer, "../%s", path);
    if ((tmp = IMG_Load(buffer)) != NULL) goto ok;

    sprintf(buffer, "../../%s", path);
    if ((tmp = IMG_Load(buffer)) != NULL) goto ok;

    fprintf(stderr, "Can't load image %s: %s\n", path, SDL_GetError());
    exit(1);
    return NULL;
  }
ok:

  SdlImage* img = new SdlImage(tmp->w, tmp->h);

  BlitJit::SysInt i;
  BlitJit::UInt32 *dst = (BlitJit::UInt32*)img->scanline();
  BlitJit::UInt32 *src = (BlitJit::UInt32*)tmp->pixels;
  for (i = 0; i < tmp->w * tmp->h; i++)
  {
    dst[i] = premultiply(
      (((src[i] & tmp->format->Rmask) >> tmp->format->Rshift) << 16) |
      (((src[i] & tmp->format->Gmask) >> tmp->format->Gshift) <<  8) |
      (((src[i] & tmp->format->Bmask) >> tmp->format->Bshift) <<  0) |
      (((src[i] & tmp->format->Amask) >> tmp->format->Ashift) << 24) );
  }
  SDL_FreeSurface(tmp);

  return img;
}














struct BenchmarkIt
{
  BenchmarkIt() : t(0) {}
  ~BenchmarkIt() {}

  BlitJit::UInt32 start() { t = getTicks()    ; return t; }
  BlitJit::UInt32 delta() { t = getTicks() - t; return t; }

  static AsmJit::UInt32 getTicks()
  {
#if defined(ASMJIT_WINDOWS)
    return GetTickCount();
#else
    AsmJit::UInt32 ticks;
    timeval now;

    gettimeofday(&now, NULL);
    ticks = (now.tv_sec) * 1000 + (now.tv_usec) / 1000;
    return ticks;
#endif
  }

  AsmJit::UInt32 t;
};




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

  int test_BlitJit_Blit(int count);
  int test_BlitJit_Blend(int count);

  int test_Sdl_Blit(int count);
  int test_Sdl_Blend(int count);

  int test_Gdi_Blit(int count);
  int test_Gdi_Blend(int count);

#if defined(USE_CAIRO)
  int test_Cairo_Blit(int count);
  int test_Cairo_Blend(int count);
#endif

  SdlImage* screen;
  SdlImage* img[4];

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
  if (SDL_Init(SDL_INIT_VIDEO) < 0)
  {
    fprintf(stderr, "SDL_Init() failed: %s\n", SDL_GetError());
    return 1;
  }

  SDL_Surface* screen_surf;
  if ((screen_surf = SDL_SetVideoMode(width, height, depth, SDL_HWSURFACE | SDL_DOUBLEBUF)) == NULL)
  {
    fprintf(stderr, "SDL_SetVideoMode() failed: %s\n", SDL_GetError());
    return 1;
  }

  screen = new SdlImage();
  screen->adopt(screen_surf);
  screen->setAdopted(false);

  img[0] = LoadIMG("img/babelfish.png");
  img[1] = LoadIMG("img/blockdevice.png");
  img[2] = LoadIMG("img/drop.png");
  img[3] = LoadIMG("img/kweather.png");

  return 0;
}

void Application::destroy()
{
  delete img[0];
  delete img[1];
  delete img[2];
  delete img[3];
  delete screen;

  SDL_Quit();
}

void log(const char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);

  vfprintf(stderr, fmt, ap);

  va_end(ap);
}

void Application::onRender()
{
  screen->lock();

  int count = 10000;

#if 1
  // printf("BlitJit - Copy: %d\n", test_BlitJit_Blit(count));
  // printf("BlitJit - Blend: %d\n", test_BlitJit_Blend(count));

  // printf("Sdl - Copy: %d\n", test_Sdl_Blit(count));
  // printf("Sdl - Blend: %d\n", test_Sdl_Blend(count));

  // printf("Cairo - Copy: %d\n", test_Cairo_Blit(count));
  // printf("Cairo - Blend: %d\n", test_Cairo_Blend(count));

  // printf("Gdi - Copy: %d\n", test_Gdi_Blit(count));
  // printf("Gdi - Blend: %d\n", test_Gdi_Blend(count));
#endif

#if 0
  BenchmarkIt benchmark;
  benchmark.start();
  for (int p = 0; p < 1000; p++)
  {
    for (int a = 0; a < BlitJit::Operator::Count; a++)
    {
      AsmJit::Compiler c;
      BlitJit::Generator gen(&c);

      gen.genFillSpan(
        BlitJit::Api::pixelFormats[BlitJit::PixelFormat::ARGB32],
        BlitJit::Api::pixelFormats[BlitJit::PixelFormat::ARGB32], 
        BlitJit::Api::operations[a]);
      AsmJit::MemoryManager::global()->free(c.make());
    }
  }
  benchmark.delta();
  printf("Time %d, %g per one function \n", 
    benchmark.t,
    (double)benchmark.t / (double)(1000*BlitJit::Operator::Count));
#endif

#if 0
  screen->clear(0xFFFFFFFF);

  for (BlitJit::SysUInt i = 0; i < BlitJit::Operator::Count; i++)
  {
    codemgr.getBlitRect(
      BlitJit::PixelFormat::ARGB32,
      BlitJit::PixelFormat::ARGB32,
      i);
    screen->blit(3, 1, img[0], i);
  }

#endif

#if 1
  /*
  {
    int x = 0, y = 0;
    screen->clear(0xFF000000);
    for (BlitJit::SysUInt i = 0; i < BlitJit::Operator::Count; i++)
    {
      screen->fill(x * 150, y * 130, 128, 128, 0xFFFFFFFF, i);
      if (++x == 6) { x = 0; y++; }
    }
  }
  */

  BenchmarkIt benchmark;
  benchmark.start();

  for (int i = 0; i < count; i++)
  {
    int size = 16;
    screen->fill(
      rand() % (screen->w() - size), rand() % (screen->h() - size),
      size, size,
      rand() | (rand() << 16),
      BlitJit::Operator::CompositeOver);
    /*
    int x = 0, y = 0;
    screen->clear(0xFF000000);
    for (BlitJit::SysUInt i = 0; i < 20; i++)
    {
      screen->fill(
        x * 150, y * 130, 128, 128,
        0x00FFFFFF | ((255 * i / 19) << 24),
        BlitJit::Operator::CompositeOver);
      if (++x == 6) { x = 0; y++; }
    }*/
  }

  printf ("Bench:%u\n", (unsigned int)benchmark.delta());
#endif

#if 0
  int w = screen->w();
  int h = screen->h();

  {
    int x = 0, y = 0;
    screen->clear(0xFF00FF00);
    for (BlitJit::SysUInt i = 0; i < BlitJit::Operator::Count; i++)
    {
      SdlImage* s = new SdlImage(128, 128);

      s->blit(0, 0, img[0], BlitJit::Operator::CompositeSrc);
      s->blit(0, 0, img[1], i);
      screen->blit(x * 150, y * 130, s, BlitJit::Operator::CompositeOver);
      delete s;

      if (++x == 6) { x = 0; y++; }
    }
  }
/*
  fprintf(stderr, "Used %d, Allocated %d\n", 
    AsmJit::MemoryManager::global()->used(),
    AsmJit::MemoryManager::global()->allocated());
*/
#endif

  screen->unlock();
  SDL_Flip(screen->surface());
}

void Application::onPause()
{
  SDL_Delay(1);
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

// Test BlitJit library speed
int Application::test_BlitJit_Blit(int count)
{
  BenchmarkIt benchmark;
  benchmark.start();

  for (int i = 0; i < count; i++)
  {
    AbstractImage* s = img[0];
    screen->blit(
      rand() % (screen->w() - s->w()), rand() % (screen->h() - s->h()),
      s,
      BlitJit::Operator::CompositeSrc);
  }

  return benchmark.delta();
}

int Application::test_BlitJit_Blend(int count)
{
  BenchmarkIt benchmark;
  benchmark.start();

  for (int i = 0; i < count; i++)
  {
    AbstractImage* s = img[0];
    screen->blit(
      rand() % (screen->w() - s->w()), rand() % (screen->h() - s->h()),
      s,
      BlitJit::Operator::CompositeOver);
  }

  return benchmark.delta();
}

// Test SDL library speed
int Application::test_Sdl_Blit(int count)
{
  BenchmarkIt benchmark;
  benchmark.start();

  SDL_Surface* s = SDL_CreateRGBSurfaceFrom(
    img[0]->pixels(),
    img[0]->w(),
    img[0]->h(), 
    32,
    img[0]->stride(),
    0x00FF0000, 0x0000FF00, 0x000000FF, 0);

  for (int i = 0; i < count; i++)
  {
    SDL_Rect srcrect = { 
      0, 0, s->w, s->h
    };
    SDL_Rect dstrect = { 
      rand() % (screen->w() - s->w),
      rand() % (screen->h() - s->h), s->w, s->h 
    };

    SDL_LowerBlit(s, &srcrect, screen->surface(), &dstrect);
  }

  return benchmark.delta();
}

int Application::test_Sdl_Blend(int count)
{
  BenchmarkIt benchmark;
  benchmark.start();

  for (int i = 0; i < count; i++)
  {
    SdlImage* s = img[0];

    SDL_Rect srcrect = { 
      0, 0, s->w(), s->h() 
    };
    SDL_Rect dstrect = { 
      rand() % (screen->w() - s->w()),
      rand() % (screen->h() - s->h()), s->w(), s->h() 
    };

    SDL_LowerBlit(s->surface(), &srcrect, screen->surface(), &dstrect);
  }

  return benchmark.delta();
}

// Test GDI subsystem speed
int Application::test_Gdi_Blit(int count)
{
#if defined(ASMJIT_WINDOWS)
  DibImage* di_screen = new DibImage(screen->w(), screen->h());
  di_screen->blit(0, 0, screen, BlitJit::Operator::CompositeSrc);

  DibImage* di_0 = new DibImage(img[0]->w(), img[0]->h());
  di_0->blit(0, 0, img[0], BlitJit::Operator::CompositeSrc);

  BenchmarkIt benchmark;
  benchmark.start();

  for (int i = 0; i < count; i++)
  {
    DibImage* s = di_0;

    BitBlt(
      di_screen->getDc(),
        rand() % (screen->w() - s->w()),
        rand() % (screen->h() - s->h()), s->w(), s->h(), 
      s->getDc(), 0, 0, SRCCOPY);
  }

  benchmark.delta();

  screen->blit(0, 0, di_screen, BlitJit::Operator::CompositeSrc);

  delete di_screen;
  delete di_0;

  return benchmark.t;
#else
  return 0;
#endif
}

int Application::test_Gdi_Blend(int count)
{
#if defined(ASMJIT_WINDOWS)
  DibImage* di_screen = new DibImage(screen->w(), screen->h());
  di_screen->blit(0, 0, screen, BlitJit::Operator::CompositeSrc);

  DibImage* di_0 = new DibImage(img[0]->w(), img[0]->h());
  di_0->blit(0, 0, img[0], BlitJit::Operator::CompositeSrc);

  BLENDFUNCTION bf;
  bf.BlendOp = AC_SRC_OVER;
  bf.BlendFlags = 0;
  bf.SourceConstantAlpha = 0xFF;
  bf.AlphaFormat = AC_SRC_ALPHA;

  BenchmarkIt benchmark;
  benchmark.start();

  for (int i = 0; i < count; i++)
  {
    DibImage* s = di_0;

    AlphaBlend(
      di_screen->getDc(), 
        rand() % (screen->w() - s->w()), 
        rand() % (screen->h() - s->h()), s->w(), s->h(), 
      s->getDc(), 0, 0, s->w(), s->h(),
      bf);
  }

  benchmark.delta();

  screen->blit(0, 0, di_screen, BlitJit::Operator::CompositeSrc);

  delete di_screen;
  delete di_0;

  return benchmark.t;
#else
  return 0;
#endif
}

#if defined(USE_CAIRO)
int Application::test_Cairo_Blit(int count)
{
  cairo_surface_t *c_screen = screen->as_cairo();
  cairo_surface_t *c_0 = img[0]->as_cairo();

  cairo_t *context = cairo_create(c_screen);
  cairo_set_operator(context, CAIRO_OPERATOR_SOURCE);

  BenchmarkIt benchmark;
  benchmark.start();

  for (int i = 0; i < count; i++)
  {
    cairo_surface_t* s = c_0;
    int x = rand() % (screen->w() - img[0]->w());
    int y = rand() % (screen->h() - img[0]->h());
    cairo_set_source_surface(context, s, x, y);
    cairo_rectangle(context, x, y, img[0]->w(), img[0]->h());
    cairo_fill(context);
  }

  benchmark.delta();

  cairo_destroy(context);
  cairo_surface_destroy(c_0);
  cairo_surface_destroy(c_screen);

  return benchmark.t;
}

int Application::test_Cairo_Blend(int count)
{
  cairo_surface_t *c_screen = screen->as_cairo();
  cairo_surface_t *c_0 = img[0]->as_cairo();

  cairo_t *context = cairo_create(c_screen);

  BenchmarkIt benchmark;
  benchmark.start();

  for (int i = 0; i < count; i++)
  {
    cairo_surface_t* s = c_0;
    cairo_set_source_surface(context, s,
      rand() % (screen->w() - img[0]->w()),
      rand() % (screen->h() - img[0]->h()));
    cairo_paint(context);
  }

  benchmark.delta();

  cairo_destroy(context);
  cairo_surface_destroy(c_0);
  cairo_surface_destroy(c_screen);

  return benchmark.t;
}
#endif

#undef main
int main(int argc, char* argv[])
//int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  Application app;
  return app.run(940, 540, 32);
}
