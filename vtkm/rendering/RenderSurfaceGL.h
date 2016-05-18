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
#ifndef vtk_m_rendering_RenderSurfaceGL_h
#define vtk_m_rendering_RenderSurfaceGL_h

#include <vtkm/Types.h>
#include <vtkm/rendering/View.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/RenderSurface.h>

#include <GL/gl.h>
#include <iostream>
#include <fstream>

namespace vtkm {
namespace rendering {

class RenderSurfaceGL : public RenderSurface
{
public:
    VTKM_CONT_EXPORT
    RenderSurfaceGL(std::size_t w=1024, std::size_t h=1024,
          const vtkm::rendering::Color &c=vtkm::rendering::Color(0.0f,0.0f,0.0f,1.0f))
        : RenderSurface(w,h,c)
    {
    }

    VTKM_CONT_EXPORT
    virtual void Clear()
    {
        glClearColor(bgColor.Components[0],bgColor.Components[1],bgColor.Components[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    VTKM_CONT_EXPORT
    virtual void Finish()
    {
        glFinish();
    }

    VTKM_CONT_EXPORT
    virtual void SetViewToWorldSpace(vtkm::rendering::View &v, bool clip)
    {
        vtkm::Float32 oglP[16], oglM[16];

        CreateOGLMatrix(v.CreateProjectionMatrix(), oglP);
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(oglP);

        /*
        std::cout<<"Proj: pos: "<<v.view3d.pos<<std::endl;
        std::cout<<"       up: "<<v.view3d.up<<std::endl;
        std::cout<<"   lookAt: "<<v.view3d.lookAt<<std::endl;
        std::cout<<" near/far: "<<v.view3d.nearPlane<<" "<<v.view3d.farPlane<<std::endl;
        std::cout<<"      fov: "<<v.view3d.fieldOfView<<std::endl;
        printMatrix(oglP);
        */
        
        CreateOGLMatrix(v.CreateViewMatrix(), oglM);
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(oglM);

        SetViewportClipping(v, clip);
    }

    VTKM_CONT_EXPORT
    virtual void SetViewToScreenSpace(vtkm::rendering::View &v, bool clip)
    {
        vtkm::Float32 oglP[16] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};
        vtkm::Float32 oglM[16] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};
        
        oglP[0*4+0] = 1.;
        oglP[1*4+1] = 1.;
        oglP[2*4+2] = -1.;
        oglP[3*4+3] = 1.;

        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(oglP);

        oglM[0*4+0] = 1.;
        oglM[1*4+1] = 1.;
        oglM[2*4+2] = 1.;
        oglM[3*4+3] = 1.;
        
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(oglM);

        SetViewportClipping(v, clip);
    }

    VTKM_CONT_EXPORT
    virtual void SetViewportClipping(vtkm::rendering::View &v, bool clip)
    {
        if (clip)
        {
            vtkm::Float32 vl, vr, vb, vt;
            v.GetRealViewport(vl,vr,vb,vt);
            vtkm::Float32 _x = static_cast<vtkm::Float32>(v.width)*(1.f+vl)/2.f;
            vtkm::Float32 _y = static_cast<vtkm::Float32>(v.height)*(1.f+vb)/2.f;
            vtkm::Float32 _w = static_cast<vtkm::Float32>(v.width)*(vr-vl)/2.f;
            vtkm::Float32 _h = static_cast<vtkm::Float32>(v.height)*(vt-vb)/2.f;

            glViewport(static_cast<int>(_x), static_cast<int>(_y),
                       static_cast<int>(_w), static_cast<int>(_h));
        }
        else
        {
            glViewport(0,0, v.width, v.height);
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
        int hi = static_cast<int>(height);
        for (int i=hi-1; i>=0; i--)
            for (std::size_t j=0; j < width; j++)
            { 
                const vtkm::Float32 *tuple = &(rgba[i*width*4 + j*4]);
                of<<(unsigned char)(tuple[0]*255);
                of<<(unsigned char)(tuple[1]*255);
                of<<(unsigned char)(tuple[2]*255);
            }
        of.close();
    }

    VTKM_CONT_EXPORT
    virtual void AddLine(double x0, double y0,
                         double x1, double y1,
                         float linewidth,
                         Color c)
    {
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);
        glColor3fv(c.Components);

        glLineWidth(linewidth);

        glBegin(GL_LINES);
        glVertex2f(float(x0),float(y0));
        glVertex2f(float(x1),float(y1));
        glEnd();
    }

    virtual void AddColorBar(float x, float y, 
                             float w, float h,
                             const ColorTable &ct,
                             bool horizontal)
    {
        const int n = 256;
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);
        glBegin(GL_QUADS);
        for (int i=0; i<n; i++)
        {
            float v0 = float(i)/float(n);
            float v1 = float(i+1)/float(n);
            Color c0 = ct.MapRGB(v0);
            Color c1 = ct.MapRGB(v1);
            if (horizontal)
            {
                float x0 = x + w*v0;
                float x1 = x + w*v1;
                float y0 = y;
                float y1 = y + h;
                glColor3fv(c0.Components);
                glVertex2f(x0,y0);
                glVertex2f(x0,y1);
                glColor3fv(c1.Components);
                glVertex2f(x1,y1);
                glVertex2f(x1,y0);
            }
            else // vertical
            {
                float x0 = x;
                float x1 = x + w;
                float y0 = y + h*v0;
                float y1 = y + h*v1;
                glColor3fv(c0.Components);
                glVertex2f(x0,y1);
                glVertex2f(x1,y1);
                glColor3fv(c1.Components);
                glVertex2f(x1,y0);
                glVertex2f(x0,y0);
            }
        }
        glEnd();
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
};

}} //namespace vtkm::rendering

#endif //vtk_m_rendering_RenderSurfaceGL_h
