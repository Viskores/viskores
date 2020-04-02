
//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================


#ifndef vtk_m_worklet_contour_flyingedges_pass5_h
#define vtk_m_worklet_contour_flyingedges_pass5_h


#include <vtkm/worklet/contour/FlyingEdgesHelpers.h>
#include <vtkm/worklet/contour/FlyingEdgesTables.h>

#include <vtkm/VectorAnalysis.h>
#include <vtkm/worklet/gradient/StructuredPointGradient.h>

namespace vtkm
{
namespace worklet
{
namespace flying_edges
{

template <typename T>
struct ComputePass5 : public vtkm::worklet::WorkletMapField
{

  vtkm::internal::ArrayPortalUniformPointCoordinates Coordinates;
  bool GenerateNormals;

  ComputePass5() {}
  ComputePass5(const vtkm::Id3& pdims,
               const vtkm::Vec3f& origin,
               const vtkm::Vec3f& spacing,
               bool generateNormals)
    : Coordinates(pdims, origin, spacing)
    , GenerateNormals(generateNormals)
  {
  }

  using ControlSignature = void(FieldIn interpEdgeIds,
                                FieldIn interpWeight,
                                FieldOut points,
                                WholeArrayIn field,
                                WholeArrayOut normals);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, WorkIndex);

  template <typename PT, typename WholeInputField, typename WholeNormalField>
  VTKM_EXEC void operator()(const vtkm::Id2& interpEdgeIds,
                            vtkm::FloatDefault weight,
                            vtkm::Vec<PT, 3>& outPoint,
                            const WholeInputField& field,
                            WholeNormalField& normals,
                            vtkm::Id oidx) const
  {
    {
      vtkm::Vec3f point1 = this->Coordinates.Get(interpEdgeIds[0]);
      vtkm::Vec3f point2 = this->Coordinates.Get(interpEdgeIds[1]);
      outPoint = vtkm::Lerp(point1, point2, weight);
    }

    if (this->GenerateNormals)
    {
      vtkm::Vec<T, 3> g0, g1;
      const vtkm::Id3& dims = this->Coordinates.GetDimensions();
      vtkm::Id3 ijk{ interpEdgeIds[0] % dims[0],
                     (interpEdgeIds[0] / dims[0]) % dims[1],
                     interpEdgeIds[0] / (dims[0] * dims[1]) };

      vtkm::worklet::gradient::StructuredPointGradient gradient;
      vtkm::exec::BoundaryState boundary(ijk, dims);
      vtkm::exec::FieldNeighborhood<vtkm::internal::ArrayPortalUniformPointCoordinates>
        coord_neighborhood(this->Coordinates, boundary);

      vtkm::exec::FieldNeighborhood<WholeInputField> field_neighborhood(field, boundary);


      //compute the gradient at point 1
      gradient(boundary, coord_neighborhood, field_neighborhood, g0);

      //compute the gradient at point 2. This optimization can be optimized
      boundary.IJK = vtkm::Id3{ interpEdgeIds[1] % dims[0],
                                (interpEdgeIds[1] / dims[0]) % dims[1],
                                interpEdgeIds[1] / (dims[0] * dims[1]) };
      gradient(boundary, coord_neighborhood, field_neighborhood, g1);

      vtkm::Vec3f n = vtkm::Lerp(g0, g1, weight);
      const auto mag2 = vtkm::MagnitudeSquared(n);
      if (mag2 > 0.)
      {
        n = n * vtkm::RSqrt(mag2);
      }
      normals.Set(oidx, n);
    }
  }
};
}
}
}
#endif
