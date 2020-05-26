@echo off

set glfw_inc="C:\lib\glfw-3.3.bin.WIN64\include"
set glfw_libpath="C:\LIB\GLFW-3.3.BIN.WIN64\LIB-VC2019"

set vulkan_inc="C:\VulkanSDK\1.2.135.0\Include"
set vulkan_libpath="C:\VULKANSDK\1.2.135.0\LIB"

set glm_inc=C:\lib\glm
set IncludeFlags=-I%glfw_inc% -I%vulkan_inc% -I%glm_inc%

set CompilerFlags=%IncludeFlags% /EHsc /nologo /Zi /MTd
set LinkerFlags=/LIBPATH:%glfw_libpath% /LIBPATH:%vulkan_libpath% 

set Sources1=../src/main.cpp ../src/logger.cpp ../src/vma.cpp

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

REM 64-bit build

cl %CompilerFlags% %Sources1% /link %LinkerFlags% glfw3dll.lib vulkan-1.lib

popd


