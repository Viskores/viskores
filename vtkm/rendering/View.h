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
#ifndef vtk_m_rendering_View_h
#define vtk_m_rendering_View_h
#include <vtkm/Math.h>
#include <vtkm/Matrix.h>
#include <vtkm/VectorAnalysis.h>

namespace vtkm {
namespace rendering {

static void printMtx(const std::string &nm, const vtkm::Matrix<vtkm::Float32,4,4> &mtx)
{
    std::cout<<nm<<": ["<<std::endl;
    for (int i = 0; i < 4; i++)
        std::cout<<"  "<<mtx(0,i)<<" "<<mtx(1,i)<<" "<<mtx(2,i)<<" "<<mtx(3,i)<<std::endl;
    std::cout<<"]"<<std::endl;    
}
static void printVec(const std::string &nm, const vtkm::Vec<vtkm::Float32,3> &v)
{
    std::cout<<nm<<" ["<<v[0]<<" "<<v[1]<<" "<<v[2]<<"]"<<std::endl;
}

class View
{
    class View3D
    {
    public:
        VTKM_CONT_EXPORT
        View3D() : fieldOfView(0.f), xpan(0), ypan(0), zoom(1)
        {}
        
        VTKM_CONT_EXPORT
        vtkm::Matrix<vtkm::Float32,4,4> CreateViewMatrix()
        {
            vtkm::Matrix<vtkm::Float32,4,4> V = View::ViewMtx(pos, lookAt, up);
            return V;
        }

        VTKM_CONT_EXPORT
        vtkm::Matrix<vtkm::Float32,4,4> CreateProjectionMatrix(vtkm::Int32 &width,
                                                               vtkm::Int32 &height,
                                                               vtkm::Float32 &nearPlane,
                                                               vtkm::Float32 &farPlane)
        {
            vtkm::Matrix<vtkm::Float32,4,4> mtx;
            vtkm::MatrixIdentity(mtx);
                        
            vtkm::Float32 AspectRatio = vtkm::Float32(width) / vtkm::Float32(height);
            vtkm::Float32 fovRad = (fieldOfView * 3.1415926f)/180.f;
            fovRad = vtkm::Tan( fovRad * 0.5f);
            vtkm::Float32 size = nearPlane * fovRad;
            vtkm::Float32 left = -size * AspectRatio;
            vtkm::Float32 right = size * AspectRatio;
            vtkm::Float32 bottom = -size;
            vtkm::Float32 top = size;
            
            mtx(0,0) = 2.f * nearPlane / (right - left);
            mtx(1,1) = 2.f * nearPlane / (top - bottom);
            mtx(0,2) = (right + left) / (right - left);
            mtx(1,2) = (top + bottom) / (top - bottom);
            mtx(2,2) = -(farPlane + nearPlane)  / (farPlane - nearPlane);
            mtx(3,2) = -1.f;
            mtx(2,3) = -(2.f * farPlane * nearPlane) / (farPlane - nearPlane);
            mtx(3,3) = 0.f;

            vtkm::Matrix<vtkm::Float32,4,4> T, Z;
            T = View::TranslateMtx(xpan, ypan, 0);
            Z = View::ScaleMtx(zoom, zoom, 1);
            mtx = vtkm::MatrixMultiply(Z, vtkm::MatrixMultiply(T, mtx));
            return mtx;
        }
        
        
        vtkm::Vec<vtkm::Float32,3> up, lookAt, pos;
        vtkm::Float32 fieldOfView;
        vtkm::Float32 xpan, ypan, zoom;
    };

    class View2D
    {
    public:
        VTKM_CONT_EXPORT
        View2D() : left(0.f), right(0.f), top(0.f), bottom(0.f), xScale(1.f)
        {}
        
        VTKM_CONT_EXPORT
        vtkm::Matrix<vtkm::Float32,4,4> CreateViewMatrix()
        {
            vtkm::Vec<vtkm::Float32,3> at((left+right)/2.f, (top+bottom)/2.f, 0.f);
            vtkm::Vec<vtkm::Float32,3> pos = at;
            pos[2] = 1.f;
            vtkm::Vec<vtkm::Float32,3> up(0,1,0);
            return View::ViewMtx(pos, at, up);
        }
        
        VTKM_CONT_EXPORT
        vtkm::Matrix<vtkm::Float32,4,4> CreateProjectionMatrix(vtkm::Float32 &size,
                                                               vtkm::Float32 &near,
                                                               vtkm::Float32 &far,
                                                               vtkm::Float32 &aspect)
        {
            vtkm::Matrix<vtkm::Float32,4,4> mtx(0.f);
            vtkm::Float32 L = -size/2.f * aspect;
            vtkm::Float32 R = size/2.f * aspect;
            vtkm::Float32 B = -size/2.f;
            vtkm::Float32 T = size/2.f;

            mtx(0,0) = 2.f/(R-L);
            mtx(1,1) = 2.f/(T-B);
            mtx(2,2) = -2.f/(far-near);
            mtx(0,3) = -(R+L)/(R-L);
            mtx(1,3) = -(T+B)/(T-B);
            mtx(2,3) = -(far+near)/(far-near);
            mtx(3,3) = 1.f;
            return mtx;
        }        
        
        vtkm::Float32 left, right, top, bottom;
        vtkm::Float32 xScale;
    };

private:
    static VTKM_CONT_EXPORT
    vtkm::Matrix<vtkm::Float32,4,4> ViewMtx(const vtkm::Vec<vtkm::Float32,3> &pos,
                                            const vtkm::Vec<vtkm::Float32,3> &at,
                                            const vtkm::Vec<vtkm::Float32,3> &up)
    {
        vtkm::Vec<vtkm::Float32,3> viewDir = pos-at;
        vtkm::Vec<vtkm::Float32,3> right = vtkm::Cross(up,viewDir);
        vtkm::Vec<vtkm::Float32,3> ru = vtkm::Cross(viewDir,right);
        
        vtkm::Normalize(viewDir);
        vtkm::Normalize(right);
        vtkm::Normalize(ru);
        
        vtkm::Matrix<vtkm::Float32,4,4> mtx;            
        vtkm::MatrixIdentity(mtx);
        
        mtx(0,0) = right[0];
        mtx(0,1) = right[1];
        mtx(0,2) = right[2];
        mtx(1,0) = ru[0];
        mtx(1,1) = ru[1];
        mtx(1,2) = ru[2];
        mtx(2,0) = viewDir[0];
        mtx(2,1) = viewDir[1];
        mtx(2,2) = viewDir[2];
        
        mtx(0,3) = -vtkm::dot(right,pos);
        mtx(1,3) = -vtkm::dot(ru,pos);
        mtx(2,3) = -vtkm::dot(viewDir,pos);
        
        return mtx;
    }

    static VTKM_CONT_EXPORT    
    vtkm::Matrix<vtkm::Float32,4,4> ScaleMtx(const vtkm::Vec<vtkm::Float32,3> &v)
    {
        return ScaleMtx(v[0], v[1], v[2]);
    }
    static VTKM_CONT_EXPORT    
    vtkm::Matrix<vtkm::Float32,4,4> ScaleMtx(const vtkm::Float32 &s) {return ScaleMtx(s,s,s);}
    
    static VTKM_CONT_EXPORT    
    vtkm::Matrix<vtkm::Float32,4,4> ScaleMtx(const vtkm::Float32 &x,
                                             const vtkm::Float32 &y,
                                             const vtkm::Float32 &z)
    {
        vtkm::Matrix<vtkm::Float32,4,4> S;
        vtkm::MatrixIdentity(S);
        S(0,0) = x;
        S(1,1) = y;
        S(2,2) = z;
        return S;
    }    

    static VTKM_CONT_EXPORT    
    vtkm::Matrix<vtkm::Float32,4,4> TranslateMtx(const vtkm::Vec<vtkm::Float32,3> &v)
    {
        return TranslateMtx(v[0], v[1], v[2]);
    }
    
    static VTKM_CONT_EXPORT    
    vtkm::Matrix<vtkm::Float32,4,4> TranslateMtx(const vtkm::Float32 &x,
                                                 const vtkm::Float32 &y,
                                                 const vtkm::Float32 &z)
    {
        vtkm::Matrix<vtkm::Float32,4,4> T;
        vtkm::MatrixIdentity(T);
        T(0,3) = x;
        T(1,3) = y;
        T(2,3) = z;
        return T;
    }

    VTKM_CONT_EXPORT
    vtkm::Matrix<vtkm::Float32,4,4>    
    CreateTrackball(vtkm::Float32 p1x, vtkm::Float32 p1y, vtkm::Float32 p2x, vtkm::Float32 p2y)
    {
        const vtkm::Float32 RADIUS = 0.80f; //z value at x = y = 0.0
        const vtkm::Float32 COMPRESSION = 3.5f; // multipliers for x and y.
        const vtkm::Float32 AR3 = RADIUS*RADIUS*RADIUS;
        
        vtkm::Matrix<vtkm::Float32,4,4> mtx;

        vtkm::MatrixIdentity(mtx);
        if (p1x==p2x && p1y==p2y)
            return mtx;
        
        vtkm::Vec<vtkm::Float32, 3> p1(p1x,p1y, AR3/((p1x*p1x+p1y*p1y)*COMPRESSION+AR3));
        vtkm::Vec<vtkm::Float32, 3> p2(p2x,p2y, AR3/((p2x*p2x+p2y*p2y)*COMPRESSION+AR3));
        vtkm::Vec<vtkm::Float32, 3> axis = vtkm::Normal(vtkm::Cross(p2,p1));
        //std::cout<<"Axis: "<<axis[0]<<" "<<axis[1]<<" "<<axis[2]<<std::endl;
        
        vtkm::Vec<vtkm::Float32, 3> p2_p1(p2[0]-p1[0], p2[1]-p1[1], p2[2]-p1[2]);
        vtkm::Float32 t = vtkm::Magnitude(p2_p1);
        t = vtkm::Min(vtkm::Max(t, -1.0f), 1.0f);
        vtkm::Float32 phi = static_cast<vtkm::Float32>(-2.0f*asin(t/(2.0f*RADIUS)));
        vtkm::Float32 val = static_cast<vtkm::Float32>(sin(phi/2.0f));
        axis[0] *= val;
        axis[1] *= val;
        axis[2] *= val;

        //quaternion
        vtkm::Float32 q[4] = {axis[0], axis[1], axis[2], static_cast<vtkm::Float32>(cos(phi/2.0f))};
        
        // normalize quaternion to unit magnitude
        t =  1.0f / static_cast<vtkm::Float32>(sqrt(q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3]));
        q[0] *= t;
        q[1] *= t;
        q[2] *= t;
        q[3] *= t;

        /*
        std::cout<<"P1: "<<p1[0]<<" "<<p1[1]<<" "<<p1[2]<<std::endl;
        std::cout<<"P2: "<<p2[0]<<" "<<p2[1]<<" "<<p2[2]<<std::endl;                
        std::cout<<"T= "<<t<<std::endl;
        std::cout<<"PHI= "<<phi<<std::endl;
        std::cout<<"QUAT: "<<q[0]<<" "<<q[1]<<" "<<q[2]<<" "<<q[3]<<std::endl;
        */
        
        mtx(0,0) = 1 - 2 * (q[1]*q[1] + q[2]*q[2]);
        mtx(0,1) = 2 * (q[0]*q[1] + q[2]*q[3]);
        mtx(0,2) = (2 * (q[2]*q[0] - q[1]*q[3]) );

        mtx(1,0) = 2 * (q[0]*q[1] - q[2]*q[3]);
        mtx(1,1) = 1 - 2 * (q[2]*q[2] + q[0]*q[0]);
        mtx(1,2) = (2 * (q[1]*q[2] + q[0]*q[3]) );

        mtx(2,0) = (2 * (q[2]*q[0] + q[1]*q[3]) );
        mtx(2,1) = (2 * (q[1]*q[2] - q[0]*q[3]) );
        mtx(2,2) = (1 - 2 * (q[1]*q[1] + q[0]*q[0]) );

        return mtx;
    }
    
                                            
public:
    enum ViewType { VIEW_2D, VIEW_3D };
    ViewType viewType;
    View3D view3d;
    View2D view2d;
    
    vtkm::Int32 width, height;
    vtkm::Float32 nearPlane, farPlane;
    vtkm::Float32 vl, vr, vb, vt; //viewport.
    
    VTKM_CONT_EXPORT
    View(ViewType vtype=View::VIEW_3D) : width(-1), height(-1), nearPlane(0.f), farPlane(1.f), viewType(vtype),
                                         vl(-1.f), vr(1.f), vb(-1.f), vt(1.f)
    {}

    VTKM_CONT_EXPORT
    vtkm::Matrix<vtkm::Float32,4,4> CreateViewMatrix()
    {
        if (viewType == View::VIEW_3D)
            return view3d.CreateViewMatrix();
        else
            return view2d.CreateViewMatrix();
    }
    VTKM_CONT_EXPORT
    vtkm::Matrix<vtkm::Float32,4,4> CreateProjectionMatrix()
    {
        if (viewType == View::VIEW_3D)
            return view3d.CreateProjectionMatrix(width, height, nearPlane, farPlane);
        else
        {
            vtkm::Float32 size = vtkm::Abs(view2d.top-view2d.bottom);
            vtkm::Float32 l,r,b,t;
            GetRealViewport(l,r,b,t);
            vtkm::Float32 aspect = (static_cast<vtkm::Float32>(width)*(r-l)) / (static_cast<vtkm::Float32>(height)*(t-b));
            
            return view2d.CreateProjectionMatrix(size, nearPlane, farPlane, aspect);
        }
    }

    VTKM_CONT_EXPORT
    void GetRealViewport(vtkm::Float32 &l, vtkm::Float32 &r,
                         vtkm::Float32 &b, vtkm::Float32 &t)
    {
        if (viewType == View::VIEW_3D)
        {
            l = vl;
            r = vr;
            b = vb;
            t = vt;
        }
        else
        {
            vtkm::Float32 maxvw = (vr-vl) * static_cast<vtkm::Float32>(width);
            vtkm::Float32 maxvh = (vt-vb) * static_cast<vtkm::Float32>(height);
            vtkm::Float32 waspect = maxvw / maxvh;
            vtkm::Float32 daspect = (view2d.right - view2d.left) / (view2d.top - view2d.bottom);
            daspect *= view2d.xScale;
            //cerr << "waspect="<<waspect << "   \tdaspect="<<daspect<<endl;
            const bool center = true; // if false, anchor to bottom-left
            if (waspect > daspect)
            {
                vtkm::Float32 new_w = (vr-vl) * daspect / waspect;
                if (center)
                {
                    l = (vl+vr)/2.f - new_w/2.f;
                    r = (vl+vr)/2.f + new_w/2.f;
                }
                else
                {
                    l = vl;
                    r = vl + new_w;
                }
                b = vb;
                t = vt;
            }
            else
            {
                vtkm::Float32 new_h = (vt-vb) * waspect / daspect;
                if (center)
                {
                    b = (vb+vt)/2.f - new_h/2.f;
                    t = (vb+vt)/2.f + new_h/2.f;
                }
                else
                {
                    b = vb;
                    t = vb + new_h;
                }
                l = vl;
                r = vr;
            }
        }
    }

    VTKM_CONT_EXPORT
    vtkm::Vec<vtkm::Float32, 3>
    MultVector(const vtkm::Matrix<vtkm::Float32,4,4> &mtx, vtkm::Vec<vtkm::Float32, 3> &v)
    {
        vtkm::Vec<vtkm::Float32,4> v4(v[0],v[1],v[2], 1);
        v4 = vtkm::MatrixMultiply(mtx, v4);
        v[0] = v4[0];
        v[1] = v4[1];
        v[2] = v4[2];
        return v;
    }

    VTKM_CONT_EXPORT
    void Pan3D(vtkm::Float32 dx, vtkm::Float32 dy)
    {
        //std::cout<<"Pan3d: "<<dx<<" "<<dy<<std::endl;
        view3d.xpan += dx;
        view3d.ypan += dy;
    }

    VTKM_CONT_EXPORT
    void Zoom3D(vtkm::Float32 zoom)
    {
        vtkm::Float32 factor = powf(4, zoom);
        //std::cout<<"Zoom3D: "<<zoom<<" --> "<<factor<<std::endl;
        view3d.zoom *= factor;
        view3d.xpan *= factor;
        view3d.ypan *= factor;
    }    

    VTKM_CONT_EXPORT
    void TrackballRotate(vtkm::Float32 x1, vtkm::Float32 y1, vtkm::Float32 x2, vtkm::Float32 y2)
    {
        /*
        std::cout<<std::endl;
        std::cout<<"*****************************************************************"<<std::endl;
        std::cout<<x1<<" "<<y1<<" --> "<<x2<<" "<<y2<<std::endl;
        printVec("pos", view3d.pos);
        printVec("at", view3d.lookAt);
        printVec("up", view3d.up);
        std::cout<<"*****************************************************************"<<std::endl;        
        */
        vtkm::Matrix<vtkm::Float32,4,4> R1 = CreateTrackball(x1,y1, x2,y2);

        //Translate mtx
        vtkm::Matrix<vtkm::Float32,4,4> T1 = View::TranslateMtx(-view3d.lookAt);        
        //vtkm::MatrixIdentity(T1);
        //T1(0,3) = -view3d.lookAt[0];
        //T1(1,3) = -view3d.lookAt[1];
        //T1(2,3) = -view3d.lookAt[2];

        //Translate mtx
        vtkm::Matrix<vtkm::Float32,4,4> T2 = View::TranslateMtx(view3d.lookAt);
        //T2(0,3) = view3d.lookAt[0];
        //T2(1,3) = view3d.lookAt[1];
        //T2(2,3) = view3d.lookAt[2];

        vtkm::Matrix<vtkm::Float32,4,4> V1 = CreateViewMatrix();
        V1(0,3) = 0;
        V1(1,3) = 0;
        V1(2,3) = 0;

        vtkm::Matrix<vtkm::Float32,4,4> V2 = vtkm::MatrixTranspose(V1);        
        
        //MM = T2 * V2 * R1 * V1 * T1;
        vtkm::Matrix<vtkm::Float32,4,4> MM;
        MM = vtkm::MatrixMultiply(T2,
                                  vtkm::MatrixMultiply(V2,
                                                       vtkm::MatrixMultiply(R1,
                                                                            vtkm::MatrixMultiply(V1,T1))));
        view3d.pos = MultVector(MM, view3d.pos);
        view3d.lookAt = MultVector(MM, view3d.lookAt);
        view3d.up = MultVector(MM, view3d.up);
        
        /*
        printMtx("T1", T1);
        printMtx("T2", T2);
        printMtx("V1", V1);
        printMtx("V2", V2);
        printMtx("R1", R1);
        printMtx("MM", MM);        
        printVec("pos", view3d.pos);
        printVec("at", view3d.lookAt);
        printVec("up", view3d.up);         
        */
    }
};

}} // namespace vtkm::rendering

#endif // vtk_m_rendering_View_h
