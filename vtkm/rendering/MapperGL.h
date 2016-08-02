//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_rendering_MapperGL_h
#define vtk_m_rendering_MapperGL_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/ColorTable.h>
#include <vtkm/rendering/Mapper.h>
#include <vtkm/rendering/Triangulator.h>
#include <vtkm/rendering/internal/OpenGLHeaders.h>

#include <stdio.h>
#include <iostream>
#include <fstream>

using namespace std;


namespace vtkm {
namespace rendering {

static void
copyMat(const vtkm::Matrix<vtkm::Float32,4,4> &mIn,
        GLfloat *mOut)
{
    int idx = 0;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++, idx++)
            mOut[idx] = mIn(i,j);
}

template<typename DeviceAdapter = VTKM_DEFAULT_DEVICE_ADAPTER_TAG>
class MapperGL : public Mapper
{
public:
  VTKM_CONT_EXPORT
  MapperGL() {}

  VTKM_CONT_EXPORT
  virtual void RenderCells(const vtkm::cont::DynamicCellSet &cellset,
                           const vtkm::cont::CoordinateSystem &coords,
                           const vtkm::cont::Field &scalarField,
                           const vtkm::rendering::ColorTable &colorTable,
                           const vtkm::rendering::Camera &camera,
                           const vtkm::Range &scalarRange)
  {
    vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Id, 4> > indices;
    vtkm::Id numTri;
    Triangulator<DeviceAdapter> triangulator;
    triangulator.run(cellset, indices, numTri);

    vtkm::cont::ArrayHandle<vtkm::Float32> sf;
    sf = scalarField.GetData().Cast<vtkm::cont::ArrayHandle<vtkm::Float32> >();

    vtkm::cont::DynamicArrayHandleCoordinateSystem dcoords = coords.GetData();
    vtkm::cont::ArrayHandleUniformPointCoordinates uVerts;
    vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Float32,3> > eVerts;

    if(dcoords.IsSameType(vtkm::cont::ArrayHandleUniformPointCoordinates()))
    {
      uVerts = dcoords.Cast<vtkm::cont::ArrayHandleUniformPointCoordinates>();
      RenderTriangles(numTri, uVerts, indices, sf, colorTable, scalarRange, camera);
    }
    else if(dcoords.IsSameType(vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Float32,3> >()))
    {
      eVerts = dcoords.Cast<vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Float32,3> > > ();
      RenderTriangles(numTri, eVerts, indices, sf, colorTable, scalarRange, camera);
    }
    else if(dcoords.IsSameType(vtkm::cont::ArrayHandleCartesianProduct<
                               vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                               vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                               vtkm::cont::ArrayHandle<vtkm::FloatDefault> >()))
    {
      vtkm::cont::ArrayHandleCartesianProduct<
          vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
          vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
          vtkm::cont::ArrayHandle<vtkm::FloatDefault> > rVerts;
      rVerts = dcoords.Cast<vtkm::cont::ArrayHandleCartesianProduct<
                                vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                vtkm::cont::ArrayHandle<vtkm::FloatDefault> > > ();
      RenderTriangles(numTri, rVerts, indices, sf, colorTable, scalarRange, camera);
    }
    glFinish();
    glFlush();
  }
const char* GL_type_to_string (GLenum type) 
{
  switch (type) {
    case GL_BOOL: return "bool";
    case GL_INT: return "int";
    case GL_FLOAT: return "float";
    case GL_FLOAT_VEC2: return "vec2";
    case GL_FLOAT_VEC3: return "vec3";
    case GL_FLOAT_VEC4: return "vec4";
    case GL_FLOAT_MAT2: return "mat2";
    case GL_FLOAT_MAT3: return "mat3";
    case GL_FLOAT_MAT4: return "mat4";
    case GL_SAMPLER_2D: return "sampler2D";
    case GL_SAMPLER_3D: return "sampler3D";
    case GL_SAMPLER_CUBE: return "samplerCube";
    case GL_SAMPLER_2D_SHADOW: return "sampler2DShadow";
    default: break;
  }
  return "other";
}
void _print_programme_info_log (GLuint programme) {
  int max_length = 2048;
  int actual_length = 0;
  char log[2048];
  glGetProgramInfoLog (programme, max_length, &actual_length, log);
  printf ("program info log for GL index %u:\n%s", programme, log);
}
void print_all (GLuint programme) {
  printf ("--------------------\nshader programme %i info:\n", programme);
  int params = -1;
  glGetProgramiv (programme, GL_LINK_STATUS, &params);
  printf ("GL_LINK_STATUS = %i\n", params);
  
  glGetProgramiv (programme, GL_ATTACHED_SHADERS, &params);
  printf ("GL_ATTACHED_SHADERS = %i\n", params);
  
  glGetProgramiv (programme, GL_ACTIVE_ATTRIBUTES, &params);
  printf ("GL_ACTIVE_ATTRIBUTES = %i\n", params);
  for (int i = 0; i < params; i++) {
    char name[64];
    int max_length = 64;
    int actual_length = 0;
    int size = 0;
    GLenum type;
    glGetActiveAttrib (
      programme,
      i,
      max_length,
      &actual_length,
      &size,
      &type,
      name
    );
    if (size > 1) {
      for (int j = 0; j < size; j++) {
        char long_name[64];
        sprintf (long_name, "%s[%i]", name, j);
        int location = glGetAttribLocation (programme, long_name);
        printf ("  %i) type:%s name:%s location:%i\n",
          i, GL_type_to_string (type), long_name, location);
      }
    } else {
      int location = glGetAttribLocation (programme, name);
      printf ("  %i) type:%s name:%s location:%i\n",
        i, GL_type_to_string (type), name, location);
    }
  }
  
  glGetProgramiv (programme, GL_ACTIVE_UNIFORMS, &params);
  printf ("GL_ACTIVE_UNIFORMS = %i\n", params);
  for (int i = 0; i < params; i++) {
    char name[64];
    int max_length = 64;
    int actual_length = 0;
    int size = 0;
    GLenum type;
    glGetActiveUniform (
      programme,
      i,
      max_length,
      &actual_length,
      &size,
      &type,
      name
    );
    if (size > 1) {
      for (int j = 0; j < size; j++) {
        char long_name[64];
        sprintf (long_name, "%s[%i]", name, j);
        int location = glGetUniformLocation (programme, long_name);
        printf ("  %i) type:%s name:%s location:%i\n",
          i, GL_type_to_string (type), long_name, location);
      }
    } else {
      int location = glGetUniformLocation (programme, name);
      printf ("  %i) type:%s name:%s location:%i\n",
        i, GL_type_to_string (type), name, location);
    }
  }
  
  _print_programme_info_log (programme);
}
  template <typename PtType>
  VTKM_CONT_EXPORT
  void RenderTriangles(vtkm::Id numTri, const PtType &verts,
                       const vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Id, 4> > &indices,
                       const vtkm::cont::ArrayHandle<vtkm::Float32> &scalar,
                       const vtkm::rendering::ColorTable &ct,
                       const vtkm::Range &scalarRange,
                       const vtkm::rendering::Camera &camera)
  {
    glewExperimental = GL_TRUE; 
    glewInit();
    vtkm::Float32 sMin = vtkm::Float32(scalarRange.Min);
    vtkm::Float32 sMax = vtkm::Float32(scalarRange.Max);
    vtkm::Float32 sDiff = sMax-sMin;

    vector<float> data, colors;
    int method = 2;
    for (int i = 0; i < numTri; i++)
    {
      vtkm::Vec<vtkm::Id, 4> idx = indices.GetPortalConstControl().Get(i);
      vtkm::Id i1 = idx[1];
      vtkm::Id i2 = idx[2];
      vtkm::Id i3 = idx[3];

      vtkm::Vec<vtkm::Float32, 3> p1 = verts.GetPortalConstControl().Get(idx[1]);
      vtkm::Vec<vtkm::Float32, 3> p2 = verts.GetPortalConstControl().Get(idx[2]);
      vtkm::Vec<vtkm::Float32, 3> p3 = verts.GetPortalConstControl().Get(idx[3]);

      vtkm::Float32 s;
      Color color;
      
      s = scalar.GetPortalConstControl().Get(i1);
      s = (s-sMin)/sDiff;
      color = ct.MapRGB(s);
      data.push_back(p1[0]);
      data.push_back(p1[1]);
      data.push_back(p1[2]);
      if (method==0)
      {
          data.push_back(color.Components[0]);
          data.push_back(color.Components[1]);
          data.push_back(color.Components[2]);
      }
      else if (method == 2)
      {
          colors.push_back(color.Components[0]);
          colors.push_back(color.Components[1]);
          colors.push_back(color.Components[2]);
      }

      s = scalar.GetPortalConstControl().Get(i2);
      s = (s-sMin)/sDiff;
      color = ct.MapRGB(s);
      data.push_back(p2[0]);
      data.push_back(p2[1]);
      data.push_back(p2[2]);
      if (method==0)      
      {
          data.push_back(color.Components[0]);
          data.push_back(color.Components[1]);
          data.push_back(color.Components[2]);
      }
      else if (method == 2)
      {
          colors.push_back(color.Components[0]);
          colors.push_back(color.Components[1]);
          colors.push_back(color.Components[2]);
      }          

      s = scalar.GetPortalConstControl().Get(i3);
      s = (s-sMin)/sDiff;
      color = ct.MapRGB(s);
      data.push_back(p3[0]);
      data.push_back(p3[1]);
      data.push_back(p3[2]);
      if (method==0)
      {
          data.push_back(color.Components[0]);
          data.push_back(color.Components[1]);
          data.push_back(color.Components[2]);
      }
      else if (method == 2)
      {
          colors.push_back(color.Components[0]);
          colors.push_back(color.Components[1]);
          colors.push_back(color.Components[2]);
      }          
    }
    cout<<"data.size()= "<<data.size()<<endl;

    if (method == 0)
    {
        glBegin(GL_TRIANGLES);
        for (int i = 0; i < data.size(); i+=6)
        {
            glColor3f(data[i+3], data[i+4], data[i+5]);
            glVertex3f(data[i+0], data[i+1], data[i+2]);
        }
        glEnd();
    }
    else if (method == 1)
    {   

        cout<<"Enter method 1\n";
        GLuint vbo = 0;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size()*sizeof (float), &data[0], GL_STATIC_DRAW);
        cout<<"Bind buffer\n";
        GLuint vao = 0;
        glGenVertexArrays(1, &vao);
        cout<<"Bind buffer1\n";
        glBindVertexArray(vao);
        cout<<"Bind buffer 1.5\n";
        glEnableVertexAttribArray(0);
        cout<<"Bind buffer 2\n";
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        cout<<"Vertex pointer \n";
        GLfloat mvMat[16], pMat[16];
        glGetFloatv(GL_MODELVIEW_MATRIX, (float*)mvMat);
        glGetFloatv(GL_PROJECTION_MATRIX, (float*)pMat);
        
        const char* vertex_shader =
            "#version 400\n"
            "uniform mat4 mv_matrix;"
            "uniform mat4 p_matrix;"
            "in vec4 a_vertex;"
            "uniform mat4 mvp;"
            "void main() {"
//            "  gl_Position = p_matrix * mv_matrix * gl_Vertex;"
//           "  gl_Position = p_matrix * mv_matrix * vec4(a_vertex, 1);" //vec4(position, 1);"            
            "  gl_Position = gl_Vertex;" //gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex;"
            "}\0";

        const char* fragment_shader =
            "#version 400\n"
            "out vec4 frag_colour;"
            "void main () {"
            "  frag_colour = vec4 (0.5, 0.0, 0.5, 1.0);"
            "}\0";
        cout<<"Compiling \n";
        GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vertex_shader, NULL);
        glCompileShader(vs);
        GLint isCompiled = 0;
        glGetShaderiv(vs, GL_COMPILE_STATUS, &isCompiled);
        if(isCompiled == GL_FALSE)
        {
          GLint maxLength = 0;
          glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &maxLength);

          // The maxLength includes the NULL character
          GLchar* strInfoLog = new GLchar[maxLength + 1];
          glGetShaderInfoLog(vs, maxLength, &maxLength, strInfoLog);
          fprintf(stderr, "Compilation error in shader : %s\n", strInfoLog);
          delete[] strInfoLog;
        }

        GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fragment_shader, NULL);
        glCompileShader(fs);
        GLuint shader_programme = glCreateProgram();
        GLint mvID = glGetUniformLocation(shader_programme, "mv_matrix");
        glUniformMatrix4fv(mvID, 1, GL_FALSE, mvMat);
        GLint pID = glGetUniformLocation(shader_programme, "p_matrix");
        glUniformMatrix4fv(pID, 1, GL_FALSE, pMat);        
        cout<<"Shaders Compiled\n";
        glAttachShader(shader_programme, fs);
        glAttachShader(shader_programme, vs);
        glLinkProgram(shader_programme);
        GLint success;
        glGetProgramiv(shader_programme, GL_LINK_STATUS, &success);
        if (!success)
        {
          cout<<"**********************************************LINK FAILED"<<endl;
        } 

        char log[2048];
        GLsizei len;
        glGetProgramInfoLog(shader_programme, 2048, &len, log);
        cout<<"program info: "<<log<<endl;        
        
        glUseProgram(shader_programme);
        print_all(shader_programme);
        glBindVertexArray(vao);
        glDrawArrays (GL_TRIANGLES, 0, numTri*3);
        glUseProgram(0);
    }
    else if (method == 2)
    {
        for (int i = 0; i < data.size(); i+= 3)
            cout<<i%3<<": ("<<data[i]<<","<<data[i+1]<<","<<data[i+2]<<") ["<<colors[i]<<","<<colors[i+1]<<","<<colors[i+2]<<"]"<<endl;
        GLuint points_vbo = 0;
        glGenBuffers(1, &points_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size()*sizeof (float), &data[0], GL_STATIC_DRAW);        

        GLuint colours_vbo = 0;
        glGenBuffers(1, &colours_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, colours_vbo);
        glBufferData(GL_ARRAY_BUFFER, colors.size()*sizeof(float), &colors[0], GL_STATIC_DRAW);

        GLuint vao = 0;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        glBindBuffer(GL_ARRAY_BUFFER, colours_vbo);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        GLfloat mvMat[16], pMat[16];
        vtkm::Matrix<vtkm::Float32,4,4> viewM = camera.CreateViewMatrix();
        vtkm::Matrix<vtkm::Float32,4,4> projM = camera.CreateProjectionMatrix(512,512);
        vtkm::rendering::copyMat(viewM, mvMat);
        vtkm::rendering::copyMat(projM, pMat);
        //glGetFloatv(GL_MODELVIEW_MATRIX, (float*)mvMat);
        //glGetFloatv(GL_PROJECTION_MATRIX, (float*)pMat);
        for(int i = 0; i < 16;++i) 
          {
            if(i % 4 == 0 && i != 0) cout<<"\n";
            cout<<mvMat[i]<<" ";
          }
          cout<<"\n";
        const char *vertex_shader =
            "#version 400 core\n"
            "layout(location = 1) in vec3 vertex_position;"
            "layout(location = 0) in vec3 vertex_color;"
            "out vec3 ourColor;"
            "uniform mat4 mv_matrix;"
            "uniform mat4 p_matrix;"        
        
            "void main() {"
           "  gl_Position = p_matrix*mv_matrix * vec4(vertex_position, 1.0);"
 //           "  gl_Position = vec4(vertex_color, 1.0);"
 //           "  gl_Position = vec4(vertex_position, 1.0);"
            "  ourColor = vertex_color;"
            "}";
        const char *fragment_shader =
            "#version 400 core\n"
            "in vec3 ourColor;"
            "out vec4 color;"
            "void main() {"
//           "  color = vec4 (0.5, 0.0, 0.5, 1.0);"
             "  color = vec4 (ourColor, 1.0);"
            "}";

        GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vertex_shader, NULL);
        glCompileShader(vs);
        GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fragment_shader, NULL);
        glCompileShader(fs);        
        GLuint shader_programme = glCreateProgram();
                      
        
        glAttachShader(shader_programme, fs);
        glAttachShader(shader_programme, vs);
        
        // insert location binding code here
        glBindAttribLocation (shader_programme, 0, "vertex_position");
        glBindAttribLocation (shader_programme, 1, "vertex_color");
        glLinkProgram (shader_programme);
        GLint success;
        glGetProgramiv(shader_programme, GL_LINK_STATUS, &success);
        if (!success) cout<<"**********************************************LINK FAILED"<<endl;
        char log[2048];
        GLsizei len;
        glGetProgramInfoLog(shader_programme, 2048, &len, log);
        cout<<"program info: "<<log<<endl;
        
        glUseProgram(shader_programme);
        GLint mvID = glGetUniformLocation(shader_programme, "mv_matrix");
        glUniformMatrix4fv(mvID, 1, GL_FALSE, mvMat);
        GLint pID = glGetUniformLocation(shader_programme, "p_matrix");
        glUniformMatrix4fv(pID, 1, GL_FALSE, pMat);  
        print_all(shader_programme);
        glBindVertexArray(vao);
        glDrawArrays (GL_TRIANGLES, 0, numTri*3);
        glUseProgram(0);
    }    
  }
};

}} //namespace vtkm::rendering

#endif //vtk_m_rendering_MapperGL_h
