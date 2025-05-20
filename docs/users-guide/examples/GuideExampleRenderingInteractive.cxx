//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

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

#include <viskores/io/VTKDataSetReader.h>

#include <viskores/rendering/Actor.h>
#include <viskores/rendering/Camera.h>
#include <viskores/rendering/CanvasRayTracer.h>
#include <viskores/rendering/MapperRayTracer.h>
#include <viskores/rendering/View3D.h>

#include <viskores/cont/testing/Testing.h>

#include <viskores/internal/Windows.h>

#ifdef __APPLE__
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#include <GL/glut.h>
#endif

namespace
{

viskores::rendering::View3D* gViewPointer = NULL;

int gButtonState[3] = { GLUT_UP, GLUT_UP, GLUT_UP };
int gMousePositionX;
int gMousePositionY;
bool gNoInteraction;

void DisplayCallback()
{
  viskores::rendering::View3D& view = *gViewPointer;

  ////
  //// BEGIN-EXAMPLE RenderToOpenGL
  ////
  view.Paint();

  // Get the color buffer containing the rendered image.
  viskores::cont::ArrayHandle<viskores::Vec4f_32> colorBuffer =
    view.GetCanvas().GetColorBuffer();

  // Pull the C array out of the arrayhandle.
  const void* colorArray =
    viskores::cont::ArrayHandleBasic<viskores::Vec4f_32>(colorBuffer).GetReadPointer();

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
void DoMouseRotate(viskores::rendering::View& view,
                   viskores::Id mouseStartX,
                   viskores::Id mouseStartY,
                   viskores::Id mouseEndX,
                   viskores::Id mouseEndY)
{
  viskores::Id screenWidth = view.GetCanvas().GetWidth();
  viskores::Id screenHeight = view.GetCanvas().GetHeight();

  // Convert the mouse position coordinates, given in pixels from 0 to
  // width/height, to normalized screen coordinates from -1 to 1. Note that y
  // screen coordinates are usually given from the top down whereas our
  // geometry transforms are given from bottom up, so you have to reverse the y
  // coordiantes.
  viskores::Float32 startX = (2.0f * mouseStartX) / screenWidth - 1.0f;
  viskores::Float32 startY = -((2.0f * mouseStartY) / screenHeight - 1.0f);
  viskores::Float32 endX = (2.0f * mouseEndX) / screenWidth - 1.0f;
  viskores::Float32 endY = -((2.0f * mouseEndY) / screenHeight - 1.0f);

  view.GetCamera().TrackballRotate(startX, startY, endX, endY);
}
////
//// END-EXAMPLE MouseRotate
////

////
//// BEGIN-EXAMPLE MousePan
////
void DoMousePan(viskores::rendering::View& view,
                viskores::Id mouseStartX,
                viskores::Id mouseStartY,
                viskores::Id mouseEndX,
                viskores::Id mouseEndY)
{
  viskores::Id screenWidth = view.GetCanvas().GetWidth();
  viskores::Id screenHeight = view.GetCanvas().GetHeight();

  // Convert the mouse position coordinates, given in pixels from 0 to
  // width/height, to normalized screen coordinates from -1 to 1. Note that y
  // screen coordinates are usually given from the top down whereas our
  // geometry transforms are given from bottom up, so you have to reverse the y
  // coordiantes.
  viskores::Float32 startX = (2.0f * mouseStartX) / screenWidth - 1.0f;
  viskores::Float32 startY = -((2.0f * mouseStartY) / screenHeight - 1.0f);
  viskores::Float32 endX = (2.0f * mouseEndX) / screenWidth - 1.0f;
  viskores::Float32 endY = -((2.0f * mouseEndY) / screenHeight - 1.0f);

  viskores::Float32 deltaX = endX - startX;
  viskores::Float32 deltaY = endY - startY;

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
void DoMouseZoom(viskores::rendering::View& view,
                 viskores::Id mouseStartY,
                 viskores::Id mouseEndY)
{
  viskores::Id screenHeight = view.GetCanvas().GetHeight();

  // Convert the mouse position coordinates, given in pixels from 0 to height,
  // to normalized screen coordinates from -1 to 1. Note that y screen
  // coordinates are usually given from the top down whereas our geometry
  // transforms are given from bottom up, so you have to reverse the y
  // coordiantes.
  viskores::Float32 startY = -((2.0f * mouseStartY) / screenHeight - 1.0f);
  viskores::Float32 endY = -((2.0f * mouseEndY) / screenHeight - 1.0f);

  viskores::Float32 zoomFactor = endY - startY;

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

  viskores::rendering::Canvas& canvas = gViewPointer->GetCanvas();

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
void ResetCamera(viskores::rendering::View& view)
{
  viskores::Bounds bounds = view.GetScene().GetSpatialBounds();
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

void ChangeCamera(viskores::rendering::Camera& camera)
{
  // Just set some camera parameters for demonstration purposes.
  ////
  //// BEGIN-EXAMPLE CameraPositionOrientation
  ////
  camera.SetPosition(viskores::make_Vec(10.0, 6.0, 6.0));
  camera.SetLookAt(viskores::make_Vec(0.0, 0.0, 0.0));
  camera.SetViewUp(viskores::make_Vec(0.0, 1.0, 0.0));
  camera.SetFieldOfView(60.0);
  camera.SetClippingRange(0.1, 100.0);
  ////
  //// END-EXAMPLE CameraPositionOrientation
  ////
}

void ObliqueCamera(viskores::rendering::View& view)
{
  ////
  //// BEGIN-EXAMPLE AxisAlignedCamera
  ////
  view.GetCamera().SetPosition(viskores::make_Vec(0.0, 0.0, 0.0));
  view.GetCamera().SetLookAt(viskores::make_Vec(0.0, 0.0, -1.0));
  view.GetCamera().SetViewUp(viskores::make_Vec(0.0, 1.0, 0.0));
  viskores::Bounds bounds = view.GetScene().GetSpatialBounds();
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
  // Initialize Viskores rendering classes
  viskores::cont::DataSet surfaceData;
  try
  {
    viskores::io::VTKDataSetReader reader(
      viskores::cont::testing::Testing::GetTestDataBasePath() + "unstructured/cow.vtk");
    surfaceData = reader.ReadDataSet();
  }
  catch (viskores::io::ErrorIO& error)
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
  viskores::rendering::Actor actor(
    surfaceData, "RandomPointScalars", viskores::cont::ColorTable("inferno"));
  ////
  //// END-EXAMPLE SpecifyColorTable
  ////

  viskores::rendering::Scene scene;
  scene.AddActor(actor);

  viskores::rendering::MapperRayTracer mapper;
  viskores::rendering::CanvasRayTracer canvas;

  gViewPointer = new viskores::rendering::View3D(scene, mapper, canvas);

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
  glutCreateWindow("Viskores Example");

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

  return viskores::cont::testing::Testing::Run(go, argc, argv);
}

} // anonymous namespace

int GuideExampleRenderingInteractive(int argc, char* argv[])
{
  return doMain(argc, argv);
}
