set p=%cd%
if not exist %p%\bin mkdir %p%\bin
if not exist %p%\bin\Web mkdir %p%\bin\Web

call emcc -sSIDE_MODULE -std=c++17 -I"Dependencies/GLFW/include" "Humble2/src/Humble2/Core/Application.cpp" "Humble2/src/Humble2/Core/Window.cpp" "Humble2/src/Vendor/stb_image/stb_image.cpp" -O3 -o bin/Web/Humble2.wasm

call emcc -sMAIN_MODULE -std=c++17 -I"Humble2/src/" "SampleApp/src/main.cpp" bin/Web/Humble2.wasm -s USE_GLFW=3 -s FULL_ES3=1 -DEMSCRIPTEN=1 --memory-init-file 0 -O3 -o bin/Web/SampleApp.html