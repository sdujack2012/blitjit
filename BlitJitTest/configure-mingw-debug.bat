mkdir build_mingw
cd build_mingw

set SDLDIR=../SDL
set SDLIMAGEDIR=../SDL

cmake .. -DCMAKE_BUILD_TYPE=debug -G"MinGW Makefiles"
pause

