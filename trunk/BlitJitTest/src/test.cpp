#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <AsmJit/AsmJitCompiler.h>
#include <BlitJit/BlitJit.h>

#include <SDL.h>
#include <SDL_image.h>

#if ASMJIT_OS == ASMJIT_WINDOWS
#include <windows.h>
#endif // ASMJIT_WINDOWS

#if ASMJIT_OS == ASMJIT_POSIX
#include <sys/time.h>
#endif // ASMJIT_POSIX

typedef unsigned char DATA8;
typedef unsigned short DATA16;
typedef unsigned int DATA32;

static BlitJit::CodeManager codemgr;

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
  unsigned long x, y;

  for (y = 0; y != (unsigned long)_h; y++)
  {
    DATA8* p = scanline(y);

    // unrolled loop is always faster (this is basic step for optimization)
    for (x = _w >> 2; x; x--, p += 16)
    {
      ((DATA32 *)p)[0] = c;
      ((DATA32 *)p)[1] = c;
      ((DATA32 *)p)[2] = c;
      ((DATA32 *)p)[3] = c;
    }

    for (x = _w & 3; x; x--, p += 4)
    {
      ((DATA32 *)p)[0] = c;
    }
  }
}

void AbstractImage::fill(int x, int y, int w, int h, DATA32 rgba, DATA32 op)
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

  BlitJit::FillSpanFn fillSpan = codemgr.getFillSpan(
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
  //printf("BLIT\n");
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

#if ASMJIT_OS == ASMJIT_WINDOWS

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






// #define USE_CAIRO

#if defined(USE_CAIRO)
#include <cairo.h>

cairo_surface_t* CairoFromSdl(SDL_Surface* s)
{
  return cairo_image_surface_create_for_data(
    (unsigned char*)s->pixels,
    CAIRO_FORMAT_ARGB32,
    s->w,
    s->h,
    s->pitch);
}
#endif // USE_CAIRO








struct BenchmarkIt
{
  BenchmarkIt() : t(0) {}
  ~BenchmarkIt() {}

  BlitJit::UInt32 start() { t = getTicks()    ; return t; }
  BlitJit::UInt32 delta() { t = getTicks() - t; return t; }

  static AsmJit::UInt32 getTicks()
  {
#if ASMJIT_OS == ASMJIT_WINDOWS
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
  if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
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

  //FILE* f = fopen("./log.txt", "a");
  vfprintf(stderr, fmt, ap);
  //fclose(f);

  va_end(ap);
}

void Application::onRender()
{
  screen->lock();

  BenchmarkIt benchmark;
  benchmark.start();

  int w = screen->w();
  int h = screen->h();

/*
  for (BlitJit::SysUInt i = 0; i < 100; i++)
  {
    MyFillRect(screen, rand() % (w - 200), rand() % (h - 200), 200, 200, (rand() << 16) | rand());
  }
*/

#if 1
  {
    int x = 0, y = 0;
    screen->clear(0x00000000);
    for (BlitJit::SysUInt i = 0; i < BlitJit::Operation::Count; i++)
    {
      SdlImage* s = new SdlImage(128, 128);

      s->blit(0, 0, img[0], BlitJit::Operation::CompositeSrc);
      s->blit(0, 0, img[2], i);
      screen->blit(x * 150, y * 130, s, BlitJit::Operation::CompositeSrc);
      delete s;

      if (++x == 6) { x = 0; y++; }
    }

    //fprintf(stderr, "Used %d\n", (int)codemgr.memmgr().used());
  }
#endif

#if 0
  for (BlitJit::SysUInt i = 0; i < 10000; i++)
  {
    SDL_Surface* s = img[i % 4];

    //s->format->Amask = 0;
    //SDL_Rect srcrect = { 0, 0, s->w, s->h };
    //SDL_Rect dstrect = { rand() % (w - s->w), rand() % (h - s->h), s->w, s->h };
    //SDL_LowerBlit(s, &srcrect, screen, &dstrect);

    MyBlit(screen, rand() % (w - s->w), rand() % (h - s->h), s, BlitJit::Operation::CompositeOver);
  }
#endif

#if 0 && defined (USE_CAIRO)
  cairo_surface_t *c_screen = CairoFromSdl(screen);
  cairo_surface_t *c_img[4];

  c_img[0] = CairoFromSdl(img[0]);
  c_img[1] = CairoFromSdl(img[1]);
  c_img[2] = CairoFromSdl(img[2]);
  c_img[3] = CairoFromSdl(img[3]);

  cairo_t *cr = cairo_create(c_screen);

  for (BlitJit::SysUInt i = 0; i < 10000; i++)
  {
    int r = rand() % 4;
    cairo_surface_t* s = c_img[r];
    cairo_set_source_surface(cr, s, rand() % (w - img[r]->w), rand() % (h - img[r]->h));
    cairo_paint(cr);
  }

  cairo_destroy(cr);
  cairo_surface_destroy(c_screen);
  cairo_surface_destroy(c_img[0]);
  cairo_surface_destroy(c_img[1]);
  cairo_surface_destroy(c_img[2]);
  cairo_surface_destroy(c_img[3]);

#endif // USE_CAIRO

  log("Ticks: %u\n", benchmark.delta());

  screen->unlock();
  SDL_Flip(screen->surface());
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

#undef main
int main(int argc, char* argv[])
//int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  Application app;
  return app.run(940, 540, 32);
}
