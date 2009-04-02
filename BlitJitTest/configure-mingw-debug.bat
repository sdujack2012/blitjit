mkdir build
cd build
set SDLDIR=../SDL
set SDLIMAGEDIR=../SDL
cmake .. -DCMAKE_BUILD_TYPE=debug -G"MinGW Makefiles"
cd ..
pause

