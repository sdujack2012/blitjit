cmake_minimum_required(VERSION 2.6)

Project(blittest CXX)

Set(ASMJIT_DIR "../../AsmJit")

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

Set(BLITJIT_DIR "..")

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

Add_Executable(blittest blittest.cpp ${ASMJIT_SOURCES} ${ASMJIT_HEADERS} ${BLITJIT_SOURCES} ${BLITJIT_HEADERS})
