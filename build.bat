@echo off
del plane_crawler.exe
set compiler_flags= -Wall -g -msse4.1 -std=c++11
set linker_flags= -lglfw3 -lopengl32 -lgdi32 -lopenal32 
g++ %compiler_flags% ./source/main.cpp -I ./source/ %linker_flags% -o plane_crawler.exe