mkdir build
cd build
set SDLDIR=../../Libraries/SDL
set SDLIMAGEDIR=../../Libraries/SDL
cmake .. -DCMAKE_BUILD_TYPE=Debug -G"Visual Studio 8 2005"
cd ..
pause
