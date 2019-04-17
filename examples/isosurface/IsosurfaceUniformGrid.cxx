//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/MarchingCubes.h>
#include <vtkm/worklet/DispatcherMapField.h>

#include <vtkm/Math.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Initialize.h>

//Suppress warnings about glut being deprecated on OSX
#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#if defined(__APPLE__)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "quaternion.h"

#include <vector>

static vtkm::Id3 dims(256, 256, 256);
static vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>> verticesArray, normalsArray;
static vtkm::cont::ArrayHandle<vtkm::Float32> scalarsArray;
static Quaternion qrot;
static int lastx, lasty;
static int mouse_state = 1;

namespace
{

// Define the tangle field for the input data
class TangleField : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn vertexId, FieldOut v);
  using ExecutionSignature = void(_1, _2);
  using InputDomain = _1;

  const vtkm::Id xdim, ydim, zdim;
  const vtkm::Float32 xmin, ymin, zmin, xmax, ymax, zmax;
  const vtkm::Id cellsPerLayer;

  VTKM_CONT
  TangleField(const vtkm::Id3 dims, const vtkm::Float32 mins[3], const vtkm::Float32 maxs[3])
    : xdim(dims[0])
    , ydim(dims[1])
    , zdim(dims[2])
    , xmin(mins[0])
    , ymin(mins[1])
    , zmin(mins[2])
    , xmax(maxs[0])
    , ymax(maxs[1])
    , zmax(maxs[2])
    , cellsPerLayer((xdim) * (ydim)){};

  VTKM_EXEC
  void operator()(const vtkm::Id& vertexId, vtkm::Float32& v) const
  {
    const vtkm::Id x = vertexId % (xdim);
    const vtkm::Id y = (vertexId / (xdim)) % (ydim);
    const vtkm::Id z = vertexId / cellsPerLayer;

    const vtkm::Float32 fx = static_cast<vtkm::Float32>(x) / static_cast<vtkm::Float32>(xdim - 1);
    const vtkm::Float32 fy = static_cast<vtkm::Float32>(y) / static_cast<vtkm::Float32>(xdim - 1);
    const vtkm::Float32 fz = static_cast<vtkm::Float32>(z) / static_cast<vtkm::Float32>(xdim - 1);

    const vtkm::Float32 xx = 3.0f * (xmin + (xmax - xmin) * (fx));
    const vtkm::Float32 yy = 3.0f * (ymin + (ymax - ymin) * (fy));
    const vtkm::Float32 zz = 3.0f * (zmin + (zmax - zmin) * (fz));

    v = (xx * xx * xx * xx - 5.0f * xx * xx + yy * yy * yy * yy - 5.0f * yy * yy +
         zz * zz * zz * zz - 5.0f * zz * zz + 11.8f) *
        0.2f +
      0.5f;
  }
};

// Construct an input data set using the tangle field worklet
vtkm::cont::DataSet MakeIsosurfaceTestDataSet(vtkm::Id3 dims)
{
  vtkm::cont::DataSet dataSet;

  const vtkm::Id3 vdims(dims[0] + 1, dims[1] + 1, dims[2] + 1);

  vtkm::Float32 mins[3] = { -1.0f, -1.0f, -1.0f };
  vtkm::Float32 maxs[3] = { 1.0f, 1.0f, 1.0f };

  vtkm::cont::ArrayHandle<vtkm::Float32> fieldArray;
  vtkm::cont::ArrayHandleCounting<vtkm::Id> vertexCountImplicitArray(
    0, 1, vdims[0] * vdims[1] * vdims[2]);
  vtkm::worklet::DispatcherMapField<TangleField> tangleFieldDispatcher(
    TangleField(vdims, mins, maxs));
  tangleFieldDispatcher.Invoke(vertexCountImplicitArray, fieldArray);

  vtkm::Vec<vtkm::FloatDefault, 3> origin(0.0f, 0.0f, 0.0f);
  vtkm::Vec<vtkm::FloatDefault, 3> spacing(1.0f / static_cast<vtkm::FloatDefault>(dims[0]),
                                           1.0f / static_cast<vtkm::FloatDefault>(dims[2]),
                                           1.0f / static_cast<vtkm::FloatDefault>(dims[1]));

  vtkm::cont::ArrayHandleUniformPointCoordinates coordinates(vdims, origin, spacing);
  dataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", coordinates));

  dataSet.AddField(
    vtkm::cont::Field("nodevar", vtkm::cont::Field::Association::POINTS, fieldArray));

  static const vtkm::IdComponent ndim = 3;
  vtkm::cont::CellSetStructured<ndim> cellSet("cells");
  cellSet.SetPointDimensions(vdims);
  dataSet.AddCellSet(cellSet);

  return dataSet;
}
}

// Initialize the OpenGL state
void initializeGL()
{
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glEnable(GL_DEPTH_TEST);
  glShadeModel(GL_SMOOTH);

  vtkm::Float32 white[] = { 0.8f, 0.8f, 0.8f, 1.0f };
  vtkm::Float32 black[] = { 0.0f, 0.0f, 0.0f, 1.0f };
  vtkm::Float32 lightPos[] = { 10.0f, 10.0f, 10.5f, 1.0f };

  glLightfv(GL_LIGHT0, GL_AMBIENT, white);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
  glLightfv(GL_LIGHT0, GL_SPECULAR, black);
  glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_NORMALIZE);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  glEnable(GL_COLOR_MATERIAL);
}

// Render the output using simple OpenGL
void displayCall()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(45.0f, 1.0f, 1.0f, 20.0f);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

  glPushMatrix();
  float rotationMatrix[16];
  qrot.getRotMat(rotationMatrix);
  glMultMatrixf(rotationMatrix);
  glTranslatef(-0.5f, -0.5f, -0.5f);

  glColor3f(0.1f, 0.1f, 0.6f);

  glBegin(GL_TRIANGLES);
  for (vtkm::IdComponent i = 0; i < verticesArray.GetNumberOfValues(); i++)
  {
    vtkm::Vec<vtkm::Float32, 3> curNormal = normalsArray.GetPortalConstControl().Get(i);
    vtkm::Vec<vtkm::Float32, 3> curVertex = verticesArray.GetPortalConstControl().Get(i);
    glNormal3f(curNormal[0], curNormal[1], curNormal[2]);
    glVertex3f(curVertex[0], curVertex[1], curVertex[2]);
  }
  glEnd();

  glPopMatrix();
  glutSwapBuffers();
}

// Allow rotations of the view
void mouseMove(int x, int y)
{
  vtkm::Float32 dx = static_cast<vtkm::Float32>(x - lastx);
  vtkm::Float32 dy = static_cast<vtkm::Float32>(y - lasty);

  if (mouse_state == 0)
  {
    Quaternion newRotX;
    newRotX.setEulerAngles(-0.2f * dx * vtkm::Pi_180f(), 0.0f, 0.0f);
    qrot.mul(newRotX);

    Quaternion newRotY;
    newRotY.setEulerAngles(0.0f, 0.0f, -0.2f * dy * vtkm::Pi_180f());
    qrot.mul(newRotY);
  }
  lastx = x;
  lasty = y;

  glutPostRedisplay();
}

// Respond to mouse button
void mouseCall(int button, int state, int x, int y)
{
  if (button == 0)
    mouse_state = state;
  if ((button == 0) && (state == 0))
  {
    lastx = x;
    lasty = y;
  }
}

// Compute and render an isosurface for a uniform grid example
int main(int argc, char* argv[])
{
  //parse out all vtk-m related command line options
  auto opts =
    vtkm::cont::InitializeOptions::DefaultAnyDevice | vtkm::cont::InitializeOptions::Strict;
  vtkm::cont::Initialize(argc, argv, opts);

  vtkm::cont::DataSet dataSet = MakeIsosurfaceTestDataSet(dims);

  vtkm::filter::MarchingCubes filter;
  filter.SetGenerateNormals(true);
  filter.SetMergeDuplicatePoints(false);
  filter.SetIsoValue(0, 0.1);
  filter.SetActiveField("nodevar");
  filter.SetFieldsToPass({ "nodevar" });

  vtkm::cont::DataSet outputData = filter.Execute(dataSet);

  //need to extract vertices, normals, and scalars
  using VertType = vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>>;
  vtkm::cont::CoordinateSystem coords = outputData.GetCoordinateSystem();

  verticesArray = coords.GetData().Cast<VertType>();
  normalsArray = outputData.GetField("normals").GetData().Cast<VertType>();
  scalarsArray =
    outputData.GetField("nodevar").GetData().Cast<vtkm::cont::ArrayHandle<vtkm::Float32>>();

  std::cout << "Number of output vertices: " << verticesArray.GetNumberOfValues() << std::endl;

  std::cout << "vertices: ";
  vtkm::cont::printSummary_ArrayHandle(verticesArray, std::cout);
  std::cout << std::endl;
  std::cout << "normals: ";
  vtkm::cont::printSummary_ArrayHandle(normalsArray, std::cout);
  std::cout << std::endl;
  std::cout << "scalars: ";
  vtkm::cont::printSummary_ArrayHandle(scalarsArray, std::cout);
  std::cout << std::endl;

  lastx = lasty = 0;
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  glutInitWindowSize(1000, 1000);
  glutCreateWindow("VTK-m Isosurface");
  initializeGL();
  glutDisplayFunc(displayCall);
  glutMotionFunc(mouseMove);
  glutMouseFunc(mouseCall);
  glutMainLoop();

  verticesArray.ReleaseResources();
  normalsArray.ReleaseResources();
  scalarsArray.ReleaseResources();
  return 0;
}

#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic pop
#endif
