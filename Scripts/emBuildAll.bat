set p=%cd%
start cmd /K "call %p%\..\Dependencies\Emscripten\emsdk\emsdk_env.bat & call %p%\..\Scripts\emBuild.bat"