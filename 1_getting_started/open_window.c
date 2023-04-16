#include <GLFW/glfw3.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
  if (!glfwInit()) {
    printf("Could not initialize GLFW!");
    return 1;
  }

  GLFWwindow *window = glfwCreateWindow(640, 480, "Learn WebGPU", NULL, NULL);
  if (!window) {
    printf("Could not open window!");
    glfwTerminate();
    return 1;
  }

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}