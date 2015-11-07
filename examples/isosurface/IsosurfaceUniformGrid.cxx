//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

//We first check if VTKM_DEVICE_ADAPTER is defined, so that when TBB and CUDA
//includes this file we use the device adapter that they have set.
#ifndef VTKM_DEVICE_ADAPTER
#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_SERIAL
#endif

#include <vtkm/worklet/IsosurfaceUniformGrid.h>
#include <vtkm/worklet/DispatcherMapField.h>

#include <vtkm/Math.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/DataSet.h>

//Suppress warnings about glut being deprecated on OSX
#if (defined(VTKM_GCC) || defined(VTKM_CLANG)) && !defined(VTKM_PGI)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#if defined (__APPLE__)
# include <GLUT/glut.h>
#else
# include <GL/glut.h>
#endif

#include "quaternion.h"

#include <vector>

typedef VTKM_DEFAULT_DEVICE_ADAPTER_TAG DeviceAdapter;

vtkm::Id3 dims(16,16,16);
vtkm::worklet::IsosurfaceFilterUniformGrid<vtkm::Float32, DeviceAdapter> *isosurfaceFilter;
vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32,3> > verticesArray, normalsArray;
vtkm::cont::ArrayHandle<vtkm::Float32> scalarsArray;
Quaternion qrot;
int lastx, lasty;
int mouse_state = 1;

namespace {

// Define the tangle field for the input data
class TangleField : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<IdType> vertexId, FieldOut<Scalar> v);
  typedef void ExecutionSignature(_1, _2);
  typedef _1 InputDomain;

  const vtkm::Id xdim, ydim, zdim;
  const vtkm::FloatDefault xmin, ymin, zmin, xmax, ymax, zmax;
  const vtkm::Id cellsPerLayer;

  VTKM_CONT_EXPORT
  TangleField(const vtkm::Id3 dims, const vtkm::FloatDefault mins[3], const vtkm::FloatDefault maxs[3]) : xdim(dims[0]), ydim(dims[1]), zdim(dims[2]),
              xmin(mins[0]), ymin(mins[1]), zmin(mins[2]), xmax(maxs[0]), ymax(maxs[1]), zmax(maxs[2]), cellsPerLayer((xdim) * (ydim)) { };

  VTKM_EXEC_EXPORT
  void operator()(const vtkm::Id &vertexId, vtkm::Float32 &v) const
  {
    const vtkm::Id x = vertexId % (xdim);
    const vtkm::Id y = (vertexId / (xdim)) % (ydim);
    const vtkm::Id z = vertexId / cellsPerLayer;

    const vtkm::FloatDefault fx = static_cast<vtkm::FloatDefault>(x) / static_cast<vtkm::FloatDefault>(xdim-1);
    const vtkm::FloatDefault fy = static_cast<vtkm::FloatDefault>(y) / static_cast<vtkm::FloatDefault>(xdim-1);
    const vtkm::FloatDefault fz = static_cast<vtkm::FloatDefault>(z) / static_cast<vtkm::FloatDefault>(xdim-1);

    const vtkm::Float32 xx = 3.0f*(xmin+(xmax-xmin)*(fx));
    const vtkm::Float32 yy = 3.0f*(ymin+(ymax-ymin)*(fy));
    const vtkm::Float32 zz = 3.0f*(zmin+(zmax-zmin)*(fz));

    v = (xx*xx*xx*xx - 5.0f*xx*xx + yy*yy*yy*yy - 5.0f*yy*yy + zz*zz*zz*zz - 5.0f*zz*zz + 11.8f) * 0.2f + 0.5f;
  }
};


// Construct an input data set using the tangle field worklet
vtkm::cont::DataSet MakeIsosurfaceTestDataSet(vtkm::Id3 dims)
{
  vtkm::cont::DataSet dataSet;

  const vtkm::Id3 vdims(dims[0] + 1, dims[1] + 1, dims[2] + 1);

  vtkm::FloatDefault mins[3] = {-1.0f, -1.0f, -1.0f};
  vtkm::FloatDefault maxs[3] = {1.0f, 1.0f, 1.0f};

  vtkm::cont::ArrayHandle<vtkm::Float32> fieldArray;
  vtkm::cont::ArrayHandleCounting<vtkm::Id> vertexCountImplicitArray(0, 1, vdims[0]*vdims[1]*vdims[2]);
  vtkm::worklet::DispatcherMapField<TangleField> tangleFieldDispatcher(TangleField(vdims, mins, maxs));
  tangleFieldDispatcher.Invoke(vertexCountImplicitArray, fieldArray);

  vtkm::Vec<vtkm::FloatDefault,3> origin(0.0f, 0.0f, 0.0f);
  vtkm::Vec<vtkm::FloatDefault,3> spacing(
        1.0f/static_cast<vtkm::FloatDefault>(dims[0]),
        1.0f/static_cast<vtkm::FloatDefault>(dims[2]),
        1.0f/static_cast<vtkm::FloatDefault>(dims[1]));

  vtkm::cont::ArrayHandleUniformPointCoordinates
      coordinates(vdims, origin, spacing);
  dataSet.AddCoordinateSystem(
          vtkm::cont::CoordinateSystem("coordinates", 1, coordinates));

  dataSet.AddField(vtkm::cont::Field("nodevar", 1, vtkm::cont::Field::ASSOC_POINTS, fieldArray));

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

  vtkm::FloatDefault white[] = { 0.8f, 0.8f, 0.8f, 1.0f };
  vtkm::FloatDefault black[] = { 0.0f, 0.0f, 0.0f, 1.0f };
  vtkm::FloatDefault lightPos[] = { 10.0f, 10.0f, 10.5f, 1.0f };

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
  gluPerspective( 45.0f, 1.0f, 1.0f, 20.0f);

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
  for (vtkm::IdComponent i=0; i<verticesArray.GetNumberOfValues(); i++)
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
    vtkm::Float32 pideg = static_cast<vtkm::Float32>(vtkm::Pi_2());
    Quaternion newRotX;
    newRotX.setEulerAngles(-0.2f*dx*pideg/180.0f, 0.0f, 0.0f);
    qrot.mul(newRotX);

    Quaternion newRotY;
    newRotY.setEulerAngles(0.0f, 0.0f, -0.2f*dy*pideg/180.0f);
    qrot.mul(newRotY);
  }
  lastx = x;
  lasty = y;

  glutPostRedisplay();
}


// Respond to mouse button
void mouseCall(int button, int state, int x, int y)
{
  if (button == 0) mouse_state = state;
  if ((button == 0) && (state == 0)) { lastx = x;  lasty = y; }
}


// Compute and render an isosurface for a uniform grid example
int main(int argc, char* argv[])
{
  typedef vtkm::cont::internal::DeviceAdapterTraits<DeviceAdapter>
                                                        DeviceAdapterTraits;
  std::cout << "Running IsosurfaceUniformGrid example on device adapter: "
            << DeviceAdapterTraits::GetId() << std::endl;

  vtkm::cont::DataSet dataSet = MakeIsosurfaceTestDataSet(dims);

  isosurfaceFilter = new vtkm::worklet::IsosurfaceFilterUniformGrid<vtkm::Float32, DeviceAdapter>(dims, dataSet);

  isosurfaceFilter->Run(0.5,
                        dataSet.GetField("nodevar").GetData(),
                        verticesArray,
                        normalsArray,
                        scalarsArray);

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

  return 0;
}

#if (defined(VTKM_GCC) || defined(VTKM_CLANG)) && !defined(VTKM_PGI)
# pragma GCC diagnostic pop
#endif
