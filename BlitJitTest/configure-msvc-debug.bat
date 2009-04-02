mkdir build
cd build
set SDLDIR=../SDL
set SDLIMAGEDIR=../SDL
cmake .. -DCMAKE_BUILD_TYPE=debug -G"Visual Studio 8 2005"
cd ..
pause
