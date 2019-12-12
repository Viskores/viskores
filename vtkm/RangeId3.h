//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_RangeId3_h
#define vtk_m_RangeId3_h

#include <vtkm/RangeId.h>

namespace vtkm
{

/// \brief Represent 3D integer range.
///
/// \c vtkm::RangeId3 is a helper class for representing a 3D range of integer
/// values. The typical use of this class is to express a box of indices
/// in the x, y, and z directions.
///
/// \c RangeId3 also contains several helper functions for computing and
/// maintaining the range.
///
struct RangeId3
{
  vtkm::RangeId X;
  vtkm::RangeId Y;
  vtkm::RangeId Z;

  RangeId3() = default;

  VTKM_EXEC_CONT
  RangeId3(const vtkm::RangeId& xrange, const vtkm::RangeId& yrange, const vtkm::RangeId& zrange)
    : X(xrange)
    , Y(yrange)
    , Z(zrange)
  {
  }

  VTKM_EXEC_CONT
  RangeId3(vtkm::Id minX, vtkm::Id maxX, vtkm::Id minY, vtkm::Id maxY, vtkm::Id minZ, vtkm::Id maxZ)
    : X(vtkm::RangeId(minX, maxX))
    , Y(vtkm::RangeId(minY, maxY))
    , Z(vtkm::RangeId(minZ, maxZ))
  {
  }

  /// Initialize range with an array of 6 values in the order xmin, xmax,
  /// ymin, ymax, zmin, zmax.
  ///
  VTKM_EXEC_CONT
  explicit RangeId3(const vtkm::Id range[6])
    : X(vtkm::RangeId(range[0], range[1]))
    , Y(vtkm::RangeId(range[2], range[3]))
    , Z(vtkm::RangeId(range[4], range[5]))
  {
  }

  /// Initialize range with the minimum and the maximum corners
  ///
  VTKM_EXEC_CONT
  RangeId3(const vtkm::Id3& min, const vtkm::Id3& max)
    : X(vtkm::RangeId(min[0], max[0]))
    , Y(vtkm::RangeId(min[1], max[1]))
    , Z(vtkm::RangeId(min[2], max[2]))
  {
  }

  /// \b Determine if the range is non-empty.
  ///
  /// \c IsNonEmpty returns true if the range is non-empty.
  ///
  VTKM_EXEC_CONT
  bool IsNonEmpty() const
  {
    return (this->X.IsNonEmpty() && this->Y.IsNonEmpty() && this->Z.IsNonEmpty());
  }

  /// \b Determines if an Id3 value is within the range.
  ///
  VTKM_EXEC_CONT
  bool Contains(const vtkm::Id3& val) const
  {
    return (this->X.Contains(val[0]) && this->Y.Contains(val[1]) && this->Z.Contains(val[2]));
  }

  /// \b Returns the center of the range.
  ///
  /// \c Center computes the middle of the range.
  ///
  VTKM_EXEC_CONT
  vtkm::Id3 Center() const
  {
    return vtkm::Id3(this->X.Center(), this->Y.Center(), this->Z.Center());
  }

  VTKM_EXEC_CONT
  vtkm::Id3 Dimensions() const
  {
    return vtkm::Id3(this->X.Length(), this->Y.Length(), this->Z.Length());
  }

  /// \b Expand range to include a value.
  ///
  /// This version of \c Include expands the range just enough to include the
  /// given value. If the range already include this value, then
  /// nothing is done.
  ///
  template <typename T>
  VTKM_EXEC_CONT void Include(const vtkm::Vec<T, 3>& point)
  {
    this->X.Include(point[0]);
    this->Y.Include(point[1]);
    this->Z.Include(point[2]);
  }

  /// \b Expand range to include other range.
  ///
  /// This version of \c Include expands the range just enough to include
  /// the other range. Essentially it is the union of the two ranges.
  ///
  VTKM_EXEC_CONT
  void Include(const vtkm::RangeId3& range)
  {
    this->X.Include(range.X);
    this->Y.Include(range.Y);
    this->Z.Include(range.Z);
  }

  /// \b Return the union of this and another range.
  ///
  /// This is a nondestructive form of \c Include.
  ///
  VTKM_EXEC_CONT
  vtkm::RangeId3 Union(const vtkm::RangeId3& other) const
  {
    vtkm::RangeId3 unionRangeId3(*this);
    unionRangeId3.Include(other);
    return unionRangeId3;
  }

  /// \b Operator for union
  ///
  VTKM_EXEC_CONT
  vtkm::RangeId3 operator+(const vtkm::RangeId3& other) const { return this->Union(other); }

  VTKM_EXEC_CONT
  bool operator==(const vtkm::RangeId3& range) const
  {
    return ((this->X == range.X) && (this->Y == range.Y) && (this->Z == range.Z));
  }

  VTKM_EXEC_CONT
  bool operator!=(const vtkm::RangeId3& range) const
  {
    return ((this->X != range.X) || (this->Y != range.Y) || (this->Z != range.Z));
  }
  VTKM_EXEC_CONT
  vtkm::RangeId& operator[](IdComponent c) noexcept
  {
    if (c <= 0)
    {
      return this->X;
    }
    else if (c == 1)
    {
      return this->Y;
    }
    else
    {
      return this->Z;
    }
  }

  VTKM_EXEC_CONT
  const vtkm::RangeId& operator[](IdComponent c) const noexcept
  {
    if (c <= 0)
    {
      return this->X;
    }
    else if (c == 1)
    {
      return this->Y;
    }
    else
    {
      return this->Z;
    }
  }
};

/// Helper function for printing range during testing
///
inline VTKM_CONT std::ostream& operator<<(std::ostream& stream, const vtkm::RangeId3& range)
{
  return stream << "{ X:" << range.X << ", Y:" << range.Y << ", Z:" << range.Z << " }";
} // Declared inside of vtkm namespace so that the operator work with ADL lookup
} // namespace vtkm

#endif //vtk_m_RangeId3_h
