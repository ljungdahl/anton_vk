#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "avk.cpp"

i32 main(i32 argc, const i8** argv) {
    i32 rc = glfwInit();
    ASSERT(rc == GLFW_TRUE);
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow* windowPtr;
    windowPtr = glfwCreateWindow( 1280, 720, "flavk", nullptr, nullptr );


    /// Init Vulkan

    




    
    // MAIN LOOP
    Logger::Trace("Entering main loop.");
    while(!glfwWindowShouldClose(windowPtr)) {        
        glfwPollEvents();
        
        // DRAW CALLS


        
    }




    
    // Cleanup GLFW
    if(windowPtr) {
        glfwDestroyWindow(windowPtr);
    }
    glfwTerminate();
    
    return 0;
    
}
