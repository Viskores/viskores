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
#ifndef vtk_m_rendering_raytracing_Cylinder_Intersector_h
#define vtk_m_rendering_raytracing_Cylinder_Intersector_h

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


class FindCylinderAABBs : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  FindCylinderAABBs() {}
  typedef void ControlSignature(FieldIn<>,
                                FieldIn<>,
                                FieldOut<>,
                                FieldOut<>,
                                FieldOut<>,
                                FieldOut<>,
                                FieldOut<>,
                                FieldOut<>,
                                WholeArrayIn<Vec3RenderingTypes>);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7, _8, _9);
  template <typename PointPortalType>
  VTKM_EXEC void operator()(const vtkm::Id3 cylId,
                            const vtkm::Float32& radius,
                            vtkm::Float32& xmin,
                            vtkm::Float32& ymin,
                            vtkm::Float32& zmin,
                            vtkm::Float32& xmax,
                            vtkm::Float32& ymax,
                            vtkm::Float32& zmax,
                            const PointPortalType& points) const
  {
    // cast to Float32
    vtkm::Vec<vtkm::Float32, 3> point1, point2;
    vtkm::Vec<vtkm::Float32, 3> temp;

    point1 = static_cast<vtkm::Vec<vtkm::Float32, 3>>(points.Get(cylId[1]));
    point2 = static_cast<vtkm::Vec<vtkm::Float32, 3>>(points.Get(cylId[2]));

    temp[0] = radius;
    temp[1] = 0.0f;
    temp[2] = 0.0f;
    xmin = ymin = zmin = vtkm::Infinity32();
    xmax = ymax = zmax = vtkm::NegativeInfinity32();


    //set first point to max and min
    Bounds(point1, radius, xmin, ymin, zmin, xmax, ymax, zmax);

    Bounds(point2, radius, xmin, ymin, zmin, xmax, ymax, zmax);
  }

  VTKM_EXEC void Bounds(const vtkm::Vec<vtkm::Float32, 3>& point,
                        const vtkm::Float32& radius,
                        vtkm::Float32& xmin,
                        vtkm::Float32& ymin,
                        vtkm::Float32& zmin,
                        vtkm::Float32& xmax,
                        vtkm::Float32& ymax,
                        vtkm::Float32& zmax) const
  {
    vtkm::Vec<vtkm::Float32, 3> temp, p;
    temp[0] = radius;
    temp[1] = 0.0f;
    temp[2] = 0.0f;
    p = point + temp;

    xmin = vtkm::Min(xmin, p[0]);
    xmax = vtkm::Max(xmax, p[0]);
    ymin = vtkm::Min(ymin, p[1]);
    ymax = vtkm::Max(ymax, p[1]);
    zmin = vtkm::Min(zmin, p[2]);
    zmax = vtkm::Max(zmax, p[2]);

    p = point - temp;
    xmin = vtkm::Min(xmin, p[0]);
    xmax = vtkm::Max(xmax, p[0]);
    ymin = vtkm::Min(ymin, p[1]);
    ymax = vtkm::Max(ymax, p[1]);
    zmin = vtkm::Min(zmin, p[2]);
    zmax = vtkm::Max(zmax, p[2]);

    temp[0] = 0.f;
    temp[1] = radius;
    temp[2] = 0.f;

    p = point + temp;
    xmin = vtkm::Min(xmin, p[0]);
    xmax = vtkm::Max(xmax, p[0]);
    ymin = vtkm::Min(ymin, p[1]);
    ymax = vtkm::Max(ymax, p[1]);
    zmin = vtkm::Min(zmin, p[2]);
    zmax = vtkm::Max(zmax, p[2]);

    p = point - temp;
    xmin = vtkm::Min(xmin, p[0]);
    xmax = vtkm::Max(xmax, p[0]);
    ymin = vtkm::Min(ymin, p[1]);
    ymax = vtkm::Max(ymax, p[1]);
    zmin = vtkm::Min(zmin, p[2]);
    zmax = vtkm::Max(zmax, p[2]);

    temp[0] = 0.f;
    temp[1] = 0.f;
    temp[2] = radius;

    p = point + temp;
    xmin = vtkm::Min(xmin, p[0]);
    xmax = vtkm::Max(xmax, p[0]);
    ymin = vtkm::Min(ymin, p[1]);
    ymax = vtkm::Max(ymax, p[1]);
    zmin = vtkm::Min(zmin, p[2]);
    zmax = vtkm::Max(zmax, p[2]);

    p = point - temp;
    xmin = vtkm::Min(xmin, p[0]);
    xmax = vtkm::Max(xmax, p[0]);
    ymin = vtkm::Min(ymin, p[1]);
    ymax = vtkm::Max(ymax, p[1]);
    zmin = vtkm::Min(zmin, p[2]);
    zmax = vtkm::Max(zmax, p[2]);
  }
}; //class FindAABBs
}
class CylinderIntersector : public ShapeIntersector
{
protected:
  vtkm::cont::ArrayHandle<vtkm::Id3> CylIds;
  vtkm::cont::ArrayHandle<vtkm::Float32> Radii;

public:
  CylinderIntersector();
  virtual ~CylinderIntersector() override;


  void SetData(const vtkm::cont::CoordinateSystem& coords,
               vtkm::cont::ArrayHandle<vtkm::Id3> cylIds,
               vtkm::cont::ArrayHandle<vtkm::Float32> radii)
  {
    this->Radii = radii;
    this->CylIds = cylIds;
    this->CoordsHandle = coords;
    AABBs AABB;

    vtkm::worklet::DispatcherMapField<detail::FindCylinderAABBs>(detail::FindCylinderAABBs())
      .Invoke(this->CylIds,
              this->Radii,
              AABB.xmins,
              AABB.ymins,
              AABB.zmins,
              AABB.xmaxs,
              AABB.ymaxs,
              AABB.zmaxs,
              CoordsHandle);

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
