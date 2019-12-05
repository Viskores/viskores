//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/MapperGL.h>

#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/TryExecute.h>
#include <vtkm/rendering/internal/OpenGLHeaders.h>
#include <vtkm/rendering/internal/RunTriangulator.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/exec/ColorTable.h>

#include <vector>

namespace vtkm
{
namespace rendering
{

namespace
{

using Id4Type = TypeListId4;

class MapColorAndVertices : public vtkm::worklet::WorkletMapField
{
public:
  const vtkm::exec::ColorTableBase* ColorTable;
  const vtkm::Float32 SMin, SDiff;

  VTKM_CONT
  MapColorAndVertices(const vtkm::exec::ColorTableBase* table,
                      vtkm::Float32 sMin,
                      vtkm::Float32 sDiff)
    : ColorTable(table)
    , SMin(sMin)
    , SDiff(sDiff)
  {
  }
  using ControlSignature = void(FieldIn vertexId,
                                WholeArrayIn indices,
                                WholeArrayIn scalar,
                                WholeArrayIn verts,
                                WholeArrayOut out_color,
                                WholeArrayOut out_vertices);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6);

  template <typename InputArrayIndexPortalType,
            typename InputArrayPortalType,
            typename InputArrayV3PortalType,
            typename OutputArrayPortalType>
  VTKM_EXEC void operator()(const vtkm::Id& i,
                            InputArrayIndexPortalType& indices,
                            const InputArrayPortalType& scalar,
                            const InputArrayV3PortalType& verts,
                            OutputArrayPortalType& c_array,
                            OutputArrayPortalType& v_array) const
  {
    vtkm::Id4 idx = indices.Get(i);
    vtkm::Id i1 = idx[1];
    vtkm::Id i2 = idx[2];
    vtkm::Id i3 = idx[3];

    vtkm::Vec3f_32 p1 = verts.Get(idx[1]);
    vtkm::Vec3f_32 p2 = verts.Get(idx[2]);
    vtkm::Vec3f_32 p3 = verts.Get(idx[3]);

    vtkm::Float32 s;
    vtkm::Vec<float, 3> color1;
    vtkm::Vec<float, 3> color2;
    vtkm::Vec<float, 3> color3;

    if (SDiff == 0)
    {
      s = 0;
      color1 = ColorTable->MapThroughColorSpace(s);
      color2 = ColorTable->MapThroughColorSpace(s);
      color3 = ColorTable->MapThroughColorSpace(s);
    }
    else
    {
      s = scalar.Get(i1);
      s = (s - SMin) / SDiff;
      color1 = ColorTable->MapThroughColorSpace(s);

      s = scalar.Get(i2);
      s = (s - SMin) / SDiff;
      color2 = ColorTable->MapThroughColorSpace(s);

      s = scalar.Get(i3);
      s = (s - SMin) / SDiff;
      color3 = ColorTable->MapThroughColorSpace(s);
    }

    const vtkm::Id offset = 9;

    v_array.Set(i * offset, p1[0]);
    v_array.Set(i * offset + 1, p1[1]);
    v_array.Set(i * offset + 2, p1[2]);
    c_array.Set(i * offset, color1[0]);
    c_array.Set(i * offset + 1, color1[1]);
    c_array.Set(i * offset + 2, color1[2]);

    v_array.Set(i * offset + 3, p2[0]);
    v_array.Set(i * offset + 4, p2[1]);
    v_array.Set(i * offset + 5, p2[2]);
    c_array.Set(i * offset + 3, color2[0]);
    c_array.Set(i * offset + 4, color2[1]);
    c_array.Set(i * offset + 5, color2[2]);

    v_array.Set(i * offset + 6, p3[0]);
    v_array.Set(i * offset + 7, p3[1]);
    v_array.Set(i * offset + 8, p3[2]);
    c_array.Set(i * offset + 6, color3[0]);
    c_array.Set(i * offset + 7, color3[1]);
    c_array.Set(i * offset + 8, color3[2]);
  }
};

template <typename PtType>
struct MapColorAndVerticesInvokeFunctor
{
  vtkm::cont::ArrayHandle<vtkm::Id4> TriangleIndices;
  vtkm::cont::ColorTable ColorTable;
  const vtkm::cont::ArrayHandle<vtkm::Float32> Scalar;
  const vtkm::Range ScalarRange;
  const PtType Vertices;
  vtkm::Float32 SMin;
  vtkm::Float32 SDiff;
  vtkm::cont::ArrayHandle<vtkm::Float32> OutColor;
  vtkm::cont::ArrayHandle<vtkm::Float32> OutVertices;

  VTKM_CONT
  MapColorAndVerticesInvokeFunctor(const vtkm::cont::ArrayHandle<vtkm::Id4>& indices,
                                   const vtkm::cont::ColorTable& colorTable,
                                   const vtkm::cont::ArrayHandle<Float32>& scalar,
                                   const vtkm::Range& scalarRange,
                                   const PtType& vertices,
                                   vtkm::Float32 s_min,
                                   vtkm::Float32 s_max,
                                   vtkm::cont::ArrayHandle<Float32>& out_color,
                                   vtkm::cont::ArrayHandle<Float32>& out_vertices)
    : TriangleIndices(indices)
    , ColorTable(colorTable)
    , Scalar(scalar)
    , ScalarRange(scalarRange)
    , Vertices(vertices)
    , SMin(s_min)
    , SDiff(s_max - s_min)
    , OutColor(out_color)
    , OutVertices(out_vertices)
  {
  }

  template <typename Device>
  VTKM_CONT bool operator()(Device device) const
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);


    MapColorAndVertices worklet(
      this->ColorTable.PrepareForExecution(device), this->SMin, this->SDiff);
    vtkm::worklet::DispatcherMapField<MapColorAndVertices, Device> dispatcher(worklet);

    vtkm::cont::ArrayHandleIndex indexArray(this->TriangleIndices.GetNumberOfValues());
    dispatcher.Invoke(indexArray,
                      this->TriangleIndices,
                      this->Scalar,
                      this->Vertices,
                      this->OutColor,
                      this->OutVertices);
    return true;
  }
};

template <typename PtType>
VTKM_CONT void RenderStructuredLineSegments(vtkm::Id numVerts,
                                            const PtType& verts,
                                            const vtkm::cont::ArrayHandle<vtkm::Float32>& scalar,
                                            vtkm::cont::ColorTable ct,
                                            bool logY)
{
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
  glLineWidth(1);
  vtkm::UInt8 r, g, b, a;

  //This is horrible as the color table API isn't designed for users to query
  //on a single value basis. We use the GetPoint and GetPointAlpha escape hatches
  //and manually convert from float to uchar
  vtkm::Vec<double, 4> data;
  ct.GetPoint(0, data);
  r = static_cast<vtkm::UInt8>(data[1] * 255.0 + 0.5);
  g = static_cast<vtkm::UInt8>(data[2] * 255.0 + 0.5);
  b = static_cast<vtkm::UInt8>(data[3] * 255.0 + 0.5);
  ct.GetPointAlpha(0, data);
  a = static_cast<vtkm::UInt8>(data[1] * 255.0 + 0.5);

  glColor4ub(r, g, b, a);
  glBegin(GL_LINE_STRIP);
  for (int i = 0; i < numVerts; i++)
  {
    vtkm::Vec3f_32 pt = verts.GetPortalConstControl().Get(i);
    vtkm::Float32 s = scalar.GetPortalConstControl().Get(i);
    if (logY)
      s = vtkm::Float32(log10(s));
    glVertex3f(pt[0], s, 0.0f);
  }
  glEnd();
}

template <typename PtType>
VTKM_CONT void RenderExplicitLineSegments(vtkm::Id numVerts,
                                          const PtType& verts,
                                          const vtkm::cont::ArrayHandle<vtkm::Float32>& scalar,
                                          vtkm::cont::ColorTable ct,
                                          bool logY)
{
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
  glLineWidth(1);
  vtkm::UInt8 r, g, b, a;
  //This is horrible as the color table API isn't designed for users to query
  //on a single value basis. We use the GetPoint and GetPointAlpha escape hatches
  //and manually convert from float to uchar
  vtkm::Vec<double, 4> data;
  ct.GetPoint(0, data);
  r = static_cast<vtkm::UInt8>(data[1] * 255.0 + 0.5);
  g = static_cast<vtkm::UInt8>(data[2] * 255.0 + 0.5);
  b = static_cast<vtkm::UInt8>(data[3] * 255.0 + 0.5);
  ct.GetPointAlpha(0, data);
  a = static_cast<vtkm::UInt8>(data[1] * 255.0 + 0.5);

  glColor4ub(r, g, b, a);
  glBegin(GL_LINE_STRIP);
  for (int i = 0; i < numVerts; i++)
  {
    vtkm::Vec3f_32 pt = verts.GetPortalConstControl().Get(i);
    vtkm::Float32 s = scalar.GetPortalConstControl().Get(i);
    if (logY)
      s = vtkm::Float32(log10(s));
    glVertex3f(pt[0], s, 0.0f);
  }
  glEnd();
}

template <typename PtType>
VTKM_CONT void RenderTriangles(MapperGL& mapper,
                               vtkm::Id numTri,
                               const PtType& verts,
                               const vtkm::cont::ArrayHandle<vtkm::Id4>& indices,
                               const vtkm::cont::ArrayHandle<vtkm::Float32>& scalar,
                               const vtkm::cont::ColorTable& ct,
                               const vtkm::Range& scalarRange,
                               const vtkm::rendering::Camera& camera)
{
  if (!mapper.loaded)
  {
    // The glewExperimental global switch can be  turned on by setting it to
    // GL_TRUE before calling glewInit(), which  ensures that all extensions
    // with valid entry points will be exposed. This is needed as the glut
    // context that is being created is not a valid 'core' context but
    // instead a 'compatibility' context
    //
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();
    if (GlewInitResult)
      std::cerr << "ERROR: " << glewGetErrorString(GlewInitResult) << std::endl;
    mapper.loaded = true;

    vtkm::Float32 sMin = vtkm::Float32(scalarRange.Min);
    vtkm::Float32 sMax = vtkm::Float32(scalarRange.Max);
    vtkm::cont::ArrayHandle<vtkm::Float32> out_vertices, out_color;
    out_vertices.Allocate(9 * indices.GetNumberOfValues());
    out_color.Allocate(9 * indices.GetNumberOfValues());

    vtkm::cont::TryExecute(MapColorAndVerticesInvokeFunctor<PtType>(
      indices, ct, scalar, scalarRange, verts, sMin, sMax, out_color, out_vertices));


    vtkm::Id vtx_cnt = out_vertices.GetNumberOfValues();
    vtkm::Float32* v_ptr = out_vertices.GetStorage().GetArray();
    vtkm::Float32* c_ptr = out_color.GetStorage().GetArray();

    vtkm::Id floatSz = static_cast<vtkm::Id>(sizeof(vtkm::Float32));
    GLsizeiptr sz = static_cast<GLsizeiptr>(vtx_cnt * floatSz);

    GLuint points_vbo = 0;
    glGenBuffers(1, &points_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
    glBufferData(GL_ARRAY_BUFFER, sz, v_ptr, GL_STATIC_DRAW);

    GLuint colours_vbo = 0;
    glGenBuffers(1, &colours_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, colours_vbo);
    glBufferData(GL_ARRAY_BUFFER, sz, c_ptr, GL_STATIC_DRAW);

    mapper.vao = 0;
    glGenVertexArrays(1, &mapper.vao);
    glBindVertexArray(mapper.vao);
    glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindBuffer(GL_ARRAY_BUFFER, colours_vbo);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    const char* vertex_shader = "#version 120\n"
                                "attribute vec3 vertex_position;"
                                "attribute vec3 vertex_color;"
                                "varying vec3 ourColor;"
                                "uniform mat4 mv_matrix;"
                                "uniform mat4 p_matrix;"

                                "void main() {"
                                "  gl_Position = p_matrix*mv_matrix * vec4(vertex_position, 1.0);"
                                "  ourColor = vertex_color;"
                                "}";
    const char* fragment_shader = "#version 120\n"
                                  "varying vec3 ourColor;"
                                  "void main() {"
                                  "  gl_FragColor = vec4 (ourColor, 1.0);"
                                  "}";

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertex_shader, nullptr);
    glCompileShader(vs);
    GLint isCompiled = 0;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE)
    {
      GLint maxLength = 0;
      glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &maxLength);

      std::string msg;
      if (maxLength <= 0)
        msg = "No error message";
      else
      {
        // The maxLength includes the nullptr character
        GLchar* strInfoLog = new GLchar[maxLength + 1];
        glGetShaderInfoLog(vs, maxLength, &maxLength, strInfoLog);
        msg = std::string(strInfoLog);
        delete[] strInfoLog;
      }
      throw vtkm::cont::ErrorBadValue("Shader compile error:" + msg);
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragment_shader, nullptr);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE)
    {
      GLint maxLength = 0;
      glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &maxLength);

      std::string msg;
      if (maxLength <= 0)
        msg = "No error message";
      else
      {
        // The maxLength includes the nullptr character
        GLchar* strInfoLog = new GLchar[maxLength + 1];
        glGetShaderInfoLog(vs, maxLength, &maxLength, strInfoLog);
        msg = std::string(strInfoLog);
        delete[] strInfoLog;
      }
      throw vtkm::cont::ErrorBadValue("Shader compile error:" + msg);
    }

    mapper.shader_programme = glCreateProgram();
    if (mapper.shader_programme > 0)
    {
      glAttachShader(mapper.shader_programme, fs);
      glAttachShader(mapper.shader_programme, vs);
      glBindAttribLocation(mapper.shader_programme, 0, "vertex_position");
      glBindAttribLocation(mapper.shader_programme, 1, "vertex_color");

      glLinkProgram(mapper.shader_programme);
      GLint linkStatus;
      glGetProgramiv(mapper.shader_programme, GL_LINK_STATUS, &linkStatus);
      if (!linkStatus)
      {
        char log[2048];
        GLsizei len;
        glGetProgramInfoLog(mapper.shader_programme, 2048, &len, log);
        std::string msg = std::string("Shader program link failed: ") + std::string(log);
        throw vtkm::cont::ErrorBadValue(msg);
      }
    }
  }

  if (mapper.shader_programme > 0)
  {
    vtkm::Id width = mapper.GetCanvas()->GetWidth();
    vtkm::Id height = mapper.GetCanvas()->GetWidth();
    vtkm::Matrix<vtkm::Float32, 4, 4> viewM = camera.CreateViewMatrix();
    vtkm::Matrix<vtkm::Float32, 4, 4> projM = camera.CreateProjectionMatrix(width, height);

    MatrixHelpers::CreateOGLMatrix(viewM, mapper.mvMat);
    MatrixHelpers::CreateOGLMatrix(projM, mapper.pMat);

    glUseProgram(mapper.shader_programme);
    GLint mvID = glGetUniformLocation(mapper.shader_programme, "mv_matrix");
    glUniformMatrix4fv(mvID, 1, GL_FALSE, mapper.mvMat);
    GLint pID = glGetUniformLocation(mapper.shader_programme, "p_matrix");
    glUniformMatrix4fv(pID, 1, GL_FALSE, mapper.pMat);
    glBindVertexArray(mapper.vao);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(numTri * 3));
    glUseProgram(0);
  }
}

} // anonymous namespace

MapperGL::MapperGL()
  : Canvas(nullptr)
  , loaded(false)
{
}

MapperGL::~MapperGL()
{
}

void MapperGL::RenderCells(const vtkm::cont::DynamicCellSet& cellset,
                           const vtkm::cont::CoordinateSystem& coords,
                           const vtkm::cont::Field& scalarField,
                           const vtkm::cont::ColorTable& colorTable,
                           const vtkm::rendering::Camera& camera,
                           const vtkm::Range& scalarRange)
{
  vtkm::cont::ArrayHandle<vtkm::Float32> sf;
  sf = scalarField.GetData().Cast<vtkm::cont::ArrayHandle<vtkm::Float32>>();
  auto dcoords = coords.GetData();
  vtkm::Id numVerts = coords.GetNumberOfPoints();

  //Handle 1D cases.
  if (cellset.IsSameType(vtkm::cont::CellSetStructured<1>()))
  {
    vtkm::cont::ArrayHandleUniformPointCoordinates verts;
    verts = dcoords.Cast<vtkm::cont::ArrayHandleUniformPointCoordinates>();
    RenderStructuredLineSegments(numVerts, verts, sf, colorTable, this->LogarithmY);
  }
  else if (cellset.IsSameType(vtkm::cont::CellSetSingleType<>()) &&
           cellset.Cast<vtkm::cont::CellSetSingleType<>>().GetCellShapeAsId() ==
             vtkm::CELL_SHAPE_LINE)
  {
    vtkm::cont::ArrayHandle<vtkm::Vec3f_32> verts;
    verts = dcoords.Cast<vtkm::cont::ArrayHandle<vtkm::Vec3f_32>>();
    RenderExplicitLineSegments(numVerts, verts, sf, colorTable, this->LogarithmY);
  }
  else
  {
    vtkm::cont::ArrayHandle<vtkm::Id4> indices;
    vtkm::Id numTri;
    vtkm::rendering::internal::RunTriangulator(cellset, indices, numTri);

    vtkm::cont::ArrayHandleUniformPointCoordinates uVerts;
    vtkm::cont::ArrayHandle<vtkm::Vec3f_32> eVerts;

    if (dcoords.IsType<vtkm::cont::ArrayHandleUniformPointCoordinates>())
    {
      uVerts = dcoords.Cast<vtkm::cont::ArrayHandleUniformPointCoordinates>();
      RenderTriangles(*this, numTri, uVerts, indices, sf, colorTable, scalarRange, camera);
    }
    else if (dcoords.IsType<vtkm::cont::ArrayHandle<vtkm::Vec3f_32>>())
    {
      eVerts = dcoords.Cast<vtkm::cont::ArrayHandle<vtkm::Vec3f_32>>();
      RenderTriangles(*this, numTri, eVerts, indices, sf, colorTable, scalarRange, camera);
    }
    else if (dcoords.IsType<vtkm::cont::ArrayHandleCartesianProduct<
               vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
               vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
               vtkm::cont::ArrayHandle<vtkm::FloatDefault>>>())
    {
      vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                              vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                              vtkm::cont::ArrayHandle<vtkm::FloatDefault>>
        rVerts;
      rVerts = dcoords.Cast<
        vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                                vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                                vtkm::cont::ArrayHandle<vtkm::FloatDefault>>>();
      RenderTriangles(*this, numTri, rVerts, indices, sf, colorTable, scalarRange, camera);
    }
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

void MapperGL::SetCanvas(vtkm::rendering::Canvas* c)
{
  if (c != nullptr)
  {
    this->Canvas = dynamic_cast<vtkm::rendering::CanvasGL*>(c);
    if (this->Canvas == nullptr)
      throw vtkm::cont::ErrorBadValue("Bad canvas type for MapperGL. Must be CanvasGL");
  }
}

vtkm::rendering::Canvas* MapperGL::GetCanvas() const
{
  return this->Canvas;
}

vtkm::rendering::Mapper* MapperGL::NewCopy() const
{
  return new vtkm::rendering::MapperGL(*this);
}
}
} // namespace vtkm::rendering
