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
#ifndef vtk_m_rendering_raytracing_TriagnleIntersector_h
#define vtk_m_rendering_raytracing_TriagnleIntersector_h
#include <cstring>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCompositeVector.h>
#include <vtkm/exec/ExecutionWholeArray.h>
#include <vtkm/rendering/raytracing/BoundingVolumeHierarchy.h>
#include <vtkm/rendering/raytracing/Ray.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>
namespace vtkm
{
namespace rendering
{
namespace raytracing
{

namespace
{
const static vtkm::Int32 END_FLAG2 = -1000000000;
const static vtkm::Float32 EPSILON2 = 0.0001f;
}

template <typename DeviceAdapter>
class TriangleIntersector
{
public:
  typedef typename vtkm::cont::ArrayHandle<Vec<vtkm::Float32, 4>> Float4ArrayHandle;
  typedef typename vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Int32, 2>> Int2Handle;
  typedef typename vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Int32, 4>> Int4Handle;
  typedef typename Float4ArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst Float4ArrayPortal;
  typedef typename Int2Handle::ExecutionTypes<DeviceAdapter>::PortalConst Int2ArrayPortal;
  typedef typename Int4Handle::ExecutionTypes<DeviceAdapter>::PortalConst Int4ArrayPortal;

  class Intersector : public vtkm::worklet::WorkletMapField
  {
  private:
    bool Occlusion;
    vtkm::Float32 MaxDistance;
    Float4ArrayPortal FlatBVH;
    Int4ArrayPortal Leafs;
    VTKM_EXEC
    vtkm::Float32 rcp(vtkm::Float32 f) const { return 1.0f / f; }
    VTKM_EXEC
    vtkm::Float32 rcp_safe(vtkm::Float32 f) const { return rcp((fabs(f) < 1e-8f) ? 1e-8f : f); }
  public:
    VTKM_CONT
    Intersector(bool occlusion, vtkm::Float32 maxDistance, LinearBVH& bvh)
      : Occlusion(occlusion)
      , MaxDistance(maxDistance)
      , FlatBVH(bvh.FlatBVH.PrepareForInput(DeviceAdapter()))
      , Leafs(bvh.LeafNodes.PrepareForInput(DeviceAdapter()))
    {
    }
    typedef void ControlSignature(FieldIn<>, FieldIn<>, FieldOut<>, FieldOut<>, FieldOut<>,
                                  FieldOut<>, WholeArrayIn<Vec3RenderingTypes>);
    typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7);
    template <typename PointPortalType>
    VTKM_EXEC void operator()(const vtkm::Vec<vtkm::Float32, 3>& rayDir,
                              const vtkm::Vec<vtkm::Float32, 3>& rayOrigin, vtkm::Float32& distance,
                              vtkm::Float32& minU, vtkm::Float32& minV, vtkm::Id& hitIndex,
                              const PointPortalType& points) const
    {
      float minDistance = MaxDistance;
      hitIndex = -1;
      float dirx = rayDir[0];
      float diry = rayDir[1];
      float dirz = rayDir[2];

      float invDirx = rcp_safe(dirx);
      float invDiry = rcp_safe(diry);
      float invDirz = rcp_safe(dirz);
      int currentNode;

      int todo[64];
      int stackptr = 0;
      int barrier = (int)END_FLAG2;
      currentNode = 0;

      todo[stackptr] = barrier;

      float originX = rayOrigin[0];
      float originY = rayOrigin[1];
      float originZ = rayOrigin[2];
      float originDirX = originX * invDirx;
      float originDirY = originY * invDiry;
      float originDirZ = originZ * invDirz;
      while (currentNode != END_FLAG2)
      {
        if (currentNode > -1)
        {

          vtkm::Vec<vtkm::Float32, 4> first4 = FlatBVH.Get(currentNode);
          vtkm::Vec<vtkm::Float32, 4> second4 = FlatBVH.Get(currentNode + 1);
          vtkm::Vec<vtkm::Float32, 4> third4 = FlatBVH.Get(currentNode + 2);
          bool hitLeftChild, hitRightChild;

          vtkm::Float32 xmin0 = first4[0] * invDirx - originDirX;
          vtkm::Float32 ymin0 = first4[1] * invDiry - originDirY;
          vtkm::Float32 zmin0 = first4[2] * invDirz - originDirZ;
          vtkm::Float32 xmax0 = first4[3] * invDirx - originDirX;
          vtkm::Float32 ymax0 = second4[0] * invDiry - originDirY;
          vtkm::Float32 zmax0 = second4[1] * invDirz - originDirZ;

          vtkm::Float32 min0 =
            vtkm::Max(vtkm::Max(vtkm::Max(vtkm::Min(ymin0, ymax0), vtkm::Min(xmin0, xmax0)),
                                vtkm::Min(zmin0, zmax0)),
                      0.f);
          vtkm::Float32 max0 =
            vtkm::Min(vtkm::Min(vtkm::Min(vtkm::Max(ymin0, ymax0), vtkm::Max(xmin0, xmax0)),
                                vtkm::Max(zmin0, zmax0)),
                      minDistance);
          hitLeftChild = (max0 >= min0);

          vtkm::Float32 xmin1 = second4[2] * invDirx - originDirX;
          vtkm::Float32 ymin1 = second4[3] * invDiry - originDirY;
          vtkm::Float32 zmin1 = third4[0] * invDirz - originDirZ;
          vtkm::Float32 xmax1 = third4[1] * invDirx - originDirX;
          vtkm::Float32 ymax1 = third4[2] * invDiry - originDirY;
          vtkm::Float32 zmax1 = third4[3] * invDirz - originDirZ;

          vtkm::Float32 min1 =
            vtkm::Max(vtkm::Max(vtkm::Max(vtkm::Min(ymin1, ymax1), vtkm::Min(xmin1, xmax1)),
                                vtkm::Min(zmin1, zmax1)),
                      0.f);
          vtkm::Float32 max1 =
            vtkm::Min(vtkm::Min(vtkm::Min(vtkm::Max(ymin1, ymax1), vtkm::Max(xmin1, xmax1)),
                                vtkm::Max(zmin1, zmax1)),
                      minDistance);
          hitRightChild = (max1 >= min1);

          if (!hitLeftChild && !hitRightChild)
          {
            currentNode = todo[stackptr];
            stackptr--;
          }
          else
          {
            vtkm::Vec<vtkm::Float32, 4> children =
              FlatBVH.Get(currentNode + 3); //Children.Get(currentNode);
            vtkm::Int32 leftChild;
            memcpy(&leftChild, &children[0], 4);
            vtkm::Int32 rightChild;
            memcpy(&rightChild, &children[1], 4);
            currentNode = (hitLeftChild) ? leftChild : rightChild;
            if (hitLeftChild && hitRightChild)
            {
              if (min0 > min1)
              {
                currentNode = rightChild;
                stackptr++;
                todo[stackptr] = leftChild;
              }
              else
              {
                stackptr++;
                todo[stackptr] = rightChild;
              }
            }
          }
        } // if inner node

        if (currentNode < 0 && currentNode != barrier) //check register usage
        {
          currentNode = -currentNode - 1; //swap the neg address
          vtkm::Vec<Int32, 4> leafnode = Leafs.Get(currentNode);
          vtkm::Vec<vtkm::Float32, 3> a = vtkm::Vec<vtkm::Float32, 3>(points.Get(leafnode[1]));
          vtkm::Vec<vtkm::Float32, 3> b = vtkm::Vec<vtkm::Float32, 3>(points.Get(leafnode[2]));
          vtkm::Vec<vtkm::Float32, 3> c = vtkm::Vec<vtkm::Float32, 3>(points.Get(leafnode[3]));

          vtkm::Vec<vtkm::Float32, 3> e1 = b - a;
          vtkm::Vec<vtkm::Float32, 3> e2 = c - a;

          vtkm::Vec<vtkm::Float32, 3> p;
          p[0] = diry * e2[2] - dirz * e2[1];
          p[1] = dirz * e2[0] - dirx * e2[2];
          p[2] = dirx * e2[1] - diry * e2[0];
          vtkm::Float32 dot = e1[0] * p[0] + e1[1] * p[1] + e1[2] * p[2];
          if (dot != 0.f)
          {
            dot = 1.f / dot;
            vtkm::Vec<vtkm::Float32, 3> t;
            t[0] = originX - a[0];
            t[1] = originY - a[1];
            t[2] = originZ - a[2];

            float u = (t[0] * p[0] + t[1] * p[1] + t[2] * p[2]) * dot;
            if (u >= (0.f - EPSILON2) && u <= (1.f + EPSILON2))
            {

              vtkm::Vec<Float32, 3> q; // = t % e1;
              q[0] = t[1] * e1[2] - t[2] * e1[1];
              q[1] = t[2] * e1[0] - t[0] * e1[2];
              q[2] = t[0] * e1[1] - t[1] * e1[0];
              vtkm::Float32 v = (dirx * q[0] + diry * q[1] + dirz * q[2]) * dot;

              if (v >= (0.f - EPSILON2) && v <= (1.f + EPSILON2))
              {

                vtkm::Float32 dist = (e2[0] * q[0] + e2[1] * q[1] + e2[2] * q[2]) * dot;
                if ((dist > EPSILON2 && dist < minDistance) && !(u + v > 1))
                {
                  minDistance = dist;
                  hitIndex = currentNode;
                  minU = u;
                  minV = v;
                  if (Occlusion)
                    return; //or set todo to -1
                }
              }
            }
          }

          currentNode = todo[stackptr];
          stackptr--;
        } // if leaf node

      } //while
      distance = minDistance;

    } // ()
  };

  VTKM_CONT
  void run(Ray<DeviceAdapter>& rays, LinearBVH& bvh,
           vtkm::cont::DynamicArrayHandleCoordinateSystem coordsHandle)
  {
    vtkm::worklet::DispatcherMapField<Intersector>(Intersector(false, 10000000.f, bvh))
      .Invoke(rays.Dir, rays.Origin, rays.Distance, rays.U, rays.V, rays.HitIdx, coordsHandle);
  }

}; // class intersector
}
}
} //namespace vtkm::rendering::raytracing
#endif //vtk_m_rendering_raytracing_TriagnleIntersector_h
