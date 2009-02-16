#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <AsmJit/AsmJit.h>
#include <BlitJit/BlitJit.h>

char toHex(BlitJit::UInt8 val) { return (val < 10 ? '0' : 'A'-10) + val; }

void dump32(const BlitJit::UInt32* data, BlitJit::SysUInt len, const char* msg)
{
  char* buf = (char*)malloc(len * 9 + 1);
  char* cur = buf;
  if (!buf) return;

  for (BlitJit::SysUInt i = 0; i < len*4; i++)
  {
    if (i && i % 4 == 0) *cur++ = '|';
    *cur++ = toHex(((BlitJit::UInt8*)data)[i] >> 4);
    *cur++ = toHex(((BlitJit::UInt8*)data)[i] & 15);
  }

  *cur = '\0';

  printf("%s:%s\n", msg, buf);
  free(buf);
}

void set32(BlitJit::UInt32* data, BlitJit::UInt32 c, BlitJit::SysUInt len)
{
  for (BlitJit::SysUInt i = 0; i < len; i++) data[i] = c;
}

int main(int argc, char* argv[])
{
  // Memory manager
  BlitJit::MemoryManager memmgr;

  // Function
  BlitJit::BlitSpanFn fn;

  // Generator
  {
    AsmJit::Assembler a;
    BlitJit::Generator gen(a);

    gen.blitSpan(
      BlitJit::Api::pixelFormats[BlitJit::PixelFormat::ARGB32],
      BlitJit::Api::pixelFormats[BlitJit::PixelFormat::ARGB32],
      BlitJit::Api::operations[BlitJit::Operation::CombineCopy]);

    fn = reinterpret_cast<BlitJit::BlitSpanFn>
      (memmgr.submit(a));
  }

  const int Size = 8;
  BlitJit::UInt32 dst[128];
  BlitJit::UInt32 src[128];

  set32(dst, 0x00000000, Size);
  set32(src, 0xFFFFFFFF, Size);

  dump32(dst, Size, "dst");
  dump32(src, Size, "src");
  printf("\n");

  fn(dst, src, Size);

  dump32(dst, Size, "dst");
  dump32(src, Size, "src");
  printf("\n");

  return 0;
}
