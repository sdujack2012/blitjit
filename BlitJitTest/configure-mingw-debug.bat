mkdir build
cd build
set SDLDIR=../../Libraries/SDL
set SDLIMAGEDIR=../../Libraries/SDL
cmake .. -DCMAKE_BUILD_TYPE=Debug -G"MinGW Makefiles"
cd ..
pause

