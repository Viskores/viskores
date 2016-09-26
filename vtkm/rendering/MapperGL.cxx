//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 Sandia Corporation.
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/rendering/MapperGL.h>

#include <vtkm/cont/TryExecute.h>
#include <vtkm/rendering/internal/OpenGLHeaders.h>
#include <vtkm/rendering/internal/RunTriangulator.h>
#include <vector>

namespace vtkm {
namespace rendering {

namespace {


template <typename PtType>
VTKM_CONT_EXPORT
void RenderTriangles(MapperGL &mapper,
                     vtkm::Id numTri, const PtType &verts,
                     const vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Id, 4> > &indices,
                     const vtkm::cont::ArrayHandle<vtkm::Float32> &scalar,
                     const vtkm::rendering::ColorTable &ct,
                     const vtkm::Range &scalarRange,
                     const vtkm::rendering::Camera &camera)
{

  if (!mapper.loaded){
      GLenum GlewInitResult = glewInit();
      if (GlewInitResult){
        std::cout << "ERROR: " << glewGetErrorString(GlewInitResult) << std::endl;
      }
      mapper.loaded = true;

      std::vector<float> data, colors;
      vtkm::Float32 sMin = vtkm::Float32(scalarRange.Min);
      vtkm::Float32 sMax = vtkm::Float32(scalarRange.Max);
      vtkm::Float32 sDiff = sMax-sMin;


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
        colors.push_back(color.Components[0]);
        colors.push_back(color.Components[1]);
        colors.push_back(color.Components[2]);

        s = scalar.GetPortalConstControl().Get(i2);
        s = (s-sMin)/sDiff;
        color = ct.MapRGB(s);
        data.push_back(p2[0]);
        data.push_back(p2[1]);
        data.push_back(p2[2]);
        colors.push_back(color.Components[0]);
        colors.push_back(color.Components[1]);
        colors.push_back(color.Components[2]);

        s = scalar.GetPortalConstControl().Get(i3);
        s = (s-sMin)/sDiff;
        color = ct.MapRGB(s);
        data.push_back(p3[0]);
        data.push_back(p3[1]);
        data.push_back(p3[2]);
        colors.push_back(color.Components[0]);
        colors.push_back(color.Components[1]);
        colors.push_back(color.Components[2]);
      }
      GLuint points_vbo = 0;
      glGenBuffers(1, &points_vbo);
      glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
      GLsizeiptr sz = static_cast<GLsizeiptr>(data.size()*sizeof(float));
      glBufferData(GL_ARRAY_BUFFER, sz, &data[0], GL_STATIC_DRAW);

      GLuint colours_vbo = 0;
      glGenBuffers(1, &colours_vbo);
      glBindBuffer(GL_ARRAY_BUFFER, colours_vbo);
      sz = static_cast<GLsizeiptr>(colors.size()*sizeof(float));
      glBufferData(GL_ARRAY_BUFFER, sz, &colors[0], GL_STATIC_DRAW);

      mapper.vao = 0;
      glGenVertexArrays(1, &mapper.vao);
      glBindVertexArray(mapper.vao);
      glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
      glBindBuffer(GL_ARRAY_BUFFER, colours_vbo);
      glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

      glEnableVertexAttribArray(0);
      glEnableVertexAttribArray(1);

      vtkm::Matrix<vtkm::Float32,4,4> viewM = camera.CreateViewMatrix();
      vtkm::Matrix<vtkm::Float32,4,4> projM = camera.CreateProjectionMatrix(512,512);

      MatrixHelpers::CreateOGLMatrix(viewM, mapper.mvMat);
      MatrixHelpers::CreateOGLMatrix(projM, mapper.pMat);
      const char *vertex_shader =
          "#version 120\n"
          "attribute vec3 vertex_position;"
          "attribute vec3 vertex_color;"
          "varying vec3 ourColor;"
          "uniform mat4 mv_matrix;"
          "uniform mat4 p_matrix;"

          "void main() {"
          "  gl_Position = p_matrix*mv_matrix * vec4(vertex_position, 1.0);"
          "  ourColor = vertex_color;"
          "}";
      const char *fragment_shader =
          "#version 120\n"
          "varying vec3 ourColor;"
          "void main() {"
          "  gl_FragColor = vec4 (ourColor, 1.0);"
          "}";

      GLuint vs = glCreateShader(GL_VERTEX_SHADER);
      glShaderSource(vs, 1, &vertex_shader, NULL);
      glCompileShader(vs);
      GLint isCompiled = 0;
      glGetShaderiv(vs, GL_COMPILE_STATUS, &isCompiled);
      if(isCompiled == GL_FALSE)
      {
          GLint maxLength = 0;
          glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &maxLength);

          if (maxLength <= 0){
              fprintf(stderr, "VS: Compilation error in shader with no error message\n");
          }
          else{
              // The maxLength includes the NULL character
              GLchar *strInfoLog = new GLchar[maxLength + 1];
              glGetShaderInfoLog(vs, maxLength, &maxLength, strInfoLog);
              fprintf(stderr, "VS: Compilation error in shader : %s\n", strInfoLog);
              delete [] strInfoLog;
          }
      }

      GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
      glShaderSource(fs, 1, &fragment_shader, NULL);
      glCompileShader(fs);
      glGetShaderiv(fs, GL_COMPILE_STATUS, &isCompiled);
      if(isCompiled == GL_FALSE)
      {
          GLint maxLength = 0;
          glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &maxLength);

          if (maxLength <= 0){
              fprintf(stderr, "VS: Compilation error in shader with no error message\n");
          }
          else{
              // The maxLength includes the NULL character
              GLchar *strInfoLog = new GLchar[maxLength + 1];
              glGetShaderInfoLog(vs, maxLength, &maxLength, strInfoLog);
              fprintf(stderr, "VS: Compilation error in shader : %s\n", strInfoLog);
              delete [] strInfoLog;
          }
      }

      mapper.shader_programme = glCreateProgram();
      if (mapper.shader_programme > 0)
      {

          glAttachShader(mapper.shader_programme, fs);
          glAttachShader(mapper.shader_programme, vs);
          glBindAttribLocation (mapper.shader_programme, 0, "vertex_position");
          glBindAttribLocation (mapper.shader_programme, 1, "vertex_color");

          glLinkProgram (mapper.shader_programme);
          GLint linkStatus;
          glGetProgramiv(mapper.shader_programme, GL_LINK_STATUS, &linkStatus);
          if (!linkStatus)
          {
              char log[2048];
              GLsizei len;
              glGetProgramInfoLog(mapper.shader_programme, 2048, &len, log);
              std::string msg = std::string("Shader program link failed: ")+std::string(log);
              throw vtkm::cont::ErrorControlBadValue(msg);
          }
      }
  }


  if (mapper.shader_programme > 0)
  {
      glUseProgram(mapper.shader_programme);
      GLint mvID = glGetUniformLocation(mapper.shader_programme, "mv_matrix");
      glUniformMatrix4fv(mvID, 1, GL_FALSE, mapper.mvMat);
      GLint pID = glGetUniformLocation(mapper.shader_programme, "p_matrix");
      glUniformMatrix4fv(pID, 1, GL_FALSE, mapper.pMat);
      glBindVertexArray(mapper.vao);
      glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(numTri*3));
      glUseProgram(0);
    }
}

} // anonymous namespace

MapperGL::MapperGL()
{ this->loaded = false; }

MapperGL::~MapperGL()
{  }

void MapperGL::RenderCells(const vtkm::cont::DynamicCellSet &cellset,
                           const vtkm::cont::CoordinateSystem &coords,
                           const vtkm::cont::Field &scalarField,
                           const vtkm::rendering::ColorTable &colorTable,
                           const vtkm::rendering::Camera &camera,
                           const vtkm::Range &scalarRange)
{
  vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Id, 4> > indices;
  vtkm::Id numTri;
  vtkm::rendering::internal::RunTriangulator(cellset, indices, numTri);

  vtkm::cont::ArrayHandle<vtkm::Float32> sf;
  sf = scalarField.GetData().Cast<vtkm::cont::ArrayHandle<vtkm::Float32> >();

  vtkm::cont::DynamicArrayHandleCoordinateSystem dcoords = coords.GetData();
  vtkm::cont::ArrayHandleUniformPointCoordinates uVerts;
  vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Float32,3> > eVerts;

  if(dcoords.IsSameType(vtkm::cont::ArrayHandleUniformPointCoordinates()))
  {
    uVerts = dcoords.Cast<vtkm::cont::ArrayHandleUniformPointCoordinates>();
    RenderTriangles(*this, numTri, uVerts, indices, sf, colorTable, scalarRange, camera);
  }
  else if(dcoords.IsSameType(vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Float32,3> >()))
  {
    eVerts = dcoords.Cast<vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Float32,3> > > ();
    RenderTriangles(*this, numTri, eVerts, indices, sf, colorTable, scalarRange, camera);
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
    RenderTriangles(*this, numTri, rVerts, indices, sf, colorTable, scalarRange, camera);
  }
  glFinish();
  glFlush();
}

void MapperGL::StartScene()
{
  // Nothing needs to be done.
}

void MapperGL::EndScene()
{
  // Nothing needs to be done.
}

void MapperGL::SetCanvas(vtkm::rendering::Canvas *)
{
  // Nothing needs to be done.
}

vtkm::rendering::Mapper *MapperGL::NewCopy() const
{
  return new vtkm::rendering::MapperGL(*this);
}

}
} // namespace vtkm::rendering
