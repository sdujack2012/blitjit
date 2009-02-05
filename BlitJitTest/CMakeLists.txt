cmake_minimum_required(VERSION 2.6)

Project(blittest C CXX)

Set(ASMJIT_DIR "../AsmJit")
Set(BLITJIT_DIR "../BlitJit")

Set(ASMJIT_SOURCES
  ${ASMJIT_DIR}/AsmJit/AsmJitCpuInfo.cpp
  ${ASMJIT_DIR}/AsmJit/AsmJitVM.cpp
  ${ASMJIT_DIR}/AsmJit/AsmJitX86.cpp
)

Set(ASMJIT_HEADERS
  ${ASMJIT_DIR}/AsmJit/AsmJit.h
  ${ASMJIT_DIR}/AsmJit/AsmJitConfig.h
  ${ASMJIT_DIR}/AsmJit/AsmJitCpuInfo.h
  ${ASMJIT_DIR}/AsmJit/AsmJitVM.h
  ${ASMJIT_DIR}/AsmJit/AsmJitX86.h
)

Set(BLITJIT_SOURCES
  ${BLITJIT_DIR}/BlitJit/BlitJitApi.cpp
  ${BLITJIT_DIR}/BlitJit/BlitJitDefs.cpp
  ${BLITJIT_DIR}/BlitJit/BlitJitGenerator.cpp
  ${BLITJIT_DIR}/BlitJit/BlitJitLock.cpp
  ${BLITJIT_DIR}/BlitJit/BlitJitMemoryManager.cpp
)

Set(BLITJIT_HEADERS
  ${BLITJIT_DIR}/BlitJit/BlitJit.h
  ${BLITJIT_DIR}/BlitJit/BlitJitApi.h
  ${BLITJIT_DIR}/BlitJit/BlitJitConfig.h
  ${BLITJIT_DIR}/BlitJit/BlitJitDefs.h
  ${BLITJIT_DIR}/BlitJit/BlitJitGenerator.h
  ${BLITJIT_DIR}/BlitJit/BlitJitLock.h
  ${BLITJIT_DIR}/BlitJit/BlitJitMemoryManager.h
)

Include_Directories(${ASMJIT_DIR})
Include_Directories(${BLITJIT_DIR})

Find_Package(SDL REQUIRED)
If(NOT SDL_FOUND)
  Message(FATAL "SDL Not Found")
EndIf(NOT SDL_FOUND)
Link_Libraries (${SDL_LIBRARY})
Include_Directories(${SDL_INCLUDE_DIR})

Add_Executable(test WIN32 src/test.cpp ${ASMJIT_SOURCES} ${ASMJIT_HEADERS} ${BLITJIT_SOURCES} ${BLITJIT_HEADERS})
