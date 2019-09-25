//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.md for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_c_Tetra_h
#define vtk_c_Tetra_h

#include <vtkc/ErrorCode.h>
#include <vtkc/Shapes.h>

#include <vtkc/internal/Common.h>

namespace vtkc
{

class Tetra : public Cell
{
public:
  constexpr VTKC_EXEC Tetra() : Cell(ShapeId::TETRA, 4) {}
  constexpr VTKC_EXEC explicit Tetra(const Cell& cell) : Cell(cell) {}
};

VTKC_EXEC inline vtkc::ErrorCode validate(Tetra tag) noexcept
{
  if (tag.shape() != ShapeId::TETRA)
  {
    return ErrorCode::WRONG_SHAPE_ID_FOR_TAG_TYPE;
  }
  if (tag.numberOfPoints() != 4)
  {
    return ErrorCode::INVALID_NUMBER_OF_POINTS;
  }

  return ErrorCode::SUCCESS;
}

template<typename CoordType>
VTKC_EXEC inline vtkc::ErrorCode parametricCenter(Tetra, CoordType&& pcoords) noexcept
{
  VTKC_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  component(pcoords, 0) = 0.25f;
  component(pcoords, 1) = 0.25f;
  component(pcoords, 2) = 0.25f;
  return ErrorCode::SUCCESS;
}

template<typename CoordType>
VTKC_EXEC inline vtkc::ErrorCode parametricPoint(
  Tetra, IdComponent pointId, CoordType&& pcoords) noexcept
{
  VTKC_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  switch (pointId)
  {
    case 0:
      component(pcoords, 0) = 0.0f;
      component(pcoords, 1) = 0.0f;
      component(pcoords, 2) = 0.0f;
      break;
    case 1:
      component(pcoords, 0) = 1.0f;
      component(pcoords, 1) = 0.0f;
      component(pcoords, 2) = 0.0f;
      break;
    case 2:
      component(pcoords, 0) = 0.0f;
      component(pcoords, 1) = 1.0f;
      component(pcoords, 2) = 0.0f;
      break;
    case 3:
      component(pcoords, 0) = 0.0f;
      component(pcoords, 1) = 0.0f;
      component(pcoords, 2) = 1.0f;
      break;
    default:
      return ErrorCode::INVALID_POINT_ID;
  }

  return ErrorCode::SUCCESS;
}

template<typename CoordType>
VTKC_EXEC inline ComponentType<CoordType> parametricDistance(Tetra, const CoordType& pcoords) noexcept
{
  VTKC_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  ComponentType<CoordType> weights[4];
  weights[0] = ComponentType<CoordType>{1} - component(pcoords, 0) - component(pcoords, 1) - component(pcoords, 2);
  weights[1] = component(pcoords, 0);
  weights[2] = component(pcoords, 1);
  weights[3] = component(pcoords, 2);
  interpolationWeights(Tetra{}, pcoords, weights);
  return internal::findParametricDistance(weights, 4);
}

template<typename CoordType>
VTKC_EXEC inline bool cellInside(Tetra, const CoordType& pcoords) noexcept
{
  VTKC_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  using T = ComponentType<CoordType>;
  return component(pcoords, 0) >= T{0} &&
         component(pcoords, 1) >= T{0} &&
         component(pcoords, 2) >= T{0} &&
         (component(pcoords, 0) + component(pcoords, 1) + component(pcoords, 2)) <= T{1};
}

template <typename CoordType, typename T>
VTKC_EXEC inline void interpolationWeights(Tetra, const CoordType& pcoords, T weights[4]) noexcept
{
  weights[1] = static_cast<T>(component(pcoords, 0));
  weights[2] = static_cast<T>(component(pcoords, 1));
  weights[3] = static_cast<T>(component(pcoords, 2));
  weights[0] = T(1) - weights[1] - weights[2] - weights[3];
}

template <typename Values, typename CoordType, typename Result>
VTKC_EXEC inline vtkc::ErrorCode interpolate(
  Tetra,
  const Values& values,
  const CoordType& pcoords,
  Result&& result) noexcept
{
  VTKC_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  using T = internal::ClosestFloatType<typename Values::ValueType>;

  auto w0 = T(1) - static_cast<T>(component(pcoords, 0) + component(pcoords, 1) + component(pcoords, 2));
  auto w1 = static_cast<T>(component(pcoords, 0));
  auto w2 = static_cast<T>(component(pcoords, 1));
  auto w3 = static_cast<T>(component(pcoords, 2));

  for (IdComponent c = 0; c < values.getNumberOfComponents(); ++c)
  {
    auto v = static_cast<T>(values.getValue(0, c)) * w0 +
             static_cast<T>(values.getValue(1, c)) * w1 +
             static_cast<T>(values.getValue(2, c)) * w2 +
             static_cast<T>(values.getValue(3, c)) * w3;
    component(result, c) = static_cast<ComponentType<Result>>(v);
  }
  return ErrorCode::SUCCESS;
}

namespace internal
{

template <typename Values, typename CoordType, typename Result>
VTKC_EXEC inline void parametricDerivative(
  Tetra, const Values& values, IdComponent comp, const CoordType&, Result&& result) noexcept
{
  component(result, 0) = static_cast<ComponentType<Result>>(values.getValue(1, comp) -
                                                            values.getValue(0, comp));
  component(result, 1) = static_cast<ComponentType<Result>>(values.getValue(2, comp) -
                                                            values.getValue(0, comp));
  component(result, 2) = static_cast<ComponentType<Result>>(values.getValue(3, comp) -
                                                            values.getValue(0, comp));
}

} // internal

template <typename Points, typename Values, typename CoordType, typename Result>
VTKC_EXEC inline vtkc::ErrorCode derivative(
  Tetra,
  const Points& points,
  const Values& values,
  const CoordType& pcoords,
  Result&& dx,
  Result&& dy,
  Result&& dz) noexcept
{
  return internal::derivative3D(Tetra{},
                                points,
                                values,
                                pcoords,
                                std::forward<Result>(dx),
                                std::forward<Result>(dy),
                                std::forward<Result>(dz));
}

template <typename Points, typename PCoordType, typename WCoordType>
VTKC_EXEC inline vtkc::ErrorCode parametricToWorld(
  Tetra,
  const Points& points,
  const PCoordType& pcoords,
  WCoordType&& wcoords) noexcept
{
  return interpolate(Tetra{}, points, pcoords, std::forward<WCoordType>(wcoords));
}

template <typename Points, typename WCoordType, typename PCoordType>
VTKC_EXEC inline vtkc::ErrorCode worldToParametric(
  Tetra,
  const Points& points,
  const WCoordType& wcoords,
  PCoordType&& pcoords) noexcept
{
  return internal::worldToParametric3D(Tetra{}, points, wcoords, std::forward<PCoordType>(pcoords));
}

} // vtkc

#endif // vtk_c_Tetra_h
