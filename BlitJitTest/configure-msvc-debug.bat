mkdir build_msvc
cd build_msvc

set SDLDIR=../SDL
set SDLIMAGEDIR=../SDL

cmake .. -DCMAKE_BUILD_TYPE=debug -G"Visual Studio 8 2005"
pause
