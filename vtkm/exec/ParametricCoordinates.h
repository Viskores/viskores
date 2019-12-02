//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_ParametricCoordinates_h
#define vtk_m_exec_ParametricCoordinates_h

#include <vtkm/Assert.h>
#include <vtkm/CellShape.h>
#include <vtkm/VecAxisAlignedPointCoordinates.h>
#include <vtkm/exec/CellInterpolate.h>
#include <vtkm/exec/FunctorBase.h>
#include <vtkm/exec/internal/FastVec.h>
#include <vtkm/internal/Assume.h>

#include <lcl/lcl.h>

namespace vtkm
{
namespace exec
{

//-----------------------------------------------------------------------------
template <typename ParametricCoordType, typename CellShapeTag>
static inline VTKM_EXEC void ParametricCoordinatesCenter(vtkm::IdComponent numPoints,
                                                         vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                         CellShapeTag,
                                                         const vtkm::exec::FunctorBase&)
{
  auto lclTag = typename vtkm::internal::CellShapeTagVtkmToVtkc<CellShapeTag>::Type{};

  (void)numPoints; // Silence compiler warnings.
  VTKM_ASSERT(numPoints == lclTag.numberOfPoints());

  pcoords = vtkm::TypeTraits<vtkm::Vec<ParametricCoordType, 3>>::ZeroInitialization();
  lcl::parametricCenter(lclTag, pcoords);
}

template <typename ParametricCoordType>
static inline VTKM_EXEC void ParametricCoordinatesCenter(vtkm::IdComponent numPoints,
                                                         vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                         vtkm::CellShapeTagEmpty,
                                                         const vtkm::exec::FunctorBase&)
{
  (void)numPoints; // Silence compiler warnings.
  VTKM_ASSERT(numPoints == 0);
  pcoords = vtkm::TypeTraits<vtkm::Vec<ParametricCoordType, 3>>::ZeroInitialization();
}

template <typename ParametricCoordType>
static inline VTKM_EXEC void ParametricCoordinatesCenter(vtkm::IdComponent numPoints,
                                                         vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                         vtkm::CellShapeTagVertex,
                                                         const vtkm::exec::FunctorBase&)
{
  (void)numPoints; // Silence compiler warnings.
  VTKM_ASSERT(numPoints == 1);
  pcoords = vtkm::TypeTraits<vtkm::Vec<ParametricCoordType, 3>>::ZeroInitialization();
}

template <typename ParametricCoordType>
static inline VTKM_EXEC void ParametricCoordinatesCenter(vtkm::IdComponent numPoints,
                                                         vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                         vtkm::CellShapeTagPolyLine,
                                                         const vtkm::exec::FunctorBase& worklet)
{
  switch (numPoints)
  {
    case 1:
      ParametricCoordinatesCenter(numPoints, pcoords, vtkm::CellShapeTagVertex(), worklet);
      return;
    case 2:
      ParametricCoordinatesCenter(numPoints, pcoords, vtkm::CellShapeTagLine(), worklet);
      return;
  }
  pcoords[0] = 0.5;
  pcoords[1] = 0;
  pcoords[2] = 0;
}

template <typename ParametricCoordType>
static inline VTKM_EXEC void ParametricCoordinatesCenter(vtkm::IdComponent numPoints,
                                                         vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                         vtkm::CellShapeTagPolygon,
                                                         const vtkm::exec::FunctorBase& worklet)
{
  VTKM_ASSERT(numPoints > 0);
  switch (numPoints)
  {
    case 1:
      ParametricCoordinatesCenter(numPoints, pcoords, vtkm::CellShapeTagVertex(), worklet);
      break;
    case 2:
      ParametricCoordinatesCenter(numPoints, pcoords, vtkm::CellShapeTagLine(), worklet);
      break;
    default:
      pcoords = vtkm::TypeTraits<vtkm::Vec<ParametricCoordType, 3>>::ZeroInitialization();
      lcl::parametricCenter(lcl::Polygon(numPoints), pcoords);
      break;
  }
}

//-----------------------------------------------------------------------------
/// Returns the parametric center of the given cell shape with the given number
/// of points.
///
template <typename ParametricCoordType>
static inline VTKM_EXEC void ParametricCoordinatesCenter(vtkm::IdComponent numPoints,
                                                         vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                         vtkm::CellShapeTagGeneric shape,
                                                         const vtkm::exec::FunctorBase& worklet)
{
  switch (shape.Id)
  {
    vtkmGenericCellShapeMacro(
      ParametricCoordinatesCenter(numPoints, pcoords, CellShapeTag(), worklet));
    default:
      worklet.RaiseError("Bad shape given to ParametricCoordinatesCenter.");
      pcoords[0] = pcoords[1] = pcoords[2] = 0;
      break;
  }
}

/// Returns the parametric center of the given cell shape with the given number
/// of points.
///
template <typename CellShapeTag>
static inline VTKM_EXEC vtkm::Vec3f ParametricCoordinatesCenter(
  vtkm::IdComponent numPoints,
  CellShapeTag shape,
  const vtkm::exec::FunctorBase& worklet)
{
  vtkm::Vec3f pcoords(0.0f);
  ParametricCoordinatesCenter(numPoints, pcoords, shape, worklet);
  return pcoords;
}

//-----------------------------------------------------------------------------
template <typename ParametricCoordType, typename CellShapeTag>
static inline VTKM_EXEC void ParametricCoordinatesPoint(vtkm::IdComponent numPoints,
                                                        vtkm::IdComponent pointIndex,
                                                        vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                        CellShapeTag,
                                                        const vtkm::exec::FunctorBase&)
{
  auto lclTag = typename vtkm::internal::CellShapeTagVtkmToVtkc<CellShapeTag>::Type{};

  (void)numPoints; // Silence compiler warnings.
  VTKM_ASSUME(numPoints == lclTag.numberOfPoints());
  VTKM_ASSUME((pointIndex >= 0) && (pointIndex < numPoints));

  pcoords = vtkm::TypeTraits<vtkm::Vec<ParametricCoordType, 3>>::ZeroInitialization();
  lcl::parametricPoint(lclTag, pointIndex, pcoords);
}

template <typename ParametricCoordType>
static inline VTKM_EXEC void ParametricCoordinatesPoint(vtkm::IdComponent,
                                                        vtkm::IdComponent,
                                                        vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                        vtkm::CellShapeTagEmpty,
                                                        const vtkm::exec::FunctorBase& worklet)
{
  worklet.RaiseError("Empty cell has no points.");
  pcoords[0] = pcoords[1] = pcoords[2] = 0;
}

template <typename ParametricCoordType>
static inline VTKM_EXEC void ParametricCoordinatesPoint(vtkm::IdComponent numPoints,
                                                        vtkm::IdComponent pointIndex,
                                                        vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                        vtkm::CellShapeTagVertex,
                                                        const vtkm::exec::FunctorBase&)
{
  (void)numPoints;  // Silence compiler warnings.
  (void)pointIndex; // Silence compiler warnings.
  VTKM_ASSUME(numPoints == 1);
  VTKM_ASSUME(pointIndex == 0);
  pcoords = vtkm::TypeTraits<vtkm::Vec<ParametricCoordType, 3>>::ZeroInitialization();
}

template <typename ParametricCoordType>
static inline VTKM_EXEC void ParametricCoordinatesPoint(vtkm::IdComponent numPoints,
                                                        vtkm::IdComponent pointIndex,
                                                        vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                        vtkm::CellShapeTagPolyLine,
                                                        const vtkm::exec::FunctorBase& functor)
{
  switch (numPoints)
  {
    case 1:
      ParametricCoordinatesPoint(
        numPoints, pointIndex, pcoords, vtkm::CellShapeTagVertex(), functor);
      return;
    case 2:
      ParametricCoordinatesPoint(numPoints, pointIndex, pcoords, vtkm::CellShapeTagLine(), functor);
      return;
  }
  pcoords[0] =
    static_cast<ParametricCoordType>(pointIndex) / static_cast<ParametricCoordType>(numPoints - 1);
  pcoords[1] = 0;
  pcoords[2] = 0;
}

template <typename ParametricCoordType>
static inline VTKM_EXEC void ParametricCoordinatesPoint(vtkm::IdComponent numPoints,
                                                        vtkm::IdComponent pointIndex,
                                                        vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                        vtkm::CellShapeTagPolygon,
                                                        const vtkm::exec::FunctorBase& worklet)
{
  VTKM_ASSUME((numPoints > 0));
  VTKM_ASSUME((pointIndex >= 0) && (pointIndex < numPoints));

  switch (numPoints)
  {
    case 1:
      ParametricCoordinatesPoint(
        numPoints, pointIndex, pcoords, vtkm::CellShapeTagVertex(), worklet);
      return;
    case 2:
      ParametricCoordinatesPoint(numPoints, pointIndex, pcoords, vtkm::CellShapeTagLine(), worklet);
      return;
    default:
      pcoords = vtkm::TypeTraits<vtkm::Vec<ParametricCoordType, 3>>::ZeroInitialization();
      lcl::parametricPoint(lcl::Polygon(numPoints), pointIndex, pcoords);
      return;
  }
}

//-----------------------------------------------------------------------------
/// Returns the parametric coordinate of a cell point of the given shape with
/// the given number of points.
///
template <typename ParametricCoordType>
static inline VTKM_EXEC void ParametricCoordinatesPoint(vtkm::IdComponent numPoints,
                                                        vtkm::IdComponent pointIndex,
                                                        vtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                        vtkm::CellShapeTagGeneric shape,
                                                        const vtkm::exec::FunctorBase& worklet)
{
  switch (shape.Id)
  {
    vtkmGenericCellShapeMacro(
      ParametricCoordinatesPoint(numPoints, pointIndex, pcoords, CellShapeTag(), worklet));
    default:
      worklet.RaiseError("Bad shape given to ParametricCoordinatesPoint.");
      pcoords[0] = pcoords[1] = pcoords[2] = 0;
      break;
  }
}

/// Returns the parametric coordinate of a cell point of the given shape with
/// the given number of points.
///
template <typename CellShapeTag>
static inline VTKM_EXEC vtkm::Vec3f ParametricCoordinatesPoint(
  vtkm::IdComponent numPoints,
  vtkm::IdComponent pointIndex,
  CellShapeTag shape,
  const vtkm::exec::FunctorBase& worklet)
{
  vtkm::Vec3f pcoords(0.0f);
  ParametricCoordinatesPoint(numPoints, pointIndex, pcoords, shape, worklet);
  return pcoords;
}

//-----------------------------------------------------------------------------
namespace internal
{

template <typename VtkcCellShapeTag, typename WorldCoordVector, typename PCoordType>
static inline VTKM_EXEC typename WorldCoordVector::ComponentType
ParametricCoordinatesToWorldCoordinatesImpl(VtkcCellShapeTag tag,
                                            const WorldCoordVector& pointWCoords,
                                            const PCoordType& pcoords,
                                            const vtkm::exec::FunctorBase& worklet)
{
  typename WorldCoordVector::ComponentType wcoords(0);
  auto status =
    lcl::parametricToWorld(tag, lcl::makeFieldAccessorNestedSOA(pointWCoords, 3), pcoords, wcoords);
  if (status != lcl::ErrorCode::SUCCESS)
  {
    worklet.RaiseError(lcl::errorString(status));
  }
  return wcoords;
}

} // namespace internal

template <typename WorldCoordVector, typename PCoordType, typename CellShapeTag>
static inline VTKM_EXEC typename WorldCoordVector::ComponentType
ParametricCoordinatesToWorldCoordinates(const WorldCoordVector& pointWCoords,
                                        const vtkm::Vec<PCoordType, 3>& pcoords,
                                        CellShapeTag shape,
                                        const vtkm::exec::FunctorBase& worklet)
{
  auto numPoints = pointWCoords.GetNumberOfComponents();
  return internal::ParametricCoordinatesToWorldCoordinatesImpl(
    vtkm::internal::make_VtkcCellShapeTag(shape, numPoints), pointWCoords, pcoords, worklet);
}

template <typename WorldCoordVector, typename PCoordType>
static inline VTKM_EXEC typename WorldCoordVector::ComponentType
ParametricCoordinatesToWorldCoordinates(const WorldCoordVector& pointWCoords,
                                        const vtkm::Vec<PCoordType, 3>& pcoords,
                                        vtkm::CellShapeTagEmpty empty,
                                        const vtkm::exec::FunctorBase& worklet)
{
  return vtkm::exec::CellInterpolate(pointWCoords, pcoords, empty, worklet);
}

template <typename WorldCoordVector, typename PCoordType>
static inline VTKM_EXEC typename WorldCoordVector::ComponentType
ParametricCoordinatesToWorldCoordinates(const WorldCoordVector& pointWCoords,
                                        const vtkm::Vec<PCoordType, 3>& pcoords,
                                        vtkm::CellShapeTagPolyLine polyLine,
                                        const vtkm::exec::FunctorBase& worklet)
{
  return vtkm::exec::CellInterpolate(pointWCoords, pcoords, polyLine, worklet);
}

template <typename WorldCoordVector, typename PCoordType>
static inline VTKM_EXEC typename WorldCoordVector::ComponentType
ParametricCoordinatesToWorldCoordinates(const WorldCoordVector& pointWCoords,
                                        const vtkm::Vec<PCoordType, 3>& pcoords,
                                        vtkm::CellShapeTagPolygon,
                                        const vtkm::exec::FunctorBase& worklet)
{
  auto numPoints = pointWCoords.GetNumberOfComponents();
  switch (numPoints)
  {
    case 1:
      return ParametricCoordinatesToWorldCoordinates(
        pointWCoords, pcoords, vtkm::CellShapeTagVertex{}, worklet);
    case 2:
      return ParametricCoordinatesToWorldCoordinates(
        pointWCoords, pcoords, vtkm::CellShapeTagLine{}, worklet);
    default:
      return internal::ParametricCoordinatesToWorldCoordinatesImpl(
        lcl::Polygon(numPoints), pointWCoords, pcoords, worklet);
  }
}

template <typename WorldCoordVector, typename PCoordType>
static inline VTKM_EXEC typename WorldCoordVector::ComponentType
ParametricCoordinatesToWorldCoordinates(const vtkm::VecAxisAlignedPointCoordinates<2>& pointWCoords,
                                        const vtkm::Vec<PCoordType, 3>& pcoords,
                                        vtkm::CellShapeTagQuad,
                                        const vtkm::exec::FunctorBase& worklet)
{
  return internal::ParametricCoordinatesToWorldCoordinatesImpl(
    lcl::Pixel{}, pointWCoords, pcoords, worklet);
}

template <typename WorldCoordVector, typename PCoordType>
static inline VTKM_EXEC typename WorldCoordVector::ComponentType
ParametricCoordinatesToWorldCoordinates(const vtkm::VecAxisAlignedPointCoordinates<3>& pointWCoords,
                                        const vtkm::Vec<PCoordType, 3>& pcoords,
                                        vtkm::CellShapeTagHexahedron,
                                        const vtkm::exec::FunctorBase& worklet)
{
  return internal::ParametricCoordinatesToWorldCoordinatesImpl(
    lcl::Voxel{}, pointWCoords, pcoords, worklet);
}

//-----------------------------------------------------------------------------
/// Returns the world coordinate corresponding to the given parametric coordinate of a cell.
///
template <typename WorldCoordVector, typename PCoordType>
static inline VTKM_EXEC typename WorldCoordVector::ComponentType
ParametricCoordinatesToWorldCoordinates(const WorldCoordVector& pointWCoords,
                                        const vtkm::Vec<PCoordType, 3>& pcoords,
                                        vtkm::CellShapeTagGeneric shape,
                                        const vtkm::exec::FunctorBase& worklet)
{
  typename WorldCoordVector::ComponentType wcoords(0);
  switch (shape.Id)
  {
    vtkmGenericCellShapeMacro(wcoords = ParametricCoordinatesToWorldCoordinates(
                                pointWCoords, pcoords, CellShapeTag(), worklet));
    default:
      worklet.RaiseError("Bad shape given to ParametricCoordinatesPoint.");
      break;
  }
  return wcoords;
}

//-----------------------------------------------------------------------------
namespace internal
{

template <typename VtkcCellShapeTag, typename WorldCoordVector>
static inline VTKM_EXEC typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinatesImpl(VtkcCellShapeTag tag,
                                            const WorldCoordVector& pointWCoords,
                                            const typename WorldCoordVector::ComponentType& wcoords,
                                            bool& success,
                                            const vtkm::exec::FunctorBase& worklet)
{
  VTKM_ASSERT(pointWCoords.GetNumberOfComponents() == tag.numberOfPoints());

  auto pcoords = vtkm::TypeTraits<typename WorldCoordVector::ComponentType>::ZeroInitialization();
  auto status =
    lcl::worldToParametric(tag, lcl::makeFieldAccessorNestedSOA(pointWCoords, 3), wcoords, pcoords);

  success = true;
  if (status != lcl::ErrorCode::SUCCESS)
  {
    worklet.RaiseError(lcl::errorString(status));
    success = false;
  }
  return pcoords;
}

} // namespace internal

template <typename WorldCoordVector, typename CellShapeTag>
static inline VTKM_EXEC typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates(const WorldCoordVector& pointWCoords,
                                        const typename WorldCoordVector::ComponentType& wcoords,
                                        CellShapeTag shape,
                                        bool& success,
                                        const vtkm::exec::FunctorBase& worklet)
{
  auto numPoints = pointWCoords.GetNumberOfComponents();
  return internal::WorldCoordinatesToParametricCoordinatesImpl(
    vtkm::internal::make_VtkcCellShapeTag(shape, numPoints),
    pointWCoords,
    wcoords,
    success,
    worklet);
}

template <typename WorldCoordVector>
static inline VTKM_EXEC typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates(const WorldCoordVector&,
                                        const typename WorldCoordVector::ComponentType&,
                                        vtkm::CellShapeTagEmpty,
                                        bool& success,
                                        const vtkm::exec::FunctorBase& worklet)
{
  worklet.RaiseError("Attempted to find point coordinates in empty cell.");
  success = false;
  return typename WorldCoordVector::ComponentType();
}

template <typename WorldCoordVector>
static inline VTKM_EXEC typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates(const WorldCoordVector& pointWCoords,
                                        const typename WorldCoordVector::ComponentType&,
                                        vtkm::CellShapeTagVertex,
                                        bool& success,
                                        const vtkm::exec::FunctorBase& vtkmNotUsed(worklet))
{
  (void)pointWCoords; // Silence compiler warnings.
  VTKM_ASSERT(pointWCoords.GetNumberOfComponents() == 1);
  success = true;
  return typename WorldCoordVector::ComponentType(0, 0, 0);
}

template <typename WorldCoordVector>
static inline VTKM_EXEC typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates(const WorldCoordVector& pointWCoords,
                                        const typename WorldCoordVector::ComponentType& wcoords,
                                        vtkm::CellShapeTagPolyLine,
                                        bool& success,
                                        const vtkm::exec::FunctorBase& worklet)
{
  vtkm::IdComponent numPoints = pointWCoords.GetNumberOfComponents();
  VTKM_ASSERT(pointWCoords.GetNumberOfComponents() >= 1);

  if (numPoints == 1)
  {
    return WorldCoordinatesToParametricCoordinates(
      pointWCoords, wcoords, vtkm::CellShapeTagVertex(), success, worklet);
  }

  using Vector3 = typename WorldCoordVector::ComponentType;
  using T = typename Vector3::ComponentType;

  //Find the closest vertex to the point.
  vtkm::IdComponent idx = 0;
  Vector3 vec = pointWCoords[0] - wcoords;
  T minDistSq = vtkm::Dot(vec, vec);
  for (vtkm::IdComponent i = 1; i < numPoints; i++)
  {
    vec = pointWCoords[i] - wcoords;
    T d = vtkm::Dot(vec, vec);

    if (d < minDistSq)
    {
      idx = i;
      minDistSq = d;
    }
  }

  //Find the right segment, and the parameterization along that segment.
  //Closest to 0, so segment is (0,1)
  if (idx == 0)
  {
    idx = 1;
  }

  vtkm::Vec<Vector3, 2> line(pointWCoords[idx - 1], pointWCoords[idx]);
  auto lpc = WorldCoordinatesToParametricCoordinates(
    line, wcoords, vtkm::CellShapeTagLine{}, success, worklet);

  //Segment param is [0,1] on that segment.
  //Map that onto the param for the entire segment.
  T dParam = static_cast<T>(1) / static_cast<T>(numPoints - 1);
  T polyLineParam = static_cast<T>(idx - 1) * dParam + lpc[0] * dParam;

  return Vector3(polyLineParam, 0, 0);
}

template <typename WorldCoordVector>
static inline VTKM_EXEC typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates(const WorldCoordVector& pointWCoords,
                                        const typename WorldCoordVector::ComponentType& wcoords,
                                        vtkm::CellShapeTagPolygon,
                                        bool& success,
                                        const vtkm::exec::FunctorBase& worklet)
{
  auto numPoints = pointWCoords.GetNumberOfComponents();
  switch (numPoints)
  {
    case 1:
      return WorldCoordinatesToParametricCoordinates(
        pointWCoords, wcoords, vtkm::CellShapeTagVertex{}, success, worklet);
    case 2:
      return WorldCoordinatesToParametricCoordinates(
        pointWCoords, wcoords, vtkm::CellShapeTagLine{}, success, worklet);
    default:
      return internal::WorldCoordinatesToParametricCoordinatesImpl(
        lcl::Polygon(numPoints), pointWCoords, wcoords, success, worklet);
  }
}

static inline VTKM_EXEC vtkm::Vec3f WorldCoordinatesToParametricCoordinates(
  const vtkm::VecAxisAlignedPointCoordinates<2>& pointWCoords,
  const vtkm::Vec3f& wcoords,
  vtkm::CellShapeTagQuad,
  bool& success,
  const FunctorBase& worklet)
{
  return internal::WorldCoordinatesToParametricCoordinatesImpl(
    lcl::Pixel{}, pointWCoords, wcoords, success, worklet);
}

static inline VTKM_EXEC vtkm::Vec3f WorldCoordinatesToParametricCoordinates(
  const vtkm::VecAxisAlignedPointCoordinates<3>& pointWCoords,
  const vtkm::Vec3f& wcoords,
  vtkm::CellShapeTagHexahedron,
  bool& success,
  const FunctorBase& worklet)
{
  return internal::WorldCoordinatesToParametricCoordinatesImpl(
    lcl::Voxel{}, pointWCoords, wcoords, success, worklet);
}

//-----------------------------------------------------------------------------
/// Returns the world paramteric corresponding to the given world coordinate for a cell.
///
template <typename WorldCoordVector>
static inline VTKM_EXEC typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates(const WorldCoordVector& pointWCoords,
                                        const typename WorldCoordVector::ComponentType& wcoords,
                                        vtkm::CellShapeTagGeneric shape,
                                        bool& success,
                                        const vtkm::exec::FunctorBase& worklet)
{
  typename WorldCoordVector::ComponentType result;
  switch (shape.Id)
  {
    vtkmGenericCellShapeMacro(result = WorldCoordinatesToParametricCoordinates(
                                pointWCoords, wcoords, CellShapeTag(), success, worklet));
    default:
      success = false;
      worklet.RaiseError("Unknown cell shape sent to world 2 parametric.");
      return typename WorldCoordVector::ComponentType();
  }

  return result;
}
}
} // namespace vtkm::exec

#endif //vtk_m_exec_ParametricCoordinates_h
