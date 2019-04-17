//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef LOAD_SHADERS
#define LOAD_SHADERS

#if defined(__APPLE__)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

namespace shaders
{
static std::string const& make_fragment_shader_code()
{
  static std::string const data = "#version 120\n"
                                  "void main(void)"
                                  "{"
                                  "    gl_FragColor = gl_Color;"
                                  "}";

  return data;
}

static std::string const& make_vertex_shader_code()
{
  static std::string const data = "#version 120\n"
                                  "attribute vec3 posAttr;"
                                  "uniform mat4 MVP;"
                                  "void main()"
                                  "{"
                                  "  vec4 pos = vec4( posAttr, 1.0 );"
                                  "  gl_FrontColor = gl_Color;"
                                  "  gl_Position = MVP * pos;"
                                  "}";

  return data;
}
}

inline GLuint LoadShaders()
{
  // Create the shaders
  GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
  GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

  // Get the Vertex Shader code
  std::string VertexShaderCode = shaders::make_vertex_shader_code();

  // Get the Fragment Shader code
  std::string FragmentShaderCode = shaders::make_fragment_shader_code();

  GLint Result = GL_FALSE;
  int InfoLogLength;

  // Compile Vertex Shader
  std::cout << "Compiling vertex shader" << std::endl;
  char const* VertexSourcePointer = VertexShaderCode.c_str();
  glShaderSource(VertexShaderID, 1, &VertexSourcePointer, nullptr);
  glCompileShader(VertexShaderID);

  // Check Vertex Shader
  glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
  glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
  std::vector<char> VertexShaderErrorMessage(static_cast<std::size_t>(InfoLogLength));
  if (VertexShaderErrorMessage.size() > 0)
  {
    glGetShaderInfoLog(VertexShaderID, InfoLogLength, nullptr, &VertexShaderErrorMessage[0]);
    std::cout << &VertexShaderErrorMessage[0] << std::endl;
  }

  // Compile Fragment Shader
  std::cout << "Compiling fragment shader" << std::endl;
  char const* FragmentSourcePointer = FragmentShaderCode.c_str();
  glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, nullptr);
  glCompileShader(FragmentShaderID);

  // Check Fragment Shader
  glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
  glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
  std::vector<char> FragmentShaderErrorMessage(static_cast<std::size_t>(InfoLogLength));
  if (FragmentShaderErrorMessage.size() > 0)
  {
    glGetShaderInfoLog(FragmentShaderID, InfoLogLength, nullptr, &FragmentShaderErrorMessage[0]);
    std::cout << &FragmentShaderErrorMessage[0] << std::endl;
  }

  // Link the program
  std::cout << "Linking program" << std::endl;
  GLuint ProgramID = glCreateProgram();
  glAttachShader(ProgramID, VertexShaderID);
  glAttachShader(ProgramID, FragmentShaderID);
  glLinkProgram(ProgramID);

  // Check the program
  glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
  glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
  std::vector<char> ProgramErrorMessage(static_cast<std::size_t>(InfoLogLength));
  if (ProgramErrorMessage.size() > 0)
  {
    glGetProgramInfoLog(ProgramID, InfoLogLength, nullptr, &ProgramErrorMessage[0]);
    std::cout << &ProgramErrorMessage[0] << std::endl;
  }

  glDeleteShader(VertexShaderID);
  glDeleteShader(FragmentShaderID);

  return ProgramID;
}

#endif
