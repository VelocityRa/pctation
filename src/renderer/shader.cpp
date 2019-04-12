#include <renderer/shader.hpp>

#include <util/log.hpp>

#include <glbinding/gl/gl.h>

#include <fstream>
#include <sstream>
#include <utility>
#include <vector>

using namespace gl;

namespace renderer {

/**
 * \brief Loads (compiles, links) GLSL shaders at specified file paths
 * \param shader_name Name of shaders
 * \return GLuint that holds the resulting program id. 0 if loading was unsuccessful
 */
GLuint load_shaders(const std::string& shader_name) {
  static const std::string SHADER_PATH = "shaders/";

  std::string vertex_file_path = SHADER_PATH + shader_name + ".vs.glsl";
  std::string fragment_file_path = SHADER_PATH + shader_name + ".fs.glsl";

  GLuint vs = glCreateShader(GL_VERTEX_SHADER);
  GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);

  // Read the vertex/fragment shader code from files
  std::string vs_code;
  std::ifstream vs_stream(vertex_file_path, std::ios::in);
  if (vs_stream.is_open()) {
    std::stringstream sstr;
    sstr << vs_stream.rdbuf();
    vs_code = sstr.str();
    vs_stream.close();
  } else {
    LOG_ERROR("Couldn't open shader: {}", vertex_file_path);
    return {};
  }

  std::string fs_code;
  std::ifstream fs_stream(fragment_file_path, std::ios::in);
  if (fs_stream.is_open()) {
    std::stringstream sstr;
    sstr << fs_stream.rdbuf();
    fs_code = sstr.str();
    fs_stream.close();
  }

  GLint result = 0;
  int info_log_length;

  // Compile vertex shader
  char const* vs_source_pointer = vs_code.c_str();
  glShaderSource(vs, 1, &vs_source_pointer, NULL);
  glCompileShader(vs);

  // Check vertex shader
  glGetShaderiv(vs, GL_COMPILE_STATUS, &result);
  glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &info_log_length);
  if (!result) {
    std::vector<char> vertex_shader_error_message(info_log_length + 1);
    glGetShaderInfoLog(vs, info_log_length, NULL, &vertex_shader_error_message[0]);
    LOG_ERROR("Error compiling vertex shader: {}\n", &vertex_shader_error_message[0]);
    return {};
  }

  // Compile fragment shader
  char const* fs_source_pointer = fs_code.c_str();
  glShaderSource(fs, 1, &fs_source_pointer, NULL);
  glCompileShader(fs);

  // Check fragment shader
  glGetShaderiv(fs, GL_COMPILE_STATUS, &result);
  glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &info_log_length);
  if (!result) {
    std::vector<char> fragment_shader_error_message(info_log_length + 1);
    glGetShaderInfoLog(fs, info_log_length, NULL, &fragment_shader_error_message[0]);
    LOG_ERROR("Error compiling fragment shader: {}\n", &fragment_shader_error_message[0]);
    return {};
  }

  // Link the program
  const auto program = glCreateProgram();
  glAttachShader(program, vs);
  glAttachShader(program, fs);
  glLinkProgram(program);

  // Check the program
  glGetProgramiv(program, GL_LINK_STATUS, &result);
  glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_log_length);
  if (!result) {
    std::vector<char> program_error_message(info_log_length + 1);
    glGetProgramInfoLog(program, info_log_length, NULL, &program_error_message[0]);
    LOG_ERROR("Error linking shader program: {}\n", &program_error_message[0]);
    return {};
  }

  glDetachShader(program, vs);
  glDetachShader(program, fs);

  glDeleteShader(vs);
  glDeleteShader(fs);

  return program;
}

}  // namespace renderer