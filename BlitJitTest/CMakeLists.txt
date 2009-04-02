cmake_minimum_required(VERSION 2.6)

Set(ASMJIT_DIR "../AsmJit")
Set(BLITJIT_DIR "../BlitJit")

Include(${ASMJIT_DIR}/CMakeLists.txt)
Include(${BLITJIT_DIR}/CMakeLists.txt)

Project(blittest C CXX)

# Build AsmJit statically
Add_Definitions(-DASMJIT_API=)
Add_Definitions(-DBLITJIT_API=)

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

Add_Library(AsmJit ${ASMJIT_SOURCES} ${ASMJIT_HEADERS})
Add_Library(BlitJit ${BLITJIT_SOURCES} ${BLITJIT_HEADERS})

Add_Executable(test src/test.cpp)
Target_Link_Libraries(test AsmJit BlitJit)
