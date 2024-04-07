#include <string>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace opengl
{

GLFWwindow *create_current_window(int width, int height, const int context_ver[2]);

GLuint create_shader(const std::string &vertex_source, const std::string &fragment_source);

void create_rect(GLuint *vao, GLuint *vbo);

}

