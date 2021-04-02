//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_RangeId2_h
#define vtk_m_RangeId2_h

#include <vtkm/RangeId.h>

namespace vtkm
{

/// \brief Represent 2D integer range.
///
/// \c vtkm::RangeId2 is a helper class for representing a 2D range of integer
/// values. The typical use of this class is to express a box of indices
/// in the x, y, and z directions.
///
/// \c RangeId2 also contains several helper functions for computing and
/// maintaining the range.
///
struct RangeId2
{
  vtkm::RangeId X;
  vtkm::RangeId Y;

  RangeId2() = default;

  VTKM_EXEC_CONT
  RangeId2(const vtkm::RangeId& xrange, const vtkm::RangeId& yrange)
    : X(xrange)
    , Y(yrange)
  {
  }

  VTKM_EXEC_CONT
  RangeId2(vtkm::Id minX, vtkm::Id maxX, vtkm::Id minY, vtkm::Id maxY)
    : X(vtkm::RangeId(minX, maxX))
    , Y(vtkm::RangeId(minY, maxY))
  {
  }

  /// Initialize range with an array of 6 values in the order xmin, xmax,
  /// ymin, ymax, zmin, zmax.
  ///
  VTKM_EXEC_CONT
  explicit RangeId2(const vtkm::Id range[4])
    : X(vtkm::RangeId(range[0], range[1]))
    , Y(vtkm::RangeId(range[2], range[3]))
  {
  }

  /// Initialize range with the minimum and the maximum corners
  ///
  VTKM_EXEC_CONT
  RangeId2(const vtkm::Id2& min, const vtkm::Id2& max)
    : X(vtkm::RangeId(min[0], max[0]))
    , Y(vtkm::RangeId(min[1], max[1]))
  {
  }

  /// \b Determine if the range is non-empty.
  ///
  /// \c IsNonEmpty returns true if the range is non-empty.
  ///
  VTKM_EXEC_CONT
  bool IsNonEmpty() const { return (this->X.IsNonEmpty() && this->Y.IsNonEmpty()); }

  /// \b Determines if an Id2 value is within the range.
  ///
  VTKM_EXEC_CONT
  bool Contains(const vtkm::Id2& val) const
  {
    return (this->X.Contains(val[0]) && this->Y.Contains(val[1]));
  }

  /// \b Returns the center of the range.
  ///
  /// \c Center computes the middle of the range.
  ///
  VTKM_EXEC_CONT
  vtkm::Id2 Center() const { return vtkm::Id2(this->X.Center(), this->Y.Center()); }

  VTKM_EXEC_CONT
  vtkm::Id2 Dimensions() const { return vtkm::Id2(this->X.Length(), this->Y.Length()); }

  /// \b Expand range to include a value.
  ///
  /// This version of \c Include expands the range just enough to include the
  /// given value. If the range already include this value, then
  /// nothing is done.
  ///
  template <typename T>
  VTKM_EXEC_CONT void Include(const vtkm::Vec<T, 2>& point)
  {
    this->X.Include(point[0]);
    this->Y.Include(point[1]);
  }

  /// \b Expand range to include other range.
  ///
  /// This version of \c Include expands the range just enough to include
  /// the other range. Essentially it is the union of the two ranges.
  ///
  VTKM_EXEC_CONT
  void Include(const vtkm::RangeId2& range)
  {
    this->X.Include(range.X);
    this->Y.Include(range.Y);
  }

  /// \b Return the union of this and another range.
  ///
  /// This is a nondestructive form of \c Include.
  ///
  VTKM_EXEC_CONT
  vtkm::RangeId2 Union(const vtkm::RangeId2& other) const
  {
    vtkm::RangeId2 unionRangeId2(*this);
    unionRangeId2.Include(other);
    return unionRangeId2;
  }

  /// \b Operator for union
  ///
  VTKM_EXEC_CONT
  vtkm::RangeId2 operator+(const vtkm::RangeId2& other) const { return this->Union(other); }

  VTKM_EXEC_CONT
  bool operator==(const vtkm::RangeId2& range) const
  {
    return ((this->X == range.X) && (this->Y == range.Y));
  }

  VTKM_EXEC_CONT
  bool operator!=(const vtkm::RangeId2& range) const
  {
    return ((this->X != range.X) || (this->Y != range.Y));
  }

  VTKM_EXEC_CONT
  vtkm::RangeId& operator[](IdComponent c) noexcept
  {
    if (c <= 0)
    {
      return this->X;
    }
    else
    {
      return this->Y;
    }
  }

  VTKM_EXEC_CONT
  const vtkm::RangeId& operator[](IdComponent c) const noexcept
  {
    if (c <= 0)
    {
      return this->X;
    }
    else
    {
      return this->Y;
    }
  }
};

} // namespace vtkm

/// Helper function for printing range during testing
///
static inline VTKM_CONT std::ostream& operator<<(std::ostream& stream, const vtkm::RangeId2& range)
{
  return stream << "{ X:" << range.X << ", Y:" << range.Y << " }";
}

#endif //vtk_m_RangeId2_h
