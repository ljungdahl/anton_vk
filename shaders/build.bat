@echo off

set glslc=C:\VulkanSDK\1.2.135.0\Bin32\glslangValidator.exe -V

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build\

%glslc% ..\shaders\mesh.vert.glsl -o mesh.vert.spv
%glslc% ..\shaders\gooch.frag.glsl -o gooch.frag.spv


popd