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

#include <vtkm/Math.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>

//Suppress warnings about glut being deprecated on OSX
#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#if defined (__APPLE__)
# include <GLUT/glut.h>
#else
# include <GL/glut.h>
#endif

#include <vtkm/rendering/CanvasGL.h>
#include <vtkm/rendering/MapperGL.h>
#include <vtkm/rendering/View.h>
#include <vtkm/rendering/WorldAnnotatorGL.h>
#include <vtkm/rendering/ColorTable.h>

vtkm::rendering::View3D<vtkm::rendering::MapperGL<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>,
                        vtkm::rendering::CanvasGL,
                        vtkm::rendering::WorldAnnotatorGL> *view = NULL;

const vtkm::Int32 W = 512, H = 512;
int buttonStates[3] = {GLUT_UP, GLUT_UP, GLUT_UP};
bool shiftKey = false;
int lastx=-1, lasty=-1;

void
reshape(int, int)
{
    //Don't allow resizing window.
    glutReshapeWindow(W,H);
}

// Render the output using simple OpenGL
void displayCall()
{
    view->Paint();
    glutSwapBuffers();
}

// Allow rotations of the camera
void mouseMove(int x, int y)
{
    //std::cout<<"MOUSE MOVE: "<<x<<" "<<y<<std::endl;

    //Map to XY
    y = static_cast<int>(view->Canvas.Height-y);

    if (lastx != -1 && lasty != -1)
    {
        vtkm::Float32 x1 = ((lastx*2.0f)/view->Canvas.Width) - 1.0f;
        vtkm::Float32 y1 = ((lasty*2.0f)/view->Canvas.Height) - 1.0f;
        vtkm::Float32 x2 = ((x*2.0f)/view->Canvas.Width) - 1.0f;
        vtkm::Float32 y2 = ((y*2.0f)/view->Canvas.Height) - 1.0f;

        if (buttonStates[0] == GLUT_DOWN)
        {
            if (shiftKey)
                view->Camera.Pan3D(x2-x1, y2-y1);
            else
                view->Camera.TrackballRotate(x1,y1, x2,y2);
        }
        else if (buttonStates[1] == GLUT_DOWN)
            view->Camera.Zoom3D(y2-y1);
    }

    lastx = x;
    lasty = y;
    glutPostRedisplay();
}


// Respond to mouse button
void mouseCall(int button, int state, int vtkmNotUsed(x), int vtkmNotUsed(y))
{
    int modifiers = glutGetModifiers();
    shiftKey = modifiers & GLUT_ACTIVE_SHIFT;
    buttonStates[button] = state;

    //std::cout<<"Buttons: "<<buttonStates[0]<<" "<<buttonStates[1]<<" "<<buttonStates[2]<<" SHIFT= "<<shiftKey<<std::endl;

    //mouse down, reset.
    if (buttonStates[button] == GLUT_DOWN)
    {
        lastx = -1;
        lasty = -1;
    }
}

void Set3DView(vtkm::rendering::Camera &camera,
               const vtkm::cont::CoordinateSystem &coords)
{
    vtkm::Bounds coordsBounds =
        coords.GetBounds(VTKM_DEFAULT_DEVICE_ADAPTER_TAG());
    //set up a default view
    vtkm::Vec<vtkm::Float32,3> totalExtent;
    totalExtent[0] = vtkm::Float32(coordsBounds.X.Length());
    totalExtent[1] = vtkm::Float32(coordsBounds.Y.Length());
    totalExtent[2] = vtkm::Float32(coordsBounds.Z.Length());
    vtkm::Float32 mag = vtkm::Magnitude(totalExtent);
    vtkm::Normalize(totalExtent);

    camera = vtkm::rendering::Camera(vtkm::rendering::Camera::VIEW_3D);
    camera.Camera3d.Position = totalExtent * (mag * 2.f);
    camera.Camera3d.Up = vtkm::Vec<vtkm::Float32,3>(0.f, 1.f, 0.f);
    camera.Camera3d.LookAt = totalExtent * (mag * .5f);
    camera.Camera3d.FieldOfView = 60.f;
    camera.NearPlane = 1.f;
    camera.FarPlane = 100.f;
}

// Compute and render an isosurface for a uniform grid example
int
main(int argc, char* argv[])
{
    vtkm::cont::testing::MakeTestDataSet maker;
    vtkm::cont::DataSet ds = maker.Make3DUniformDataSet0();

    lastx = lasty = -1;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(W,H);
    glutCreateWindow("VTK-m Rendering");
    glutDisplayFunc(displayCall);
    glutMotionFunc(mouseMove);
    glutMouseFunc(mouseCall);
    glutReshapeFunc(reshape);

    const vtkm::cont::CoordinateSystem coords = ds.GetCoordinateSystem();

    vtkm::rendering::Camera camera;
    Set3DView(camera, coords);

    vtkm::rendering::Color bg(0.2f, 0.2f, 0.2f, 1.0f);
    vtkm::rendering::CanvasGL canvas(bg);
    vtkm::rendering::MapperGL<VTKM_DEFAULT_DEVICE_ADAPTER_TAG> mapper;

    vtkm::rendering::Scene scene;
    scene.AddActor(vtkm::rendering::Actor(ds.GetCellSet(),
                                          ds.GetCoordinateSystem(),
                                          ds.GetField("pointvar"),
                                          vtkm::rendering::ColorTable("thermal")));

    //Create vtkm rendering stuff.
    view = new vtkm::rendering::View3D<vtkm::rendering::MapperGL<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>,
                                       vtkm::rendering::CanvasGL,
                                       vtkm::rendering::WorldAnnotatorGL>(scene, mapper,
                                                                          canvas, camera, bg);
    view->Initialize();
    glutMainLoop();

    return 0;
}

#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
# pragma GCC diagnostic pop
#endif
