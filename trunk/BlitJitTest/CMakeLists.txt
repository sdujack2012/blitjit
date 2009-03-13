cmake_minimum_required(VERSION 2.6)

Project(blittest C CXX)

Set(ASMJIT_DIR "../AsmJit")
Set(BLITJIT_DIR "../BlitJit")

Set(ASMJIT_BASE_SOURCES
  ${ASMJIT_DIR}/AsmJit/AsmJitAssemblerX86X64.cpp
  ${ASMJIT_DIR}/AsmJit/AsmJitCpuInfo.cpp
  ${ASMJIT_DIR}/AsmJit/AsmJitLogger.cpp
  ${ASMJIT_DIR}/AsmJit/AsmJitLoggerX86X64.cpp
  ${ASMJIT_DIR}/AsmJit/AsmJitLoggerX86X64Dump.cpp
  ${ASMJIT_DIR}/AsmJit/AsmJitSerializerX86X64.cpp
  ${ASMJIT_DIR}/AsmJit/AsmJitUtil.cpp
  ${ASMJIT_DIR}/AsmJit/AsmJitVM.cpp
)

Set(ASMJIT_BASE_HEADERS
  ${ASMJIT_DIR}/AsmJit/AsmJit.h
  ${ASMJIT_DIR}/AsmJit/AsmJitAssembler.h
  ${ASMJIT_DIR}/AsmJit/AsmJitAssemblerX86X64.h
  ${ASMJIT_DIR}/AsmJit/AsmJitBuild.h
  ${ASMJIT_DIR}/AsmJit/AsmJitConfig.h
  ${ASMJIT_DIR}/AsmJit/AsmJitCpuInfo.h
  ${ASMJIT_DIR}/AsmJit/AsmJitDefs.h
  ${ASMJIT_DIR}/AsmJit/AsmJitDefsX86X64.h
  ${ASMJIT_DIR}/AsmJit/AsmJitLogger.h
  ${ASMJIT_DIR}/AsmJit/AsmJitLoggerX86X64.h
  ${ASMJIT_DIR}/AsmJit/AsmJitSerializer.h
  ${ASMJIT_DIR}/AsmJit/AsmJitSerializerX86X64.h
  ${ASMJIT_DIR}/AsmJit/AsmJitUtil.h
  ${ASMJIT_DIR}/AsmJit/AsmJitVM.h
)

Set(ASMJIT_COMPILER_SOURCES
  ${ASMJIT_DIR}/AsmJit/AsmJitCompilerX86X64.cpp
)

Set(ASMJIT_COMPILER_HEADERS
  ${ASMJIT_DIR}/AsmJit/AsmJitCompiler.h
  ${ASMJIT_DIR}/AsmJit/AsmJitCompilerX86X64.h
)

Set(BLITJIT_SOURCES
  ${BLITJIT_DIR}/BlitJit/BlitJitApi.cpp
  ${BLITJIT_DIR}/BlitJit/BlitJitCodeManager.cpp
  ${BLITJIT_DIR}/BlitJit/BlitJitDefs.cpp
  ${BLITJIT_DIR}/BlitJit/BlitJitGenerator.cpp
  ${BLITJIT_DIR}/BlitJit/BlitJitLock.cpp
  ${BLITJIT_DIR}/BlitJit/BlitJitMemoryManager.cpp
)

Set(BLITJIT_HEADERS
  ${BLITJIT_DIR}/BlitJit/BlitJit.h
  ${BLITJIT_DIR}/BlitJit/BlitJitApi.h
  ${BLITJIT_DIR}/BlitJit/BlitJitBuild.h
  ${BLITJIT_DIR}/BlitJit/BlitJitCodeManager.h
  ${BLITJIT_DIR}/BlitJit/BlitJitConfig.h
  ${BLITJIT_DIR}/BlitJit/BlitJitDefs.h
  ${BLITJIT_DIR}/BlitJit/BlitJitGenerator.h
  ${BLITJIT_DIR}/BlitJit/BlitJitLock.h
  ${BLITJIT_DIR}/BlitJit/BlitJitMemoryManager.h
)

Include_Directories(${ASMJIT_DIR})
Include_Directories(${BLITJIT_DIR})

If (WIN32)
Link_Libraries(gdi32 msimg32)
EndIf (WIN32)

Find_Package(SDL REQUIRED)
If(NOT SDL_FOUND)
  Message(FATAL "SDL Not Found")
EndIf(NOT SDL_FOUND)
Link_Libraries (${SDL_LIBRARY})
Include_Directories(${SDL_INCLUDE_DIR})

Find_Package(SDL_image REQUIRED)
If(NOT SDLIMAGE_FOUND)
  Message(FATAL "SDL Image Not Found")
EndIf(NOT SDLIMAGE_FOUND)
Link_Libraries (${SDLIMAGE_LIBRARY})
Include_Directories(${SDLIMAGE_INCLUDE_DIR})

If(NOT WIN32)
Link_Libraries(cairo)
Include_Directories(/usr/include/cairo)
EndIf(NOT WIN32)

Add_Executable(test src/test.cpp
  ${ASMJIT_BASE_SOURCES} ${ASMJIT_BASE_HEADERS}
  ${ASMJIT_COMPILER_SOURCES} ${ASMJIT_COMPILER_HEADERS}
  ${BLITJIT_SOURCES} ${BLITJIT_HEADERS})
