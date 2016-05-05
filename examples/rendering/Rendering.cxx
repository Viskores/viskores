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

#include "quaternion.h"
#include <vector>

#include <vtkm/rendering/Window.h>
#include <vtkm/rendering/WorldAnnotatorGL.h>
#include <vtkm/rendering/RenderSurfaceGL.h>
#include <vtkm/rendering/SceneRendererGL.h>
#include <vtkm/rendering/ColorTable.h>

vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32,3> > verticesArray, normalsArray;
vtkm::cont::ArrayHandle<vtkm::Float32> scalarsArray;
vtkm::rendering::SceneRendererGL<VTKM_DEFAULT_DEVICE_ADAPTER_TAG> sceneRenderer;
vtkm::cont::DataSet ds;

vtkm::rendering::Window3D<vtkm::rendering::SceneRendererGL<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>,
                          vtkm::rendering::RenderSurfaceGL,
                          vtkm::rendering::WorldAnnotatorGL> *window = NULL;

Quaternion qrot;
int lastx, lasty;
int mouse_state = 1;

// Render the output using simple OpenGL
void displayCall()
{
    window->Paint();
    glutSwapBuffers();
}

static vtkm::Vec<vtkm::Float32, 3>
MultVector(const vtkm::Matrix<vtkm::Float32,4,4> &mtx, vtkm::Vec<vtkm::Float32, 3> &v)
{
    vtkm::Vec<vtkm::Float32,4> v4(v[0],v[1],v[2], 1);
    v4 = vtkm::MatrixMultiply(mtx, v4);
    v[0] = v4[0];
    v[1] = v4[1];
    v[2] = v4[2];
    return v;
}

// Allow rotations of the view
void mouseMove(int x, int y)
{
    vtkm::Float32 dx = static_cast<vtkm::Float32>(x-lastx);
    vtkm::Float32 dy = static_cast<vtkm::Float32>(y-lasty);

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

    vtkm::Float32 m[16];
    qrot.getRotMat(m);
    vtkm::Matrix<vtkm::Float32,4,4> mtx;
    mtx(0,0) = m[0];
    mtx(1,0) = m[1];
    mtx(2,0) = m[2];
    mtx(3,0) = m[3];

    mtx(0,1) = m[4];
    mtx(1,1) = m[5];
    mtx(2,1) = m[6];
    mtx(3,1) = m[7];

    mtx(0,2) = m[8];
    mtx(1,2) = m[9];
    mtx(2,2) = m[10];
    mtx(3,2) = m[11];

    mtx(0,3) = m[12];
    mtx(1,3) = m[13];
    mtx(2,3) = m[14];
    mtx(3,3) = m[15];

    window->view.view3d.pos = MultVector(mtx, window->view.view3d.pos);
    window->view.view3d.up = MultVector(mtx, window->view.view3d.up);
    //window->view.view3d.lookAt = MultVector(mtx, window->view.view3d.lookAt);
    
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

void Set3DView(vtkm::rendering::View &view,
               const vtkm::cont::CoordinateSystem &coords,
               vtkm::Int32 w, vtkm::Int32 h)
{
    vtkm::Float64 coordsBounds[6]; // Xmin,Xmax,Ymin..
    coords.GetBounds(coordsBounds,VTKM_DEFAULT_DEVICE_ADAPTER_TAG());
    //set up a default view  
    vtkm::Vec<vtkm::Float32,3> totalExtent; 
    totalExtent[0] = vtkm::Float32(coordsBounds[1] - coordsBounds[0]); 
    totalExtent[1] = vtkm::Float32(coordsBounds[3] - coordsBounds[2]);
    totalExtent[2] = vtkm::Float32(coordsBounds[5] - coordsBounds[4]);
    vtkm::Float32 mag = vtkm::Magnitude(totalExtent);
    vtkm::Normalize(totalExtent);

    view = vtkm::rendering::View(vtkm::rendering::View::VIEW_3D);
    view.view3d.pos = totalExtent * (mag * 2.f);
    view.view3d.up = vtkm::Vec<vtkm::Float32,3>(0.f, 1.f, 0.f);
    view.view3d.lookAt = totalExtent * (mag * .5f);
    view.view3d.fieldOfView = 60.f;
    view.nearPlane = 1.f;
    view.farPlane = 100.f;
    view.width = w;
    view.height = h;
    
    /*
    std::cout<<"View3d:  pos: "<<view.view3d.pos<<std::endl;
    std::cout<<"      lookAt: "<<view.view3d.lookAt<<std::endl;
    std::cout<<"          up: "<<view.view3d.up<<std::endl;
    std::cout<<"near/far/fov: "<<view.nearPlane<<"/"<<view.farPlane<<" "<<view.view3d.fieldOfView<<std::endl;
    std::cout<<"         w/h: "<<view.width<<"/"<<view.height<<std::endl;
    */
}

// Compute and render an isosurface for a uniform grid example
int main(int argc, char* argv[])
{
    vtkm::cont::testing::MakeTestDataSet maker;
    ds = maker.Make3DUniformDataSet0();
    
    lastx = lasty = 0;
    const vtkm::Int32 W = 512, H = 512;
    
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(W,H);
    glutCreateWindow("VTK-m Rendering");
    glutDisplayFunc(displayCall);
    glutMotionFunc(mouseMove);
    glutMouseFunc(mouseCall);

    const vtkm::cont::CoordinateSystem coords = ds.GetCoordinateSystem();
    
    vtkm::rendering::View view;
    Set3DView(view, coords, W, H);

    vtkm::rendering::Color bg(0.2f, 0.2f, 0.2f, 1.0f);
    vtkm::rendering::RenderSurfaceGL surface(W,H,bg);
    vtkm::rendering::SceneRendererGL<VTKM_DEFAULT_DEVICE_ADAPTER_TAG> sceneRenderer;

    vtkm::rendering::Scene3D scene;
    scene.plots.push_back(vtkm::rendering::Plot(ds.GetCellSet(),
                                                ds.GetCoordinateSystem(),
                                                ds.GetField("pointvar"),
                                                vtkm::rendering::ColorTable("thermal")));

    //Create vtkm rendering stuff.
    window = new vtkm::rendering::Window3D<vtkm::rendering::SceneRendererGL<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>,
                                           vtkm::rendering::RenderSurfaceGL,
                                           vtkm::rendering::WorldAnnotatorGL>(scene, sceneRenderer,
                                                                              surface, view, bg);
    window->Initialize();
    glutMainLoop();
    
    return 0;
}

#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
# pragma GCC diagnostic pop
#endif
