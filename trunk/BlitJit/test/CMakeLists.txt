cmake_minimum_required(VERSION 2.6)

Project(blittest CXX)

Set(ASMJIT_DIR "../../AsmJit")


Set(ASMJIT_SOURCES
  ${ASMJIT_DIR}/AsmJit/AsmJitAssemblerX86X64.cpp
  ${ASMJIT_DIR}/AsmJit/AsmJitCompilerX86X64.cpp
  ${ASMJIT_DIR}/AsmJit/AsmJitCpuInfo.cpp
  ${ASMJIT_DIR}/AsmJit/AsmJitSerializerX86X64.cpp
  ${ASMJIT_DIR}/AsmJit/AsmJitUtil.cpp
  ${ASMJIT_DIR}/AsmJit/AsmJitVM.cpp
)

Set(ASMJIT_HEADERS
  ${ASMJIT_DIR}/AsmJit/AsmJit.h
  ${ASMJIT_DIR}/AsmJit/AsmJitAssembler.h
  ${ASMJIT_DIR}/AsmJit/AsmJitAssemblerX86X64.h
  ${ASMJIT_DIR}/AsmJit/AsmJitConfig.h
  ${ASMJIT_DIR}/AsmJit/AsmJitCompiler.h
  ${ASMJIT_DIR}/AsmJit/AsmJitCompilerX86X64.h
  ${ASMJIT_DIR}/AsmJit/AsmJitCpuInfo.h
  ${ASMJIT_DIR}/AsmJit/AsmJitDefs.h
  ${ASMJIT_DIR}/AsmJit/AsmJitDefsX86X64.h
  ${ASMJIT_DIR}/AsmJit/AsmJitSerializer.h
  ${ASMJIT_DIR}/AsmJit/AsmJitSerializerX86X64.h
  ${ASMJIT_DIR}/AsmJit/AsmJitUtil.h
  ${ASMJIT_DIR}/AsmJit/AsmJitVM.h
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
