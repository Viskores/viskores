//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_Bounds_h
#define vtk_m_Bounds_h

#include <vtkm/Range.h>

namespace vtkm
{

/// \brief Represent an axis-aligned 3D bounds in space.
///
/// \c vtkm::Bounds is a helper class for representing the axis-aligned box
/// representing some region in space. The typical use of this class is to
/// express the containing box of some geometry. The box is specified as ranges
/// in the x, y, and z directions.
///
/// \c Bounds also contains several helper functions for computing and
/// maintaining the bounds.
///
struct Bounds
{
  vtkm::Range X;
  vtkm::Range Y;
  vtkm::Range Z;

  VTKM_EXEC_CONT
  Bounds() {}

  Bounds(const Bounds&) = default;

  VTKM_EXEC_CONT
  Bounds(const vtkm::Range& xRange, const vtkm::Range& yRange, const vtkm::Range& zRange)
    : X(xRange)
    , Y(yRange)
    , Z(zRange)
  {
  }

  template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
  VTKM_EXEC_CONT Bounds(const T1& minX,
                        const T2& maxX,
                        const T3& minY,
                        const T4& maxY,
                        const T5& minZ,
                        const T6& maxZ)
    : X(vtkm::Range(minX, maxX))
    , Y(vtkm::Range(minY, maxY))
    , Z(vtkm::Range(minZ, maxZ))
  {
  }

  /// Initialize bounds with an array of 6 values in the order xmin, xmax,
  /// ymin, ymax, zmin, zmax.
  ///
  template <typename T>
  VTKM_EXEC_CONT explicit Bounds(const T bounds[6])
    : X(vtkm::Range(bounds[0], bounds[1]))
    , Y(vtkm::Range(bounds[2], bounds[3]))
    , Z(vtkm::Range(bounds[4], bounds[5]))
  {
  }

  /// Initialize bounds with the minimum corner point and the maximum corner
  /// point.
  ///
  template <typename T>
  VTKM_EXEC_CONT Bounds(const vtkm::Vec<T, 3>& minPoint, const vtkm::Vec<T, 3>& maxPoint)
    : X(vtkm::Range(minPoint[0], maxPoint[0]))
    , Y(vtkm::Range(minPoint[1], maxPoint[1]))
    , Z(vtkm::Range(minPoint[2], maxPoint[2]))
  {
  }

  vtkm::Bounds& operator=(const vtkm::Bounds& src) = default;

  /// \b Determine if the bounds are valid (i.e. has at least one valid point).
  ///
  /// \c IsNonEmpty returns true if the bounds contain some valid points. If
  /// the bounds are any real region, even if a single point or it expands to
  /// infinity, true is returned.
  ///
  VTKM_EXEC_CONT
  bool IsNonEmpty() const
  {
    return (this->X.IsNonEmpty() && this->Y.IsNonEmpty() && this->Z.IsNonEmpty());
  }

  /// \b Determines if a point coordinate is within the bounds.
  ///
  template <typename T>
  VTKM_EXEC_CONT bool Contains(const vtkm::Vec<T, 3>& point) const
  {
    return (this->X.Contains(point[0]) && this->Y.Contains(point[1]) && this->Z.Contains(point[2]));
  }

  /// \b Returns the center of the range.
  ///
  /// \c Center computes the point at the middle of the bounds. If the bounds
  /// are empty, the results are undefined.
  ///
  VTKM_EXEC_CONT
  vtkm::Vec3f_64 Center() const
  {
    return vtkm::Vec3f_64(this->X.Center(), this->Y.Center(), this->Z.Center());
  }

  /// \b Expand bounds to include a point.
  ///
  /// This version of \c Include expands the bounds just enough to include the
  /// given point coordinates. If the bounds already include this point, then
  /// nothing is done.
  ///
  template <typename T>
  VTKM_EXEC_CONT void Include(const vtkm::Vec<T, 3>& point)
  {
    this->X.Include(point[0]);
    this->Y.Include(point[1]);
    this->Z.Include(point[2]);
  }

  /// \b Expand bounds to include other bounds.
  ///
  /// This version of \c Include expands these bounds just enough to include
  /// that of another bounds. Essentially it is the union of the two bounds.
  ///
  VTKM_EXEC_CONT
  void Include(const vtkm::Bounds& bounds)
  {
    this->X.Include(bounds.X);
    this->Y.Include(bounds.Y);
    this->Z.Include(bounds.Z);
  }

  /// \b Return the union of this and another bounds.
  ///
  /// This is a nondestructive form of \c Include.
  ///
  VTKM_EXEC_CONT
  vtkm::Bounds Union(const vtkm::Bounds& otherBounds) const
  {
    vtkm::Bounds unionBounds(*this);
    unionBounds.Include(otherBounds);
    return unionBounds;
  }

  /// \b Operator for union
  ///
  VTKM_EXEC_CONT
  vtkm::Bounds operator+(const vtkm::Bounds& otherBounds) const { return this->Union(otherBounds); }

  VTKM_EXEC_CONT
  bool operator==(const vtkm::Bounds& bounds) const
  {
    return ((this->X == bounds.X) && (this->Y == bounds.Y) && (this->Z == bounds.Z));
  }

  VTKM_EXEC_CONT
  bool operator!=(const vtkm::Bounds& bounds) const
  {
    return ((this->X != bounds.X) || (this->Y != bounds.Y) || (this->Z != bounds.Z));
  }
};

/// Helper function for printing bounds during testing
///
static inline VTKM_CONT std::ostream& operator<<(std::ostream& stream, const vtkm::Bounds& bounds)
{
  return stream << "{ X:" << bounds.X << ", Y:" << bounds.Y << ", Z:" << bounds.Z << " }";
}

} // namespace vtkm

#endif //vtk_m_Bounds_h
