//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_rendering_raytracing_Quad_Intersector_h
#define vtk_m_rendering_raytracing_Quad_Intersector_h

#include <vtkm/cont/Algorithm.h>
#include <vtkm/rendering/raytracing/ShapeIntersector.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{
namespace detail
{

#define AABB_EPSILON 1.0e-4f
class FindQuadAABBs : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  FindQuadAABBs() {}
  typedef void ControlSignature(FieldIn<>,
                                FieldOut<>,
                                FieldOut<>,
                                FieldOut<>,
                                FieldOut<>,
                                FieldOut<>,
                                FieldOut<>,
                                WholeArrayIn<Vec3RenderingTypes>);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7, _8);
  template <typename PointPortalType>
  VTKM_EXEC void operator()(const vtkm::Vec<vtkm::Id, 5> quadId,
                            vtkm::Float32& xmin,
                            vtkm::Float32& ymin,
                            vtkm::Float32& zmin,
                            vtkm::Float32& xmax,
                            vtkm::Float32& ymax,
                            vtkm::Float32& zmax,
                            const PointPortalType& points) const
  {
    // cast to Float32
    vtkm::Vec<vtkm::Float32, 3> q, r, s, t;

    q = static_cast<vtkm::Vec<vtkm::Float32, 3>>(points.Get(quadId[1]));
    r = static_cast<vtkm::Vec<vtkm::Float32, 3>>(points.Get(quadId[2]));
    s = static_cast<vtkm::Vec<vtkm::Float32, 3>>(points.Get(quadId[3]));
    t = static_cast<vtkm::Vec<vtkm::Float32, 3>>(points.Get(quadId[4]));

    xmin = q[0];
    ymin = q[1];
    zmin = q[2];
    xmax = xmin;
    ymax = ymin;
    zmax = zmin;
    xmin = vtkm::Min(xmin, r[0]);
    ymin = vtkm::Min(ymin, r[1]);
    zmin = vtkm::Min(zmin, r[2]);
    xmax = vtkm::Max(xmax, r[0]);
    ymax = vtkm::Max(ymax, r[1]);
    zmax = vtkm::Max(zmax, r[2]);
    xmin = vtkm::Min(xmin, s[0]);
    ymin = vtkm::Min(ymin, s[1]);
    zmin = vtkm::Min(zmin, s[2]);
    xmax = vtkm::Max(xmax, s[0]);
    ymax = vtkm::Max(ymax, s[1]);
    zmax = vtkm::Max(zmax, s[2]);
    xmin = vtkm::Min(xmin, t[0]);
    ymin = vtkm::Min(ymin, t[1]);
    zmin = vtkm::Min(zmin, t[2]);
    xmax = vtkm::Max(xmax, t[0]);
    ymax = vtkm::Max(ymax, t[1]);
    zmax = vtkm::Max(zmax, t[2]);

    vtkm::Float32 xEpsilon, yEpsilon, zEpsilon;
    const vtkm::Float32 minEpsilon = 1e-6f;
    xEpsilon = vtkm::Max(minEpsilon, AABB_EPSILON * (xmax - xmin));
    yEpsilon = vtkm::Max(minEpsilon, AABB_EPSILON * (ymax - ymin));
    zEpsilon = vtkm::Max(minEpsilon, AABB_EPSILON * (zmax - zmin));

    xmin -= xEpsilon;
    ymin -= yEpsilon;
    zmin -= zEpsilon;
    xmax += xEpsilon;
    ymax += yEpsilon;
    zmax += zEpsilon;
  }

}; //class FindAABBs
}
class QuadIntersector : public ShapeIntersector
{
protected:
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 5>> QuadIds;

public:
  QuadIntersector();
  virtual ~QuadIntersector() override;


  void SetData(const vtkm::cont::CoordinateSystem& coords,
               vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 5>> quadIds)
  {

    this->QuadIds = quadIds;
    this->CoordsHandle = coords;
    AABBs AABB;

    vtkm::worklet::DispatcherMapField<detail::FindQuadAABBs> faabbsInvoker;
    faabbsInvoker.Invoke(this->QuadIds,
                         AABB.xmins,
                         AABB.ymins,
                         AABB.zmins,
                         AABB.xmaxs,
                         AABB.ymaxs,
                         AABB.zmaxs,
                         CoordsHandle);

    //vtkm::worklet::DispatcherMapField<detail::FindQuadAABBs> faabbsInvoker(detail::FindQuadAABBs())
    //  .Invoke(this->QuadIds,
    //          AABB.xmins,
    //          AABB.ymins,
    //          AABB.zmins,
    //          AABB.xmaxs,
    //          AABB.ymaxs,
    //          AABB.zmaxs,
    //          CoordsHandle);

    this->SetAABBs(AABB);
  }
  void IntersectRays(Ray<vtkm::Float32>& rays, bool returnCellIndex = false) override;


  void IntersectRays(Ray<vtkm::Float64>& rays, bool returnCellIndex = false) override;

  template <typename Device, typename Precision>
  void IntersectRaysImp(Device, Ray<Precision>& rays, bool returnCellIndex);


  template <typename Precision>
  void IntersectionDataImp(Ray<Precision>& rays,
                           const vtkm::cont::Field* scalarField,
                           const vtkm::Range& scalarRange);

  void IntersectionData(Ray<vtkm::Float32>& rays,
                        const vtkm::cont::Field* scalarField,
                        const vtkm::Range& scalarRange) override;

  void IntersectionData(Ray<vtkm::Float64>& rays,
                        const vtkm::cont::Field* scalarField,
                        const vtkm::Range& scalarRange) override;

  vtkm::Id GetNumberOfShapes() const override;
}; // class ShapeIntersector
}
}
} //namespace vtkm::rendering::raytracing
#endif //vtk_m_rendering_raytracing_Shape_Intersector_h
