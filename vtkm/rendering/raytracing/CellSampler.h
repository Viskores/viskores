//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_raytracing_CellSampler_h
#define vtk_m_rendering_raytracing_CellSampler_h

#include <vtkm/VecVariable.h>
#include <vtkm/exec/CellInterpolate.h>
#include <vtkm/exec/ParametricCoordinates.h>

#ifndef CELL_SHAPE_ZOO
#define CELL_SHAPE_ZOO 255
#endif

#ifndef CELL_SHAPE_STRUCTURED
#define CELL_SHAPE_STRUCTURED 254
#endif

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

namespace detail
{
template <typename CellTag>
VTKM_EXEC_CONT inline vtkm::Int32 GetNumberOfPoints(CellTag tag);

template <>
VTKM_EXEC_CONT inline vtkm::Int32 GetNumberOfPoints<vtkm::CellShapeTagHexahedron>(
  vtkm::CellShapeTagHexahedron vtkmNotUsed(tag))
{
  return 8;
}

template <>
VTKM_EXEC_CONT inline vtkm::Int32 GetNumberOfPoints<vtkm::CellShapeTagTetra>(
  vtkm::CellShapeTagTetra vtkmNotUsed(tag))
{
  return 4;
}

template <>
VTKM_EXEC_CONT inline vtkm::Int32 GetNumberOfPoints<vtkm::CellShapeTagWedge>(
  vtkm::CellShapeTagWedge vtkmNotUsed(tag))
{
  return 6;
}

template <>
VTKM_EXEC_CONT inline vtkm::Int32 GetNumberOfPoints<vtkm::CellShapeTagPyramid>(
  vtkm::CellShapeTagPyramid vtkmNotUsed(tag))
{
  return 5;
}

template <typename P, typename S, typename WorkletType, typename CellShapeTagType>
VTKM_EXEC_CONT inline bool Sample(const vtkm::Vec<vtkm::Vec<P, 3>, 8>& points,
                                  const vtkm::Vec<S, 8>& scalars,
                                  const vtkm::Vec<P, 3>& sampleLocation,
                                  S& lerpedScalar,
                                  const WorkletType& callingWorklet,
                                  const CellShapeTagType& shapeTag)
{

  bool validSample = true;
  vtkm::VecVariable<vtkm::Vec<P, 3>, 8> pointsVec;
  vtkm::VecVariable<S, 8> scalarVec;
  for (vtkm::Int32 i = 0; i < GetNumberOfPoints(shapeTag); ++i)
  {
    pointsVec.Append(points[i]);
    scalarVec.Append(scalars[i]);
  }
  bool success = false; // ignored
  vtkm::Vec<P, 3> pcoords = vtkm::exec::WorldCoordinatesToParametricCoordinates(
    pointsVec, sampleLocation, shapeTag, success, callingWorklet);
  P pmin, pmax;
  pmin = vtkm::Min(vtkm::Min(pcoords[0], pcoords[1]), pcoords[2]);
  pmax = vtkm::Max(vtkm::Max(pcoords[0], pcoords[1]), pcoords[2]);
  if (pmin < 0.f || pmax > 1.f)
  {
    validSample = false;
  }
  lerpedScalar = vtkm::exec::CellInterpolate(scalarVec, pcoords, shapeTag, callingWorklet);
  return validSample;
}

template <typename S, typename P, typename WorkletType, typename CellShapeTagType>
VTKM_EXEC_CONT inline bool Sample(const vtkm::VecAxisAlignedPointCoordinates<3>& points,
                                  const vtkm::Vec<S, 8>& scalars,
                                  const vtkm::Vec<P, 3>& sampleLocation,
                                  S& lerpedScalar,
                                  const WorkletType& callingWorklet,
                                  const CellShapeTagType& vtkmNotUsed(shapeTag))
{

  bool validSample = true;
  bool success;
  vtkm::Vec<P, 3> pcoords = vtkm::exec::WorldCoordinatesToParametricCoordinates(
    points, sampleLocation, vtkm::CellShapeTagHexahedron(), success, callingWorklet);
  P pmin, pmax;
  pmin = vtkm::Min(vtkm::Min(pcoords[0], pcoords[1]), pcoords[2]);
  pmax = vtkm::Max(vtkm::Max(pcoords[0], pcoords[1]), pcoords[2]);
  if (pmin < 0.f || pmax > 1.f)
  {
    validSample = false;
  }
  lerpedScalar =
    vtkm::exec::CellInterpolate(scalars, pcoords, vtkm::CellShapeTagHexahedron(), callingWorklet);
  return validSample;
}
} // namespace detail

//
//  General Template: returns false if sample location is outside the cell
//
template <int CellType>
class CellSampler
{
public:
  template <typename P, typename S, typename WorkletType>
  VTKM_EXEC_CONT inline bool SampleCell(const vtkm::Vec<vtkm::Vec<P, 3>, 8>& vtkmNotUsed(points),
                                        const vtkm::Vec<S, 8>& vtkmNotUsed(scalars),
                                        const vtkm::Vec<P, 3>& vtkmNotUsed(sampleLocation),
                                        S& vtkmNotUsed(lerpedScalar),
                                        const WorkletType& vtkmNotUsed(callingWorklet),
                                        const vtkm::Int32& vtkmNotUsed(cellShape = CellType)) const
  {
    static_assert(CellType != CELL_SHAPE_ZOO && CellType != CELL_SHAPE_STRUCTURED &&
                    CellType != CELL_SHAPE_HEXAHEDRON && CellType != CELL_SHAPE_TETRA &&
                    CellType != CELL_SHAPE_WEDGE && CellType != CELL_SHAPE_PYRAMID,
                  "Cell Sampler: Default template. This should not happen.\n");
    return false;
  }
};

//
// Zoo Sampler
//
template <>
class CellSampler<255>
{
public:
  template <typename P, typename S, typename WorkletType>
  VTKM_EXEC_CONT inline bool SampleCell(const vtkm::Vec<vtkm::Vec<P, 3>, 8>& points,
                                        const vtkm::Vec<S, 8>& scalars,
                                        const vtkm::Vec<P, 3>& sampleLocation,
                                        S& lerpedScalar,
                                        const WorkletType& callingWorklet,
                                        const vtkm::Int32& cellShape) const
  {
    bool valid = false;
    if (cellShape == CELL_SHAPE_HEXAHEDRON)
    {
      valid = detail::Sample(points,
                             scalars,
                             sampleLocation,
                             lerpedScalar,
                             callingWorklet,
                             vtkm::CellShapeTagHexahedron());
    }

    if (cellShape == CELL_SHAPE_TETRA)
    {
      valid = detail::Sample(
        points, scalars, sampleLocation, lerpedScalar, callingWorklet, vtkm::CellShapeTagTetra());
    }

    if (cellShape == CELL_SHAPE_WEDGE)
    {
      valid = detail::Sample(
        points, scalars, sampleLocation, lerpedScalar, callingWorklet, vtkm::CellShapeTagWedge());
    }
    if (cellShape == CELL_SHAPE_PYRAMID)
    {
      valid = detail::Sample(
        points, scalars, sampleLocation, lerpedScalar, callingWorklet, vtkm::CellShapeTagPyramid());
    }
    return valid;
  }
};

//
//  Single type hex
//
template <>
class CellSampler<CELL_SHAPE_HEXAHEDRON>
{
public:
  template <typename P, typename S, typename WorkletType>
  VTKM_EXEC_CONT inline bool SampleCell(
    const vtkm::Vec<vtkm::Vec<P, 3>, 8>& points,
    const vtkm::Vec<S, 8>& scalars,
    const vtkm::Vec<P, 3>& sampleLocation,
    S& lerpedScalar,
    const WorkletType& callingWorklet,
    const vtkm::Int32& vtkmNotUsed(cellShape = CELL_SHAPE_HEXAHEDRON)) const
  {
    return detail::Sample(points,
                          scalars,
                          sampleLocation,
                          lerpedScalar,
                          callingWorklet,
                          vtkm::CellShapeTagHexahedron());
  }
};

//
//  Single type hex uniform and rectilinear
//  calls fast path for sampling
//
template <>
class CellSampler<CELL_SHAPE_STRUCTURED>
{
public:
  template <typename P, typename S, typename WorkletType>
  VTKM_EXEC_CONT inline bool SampleCell(
    const vtkm::Vec<vtkm::Vec<P, 3>, 8>& points,
    const vtkm::Vec<S, 8>& scalars,
    const vtkm::Vec<P, 3>& sampleLocation,
    S& lerpedScalar,
    const WorkletType& callingWorklet,
    const vtkm::Int32& vtkmNotUsed(cellShape = CELL_SHAPE_HEXAHEDRON)) const
  {
    vtkm::VecAxisAlignedPointCoordinates<3> rPoints(points[0], points[6] - points[0]);
    return detail::Sample(rPoints,
                          scalars,
                          sampleLocation,
                          lerpedScalar,
                          callingWorklet,
                          vtkm::CellShapeTagHexahedron());
  }
};

//
//  Single type pyramid
//
template <>
class CellSampler<CELL_SHAPE_PYRAMID>
{
public:
  template <typename P, typename S, typename WorkletType>
  VTKM_EXEC_CONT inline bool SampleCell(
    const vtkm::Vec<vtkm::Vec<P, 3>, 8>& points,
    const vtkm::Vec<S, 8>& scalars,
    const vtkm::Vec<P, 3>& sampleLocation,
    S& lerpedScalar,
    const WorkletType& callingWorklet,
    const vtkm::Int32& vtkmNotUsed(cellShape = CELL_SHAPE_PYRAMID)) const
  {
    return detail::Sample(
      points, scalars, sampleLocation, lerpedScalar, callingWorklet, vtkm::CellShapeTagPyramid());
  }
};


//
//  Single type Tet
//
template <>
class CellSampler<CELL_SHAPE_TETRA>
{
public:
  template <typename P, typename S, typename WorkletType>
  VTKM_EXEC_CONT inline bool SampleCell(
    const vtkm::Vec<vtkm::Vec<P, 3>, 8>& points,
    const vtkm::Vec<S, 8>& scalars,
    const vtkm::Vec<P, 3>& sampleLocation,
    S& lerpedScalar,
    const WorkletType& callingWorklet,
    const vtkm::Int32& vtkmNotUsed(cellShape = CELL_SHAPE_TETRA)) const
  {
    return detail::Sample(
      points, scalars, sampleLocation, lerpedScalar, callingWorklet, vtkm::CellShapeTagTetra());
  }
};

//
//  Single type Wedge
//
template <>
class CellSampler<CELL_SHAPE_WEDGE>
{
public:
  template <typename P, typename S, typename WorkletType>
  VTKM_EXEC_CONT inline bool SampleCell(
    const vtkm::Vec<vtkm::Vec<P, 3>, 8>& points,
    const vtkm::Vec<S, 8>& scalars,
    const vtkm::Vec<P, 3>& sampleLocation,
    S& lerpedScalar,
    const WorkletType& callingWorklet,
    const vtkm::Int32& vtkmNotUsed(cellShape = CELL_SHAPE_WEDGE)) const
  {
    return detail::Sample(
      points, scalars, sampleLocation, lerpedScalar, callingWorklet, vtkm::CellShapeTagWedge());
  }
};
}
}
} // namespace vtkm::rendering::raytracing
#endif
