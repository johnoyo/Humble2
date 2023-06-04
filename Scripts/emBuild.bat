set p=%cd%
if not exist %p%\..\bin mkdir %p%\..\bin
if not exist %p%\..\bin\Web mkdir %p%\..\bin\Web

call emcc -std=c++17 -sUSE_GLFW=3 -sFULL_ES3=1 -sALLOW_MEMORY_GROWTH -sASSERTIONS -DEMSCRIPTEN=1 --memory-init-file 0 ^
 -I"../Dependencies/GLFW/include"^
 -I"../Humble2/src/Vendor/spdlog-1.x/include"^
 -I"../Humble2/src/Vendor"^
 -I"../Humble2/src"^
 "../Humble2/src/Humble2/Core/Application.cpp"^
 "../Humble2/src/Humble2/Core/Window.cpp"^
 "../Humble2/src/Humble2/Renderer/IndexBuffer.cpp"^
 "../Humble2/src/Humble2/Renderer/RenderCommand.cpp"^
 "../Humble2/src/Humble2/Renderer/Renderer2D.cpp"^
 "../Humble2/src/Humble2/Renderer/Shader.cpp"^
 "../Humble2/src/Humble2/Renderer/VertexArray.cpp"^
 "../Humble2/src/Humble2/Renderer/VertexBuffer.cpp"^
 "../Humble2/src/Humble2/Renderer/Texture.cpp"^
 "../Humble2/src/Platform/OpenGL/OpenGLIndexBuffer.cpp"^
 "../Humble2/src/Platform/OpenGL/OpenGLRendererAPI.cpp"^
 "../Humble2/src/Platform/OpenGL/OpenGLShader.cpp"^
 "../Humble2/src/Platform/OpenGL/OpenGLVertexArray.cpp"^
 "../Humble2/src/Platform/OpenGL/OpenGLVertexBuffer.cpp"^
 "../Humble2/src/Platform/OpenGL/OpenGLTexture.cpp"^
 "../Humble2/src/Humble2/Utilities/Log.cpp"^
 "../Humble2/src/Humble2/Utilities/JobSystem.cpp"^
 "../Humble2/src/Vendor/stb_image/stb_image.cpp"^
 "../HumbleApp/src/main.cpp"^
 --embed-file "../HumbleApp/assets" -O3 -o ../bin/Web/HumbleApp.html