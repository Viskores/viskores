//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.md for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_c_Pixel_h
#define vtk_c_Pixel_h

#include <vtkc/ErrorCode.h>
#include <vtkc/Quad.h>

#include <vtkc/internal/Common.h>

namespace vtkc
{

class Pixel : public Quad
{
public:
  constexpr VTKC_EXEC Pixel() : Quad(Cell(ShapeId::PIXEL, 4)) {}
  constexpr VTKC_EXEC explicit Pixel(const Cell& cell) : Quad(cell) {}
};

namespace internal
{

template <typename Points, typename T>
VTKC_EXEC inline int getPixelSpacing(const Points& points, T spacing[3])
{
  int zeros = 0;
  for (int i = 0; i < 3; ++i)
  {
    spacing[i] = static_cast<T>(points.getValue(2, i) - points.getValue(0, i));
    if (spacing[i] == T{0})
    {
      zeros |= 1 << i;
    }
  }
  return zeros;
}

} // internal

template <typename Points, typename Values, typename CoordType, typename Result>
VTKC_EXEC inline vtkc::ErrorCode derivative(
  Pixel,
  const Points& points,
  const Values& values,
  const CoordType& pcoords,
  Result&& dx,
  Result&& dy,
  Result&& dz) noexcept
{
  VTKC_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  using ProcessingType = internal::ClosestFloatType<typename Values::ValueType>;
  using ResultCompType = ComponentType<Result>;

  ProcessingType spacing[3];
  int zeros = internal::getPixelSpacing(points, spacing);

  for (IdComponent c = 0; c < values.getNumberOfComponents(); ++c)
  {
    ProcessingType dvdp[2];
    internal::parametricDerivative(Quad{}, values, c, pcoords, dvdp);
    switch (zeros)
    {
      case 1: // yz plane
        component(dx, c) = ResultCompType{0};
        component(dy, c) = static_cast<ResultCompType>(dvdp[0] / spacing[1]);
        component(dz, c) = static_cast<ResultCompType>(dvdp[1] / spacing[2]);
        break;
      case 2: // xz plane
        component(dx, c) = static_cast<ResultCompType>(dvdp[0] / spacing[0]);
        component(dy, c) = ResultCompType{0};
        component(dz, c) = static_cast<ResultCompType>(dvdp[1] / spacing[2]);
        break;
      case 4: // xy plane
        component(dx, c) = static_cast<ResultCompType>(dvdp[0] / spacing[0]);
        component(dy, c) = static_cast<ResultCompType>(dvdp[1] / spacing[1]);
        component(dz, c) = ResultCompType{0};
        break;
      default:
        return ErrorCode::DEGENERATE_CELL_DETECTED;
    }
  }

  return ErrorCode::SUCCESS;
}

template <typename Points, typename PCoordType, typename WCoordType>
VTKC_EXEC vtkc::ErrorCode parametricToWorld(
  Pixel,
  const Points& points,
  const PCoordType& pcoords,
  WCoordType&& wcoords) noexcept
{
  VTKC_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(PCoordType);

  using T = typename Points::ValueType;

  T spacing[3];
  auto zeros = internal::getPixelSpacing(points, spacing);

  switch (zeros)
  {
    case 1: // yz plane
      component(wcoords, 0) = points.getValue(0, 0);
      component(wcoords, 1) = points.getValue(0, 1) +
                              (spacing[1] * static_cast<T>(component(pcoords, 0)));
      component(wcoords, 2) = points.getValue(0, 2) +
                              (spacing[2] * static_cast<T>(component(pcoords, 1)));
      return ErrorCode::SUCCESS;
    case 2: // xz plane
      component(wcoords, 0) = points.getValue(0, 0) +
                              (spacing[0] * static_cast<T>(component(pcoords, 0)));
      component(wcoords, 1) = points.getValue(0, 1);
      component(wcoords, 2) = points.getValue(0, 2) +
                              (spacing[2] * static_cast<T>(component(pcoords, 1)));
      return ErrorCode::SUCCESS;
    case 4: // xy plane
      component(wcoords, 0) = points.getValue(0, 0) +
                              (spacing[0] * static_cast<T>(component(pcoords, 0)));
      component(wcoords, 1) = points.getValue(0, 1) +
                              (spacing[1] * static_cast<T>(component(pcoords, 1)));
      component(wcoords, 2) = points.getValue(0, 2);
      return ErrorCode::SUCCESS;
    default:
      return ErrorCode::DEGENERATE_CELL_DETECTED;
  }
}

template <typename Points, typename WCoordType, typename PCoordType>
VTKC_EXEC vtkc::ErrorCode worldToParametric(
  Pixel,
  const Points& points,
  const WCoordType& wcoords,
  PCoordType&& pcoords) noexcept
{
  VTKC_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(PCoordType);

  using T = ComponentType<PCoordType>;

  T spacing[3];
  int zeros = internal::getPixelSpacing(points, spacing);

  switch (zeros)
  {
  case 1: // yz plane
    component(pcoords, 0) = static_cast<T>(component(wcoords, 1) - points.getValue(0, 1)) /
                            spacing[1];
    component(pcoords, 1) = static_cast<T>(component(wcoords, 2) - points.getValue(0, 2)) /
                            spacing[2];
    return ErrorCode::SUCCESS;
  case 2: // xz plane
    component(pcoords, 0) = static_cast<T>(component(wcoords, 0) - points.getValue(0, 0)) /
                            spacing[0];
    component(pcoords, 1) = static_cast<T>(component(wcoords, 2) - points.getValue(0, 2)) /
                            spacing[2];
    return ErrorCode::SUCCESS;
  case 4: // xy plane
    component(pcoords, 0) = static_cast<T>(component(wcoords, 0) - points.getValue(0, 0)) /
                            spacing[0];
    component(pcoords, 1) = static_cast<T>(component(wcoords, 1) - points.getValue(0, 1)) /
                            spacing[1];
    return ErrorCode::SUCCESS;
  default:
    return ErrorCode::DEGENERATE_CELL_DETECTED;
  }
}

} //namespace vtkc

#endif //vtk_c_Pixel_h
