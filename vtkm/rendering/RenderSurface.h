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
#ifndef vtk_m_rendering_RenderSurface_h
#define vtk_m_rendering_RenderSurface_h

#include <vtkm/Types.h>
#include <vtkm/cont/ErrorControlBadValue.h>
#include <vtkm/rendering/View.h>
#include <vector>
#include <GL/osmesa.h>
#include <GL/gl.h>
#include <iostream>
#include <fstream>

namespace vtkm {
namespace rendering {

class RenderSurface
{
public:
    VTKM_CONT_EXPORT
    RenderSurface():bgColor(0.0f,0.0f,0.0f,1.0f) {}

    VTKM_CONT_EXPORT
    virtual void Initialize() {}
    VTKM_CONT_EXPORT
    virtual void Activate() {}
    VTKM_CONT_EXPORT
    virtual void Clear() {}
    VTKM_CONT_EXPORT
    virtual void Finish() {}

    VTKM_CONT_EXPORT
    virtual void SaveAs(const std::string &) {}

    vtkm::Vec<vtkm::Float32,4> bgColor;
};

class RenderSurfaceOSMesa : public RenderSurface
{
public:
    VTKM_CONT_EXPORT
    RenderSurfaceOSMesa()
    {
        ctx = NULL;
        width = 1024;
        height = 1024;
        rgba.resize(width*height*4);
        zbuff.resize(width*height*4);
    }

    VTKM_CONT_EXPORT
    virtual void Initialize()
    {
        ctx = OSMesaCreateContextExt(OSMESA_RGBA, 32, 0, 0, NULL);
        if (!ctx)
            throw vtkm::cont::ErrorControlBadValue("OSMesa context creation failed.");
        rgba.resize(width*height*4);
        if (!OSMesaMakeCurrent(ctx, &rgba[0], GL_FLOAT, width, height))
            throw vtkm::cont::ErrorControlBadValue("OSMesa context activation failed.");
    }
    
    VTKM_CONT_EXPORT
    virtual void Clear()
    {
        glClearColor(bgColor[0],bgColor[1],bgColor[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    VTKM_CONT_EXPORT
    virtual void Finish()
    {
        glFinish();
        
        //Copy zbuff into floating point array.
        unsigned int *raw_zbuff;
        int zbytes, w, h;
        OSMesaGetDepthBuffer(ctx, &w, &h, &zbytes, (void**)&raw_zbuff);
        if (w!=width || h!=height)
            throw vtkm::cont::ErrorControlBadValue("Wrong width/height in ZBuffer");
        int npixels = width*height;
        for (int i=0; i<npixels; i++)
            zbuff[i] = float(raw_zbuff[i]) / float(UINT_MAX);
    }

    VTKM_CONT_EXPORT
    void SetViewToWorldSpace(vtkm::rendering::View3D &v, bool clip)
    {
        vtkm::Float32 oglP[16], oglM[16];

        CreateOGLMatrix(v.CreateProjectionMatrix(), oglP);
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(oglP);

        CreateOGLMatrix(v.CreateViewMatrix(), oglM);
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(oglM);

        //std::cout<<"proj matrix: "<<std::endl; printMatrix(oglP);
        //std::cout<<"view matrix: "<<std::endl; printMatrix(oglM);
        SetViewportClipping(v, clip);
    }

    VTKM_CONT_EXPORT
    void SetViewportClipping(vtkm::rendering::View3D &v, bool clip)
    {
        if (clip)
        {
            vtkm::Float32 vl, vr, vt, vb;
            v.GetRealViewport(vl,vr,vt,vb);
            double x = vtkm::Float32(v.Width)*(1.+vl)/2.;
            double y = vtkm::Float32(v.Height)*(1.+vb)/2.;
            double a = vtkm::Float32(v.Width)*(vr-vl)/2.;
            double b = vtkm::Float32(v.Height)*(vt-vb)/2.;
            glViewport(int(float(v.Width)*(1.+vl)/2.),
                       int(float(v.Height)*(1.+vb)/2.),
                       int(float(v.Width)*(vr-vl)/2.),
                       int(float(v.Height)*(vt-vb)/2.));
        }
        else
        {
            glViewport(0,0, v.Width, v.Height);
        }
    }

    VTKM_CONT_EXPORT
    void CreateOGLMatrix(const vtkm::Matrix<vtkm::Float32,4,4> &mtx,
                         vtkm::Float32 *oglM)
    {
        oglM[ 0] = mtx[0][0];
        oglM[ 1] = mtx[1][0];
        oglM[ 2] = mtx[2][0];
        oglM[ 3] = mtx[3][0];
        oglM[ 4] = mtx[0][1];
        oglM[ 5] = mtx[1][1];
        oglM[ 6] = mtx[2][1];
        oglM[ 7] = mtx[3][1];
        oglM[ 8] = mtx[0][2];
        oglM[ 9] = mtx[1][2];
        oglM[10] = mtx[2][2];
        oglM[11] = mtx[3][2];
        oglM[12] = mtx[0][3];
        oglM[13] = mtx[1][3];
        oglM[14] = mtx[2][3];
        oglM[15] = mtx[3][3];
    }

    VTKM_CONT_EXPORT
    virtual void SaveAs(const std::string &fileName)
    {
        std::ofstream of(fileName.c_str());
        of<<"P6"<<std::endl<<width<<" "<<height<<std::endl<<255<<std::endl;
        for (int i=height-1; i>=0; i--)
            for (int j=0; j < width; j++)
            { 
                const vtkm::Float32 *tuple = &(rgba[i*width*4 + j*4]);
                unsigned char r = (unsigned char)(tuple[0]*255);
                unsigned char g = (unsigned char)(tuple[1]*255);
                unsigned char b = (unsigned char)(tuple[2]*255);
                of<<r<<g<<b;
            }
        of.close();
    }

    VTKM_CONT_EXPORT
    void printMatrix(vtkm::Float32 *m)
    {
        std::cout<<"["<<m[0]<<" "<<m[1]<<" "<<m[2]<<" "<<m[3]<<std::endl;
        std::cout<<" "<<m[4]<<" "<<m[5]<<" "<<m[6]<<" "<<m[7]<<std::endl;
        std::cout<<" "<<m[8]<<" "<<m[9]<<" "<<m[10]<<" "<<m[11]<<std::endl;
        std::cout<<" "<<m[12]<<" "<<m[13]<<" "<<m[14]<<" "<<m[15]<<"]"<<std::endl;
    }

private:
  OSMesaContext ctx;
  std::vector<vtkm::Float32> rgba;
  std::vector<vtkm::Float32> zbuff;
  int width, height;
};

}} //namespace vtkm::rendering

#endif //vtk_m_rendering_RenderSurface_h
