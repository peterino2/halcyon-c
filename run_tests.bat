mkdir build
cd build
cmake ../
cmake --build .
if %errorlevel% neq 0 cd .. && exit /b %errorlevel%
cd ..
build\Debug\halcyon_test.exe -a
