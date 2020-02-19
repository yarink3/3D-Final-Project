
#include "igl/opengl/glfw/renderer.h"
#include "tutorial/sandBox/inputManager.h"
#include <igl/opengl/glfw/imgui/ImGuiMenu.h>
#include <igl/opengl/glfw/imgui/ImGuiHelpers.h>
#include <external/imgui/imgui.h>
#include "OurMesh.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <stdio.h>  /* defines FILENAME_MAX */
// #define WINDOWS  /* uncomment this line to use it for windows.*/
#ifdef WINDOWS
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif
#include<iostream>



std::string GetCurrentWorkingDir( void ) {
    char buff[FILENAME_MAX];
    GetCurrentDir( buff, FILENAME_MAX );
    std::string current_working_dir(buff);
    return current_working_dir;
}
int main(int argc, char *argv[])
{





    Display *disp = new Display(1000, 800, "Wellcome");
    Renderer renderer;
    igl::opengl::glfw::Viewer viewer;
    viewer.init(1);

//   igl::opengl::glfw::imgui::ImGuiMenu menu;

    Init(*disp);
    renderer.init(&viewer);
    disp->SetRenderer(&renderer);
    disp->launch_rendering(true);




    delete disp;
}

