//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifdef __APPLE__
// Glut is depricated on apple, but is sticking around for now. Hopefully
// someone will step up and make FreeGlut or OpenGlut compatible. Or perhaps
// we should move to GLFW. For now, just disable the warnings.
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include <vtkm/io/VTKDataSetReader.h>

#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperRayTracer.h>
#include <vtkm/rendering/View3D.h>

#include <vtkm/cont/testing/Testing.h>

#include <vtkm/internal/Windows.h>

#ifdef __APPLE__
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#include <GL/glut.h>
#endif

namespace
{

vtkm::rendering::View3D* gViewPointer = NULL;

int gButtonState[3] = { GLUT_UP, GLUT_UP, GLUT_UP };
int gMousePositionX;
int gMousePositionY;
bool gNoInteraction;

void DisplayCallback()
{
  vtkm::rendering::View3D& view = *gViewPointer;

  ////
  //// BEGIN-EXAMPLE RenderToOpenGL
  ////
  view.Paint();

  // Get the color buffer containing the rendered image.
  vtkm::cont::ArrayHandle<vtkm::Vec4f_32> colorBuffer =
    view.GetCanvas().GetColorBuffer();

  // Pull the C array out of the arrayhandle.
  const void* colorArray =
    vtkm::cont::ArrayHandleBasic<vtkm::Vec4f_32>(colorBuffer).GetReadPointer();

  // Write the C array to an OpenGL buffer.
  glDrawPixels((GLint)view.GetCanvas().GetWidth(),
               (GLint)view.GetCanvas().GetHeight(),
               GL_RGBA,
               GL_FLOAT,
               colorArray);

  // Swap the OpenGL buffers (system dependent).
  ////
  //// END-EXAMPLE RenderToOpenGL
  ////
  glutSwapBuffers();
  if (gNoInteraction)
  {
    delete gViewPointer;
    gViewPointer = NULL;
    exit(0);
  }
}

void WindowReshapeCallback(int width, int height)
{
  gViewPointer->GetCanvas().ResizeBuffers(width, height);
}

void MouseButtonCallback(int buttonIndex, int state, int x, int y)
{
  gButtonState[buttonIndex] = state;
  gMousePositionX = x;
  gMousePositionY = y;
}

////
//// BEGIN-EXAMPLE MouseRotate
////
void DoMouseRotate(vtkm::rendering::View& view,
                   vtkm::Id mouseStartX,
                   vtkm::Id mouseStartY,
                   vtkm::Id mouseEndX,
                   vtkm::Id mouseEndY)
{
  vtkm::Id screenWidth = view.GetCanvas().GetWidth();
  vtkm::Id screenHeight = view.GetCanvas().GetHeight();

  // Convert the mouse position coordinates, given in pixels from 0 to
  // width/height, to normalized screen coordinates from -1 to 1. Note that y
  // screen coordinates are usually given from the top down whereas our
  // geometry transforms are given from bottom up, so you have to reverse the y
  // coordiantes.
  vtkm::Float32 startX = (2.0f * mouseStartX) / screenWidth - 1.0f;
  vtkm::Float32 startY = -((2.0f * mouseStartY) / screenHeight - 1.0f);
  vtkm::Float32 endX = (2.0f * mouseEndX) / screenWidth - 1.0f;
  vtkm::Float32 endY = -((2.0f * mouseEndY) / screenHeight - 1.0f);

  view.GetCamera().TrackballRotate(startX, startY, endX, endY);
}
////
//// END-EXAMPLE MouseRotate
////

////
//// BEGIN-EXAMPLE MousePan
////
void DoMousePan(vtkm::rendering::View& view,
                vtkm::Id mouseStartX,
                vtkm::Id mouseStartY,
                vtkm::Id mouseEndX,
                vtkm::Id mouseEndY)
{
  vtkm::Id screenWidth = view.GetCanvas().GetWidth();
  vtkm::Id screenHeight = view.GetCanvas().GetHeight();

  // Convert the mouse position coordinates, given in pixels from 0 to
  // width/height, to normalized screen coordinates from -1 to 1. Note that y
  // screen coordinates are usually given from the top down whereas our
  // geometry transforms are given from bottom up, so you have to reverse the y
  // coordiantes.
  vtkm::Float32 startX = (2.0f * mouseStartX) / screenWidth - 1.0f;
  vtkm::Float32 startY = -((2.0f * mouseStartY) / screenHeight - 1.0f);
  vtkm::Float32 endX = (2.0f * mouseEndX) / screenWidth - 1.0f;
  vtkm::Float32 endY = -((2.0f * mouseEndY) / screenHeight - 1.0f);

  vtkm::Float32 deltaX = endX - startX;
  vtkm::Float32 deltaY = endY - startY;

  ////
  //// BEGIN-EXAMPLE Pan
  ////
  view.GetCamera().Pan(deltaX, deltaY);
  ////
  //// END-EXAMPLE Pan
  ////
}
////
//// END-EXAMPLE MousePan
////

////
//// BEGIN-EXAMPLE MouseZoom
////
void DoMouseZoom(vtkm::rendering::View& view, vtkm::Id mouseStartY, vtkm::Id mouseEndY)
{
  vtkm::Id screenHeight = view.GetCanvas().GetHeight();

  // Convert the mouse position coordinates, given in pixels from 0 to height,
  // to normalized screen coordinates from -1 to 1. Note that y screen
  // coordinates are usually given from the top down whereas our geometry
  // transforms are given from bottom up, so you have to reverse the y
  // coordiantes.
  vtkm::Float32 startY = -((2.0f * mouseStartY) / screenHeight - 1.0f);
  vtkm::Float32 endY = -((2.0f * mouseEndY) / screenHeight - 1.0f);

  vtkm::Float32 zoomFactor = endY - startY;

  ////
  //// BEGIN-EXAMPLE Zoom
  ////
  view.GetCamera().Zoom(zoomFactor);
  ////
  //// END-EXAMPLE Zoom
  ////
}
////
//// END-EXAMPLE MouseZoom
////

void MouseMoveCallback(int x, int y)
{
  if (gButtonState[0] == GLUT_DOWN)
  {
    DoMouseRotate(*gViewPointer, gMousePositionX, gMousePositionY, x, y);
  }
  else if (gButtonState[1] == GLUT_DOWN)
  {
    DoMousePan(*gViewPointer, gMousePositionX, gMousePositionY, x, y);
  }
  else if (gButtonState[2] == GLUT_DOWN)
  {
    DoMouseZoom(*gViewPointer, gMousePositionY, y);
  }

  gMousePositionX = x;
  gMousePositionY = y;

  glutPostRedisplay();
}

void SaveImage()
{
  std::cout << "Saving image." << std::endl;

  vtkm::rendering::Canvas& canvas = gViewPointer->GetCanvas();

  ////
  //// BEGIN-EXAMPLE SaveCanvasImage
  ////
  canvas.SaveAs("MyVis.ppm");
  ////
  //// END-EXAMPLE SaveCanvasImage
  ////
}

////
//// BEGIN-EXAMPLE ResetCamera
////
void ResetCamera(vtkm::rendering::View& view)
{
  vtkm::Bounds bounds = view.GetScene().GetSpatialBounds();
  view.GetCamera().ResetToBounds(bounds);
  //// PAUSE-EXAMPLE
  std::cout << "Position:  " << view.GetCamera().GetPosition() << std::endl;
  std::cout << "LookAt:    " << view.GetCamera().GetLookAt() << std::endl;
  std::cout << "ViewUp:    " << view.GetCamera().GetViewUp() << std::endl;
  std::cout << "FOV:       " << view.GetCamera().GetFieldOfView() << std::endl;
  std::cout << "ClipRange: " << view.GetCamera().GetClippingRange() << std::endl;
  //// RESUME-EXAMPLE
}
////
//// END-EXAMPLE ResetCamera
////

void ChangeCamera(vtkm::rendering::Camera& camera)
{
  // Just set some camera parameters for demonstration purposes.
  ////
  //// BEGIN-EXAMPLE CameraPositionOrientation
  ////
  camera.SetPosition(vtkm::make_Vec(10.0, 6.0, 6.0));
  camera.SetLookAt(vtkm::make_Vec(0.0, 0.0, 0.0));
  camera.SetViewUp(vtkm::make_Vec(0.0, 1.0, 0.0));
  camera.SetFieldOfView(60.0);
  camera.SetClippingRange(0.1, 100.0);
  ////
  //// END-EXAMPLE CameraPositionOrientation
  ////
}

void ObliqueCamera(vtkm::rendering::View& view)
{
  ////
  //// BEGIN-EXAMPLE AxisAlignedCamera
  ////
  view.GetCamera().SetPosition(vtkm::make_Vec(0.0, 0.0, 0.0));
  view.GetCamera().SetLookAt(vtkm::make_Vec(0.0, 0.0, -1.0));
  view.GetCamera().SetViewUp(vtkm::make_Vec(0.0, 1.0, 0.0));
  vtkm::Bounds bounds = view.GetScene().GetSpatialBounds();
  view.GetCamera().ResetToBounds(bounds);
  ////
  //// END-EXAMPLE AxisAlignedCamera
  ////
  ////
  //// BEGIN-EXAMPLE CameraMovement
  ////
  view.GetCamera().Azimuth(45.0);
  view.GetCamera().Elevation(45.0);
  ////
  //// END-EXAMPLE CameraMovement
  ////
}

void KeyPressCallback(unsigned char key, int x, int y)
{
  switch (key)
  {
    case 'q':
    case 'Q':
      delete gViewPointer;
      gViewPointer = NULL;
      exit(0);
      break;
    case 's':
    case 'S':
      SaveImage();
      break;
    case 'r':
    case 'R':
      ResetCamera(*gViewPointer);
      break;
    case 'c':
    case 'C':
      ChangeCamera(gViewPointer->GetCamera());
      break;
    case 'o':
    case 'O':
      ObliqueCamera(*gViewPointer);
  }
  glutPostRedisplay();
  (void)x;
  (void)y;
}

int go()
{
  // Initialize VTK-m rendering classes
  vtkm::cont::DataSet surfaceData;
  try
  {
    vtkm::io::VTKDataSetReader reader(
      vtkm::cont::testing::Testing::GetTestDataBasePath() + "unstructured/cow.vtk");
    surfaceData = reader.ReadDataSet();
  }
  catch (vtkm::io::ErrorIO& error)
  {
    std::cout << "Could not read file:" << std::endl << error.GetMessage() << std::endl;
    exit(1);
  }
  catch (...)
  {
    throw;
  }

  ////
  //// BEGIN-EXAMPLE SpecifyColorTable
  ////
  vtkm::rendering::Actor actor(surfaceData.GetCellSet(),
                               surfaceData.GetCoordinateSystem(),
                               surfaceData.GetField("RandomPointScalars"),
                               vtkm::cont::ColorTable("inferno"));
  ////
  //// END-EXAMPLE SpecifyColorTable
  ////

  vtkm::rendering::Scene scene;
  scene.AddActor(actor);

  vtkm::rendering::MapperRayTracer mapper;
  vtkm::rendering::CanvasRayTracer canvas;

  gViewPointer = new vtkm::rendering::View3D(scene, mapper, canvas);

  // Start the GLUT rendering system. This function typically does not return.
  glutMainLoop();

  return 0;
}

int doMain(int argc, char* argv[])
{
  // Initialize GLUT window and callbacks
  glutInit(&argc, argv);
  glutInitWindowSize(960, 600);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH);
  glutCreateWindow("VTK-m Example");

  glutDisplayFunc(DisplayCallback);
  glutReshapeFunc(WindowReshapeCallback);
  glutMouseFunc(MouseButtonCallback);
  glutMotionFunc(MouseMoveCallback);
  glutKeyboardFunc(KeyPressCallback);

  gNoInteraction = false;
  for (int arg = 1; arg < argc; ++arg)
  {
    if (strcmp(argv[arg], "--no-interaction") == 0)
    {
      gNoInteraction = true;
    }
  }

  return vtkm::cont::testing::Testing::Run(go, argc, argv);
}

} // anonymous namespace

int GuideExampleRenderingInteractive(int argc, char* argv[])
{
  return doMain(argc, argv);
}
