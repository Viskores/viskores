//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_testing_TestingGeometry_h
#define vtk_m_testing_TestingGeometry_h

#include <vtkm/Geometry.h>
#include <vtkm/Math.h>

#include <vtkm/TypeList.h>
#include <vtkm/VecTraits.h>

#include <vtkm/exec/FunctorBase.h>

#include <vtkm/cont/DeviceAdapterAlgorithm.h>

#include <vtkm/cont/testing/Testing.h>

#define VTKM_MATH_ASSERT(condition, message)                                                       \
  if (!(condition))                                                                                \
  {                                                                                                \
    this->RaiseError(message);                                                                     \
  }

//-----------------------------------------------------------------------------
namespace UnitTestGeometryNamespace
{

class Coords
{
public:
  static constexpr vtkm::IdComponent NUM_COORDS = 5;

  template <typename T>
  VTKM_EXEC_CONT vtkm::Vec<T, 3> EndpointList(vtkm::Int32 i) const
  {
    vtkm::Float64 coords[NUM_COORDS][3] = {
      { 1.0, 0.0, 0.0 },  { 0.0, 1.0, 0.0 },  { -1.0, 0.0, 0.0 },
      { -2.0, 0.0, 0.0 }, { 0.0, -2.0, 0.0 },
    };
    vtkm::Int32 j = i % NUM_COORDS;
    return vtkm::Vec<T, 3>(
      static_cast<T>(coords[j][0]), static_cast<T>(coords[j][1]), static_cast<T>(coords[j][2]));
  }

  template <typename T>
  VTKM_EXEC_CONT vtkm::Vec<T, 3> ClosestToOriginList(vtkm::Int32 i) const
  {
    vtkm::Float64 coords[NUM_COORDS][3] = {
      { 0.5, 0.5, 0.0 },   { -0.5, 0.5, 0.0 }, { -1.0, 0.0, 0.0 },
      { -1.0, -1.0, 0.0 }, { 0.8, -0.4, 0.0 },
    };
    vtkm::Int32 j = i % NUM_COORDS;
    return vtkm::Vec<T, 3>(
      static_cast<T>(coords[j][0]), static_cast<T>(coords[j][1]), static_cast<T>(coords[j][2]));
  }

  template <typename T>
  VTKM_EXEC_CONT T DistanceToOriginList(vtkm::Int32 i) const
  {
    vtkm::Float64 coords[NUM_COORDS] = {
      0.707107, 0.707107, 1.0, 1.41421, 0.894427,
    };
    vtkm::Int32 j = i % NUM_COORDS;
    return static_cast<T>(coords[j]);
  }
};

//-----------------------------------------------------------------------------

template <typename T>
struct RayTests : public vtkm::exec::FunctorBase
{
  VTKM_EXEC
  void operator()(vtkm::Id) const
  {
    {
      using V2 = vtkm::Vec<T, 2>;
      using Ray2 = vtkm::Ray2<T>;

      Ray2 ray0;
      VTKM_MATH_ASSERT(test_equal(ray0.Origin, V2(0., 0.)), "Bad origin for default 2D ray ctor.");
      VTKM_MATH_ASSERT(test_equal(ray0.Direction, V2(1., 0.)),
                       "Bad direction for default 2D ray ctor.");

      // Test intersection
      Ray2 ray1(V2(-1., 0.), V2(+1., +1.));
      Ray2 ray2(V2(+1., 0.), V2(-1., +1.));
      V2 point;
      bool didIntersect = ray1.Intersect(ray2, point);
      VTKM_MATH_ASSERT(test_equal(didIntersect, true), "Ray-pair 1 should intersect.");
      VTKM_MATH_ASSERT(test_equal(point, V2(0., 1.)), "Ray-pair 1 should intersect at (0,1).");

      // Test non intersection
      Ray2 ray3(V2(-1., 0.), V2(-1., -1.));
      Ray2 ray4(V2(+1., 0.), V2(+1., -1.));

      didIntersect = ray1.Intersect(ray4, point);
      VTKM_MATH_ASSERT(test_equal(didIntersect, false), "Ray-pair 2 should not intersect.");
      VTKM_MATH_ASSERT(test_equal(point, V2(0., 1.)), "Ray-pair 2 should intersect at (0,1).");

      didIntersect = ray3.Intersect(ray2, point);
      VTKM_MATH_ASSERT(test_equal(didIntersect, false), "Ray-pair 3 should not intersect.");
      VTKM_MATH_ASSERT(test_equal(point, V2(0., 1.)), "Ray-pair 3 should intersect at (0,1).");

      didIntersect = ray3.Intersect(ray4, point);
      VTKM_MATH_ASSERT(test_equal(didIntersect, false), "Ray-pair 4 should not intersect.");
      VTKM_MATH_ASSERT(test_equal(point, V2(0., 1.)), "Ray-pair 4 should intersect at (0,1).");
    }

    {
      using V3 = vtkm::Vec<T, 3>;

      vtkm::Ray<T, 3> ray0;
      VTKM_MATH_ASSERT(test_equal(ray0.Origin, V3(0., 0., 0.)),
                       "Bad origin for default 3D ray ctor.");
      VTKM_MATH_ASSERT(test_equal(ray0.Direction, V3(1., 0., 0.)),
                       "Bad direction for default 3D ray ctor.");
    }
  }
};

template <typename Device>
struct TryRayTests
{
  template <typename T>
  void operator()(const T&) const
  {
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(RayTests<T>(), 1);
  }
};

//-----------------------------------------------------------------------------

template <typename T>
struct LineSegmentTests : public vtkm::exec::FunctorBase
{
  VTKM_EXEC
  void operator()(vtkm::Id) const
  {
    {
      using V2 = vtkm::Vec<T, 2>;
      using Line2 = vtkm::Line2<T>;

      vtkm::LineSegment<T, 2> seg0;
      VTKM_MATH_ASSERT(test_equal(seg0.Endpoints[0], V2(0., 0.)),
                       "Bad origin for default 2D line segment ctor.");
      VTKM_MATH_ASSERT(test_equal(seg0.Endpoints[1], V2(1., 0.)),
                       "Bad direction for default 2D line segment ctor.");

      V2 p0(1., 1.);
      V2 p1(3., 3.);
      V2 p2(2., 2.);
      // V2 p3(static_cast<T>(1.2928932), static_cast<T>(2.7071068));
      V2 dir(static_cast<T>(-0.7071068), static_cast<T>(0.7071068));
      vtkm::LineSegment<T, 2> seg1(p0, p1);
      Line2 ray = seg1.PerpendicularBisector();
      VTKM_MATH_ASSERT(test_equal(ray.Origin, p2), "Perpendicular bisector origin failed in 2D.");
      VTKM_MATH_ASSERT(test_equal(ray.Direction, dir),
                       "Perpendicular bisector direction failed in 2D.");
    }

    {
      using V3 = vtkm::Vec<T, 3>;

      vtkm::LineSegment<T, 3> seg0;
      VTKM_MATH_ASSERT(test_equal(seg0.Endpoints[0], V3(0., 0., 0.)),
                       "Bad origin for default 3D line segment ctor.");
      VTKM_MATH_ASSERT(test_equal(seg0.Endpoints[1], V3(1., 0., 0.)),
                       "Bad direction for default 3D line segment ctor.");

      V3 p0(1., 1., 0.);
      V3 p1(3., 3., 0.);
      V3 p2(2., 2., 0.);
      V3 p3(static_cast<T>(0.70710678), static_cast<T>(0.70710678), 0.);
      vtkm::LineSegment<T, 3> seg1(p0, p1);
      vtkm::Plane<T> bisector = seg1.PerpendicularBisector();
      VTKM_MATH_ASSERT(test_equal(bisector.Origin, p2),
                       "Perpendicular bisector origin failed in 3D.");
      VTKM_MATH_ASSERT(test_equal(bisector.Normal, p3),
                       "Perpendicular bisector direction failed in 3D.");
    }

    vtkm::Vec<T, 3> origin(0., 0., 0.);
    for (vtkm::IdComponent index = 0; index < Coords::NUM_COORDS; ++index)
    {
      auto p0 = Coords{}.EndpointList<T>(index);
      auto p1 = Coords{}.EndpointList<T>((index + 1) % Coords::NUM_COORDS);

      vtkm::LineSegment<T> segment(p0, p1);
      vtkm::Vec<T, 3> closest;
      T param;
      auto dp0 = segment.DistanceTo(p0);
      auto dp1 = segment.DistanceTo(p1, param, closest);
      VTKM_MATH_ASSERT(test_equal(dp0, 0.0), "Distance to endpoint 0 not zero.");
      VTKM_MATH_ASSERT(test_equal(dp1, 0.0), "Distance to endpoint 1 not zero.");
      VTKM_MATH_ASSERT(test_equal(param, 1.0), "Parameter value of endpoint 1 not 1.0.");
      VTKM_MATH_ASSERT(test_equal(p1, closest), "Closest point not endpoint 1.");

      closest = segment.Evaluate(static_cast<T>(0.0));
      VTKM_MATH_ASSERT(test_equal(p0, closest), "Evaluated point not endpoint 0.");

      auto dpo = segment.DistanceTo(origin, param, closest);
      auto clo = Coords{}.ClosestToOriginList<T>(index);
      auto dst = Coords{}.DistanceToOriginList<T>(index);
      VTKM_MATH_ASSERT(test_equal(closest, clo), "Closest point to origin doesn't match.");
      VTKM_MATH_ASSERT(test_equal(dpo, dst), "Distance to origin doesn't match.");
    }
  }
};

template <typename Device>
struct TryLineSegmentTests
{
  template <typename T>
  void operator()(const T&) const
  {
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(LineSegmentTests<T>(), 1);
  }
};

//-----------------------------------------------------------------------------

template <typename T>
struct PlaneTests : public vtkm::exec::FunctorBase
{
  VTKM_EXEC
  void operator()(vtkm::Id) const
  {
    vtkm::Vec<T, 3> origin(0., 0., 0.);
    vtkm::Vec<T, 3> zvectr(0., 0., 5.); // intentionally not unit length to test normalization.
    vtkm::Plane<T> plane;
    vtkm::LineSegment<T> segment;
    T dist;
    bool didIntersect;
    bool isLineInPlane;
    vtkm::Vec<T, 3> nearest;
    vtkm::Vec<T, 3> p0;
    vtkm::Vec<T, 3> p1;
    T param;

    // Test signed plane-point distance
    plane = vtkm::Plane<T>(origin, zvectr);
    dist = plane.DistanceTo(vtkm::Vec<T, 3>(82., 0.5, 1.25));
    VTKM_MATH_ASSERT(test_equal(dist, 1.25), "Bad positive point-plane distance.");
    dist = plane.DistanceTo(vtkm::Vec<T, 3>(82., 0.5, -1.25));
    VTKM_MATH_ASSERT(test_equal(dist, -1.25), "Bad negative point-plane distance.");
    dist = plane.DistanceTo(vtkm::Vec<T, 3>(82., 0.5, 0.0));
    VTKM_MATH_ASSERT(test_equal(dist, 0.0), "Bad zero point-plane distance.");

    // Test line intersection
    {
      // Case 1. No intersection
      segment = vtkm::LineSegment<T>((p0 = vtkm::Vec<T, 3>(1., 1., 1.)),
                                     (p1 = vtkm::Vec<T, 3>(2., 2., 2.)));
      didIntersect = plane.Intersect(segment, param, nearest, isLineInPlane);
      VTKM_MATH_ASSERT(test_equal(didIntersect, false), "Plane and line should not intersect (1).");
      VTKM_MATH_ASSERT(test_equal(isLineInPlane, false),
                       "Line improperly reported as in plane (1).");
      VTKM_MATH_ASSERT(test_equal(nearest, p0), "Unexpected nearest point (1).");
      VTKM_MATH_ASSERT(test_equal(param, 0.0), "Unexpected nearest parameter value (1).");

      // Case 2. Degenerate intersection (entire segment lies in plane)
      segment = vtkm::LineSegment<T>((p0 = vtkm::Vec<T, 3>(1., 1., 0.)),
                                     (p1 = vtkm::Vec<T, 3>(2., 2., 0.)));
      didIntersect = plane.Intersect(segment, param, nearest, isLineInPlane);
      VTKM_MATH_ASSERT(test_equal(didIntersect, true), "Plane and line should intersect (2).");
      VTKM_MATH_ASSERT(test_equal(isLineInPlane, true),
                       "Line improperly reported as out of plane (2).");

      // Case 3. Endpoint intersection
      segment = vtkm::LineSegment<T>((p0 = vtkm::Vec<T, 3>(1., 1., 1.)),
                                     (p1 = vtkm::Vec<T, 3>(2., 2., 0.)));
      didIntersect = plane.Intersect(segment, param, nearest, isLineInPlane);
      VTKM_MATH_ASSERT(test_equal(didIntersect, true), "Plane and line should intersect (3a).");
      VTKM_MATH_ASSERT(test_equal(isLineInPlane, false),
                       "Line improperly reported as in plane (3a).");
      VTKM_MATH_ASSERT(test_equal(param, 1.0), "Invalid parameter for intersection point (3a).");
      VTKM_MATH_ASSERT(test_equal(nearest, p1), "Invalid intersection point (3a).");

      segment = vtkm::LineSegment<T>((p0 = vtkm::Vec<T, 3>(1., 1., 0.)),
                                     (p1 = vtkm::Vec<T, 3>(2., 2., 1.)));
      didIntersect = plane.Intersect(segment, param, nearest, isLineInPlane);
      VTKM_MATH_ASSERT(test_equal(didIntersect, true), "Plane and line should intersect (3b).");
      VTKM_MATH_ASSERT(test_equal(isLineInPlane, false),
                       "Line improperly reported as in plane (3b).");
      VTKM_MATH_ASSERT(test_equal(param, 0.0), "Invalid parameter for intersection point (3b).");
      VTKM_MATH_ASSERT(test_equal(nearest, p0), "Invalid intersection point (3b).");

      // Case 4. General-position intersection
      segment = vtkm::LineSegment<T>((p0 = vtkm::Vec<T, 3>(-1., -1., -1.)),
                                     (p1 = vtkm::Vec<T, 3>(2., 2., 1.)));
      didIntersect = plane.Intersect(segment, param, nearest, isLineInPlane);
      VTKM_MATH_ASSERT(test_equal(didIntersect, true), "Plane and line should intersect (4).");
      VTKM_MATH_ASSERT(test_equal(isLineInPlane, false),
                       "Line improperly reported as in plane (4).");
      VTKM_MATH_ASSERT(test_equal(param, 0.5), "Invalid parameter for intersection point (4).");
      VTKM_MATH_ASSERT(test_equal(nearest, vtkm::Vec<T, 3>(0.5, 0.5, 0)),
                       "Invalid intersection point (4).");
    }

    // Test plane-plane intersection
    {
      using V3 = vtkm::Vec<T, 3>;
      using PlaneType = vtkm::Plane<T>;
      // Case 1. Coincident planes
      p0 = V3(1., 2., 3.);
      p1 = V3(5., 7., -6.);
      V3 nn = vtkm::Normal(V3(1., 1., 1));
      PlaneType pa(p0, nn);
      PlaneType pb(p1, nn);
      vtkm::Line3<T> ii;
      bool coincident;
      didIntersect = pa.Intersect(pb, ii, coincident);
      VTKM_MATH_ASSERT(test_equal(didIntersect, false),
                       "Coincident planes should have degenerate intersection.");
      VTKM_MATH_ASSERT(test_equal(coincident, true),
                       "Coincident planes should be marked coincident.");

      // Case 2. Offset planes
      p1 = V3(5., 6., 7.);
      pb = PlaneType(p1, nn);
      didIntersect = pa.Intersect(pb, ii, coincident);
      VTKM_MATH_ASSERT(test_equal(didIntersect, false),
                       "Offset planes should have degenerate intersection.");
      VTKM_MATH_ASSERT(test_equal(coincident, false),
                       "Offset planes should not be marked coincident.");

      // Case 3. General position
      p1 = V3(1., 2., 0.);
      V3 n2(0., 0., 1.);
      pb = PlaneType(p1, n2);
      didIntersect = pa.Intersect(pb, ii, coincident);
      VTKM_MATH_ASSERT(test_equal(didIntersect, true),
                       "Proper planes should have non-degenerate intersection.");
      VTKM_MATH_ASSERT(test_equal(coincident, false),
                       "Proper planes should not be marked coincident.");
      VTKM_MATH_ASSERT(test_equal(ii.Origin, V3(2.5, 3.5, 0)),
                       "Unexpected intersection-line base point.");
      VTKM_MATH_ASSERT(test_equal(ii.Direction, vtkm::Normal(V3(1, -1, 0))),
                       "Unexpected intersection-line direction.");
    }
  }
};

template <typename Device>
struct TryPlaneTests
{
  template <typename T>
  void operator()(const T&) const
  {
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(PlaneTests<T>(), 1);
  }
};

//-----------------------------------------------------------------------------

template <typename T>
struct SphereTests : public vtkm::exec::FunctorBase
{
  VTKM_EXEC
  void operator()(vtkm::Id) const
  {
    {
      using V2 = vtkm::Vec<T, 2>;
      V2 origin(0., 0.);
      vtkm::Sphere<T, 2> defaultSphere;
      VTKM_MATH_ASSERT(test_equal(defaultSphere.Center, origin), "Default circle not at origin.");
      VTKM_MATH_ASSERT(test_equal(defaultSphere.Radius, 1.0), "Default circle not unit radius.");

      vtkm::Sphere<T, 2> sphere(origin, -2.);
      VTKM_MATH_ASSERT(test_equal(sphere.Radius, -1.0), "Negative radius should be reset to -1.");
      VTKM_MATH_ASSERT(test_equal(sphere.IsValid(), false),
                       "Negative radius should leave sphere invalid.");

      sphere = vtkm::Circle<T>(origin, 1.0);
      VTKM_MATH_ASSERT(test_equal(sphere.IsValid(), true), "Circle assignment failed.");
      VTKM_MATH_ASSERT(test_equal(sphere.Contains(origin), true),
                       "Circle does not contain its center.");
      VTKM_MATH_ASSERT(test_equal(sphere.Classify(V2(1., 0.)), 0), "Circle point not on boundary.");
      VTKM_MATH_ASSERT(test_equal(sphere.Classify(V2(0.75, 0.75)), +1),
                       "Circle contains a point that should be outside.");

      V2 p0(static_cast<T>(-0.7071), static_cast<T>(-0.7071));
      V2 p1(static_cast<T>(+0.7071), static_cast<T>(-0.7071));
      V2 p2(static_cast<T>(0.0), static_cast<T>(1.0));
      sphere = make_CircleFrom3Points(p0, p1, p2);
      VTKM_MATH_ASSERT(test_equal(sphere.IsValid(), true), "Could not create 3-point circle.");

      V2 p3(1, 1);
      V2 p4(3, 4);
      V2 p5(5, 12);
      sphere = make_CircleFrom3Points(p3, p4, p5);
      VTKM_MATH_ASSERT(test_equal(sphere.IsValid(), true), "Could not create 3-point circle.");
      T tol = static_cast<T>(1e-3); // Use a loose tolerance
      VTKM_MATH_ASSERT(test_equal(sphere.Center, vtkm::Vec<T, 2>(-12.4f, 12.1f)),
                       "Invalid circle center.");
      VTKM_MATH_ASSERT(test_equal(sphere.Radius, static_cast<T>(17.400291f)),
                       "Invalid circle radius.");
      VTKM_MATH_ASSERT(test_equal(sphere.Classify(p3, tol), 0),
                       "Generator p3 not on circle boundary.");
      VTKM_MATH_ASSERT(test_equal(sphere.Classify(p4, tol), 0),
                       "Generator p4 not on circle boundary.");
      VTKM_MATH_ASSERT(test_equal(sphere.Classify(p5, tol), 0),
                       "Generator p5 not on circle boundary.");

      V2 p6(1, 1);
      V2 p7(4, 4);
      V2 p8(5, 5);
      sphere = make_CircleFrom3Points(p6, p7, p8);
      VTKM_MATH_ASSERT(test_equal(sphere.IsValid(), false),
                       "3-point circle construction should fail with points on a line.");
    }
    {
      using V3 = vtkm::Vec<T, 3>;

      V3 p0(0., 1., 0.);
      V3 p1(1., 0., 0.);
      V3 p2(-1., 0., 0.);
      V3 p3(0., 0., 1.);
      V3 p4 = vtkm::Normal(V3(1., 1., 1.));

      V3 origin(0., 0., 0.);
      vtkm::Sphere<T, 3> defaultSphere;
      VTKM_MATH_ASSERT(test_equal(defaultSphere.Center, origin), "Default sphere not at origin.");
      VTKM_MATH_ASSERT(test_equal(defaultSphere.Radius, 1.0), "Default sphere not unit radius.");

      vtkm::Sphere<T, 3> sphere = make_SphereFrom4Points(p0, p1, p2, p3, static_cast<T>(1.0e-6));
      VTKM_MATH_ASSERT(test_equal(sphere.IsValid(), true), "Easy sphere 1 not valid.");
      VTKM_MATH_ASSERT(test_equal(sphere.Center, origin), "Easy sphere 1 not at origin.");
      VTKM_MATH_ASSERT(test_equal(sphere.Radius, 1.0), "Easy sphere 1 not unit radius.");

      sphere = make_SphereFrom4Points(p0, p1, p2, p4, static_cast<T>(1.0e-6));
      VTKM_MATH_ASSERT(test_equal(sphere.IsValid(), true), "Easy sphere 2 not valid.");
      VTKM_MATH_ASSERT(test_equal(sphere.Center, origin), "Easy sphere 2 not at origin.");
      VTKM_MATH_ASSERT(test_equal(sphere.Radius, 1.0), "Easy sphere 2 not unit radius.");

      V3 fancyCenter(1, 2, 3);
      T fancyRadius(2.5);

      V3 fp0 = fancyCenter + fancyRadius * p0;
      V3 fp1 = fancyCenter + fancyRadius * p1;
      V3 fp2 = fancyCenter + fancyRadius * p2;
      V3 fp4 = fancyCenter + fancyRadius * p4;

      sphere = make_SphereFrom4Points(fp0, fp1, fp2, fp4, static_cast<T>(1.0e-6));
      VTKM_MATH_ASSERT(test_equal(sphere.IsValid(), true), "Medium sphere 1 not valid.");
      VTKM_MATH_ASSERT(test_equal(sphere.Center, fancyCenter), "Medium sphere 1 not at (1,2,3).");
      VTKM_MATH_ASSERT(test_equal(sphere.Radius, fancyRadius), "Medium sphere 1 not radius 2.5.");
    }
  }
};

template <typename Device>
struct TrySphereTests
{
  template <typename T>
  void operator()(const T&) const
  {
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(SphereTests<T>(), 1);
  }
};

//-----------------------------------------------------------------------------
template <typename Device>
void RunGeometryTests()
{
  std::cout << "Tests for rays." << std::endl;
  vtkm::testing::Testing::TryTypes(TryRayTests<Device>(), vtkm::TypeListFieldScalar());
  std::cout << "Tests for line segments." << std::endl;
  vtkm::testing::Testing::TryTypes(TryLineSegmentTests<Device>(), vtkm::TypeListFieldScalar());
  std::cout << "Tests for planes." << std::endl;
  vtkm::testing::Testing::TryTypes(TryPlaneTests<Device>(), vtkm::TypeListFieldScalar());
  std::cout << "Tests for spheres." << std::endl;
  vtkm::testing::Testing::TryTypes(TrySphereTests<Device>(), vtkm::TypeListFieldScalar());
}

} // namespace UnitTestGeometryNamespace

#endif //vtk_m_testing_TestingGeometry_h
