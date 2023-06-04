:: Enter that directory
call cd emsdk

:: Fetch the latest version of the emsdk (not needed the first time you clone)
call git pull

:: Download and install the latest SDK tools.
call emsdk install latest

:: Make the "latest" SDK "active" for the current user. (writes .emscripten file)
call emsdk activate latest

:: Activate PATH and other environment variables in the current terminal
call emsdk_env.bat
PAUSE