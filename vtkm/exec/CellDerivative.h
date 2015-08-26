//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_exec_Derivative_h
#define vtk_m_exec_Derivative_h

#include <vtkm/CellShape.h>
#include <vtkm/Math.h>
#include <vtkm/Matrix.h>
#include <vtkm/VectorAnalysis.h>

#include <vtkm/exec/Assert.h>
#include <vtkm/exec/CellInterpolate.h>
#include <vtkm/exec/FunctorBase.h>

namespace vtkm {
namespace exec {

namespace internal {

// The derivative for a 2D polygon in 3D space is underdetermined since there
// is no information in the direction perpendicular to the polygon. To compute
// derivatives for general polygons, we build a 2D space for the polygon's
// plane and solve the derivative there.
template<typename T>
struct Space2D
{
  typedef vtkm::Vec<T,3> Vec3;
  typedef vtkm::Vec<T,2> Vec2;

  Vec3 Origin;
  Vec3 Basis0;
  Vec3 Basis1;

  VTKM_EXEC_EXPORT
  Space2D(const Vec3 &origin, const Vec3 &pointFirst, const Vec3 &pointLast)
  {
    this->Origin = origin;

    this->Basis0 = vtkm::Normal(pointFirst - this->Origin);

    Vec3 n = vtkm::Cross(this->Basis0, pointLast - this->Origin);
    this->Basis1 = vtkm::Normal(vtkm::Cross(this->Basis0, n));
  }

  VTKM_EXEC_EXPORT
  Vec2 ConvertCoordToSpace(const Vec3 coord) const {
    Vec3 vec = coord - this->Origin;
    return Vec2(vtkm::dot(vec, this->Basis0), vtkm::dot(vec, this->Basis1));
  }

  VTKM_EXEC_EXPORT
  Vec3 ConvertVecFromSpace(const Vec2 vec) const {
    return vec[0]*this->Basis0 + vec[1]*this->Basis1;
  }
};

#define VTKM_DERIVATIVE_WEIGHTS_HEXAHEDRON(pc, rc, call) \
  call(0, -rc[1]*rc[2], -rc[0]*rc[2], -rc[0]*rc[1]); \
  call(1,  rc[1]*rc[2], -pc[0]*rc[2], -pc[0]*rc[1]); \
  call(2,  pc[1]*rc[2],  pc[0]*rc[2], -pc[0]*pc[1]); \
  call(3, -pc[1]*rc[2],  rc[0]*rc[2], -rc[0]*pc[1]); \
  call(4, -rc[1]*pc[2], -rc[0]*pc[2],  rc[0]*rc[1]); \
  call(5,  rc[1]*pc[2], -pc[0]*pc[2],  pc[0]*rc[1]); \
  call(6,  pc[1]*pc[2],  pc[0]*pc[2],  pc[0]*pc[1]); \
  call(7, -pc[1]*pc[2],  rc[0]*pc[2],  rc[0]*pc[1])

#define VTKM_DERIVATIVE_WEIGHTS_VOXEL(pc, rc, call) \
  call(0, -rc[1]*rc[2], -rc[0]*rc[2], -rc[0]*rc[1]); \
  call(1,  rc[1]*rc[2], -pc[0]*rc[2], -pc[0]*rc[1]); \
  call(2, -pc[1]*rc[2],  rc[0]*rc[2], -rc[0]*pc[1]); \
  call(3,  pc[1]*rc[2],  pc[0]*rc[2], -pc[0]*pc[1]); \
  call(4, -rc[1]*pc[2], -rc[0]*pc[2],  rc[0]*rc[1]); \
  call(5,  rc[1]*pc[2], -pc[0]*pc[2],  pc[0]*rc[1]); \
  call(6, -pc[1]*pc[2],  rc[0]*pc[2],  rc[0]*pc[1]); \
  call(7,  pc[1]*pc[2],  pc[0]*pc[2],  pc[0]*pc[1])

#define VTKM_DERIVATIVE_WEIGHTS_WEDGE(pc, rc, call) \
  call(0, -rc[2], -rc[2], -1.0f+pc[0]+pc[1]); \
  call(1,   0.0f,  rc[2],            -pc[1]); \
  call(2,  rc[2],   0.0f,            -pc[0]); \
  call(3, -pc[2], -pc[2],  1.0f-pc[0]-pc[1]); \
  call(4,   0.0f,  pc[2],             pc[1]); \
  call(5,  pc[2],   0.0f,             pc[0])

#define VTKM_DERIVATIVE_WEIGHTS_PYRAMID(pc, rc, call) \
  call(0, -rc[1]*rc[2], -rc[0]*rc[2], -rc[0]*rc[1]); \
  call(1,  rc[1]*rc[2], -pc[0]*rc[2], -pc[0]*rc[1]); \
  call(2,  pc[1]*rc[2],  pc[0]*rc[2], -pc[0]*pc[1]); \
  call(3, -pc[1]*rc[2],  rc[0]*rc[2], -rc[0]*pc[1]); \
  call(3,         0.0f,         0.0f,         1.0f)


#define VTKM_DERIVATIVE_WEIGHTS_QUAD(pc, rc, call) \
  call(0, -rc[1], -rc[0]); \
  call(1,  rc[1], -pc[0]); \
  call(2,  pc[1],  pc[0]); \
  call(3, -pc[1],  rc[0])

#define VTKM_DERIVATIVE_WEIGHTS_PIXEL(pc, rc, call) \
  call(0, -rc[1], -rc[0]); \
  call(1,  rc[1], -pc[0]); \
  call(2, -pc[1],  rc[0]); \
  call(3,  pc[1],  pc[0])

// Given a series of point values for a wedge, return a new series of point
// for a hexahedron that has the same interpolation within the wedge.
template<typename FieldVecType>
VTKM_EXEC_EXPORT
vtkm::Vec<typename FieldVecType::ComponentType,8>
PermuteWedgeToHex(const FieldVecType &field)
{
  vtkm::Vec<typename FieldVecType::ComponentType,8> hexField;

  hexField[0] = field[0];
  hexField[1] = field[2];
  hexField[2] = field[2] + field[1] - field[0];
  hexField[3] = field[1];
  hexField[4] = field[3];
  hexField[5] = field[5];
  hexField[6] = field[5] + field[4] - field[3];
  hexField[7] = field[4];

  return hexField;
}

// Given a series of point values for a pyramid, return a new series of point
// for a hexahedron that has the same interpolation within the pyramid.
template<typename FieldVecType>
VTKM_EXEC_EXPORT
vtkm::Vec<typename FieldVecType::ComponentType,8>
PermutePyramidToHex(const FieldVecType &field)
{
  typedef typename FieldVecType::ComponentType T;

  vtkm::Vec<T,8> hexField;

  T baseCenter = T(0.25f)*(field[0]+field[1]+field[2]+field[3]);

  hexField[0] = field[0];
  hexField[1] = field[1];
  hexField[2] = field[2];
  hexField[3] = field[3];
  hexField[4] = field[4]+(field[0]-baseCenter);
  hexField[5] = field[4]+(field[1]-baseCenter);
  hexField[6] = field[4]+(field[2]-baseCenter);
  hexField[7] = field[4]+(field[3]-baseCenter);

  return hexField;
}

//-----------------------------------------------------------------------------
// This returns the Jacobian of a hexahedron's (or other 3D cell's) coordinates
// with respect to parametric coordinates. Explicitly, this is (d is partial
// derivative):
//
//   |                     |
//   | dx/du  dx/dv  dx/dw |
//   |                     |
//   | dy/du  dy/dv  dy/dw |
//   |                     |
//   | dz/du  dz/dv  dz/dw |
//   |                     |
//

#define VTKM_ACCUM_JACOBIAN_3D(pointIndex, weight0, weight1, weight2) \
  jacobian(0,0) += wCoords[pointIndex][0] * (weight0); \
  jacobian(1,0) += wCoords[pointIndex][1] * (weight0); \
  jacobian(2,0) += wCoords[pointIndex][2] * (weight0); \
  jacobian(0,1) += wCoords[pointIndex][0] * (weight1); \
  jacobian(1,1) += wCoords[pointIndex][1] * (weight1); \
  jacobian(2,1) += wCoords[pointIndex][2] * (weight1); \
  jacobian(0,2) += wCoords[pointIndex][0] * (weight2); \
  jacobian(1,2) += wCoords[pointIndex][1] * (weight2); \
  jacobian(2,2) += wCoords[pointIndex][2] * (weight2)

template<typename WorldCoordType,
         typename ParametricCoordType,
         typename JacobianType>
VTKM_EXEC_EXPORT
void JacobianFor3DCell(const WorldCoordType &wCoords,
                       const vtkm::Vec<ParametricCoordType,3> &pcoords,
                       vtkm::Matrix<JacobianType,3,3> &jacobian,
                       vtkm::CellShapeTagHexahedron)
{
  vtkm::Vec<JacobianType,3> pc(pcoords);
  vtkm::Vec<JacobianType,3> rc = vtkm::Vec<JacobianType,3>(1) - pc;

  jacobian = vtkm::Matrix<JacobianType,3,3>(0);
  VTKM_DERIVATIVE_WEIGHTS_HEXAHEDRON(pc, rc, VTKM_ACCUM_JACOBIAN_3D);
}

template<typename WorldCoordType,
         typename ParametricCoordType,
         typename JacobianType>
VTKM_EXEC_EXPORT
void JacobianFor3DCell(const WorldCoordType &wCoords,
                       const vtkm::Vec<ParametricCoordType,3> &pcoords,
                       vtkm::Matrix<JacobianType,3,3> &jacobian,
                       vtkm::CellShapeTagVoxel)
{
  vtkm::Vec<JacobianType,3> pc(pcoords);
  vtkm::Vec<JacobianType,3> rc = vtkm::Vec<JacobianType,3>(1) - pc;

  jacobian = vtkm::Matrix<JacobianType,3,3>(0);
  VTKM_DERIVATIVE_WEIGHTS_VOXEL(pc, rc, VTKM_ACCUM_JACOBIAN_3D);
}

template<typename WorldCoordType,
         typename ParametricCoordType,
         typename JacobianType>
VTKM_EXEC_EXPORT
void JacobianFor3DCell(const WorldCoordType &wCoords,
                       const vtkm::Vec<ParametricCoordType,3> &pcoords,
                       vtkm::Matrix<JacobianType,3,3> &jacobian,
                       vtkm::CellShapeTagWedge)
{
#if 0
  // This is not working. Just leverage the hexahedron code that is working.
  vtkm::Vec<JacobianType,3> pc(pcoords);
  vtkm::Vec<JacobianType,3> rc = vtkm::Vec<JacobianType,3>(1) - pc;

  jacobian = vtkm::Matrix<JacobianType,3,3>(0);
  VTKM_DERIVATIVE_WEIGHTS_WEDGE(pc, rc, VTKM_ACCUM_JACOBIAN_3D);
#else
  JacobianFor3DCell(vtkm::exec::internal::PermuteWedgeToHex(wCoords),
                    pcoords,
                    jacobian,
                    vtkm::CellShapeTagHexahedron());
#endif
}

template<typename WorldCoordType,
         typename ParametricCoordType,
         typename JacobianType>
VTKM_EXEC_EXPORT
void JacobianFor3DCell(const WorldCoordType &wCoords,
                       const vtkm::Vec<ParametricCoordType,3> &pcoords,
                       vtkm::Matrix<JacobianType,3,3> &jacobian,
                       vtkm::CellShapeTagPyramid)
{
#if 0
  // This is not working. Just leverage the hexahedron code that is working.
  vtkm::Vec<JacobianType,3> pc(pcoords);
  vtkm::Vec<JacobianType,3> rc = vtkm::Vec<JacobianType,3>(1) - pc;

  jacobian = vtkm::Matrix<JacobianType,3,3>(0);
  VTKM_DERIVATIVE_WEIGHTS_PYRAMID(pc, rc, VTKM_ACCUM_JACOBIAN_3D);
#else
  JacobianFor3DCell(vtkm::exec::internal::PermutePyramidToHex(wCoords),
                    pcoords,
                    jacobian,
                    vtkm::CellShapeTagHexahedron());
#endif
}

#undef VTKM_ACCUM_JACOBIAN_3D

// Derivatives in quadrilaterals are computed in much the same way as
// hexahedra.  Review the documentation for hexahedra derivatives for details
// on the math.  The major difference is that the equations are performed in
// a 2D space built with make_SpaceForQuadrilateral.

#define VTKM_ACCUM_JACOBIAN_2D(pointIndex, weight0, weight1) \
  wcoords2d = space.ConvertCoordToSpace(wCoords[pointIndex]); \
  jacobian(0,0) += wcoords2d[0] * (weight0); \
  jacobian(1,0) += wcoords2d[1] * (weight0); \
  jacobian(0,1) += wcoords2d[0] * (weight1); \
  jacobian(1,1) += wcoords2d[1] * (weight1)

template<typename WorldCoordType,
         typename ParametricCoordType,
         typename JacobianType>
VTKM_EXEC_EXPORT
void JacobianFor2DCell(const WorldCoordType &wCoords,
                       const vtkm::Vec<ParametricCoordType,3> &pcoords,
                       const vtkm::exec::internal::Space2D<JacobianType> &space,
                       vtkm::Matrix<JacobianType,2,2> &jacobian,
                       vtkm::CellShapeTagQuad)
{
  vtkm::Vec<JacobianType,2> pc(pcoords[0], pcoords[1]);
  vtkm::Vec<JacobianType,2> rc = vtkm::Vec<JacobianType,2>(1) - pc;

  vtkm::Vec<JacobianType,2> wcoords2d;
  jacobian = vtkm::Matrix<JacobianType,2,2>(0);
  VTKM_DERIVATIVE_WEIGHTS_QUAD(pc, rc, VTKM_ACCUM_JACOBIAN_2D);
}

template<typename WorldCoordType,
         typename ParametricCoordType,
         typename JacobianType>
VTKM_EXEC_EXPORT
void JacobianFor2DCell(const WorldCoordType &wCoords,
                       const vtkm::Vec<ParametricCoordType,3> &pcoords,
                       const vtkm::exec::internal::Space2D<JacobianType> &space,
                       vtkm::Matrix<JacobianType,2,2> &jacobian,
                       vtkm::CellShapeTagPixel)
{
  vtkm::Vec<JacobianType,2> pc(pcoords[0], pcoords[1]);
  vtkm::Vec<JacobianType,2> rc = vtkm::Vec<JacobianType,2>(1) - pc;

  vtkm::Vec<JacobianType,2> wcoords2d;
  jacobian = vtkm::Matrix<JacobianType,2,2>(0);
  VTKM_DERIVATIVE_WEIGHTS_PIXEL(pc, rc, VTKM_ACCUM_JACOBIAN_2D);
}

#if 0
// This code doesn't work, so I'm bailing on it. Instead, I'm just grabbing a
// triangle and finding the derivative of that. If you can do better, please
// implement it.
template<typename WorldCoordType,
         typename ParametricCoordType,
         typename JacobianType>
VTKM_EXEC_EXPORT
void JacobianFor2DCell(const WorldCoordType &wCoords,
                       const vtkm::Vec<ParametricCoordType,3> &pcoords,
                       const vtkm::exec::internal::Space2D<JacobianType> &space,
                       vtkm::Matrix<JacobianType,2,2> &jacobian,
                       vtkm::CellShapeTagPolygon)
{
  const vtkm::IdComponent numPoints = wCoords.GetNumberOfComponents();
  vtkm::Vec<JacobianType,2> pc(pcoords[0], pcoords[1]);
  JacobianType deltaAngle = static_cast<JacobianType>(2*vtkm::Pi()/numPoints);

  jacobian = vtkm::Matrix<JacobianType,2,2>(0);
  for (vtkm::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
  {
    JacobianType angle = pointIndex*deltaAngle;
    vtkm::Vec<JacobianType,2> nodePCoords(0.5f*(vtkm::Cos(angle)+1),
                                          0.5f*(vtkm::Sin(angle)+1));

    // This is the vector pointing from the user provided parametric coordinate
    // to the node at pointIndex in parametric space.
    vtkm::Vec<JacobianType,2> pvec = nodePCoords - pc;

    // The weight (the derivative of the interpolation factor) happens to be
    // pvec scaled by the cube root of pvec's magnitude.
    JacobianType magSqr = vtkm::MagnitudeSquared(pvec);
    JacobianType invMag = vtkm::RSqrt(magSqr);
    JacobianType scale = invMag*invMag*invMag;
    vtkm::Vec<JacobianType,2> weight = scale*pvec;

    vtkm::Vec<JacobianType,2> wcoords2d =
        space.ConvertCoordToSpace(wCoords[pointIndex]);
    jacobian(0,0) += wcoords2d[0] * weight[0];
    jacobian(1,0) += wcoords2d[1] * weight[0];
    jacobian(0,1) += wcoords2d[0] * weight[1];
    jacobian(1,1) += wcoords2d[1] * weight[1];
  }
}
#endif

#undef VTKM_ACCUM_JACOBIAN_2D


#define VTKM_ACCUM_PARAMETRIC_DERIVATIVE_3D(pointIndex,weight0,weight1,weight2)\
  parametricDerivative[0] += field[pointIndex] * weight0; \
  parametricDerivative[1] += field[pointIndex] * weight1; \
  parametricDerivative[2] += field[pointIndex] * weight2

// Find the derivative of a field in parametric space. That is, find the
// vector [ds/du, ds/dv, ds/dw].
template<typename FieldVecType,
         typename ParametricCoordType>
VTKM_EXEC_EXPORT
vtkm::Vec<typename FieldVecType::ComponentType,3>
ParametricDerivative(const FieldVecType &field,
                     const vtkm::Vec<ParametricCoordType,3> &pcoords,
                     vtkm::CellShapeTagHexahedron)
{
  typedef typename FieldVecType::ComponentType FieldType;
  typedef vtkm::Vec<FieldType,3> GradientType;

  GradientType pc(pcoords);
  GradientType rc = GradientType(1) - pc;

  GradientType parametricDerivative(0);
  VTKM_DERIVATIVE_WEIGHTS_HEXAHEDRON(pc,rc,VTKM_ACCUM_PARAMETRIC_DERIVATIVE_3D);

  return parametricDerivative;
}

template<typename FieldVecType,
         typename ParametricCoordType>
VTKM_EXEC_EXPORT
vtkm::Vec<typename FieldVecType::ComponentType,3>
ParametricDerivative(const FieldVecType &field,
                     const vtkm::Vec<ParametricCoordType,3> &pcoords,
                     vtkm::CellShapeTagVoxel)
{
  typedef typename FieldVecType::ComponentType FieldType;
  typedef vtkm::Vec<FieldType,3> GradientType;

  GradientType pc(pcoords);
  GradientType rc = GradientType(1) - pc;

  GradientType parametricDerivative(0);
  VTKM_DERIVATIVE_WEIGHTS_VOXEL(pc, rc, VTKM_ACCUM_PARAMETRIC_DERIVATIVE_3D);

  return parametricDerivative;
}

template<typename FieldVecType,
         typename ParametricCoordType>
VTKM_EXEC_EXPORT
vtkm::Vec<typename FieldVecType::ComponentType,3>
ParametricDerivative(const FieldVecType &field,
                     const vtkm::Vec<ParametricCoordType,3> &pcoords,
                     vtkm::CellShapeTagWedge)
{
#if 0
  // This is not working. Just leverage the hexahedron code that is working.
  typedef typename FieldVecType::ComponentType FieldType;
  typedef vtkm::Vec<FieldType,3> GradientType;

  GradientType pc(pcoords);
  GradientType rc = GradientType(1) - pc;

  GradientType parametricDerivative(0);
  VTKM_DERIVATIVE_WEIGHTS_WEDGE(pc, rc, VTKM_ACCUM_PARAMETRIC_DERIVATIVE_3D);

  return parametricDerivative;
#else
  return ParametricDerivative(vtkm::exec::internal::PermuteWedgeToHex(field),
                              pcoords,
                              vtkm::CellShapeTagHexahedron());
#endif
}

template<typename FieldVecType,
         typename ParametricCoordType>
VTKM_EXEC_EXPORT
vtkm::Vec<typename FieldVecType::ComponentType,3>
ParametricDerivative(const FieldVecType &field,
                     const vtkm::Vec<ParametricCoordType,3> &pcoords,
                     vtkm::CellShapeTagPyramid)
{
#if 0
  // This is not working. Just leverage the hexahedron code that is working.
  typedef typename FieldVecType::ComponentType FieldType;
  typedef vtkm::Vec<FieldType,3> GradientType;

  GradientType pc(pcoords);
  GradientType rc = GradientType(1) - pc;

  GradientType parametricDerivative(0);
  VTKM_DERIVATIVE_WEIGHTS_PYRAMID(pc, rc, VTKM_ACCUM_PARAMETRIC_DERIVATIVE_3D);

  return parametricDerivative;
#else
  return ParametricDerivative(vtkm::exec::internal::PermutePyramidToHex(field),
                              pcoords,
                              vtkm::CellShapeTagHexahedron());
#endif
}

#undef VTKM_ACCUM_PARAMETRIC_DERIVATIVE_3D

#define VTKM_ACCUM_PARAMETRIC_DERIVATIVE_2D(pointIndex, weight0, weight1) \
  parametricDerivative[0] += field[pointIndex] * weight0; \
  parametricDerivative[1] += field[pointIndex] * weight1

template<typename FieldVecType,
         typename ParametricCoordType>
VTKM_EXEC_EXPORT
vtkm::Vec<typename FieldVecType::ComponentType,2>
ParametricDerivative(const FieldVecType &field,
                     const vtkm::Vec<ParametricCoordType,3> &pcoords,
                     vtkm::CellShapeTagQuad)
{
  typedef typename FieldVecType::ComponentType FieldType;
  typedef vtkm::Vec<FieldType,2> GradientType;

  GradientType pc(pcoords[0], pcoords[1]);
  GradientType rc = GradientType(1) - pc;

  GradientType parametricDerivative(0);
  VTKM_DERIVATIVE_WEIGHTS_QUAD(pc, rc, VTKM_ACCUM_PARAMETRIC_DERIVATIVE_2D);

  return parametricDerivative;
}

template<typename FieldVecType,
         typename ParametricCoordType>
VTKM_EXEC_EXPORT
vtkm::Vec<typename FieldVecType::ComponentType,2>
ParametricDerivative(const FieldVecType &field,
                     const vtkm::Vec<ParametricCoordType,3> &pcoords,
                     vtkm::CellShapeTagPixel)
{
  typedef typename FieldVecType::ComponentType FieldType;
  typedef vtkm::Vec<FieldType,2> GradientType;

  GradientType pc(pcoords[0], pcoords[1]);
  GradientType rc = GradientType(1) - pc;

  GradientType parametricDerivative(0);
  VTKM_DERIVATIVE_WEIGHTS_PIXEL(pc, rc, VTKM_ACCUM_PARAMETRIC_DERIVATIVE_2D);

  return parametricDerivative;
}

#if 0
// This code doesn't work, so I'm bailing on it. Instead, I'm just grabbing a
// triangle and finding the derivative of that. If you can do better, please
// implement it.
template<typename FieldVecType,
         typename ParametricCoordType>
VTKM_EXEC_EXPORT
vtkm::Vec<typename FieldVecType::ComponentType,2>
ParametricDerivative(const FieldVecType &field,
                     const vtkm::Vec<ParametricCoordType,3> &pcoords,
                     vtkm::CellShapeTagPolygon)
{
  typedef typename FieldVecType::ComponentType FieldType;
  typedef vtkm::Vec<FieldType,2> GradientType;

  const vtkm::IdComponent numPoints = field.GetNumberOfComponents();
  FieldType deltaAngle = static_cast<FieldType>(2*vtkm::Pi()/numPoints);

  GradientType pc(pcoords[0], pcoords[1]);

  GradientType parametricDerivative(0);
  for (vtkm::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
  {
    FieldType angle = pointIndex*deltaAngle;
    vtkm::Vec<FieldType,2> nodePCoords(0.5f*(vtkm::Cos(angle)+1),
                                       0.5f*(vtkm::Sin(angle)+1));

    // This is the vector pointing from the user provided parametric coordinate
    // to the node at pointIndex in parametric space.
    vtkm::Vec<FieldType,2> pvec = nodePCoords - pc;

    // The weight (the derivative of the interpolation factor) happens to be
    // pvec scaled by the cube root of pvec's magnitude.
    FieldType magSqr = vtkm::MagnitudeSquared(pvec);
    FieldType invMag = vtkm::RSqrt(magSqr);
    FieldType scale = invMag*invMag*invMag;
    vtkm::Vec<FieldType,2> weight = scale*pvec;

    parametricDerivative[0] += field[pointIndex] * weight[0];
    parametricDerivative[1] += field[pointIndex] * weight[1];
  }

  return parametricDerivative;
}
#endif

#undef VTKM_ACCUM_PARAMETRIC_DERIVATIVE_2D

#undef VTKM_DERIVATIVE_WEIGHTS_HEXAHEDRON
#undef VTKM_DERIVATIVE_WEIGHTS_VOXEL
#undef VTKM_DERIVATIVE_WEIGHTS_WEDGE
#undef VTKM_DERIVATIVE_WEIGHTS_PYRAMID
#undef VTKM_DERIVATIVE_WEIGHTS_QUAD
#undef VTKM_DERIVATIVE_WEIGHTS_PIXEL


} // namespace internal

namespace detail {

template<typename FieldVecType,
         typename WorldCoordType,
         typename ParametricCoordType,
         typename CellShapeTag>
VTKM_EXEC_EXPORT
vtkm::Vec<typename FieldVecType::ComponentType,3>
CellDerivativeFor3DCell(const FieldVecType &field,
                        const WorldCoordType &wCoords,
                        const vtkm::Vec<ParametricCoordType,3> &pcoords,
                        CellShapeTag)
{
  typedef typename FieldVecType::ComponentType FieldType;
  typedef vtkm::Vec<FieldType,3> GradientType;

  // For reasons that should become apparent in a moment, we actually want
  // the transpose of the Jacobian.
  vtkm::Matrix<FieldType,3,3> jacobianTranspose;
  vtkm::exec::internal::JacobianFor3DCell(
        wCoords, pcoords, jacobianTranspose, CellShapeTag());
  jacobianTranspose = vtkm::MatrixTranspose(jacobianTranspose);

  GradientType parametricDerivative =
      vtkm::exec::internal::ParametricDerivative(field,pcoords,CellShapeTag());

  // If we write out the matrices below, it should become clear that the
  // Jacobian transpose times the field derivative in world space equals
  // the field derivative in parametric space.
  //
  //   |                     |  |       |     |       |
  //   | dx/du  dy/du  dz/du |  | ds/dx |     | ds/du |
  //   |                     |  |       |     |       |
  //   | dx/dv  dy/dv  dz/dv |  | ds/dy |  =  | ds/dv |
  //   |                     |  |       |     |       |
  //   | dx/dw  dy/dw  dz/dw |  | ds/dz |     | ds/dw |
  //   |                     |  |       |     |       |
  //
  // Now we just need to solve this linear system to find the derivative in
  // world space.

  bool valid;  // Ignored.
  return vtkm::SolveLinearSystem(jacobianTranspose,
                                 parametricDerivative,
                                 valid);
}

template<typename FieldVecType,
         typename WorldCoordType,
         typename ParametricCoordType,
         typename CellShapeTag>
VTKM_EXEC_EXPORT
vtkm::Vec<typename FieldVecType::ComponentType,3>
CellDerivativeFor2DCell(const FieldVecType &field,
                        const WorldCoordType &wCoords,
                        const vtkm::Vec<ParametricCoordType,3> &pcoords,
                        CellShapeTag)
{
  typedef typename FieldVecType::ComponentType FieldType;
  typedef vtkm::Vec<FieldType,3> GradientType;

  // We have an underdetermined system in 3D, so create a 2D space in the
  // plane that the polygon sits.
  vtkm::exec::internal::Space2D<FieldType> space(
        wCoords[0], wCoords[1], wCoords[wCoords.GetNumberOfComponents()-1]);

  // For reasons that should become apparent in a moment, we actually want
  // the transpose of the Jacobian.
  vtkm::Matrix<FieldType,2,2> jacobianTranspose;
  vtkm::exec::internal::JacobianFor2DCell(
        wCoords, pcoords, space, jacobianTranspose, CellShapeTag());
  jacobianTranspose = vtkm::MatrixTranspose(jacobianTranspose);

  // Find the derivative of the field in parametric coordinate space. That is,
  // find the vector [ds/du, ds/dv].
  vtkm::Vec<FieldType,2> parametricDerivative =
      vtkm::exec::internal::ParametricDerivative(field,pcoords,CellShapeTag());

  // If we write out the matrices below, it should become clear that the
  // Jacobian transpose times the field derivative in world space equals
  // the field derivative in parametric space.
  //
  //   |                |  |        |     |       |
  //   | db0/du  db1/du |  | ds/db0 |     | ds/du |
  //   |                |  |        |  =  |       |
  //   | db0/dv  db1/dv |  | ds/db1 |     | ds/dv |
  //   |                |  |        |     |       |
  //
  // Now we just need to solve this linear system to find the derivative in
  // world space.

  bool valid;  // Ignored.
  vtkm::Vec<FieldType,2> gradient2D =
      vtkm::SolveLinearSystem(jacobianTranspose, parametricDerivative, valid);

  return space.ConvertVecFromSpace(gradient2D);
}

} // namespace detail

//-----------------------------------------------------------------------------
/// \brief Take the derivative (get the gradient) of a point field in a cell.
///
/// Given the point field values for each node and the parametric coordinates
/// of a point within the cell, finds the derivative with respect to each
/// coordinate (i.e. the gradient) at that point. The derivative is not always
/// constant in some "linear" cells.
///
template<typename FieldVecType,
         typename WorldCoordType,
         typename ParametricCoordType>
VTKM_EXEC_EXPORT
vtkm::Vec<typename FieldVecType::ComponentType,3>
CellDerivative(const FieldVecType &pointFieldValues,
               const WorldCoordType &worldCoordinateValues,
               const vtkm::Vec<ParametricCoordType,3> &parametricCoords,
               vtkm::CellShapeTagGeneric shape,
               const vtkm::exec::FunctorBase &worklet)
{
  switch (shape.Id)
  {
    vtkmGenericCellShapeMacro(
          return CellDerivative(pointFieldValues,
                                worldCoordinateValues,
                                parametricCoords,
                                CellShapeTag(),
                                worklet));
    default:
      worklet.RaiseError("Unknown cell shape sent to derivative.");
      return vtkm::Vec<typename FieldVecType::ComponentType,3>();
  }
}

//-----------------------------------------------------------------------------
template<typename FieldVecType,
         typename WorldCoordType,
         typename ParametricCoordType>
VTKM_EXEC_EXPORT
vtkm::Vec<typename FieldVecType::ComponentType,3>
CellDerivative(const FieldVecType &,
               const WorldCoordType &,
               const vtkm::Vec<ParametricCoordType,3> &,
               vtkm::CellShapeTagEmpty,
               const vtkm::exec::FunctorBase &worklet)
{
  worklet.RaiseError("Attempted to take derivative in empty cell.");
  return vtkm::Vec<typename FieldVecType::ComponentType,3>();
}

//-----------------------------------------------------------------------------
template<typename FieldVecType,
         typename WorldCoordType,
         typename ParametricCoordType>
VTKM_EXEC_EXPORT
vtkm::Vec<typename FieldVecType::ComponentType,3>
CellDerivative(const FieldVecType &field,
               const WorldCoordType &wCoords,
               const vtkm::Vec<ParametricCoordType,3> &,
               vtkm::CellShapeTagVertex,
               const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(field.GetNumberOfComponents() == 1, worklet);
  VTKM_ASSERT_EXEC(wCoords.GetNumberOfComponents() == 1, worklet);

  typedef vtkm::Vec<typename FieldVecType::ComponentType,3> GradientType;
  return GradientType(0,0,0);
}

//-----------------------------------------------------------------------------
template<typename FieldVecType,
         typename WorldCoordType,
         typename ParametricCoordType>
VTKM_EXEC_EXPORT
vtkm::Vec<typename FieldVecType::ComponentType,3>
CellDerivative(const FieldVecType &field,
               const WorldCoordType &wCoords,
               const vtkm::Vec<ParametricCoordType,3> &vtkmNotUsed(pcoords),
               vtkm::CellShapeTagLine,
               const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(field.GetNumberOfComponents() == 2, worklet);
  VTKM_ASSERT_EXEC(wCoords.GetNumberOfComponents() == 2, worklet);

  typedef typename FieldVecType::ComponentType FieldType;
  typedef vtkm::Vec<FieldType,3> GradientType;

  // The derivative of a line is in the direction of the line. Its length is
  // equal to the difference of the scalar field divided by the length of the
  // line segment. Thus, the derivative is characterized by
  // (deltaField*vec)/mag(vec)^2.

  FieldType deltaField = field[1] - field[0];
  GradientType vec = wCoords[1] - wCoords[0];

  return (deltaField/vtkm::MagnitudeSquared(vec))*vec;
}

//-----------------------------------------------------------------------------
template<typename FieldVecType,
         typename WorldCoordType,
         typename ParametricCoordType>
VTKM_EXEC_EXPORT
vtkm::Vec<typename FieldVecType::ComponentType,3>
CellDerivative(const FieldVecType &field,
               const WorldCoordType &wCoords,
               const vtkm::Vec<ParametricCoordType,3> &vtkmNotUsed(pcoords),
               vtkm::CellShapeTagTriangle,
               const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(field.GetNumberOfComponents() == 3, worklet);
  VTKM_ASSERT_EXEC(wCoords.GetNumberOfComponents() == 3, worklet);

  typedef typename FieldVecType::ComponentType FieldType;
  typedef vtkm::Vec<FieldType,3> GradientType;

  // The scalar values of the three points in a triangle completely specify a
  // linear field (with constant gradient) assuming the field is constant in
  // the normal direction to the triangle. The field, defined by the 3-vector
  // gradient g and scalar value s_origin, can be found with this set of 4
  // equations and 4 unknowns.
  //
  // dot(p0, g) + s_origin = s0
  // dot(p1, g) + s_origin = s1
  // dot(p2, g) + s_origin = s2
  // dot(n, g)             = 0
  //
  // Where the p's are point coordinates and n is the normal vector. But we
  // don't really care about s_origin. We just want to find the gradient g.
  // With some simple elimination we, we can get rid of s_origin and be left
  // with 3 equations and 3 unknowns.
  //
  // dot(p1-p0, g) = s1 - s0
  // dot(p2-p0, g) = s2 - s0
  // dot(n, g)     = 0
  //
  // We'll solve this by putting this in matrix form Ax = b where the rows of A
  // are the differences in points and normal, b has the scalar differences,
  // and x is really the gradient g.

  GradientType v0 = wCoords[1] - wCoords[0];
  GradientType v1 = wCoords[2] - wCoords[0];
  GradientType n = vtkm::Cross(v0, v1);

  vtkm::Matrix<FieldType,3,3> A;
  vtkm::MatrixSetRow(A, 0, v0);
  vtkm::MatrixSetRow(A, 1, v1);
  vtkm::MatrixSetRow(A, 2, n);

  GradientType b(field[1] - field[0], field[2] - field[0], 0);

  // If we want to later change this method to take the gradient of multiple
  // values (for example, to find the Jacobian of a vector field), then there
  // are more efficient ways to solve them all than independently solving this
  // equation for each component of the field. You could find the inverse of
  // matrix A. Or you could alter the functions in vtkm/Matrix.h to
  // simultaneously solve multiple equations.

  // If the triangle is degenerate, then valid will be false. For now we are
  // ignoring it. We could detect it if we determine we need to although I have
  // seen singular matrices missed due to floating point error.
  //
  bool valid;

  return vtkm::SolveLinearSystem(A, b, valid);
}

//-----------------------------------------------------------------------------
template<typename FieldVecType,
         typename WorldCoordType,
         typename ParametricCoordType>
VTKM_EXEC_EXPORT
vtkm::Vec<typename FieldVecType::ComponentType,3>
CellDerivative(const FieldVecType &field,
               const WorldCoordType &wCoords,
               const vtkm::Vec<ParametricCoordType,3> &pcoords,
               vtkm::CellShapeTagPolygon,
               const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(field.GetNumberOfComponents() ==
                     wCoords.GetNumberOfComponents(),
                   worklet);
  VTKM_ASSERT_EXEC(field.GetNumberOfComponents() > 0, worklet);

  switch (field.GetNumberOfComponents())
  {
    case 1: return CellDerivative(field,
                                  wCoords,
                                  pcoords,
                                  vtkm::CellShapeTagVertex(),
                                  worklet);
    case 2: return CellDerivative(field,
                                  wCoords,
                                  pcoords,
                                  vtkm::CellShapeTagLine(),
                                  worklet);
    case 3: return CellDerivative(field,
                                  wCoords,
                                  pcoords,
                                  vtkm::CellShapeTagTriangle(),
                                  worklet);
    case 4: return CellDerivative(field,
                                  wCoords,
                                  pcoords,
                                  vtkm::CellShapeTagQuad(),
                                  worklet);
  }

  // If we are here, then our polygon has 5 or more nodes. Estimate the
  // gradient by sampling a small triangle at the point coordinates and
  // computing the gradient of that triangle.
  static const ParametricCoordType delta = 0.01f;
  vtkm::Vec<typename WorldCoordType::ComponentType,3> triWorldCoords;
  vtkm::Vec<typename FieldVecType::ComponentType,3> triField;
  triWorldCoords[0] =
      CellInterpolate(wCoords, pcoords, vtkm::CellShapeTagPolygon(), worklet);
  triField[0] =
      CellInterpolate(field, pcoords, vtkm::CellShapeTagPolygon(), worklet);
  vtkm::Vec<ParametricCoordType,3>
      pcoords1(pcoords[0]+delta, pcoords[1], pcoords[2]);
  triWorldCoords[1] =
      CellInterpolate(wCoords, pcoords1, vtkm::CellShapeTagPolygon(), worklet);
  triField[1] =
      CellInterpolate(field, pcoords1, vtkm::CellShapeTagPolygon(), worklet);
  vtkm::Vec<ParametricCoordType,3>
      pcoords2(pcoords[0], pcoords[1]+delta, pcoords[2]);
  triWorldCoords[2] =
      CellInterpolate(wCoords, pcoords2, vtkm::CellShapeTagPolygon(), worklet);
  triField[2] =
      CellInterpolate(field, pcoords2, vtkm::CellShapeTagPolygon(), worklet);

  // In the call below, pcoords is actually wrong, but that does not matter
  // since the triangle cell derivative ignores it.
  return CellDerivative(triField,
                        triWorldCoords,
                        pcoords,
                        vtkm::CellShapeTagTriangle(),
                        worklet);
}

//-----------------------------------------------------------------------------
template<typename FieldVecType,
         typename WorldCoordType,
         typename ParametricCoordType>
VTKM_EXEC_EXPORT
vtkm::Vec<typename FieldVecType::ComponentType,3>
CellDerivative(const FieldVecType &field,
               const WorldCoordType &wCoords,
               const vtkm::Vec<ParametricCoordType,3> &pcoords,
               vtkm::CellShapeTagPixel,
               const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(field.GetNumberOfComponents() == 4, worklet);
  VTKM_ASSERT_EXEC(wCoords.GetNumberOfComponents() == 4, worklet);

  return detail::CellDerivativeFor2DCell(
        field, wCoords, pcoords, vtkm::CellShapeTagPixel());
}

//-----------------------------------------------------------------------------
template<typename FieldVecType,
         typename WorldCoordType,
         typename ParametricCoordType>
VTKM_EXEC_EXPORT
vtkm::Vec<typename FieldVecType::ComponentType,3>
CellDerivative(const FieldVecType &field,
               const WorldCoordType &wCoords,
               const vtkm::Vec<ParametricCoordType,3> &pcoords,
               vtkm::CellShapeTagQuad,
               const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(field.GetNumberOfComponents() == 4, worklet);
  VTKM_ASSERT_EXEC(wCoords.GetNumberOfComponents() == 4, worklet);

  return detail::CellDerivativeFor2DCell(
        field, wCoords, pcoords, vtkm::CellShapeTagQuad());
}

//-----------------------------------------------------------------------------
template<typename FieldVecType,
         typename WorldCoordType,
         typename ParametricCoordType>
VTKM_EXEC_EXPORT
vtkm::Vec<typename FieldVecType::ComponentType,3>
CellDerivative(const FieldVecType &field,
               const WorldCoordType &wCoords,
               const vtkm::Vec<ParametricCoordType,3> &vtkmNotUsed(pcoords),
               vtkm::CellShapeTagTetra,
               const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(field.GetNumberOfComponents() == 4, worklet);
  VTKM_ASSERT_EXEC(wCoords.GetNumberOfComponents() == 4, worklet);

  typedef typename FieldVecType::ComponentType FieldType;
  typedef vtkm::Vec<FieldType,3> GradientType;

  // The scalar values of the four points in a tetrahedron completely specify a
  // linear field (with constant gradient). The field, defined by the 3-vector
  // gradient g and scalar value s_origin, can be found with this set of 4
  // equations and 4 unknowns.
  //
  // dot(p0, g) + s_origin = s0
  // dot(p1, g) + s_origin = s1
  // dot(p2, g) + s_origin = s2
  // dot(p3, g) + s_origin = s3
  //
  // Where the p's are point coordinates. But we don't really care about
  // s_origin. We just want to find the gradient g. With some simple
  // elimination we, we can get rid of s_origin and be left with 3 equations
  // and 3 unknowns.
  //
  // dot(p1-p0, g) = s1 - s0
  // dot(p2-p0, g) = s2 - s0
  // dot(p3-p0, g) = s3 - s0
  //
  // We'll solve this by putting this in matrix form Ax = b where the rows of A
  // are the differences in points and normal, b has the scalar differences,
  // and x is really the gradient g.

  GradientType v0 = wCoords[1] - wCoords[0];
  GradientType v1 = wCoords[2] - wCoords[0];
  GradientType v2 = wCoords[3] - wCoords[0];

  vtkm::Matrix<FieldType,3,3> A;
  vtkm::MatrixSetRow(A, 0, v0);
  vtkm::MatrixSetRow(A, 1, v1);
  vtkm::MatrixSetRow(A, 2, v2);

  GradientType b(field[1]-field[0], field[2]-field[0], field[3]-field[0]);

  // If we want to later change this method to take the gradient of multiple
  // values (for example, to find the Jacobian of a vector field), then there
  // are more efficient ways to solve them all than independently solving this
  // equation for each component of the field. You could find the inverse of
  // matrix A. Or you could alter the functions in vtkm/Matrix.h to
  // simultaneously solve multiple equations.

  // If the tetrahedron is degenerate, then valid will be false. For now we are
  // ignoring it. We could detect it if we determine we need to although I have
  // seen singular matrices missed due to floating point error.
  //
  bool valid;

  return vtkm::SolveLinearSystem(A, b, valid);
}

//-----------------------------------------------------------------------------
template<typename FieldVecType,
         typename WorldCoordType,
         typename ParametricCoordType>
VTKM_EXEC_EXPORT
vtkm::Vec<typename FieldVecType::ComponentType,3>
CellDerivative(const FieldVecType &field,
               const WorldCoordType &wCoords,
               const vtkm::Vec<ParametricCoordType,3> &pcoords,
               vtkm::CellShapeTagVoxel,
               const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(field.GetNumberOfComponents() == 8, worklet);
  VTKM_ASSERT_EXEC(wCoords.GetNumberOfComponents() == 8, worklet);

  return detail::CellDerivativeFor3DCell(
        field, wCoords, pcoords, vtkm::CellShapeTagVoxel());
}

//-----------------------------------------------------------------------------
template<typename FieldVecType,
         typename WorldCoordType,
         typename ParametricCoordType>
VTKM_EXEC_EXPORT
vtkm::Vec<typename FieldVecType::ComponentType,3>
CellDerivative(const FieldVecType &field,
               const WorldCoordType &wCoords,
               const vtkm::Vec<ParametricCoordType,3> &pcoords,
               vtkm::CellShapeTagHexahedron,
               const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(field.GetNumberOfComponents() == 8, worklet);
  VTKM_ASSERT_EXEC(wCoords.GetNumberOfComponents() == 8, worklet);

  return detail::CellDerivativeFor3DCell(
        field, wCoords, pcoords, vtkm::CellShapeTagHexahedron());
}


//-----------------------------------------------------------------------------
template<typename FieldVecType,
         typename WorldCoordType,
         typename ParametricCoordType>
VTKM_EXEC_EXPORT
vtkm::Vec<typename FieldVecType::ComponentType,3>
CellDerivative(const FieldVecType &field,
               const WorldCoordType &wCoords,
               const vtkm::Vec<ParametricCoordType,3> &pcoords,
               vtkm::CellShapeTagWedge,
               const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(field.GetNumberOfComponents() == 6, worklet);
  VTKM_ASSERT_EXEC(wCoords.GetNumberOfComponents() == 6, worklet);

  return detail::CellDerivativeFor3DCell(
        field, wCoords, pcoords, vtkm::CellShapeTagWedge());
}


//-----------------------------------------------------------------------------
template<typename FieldVecType,
         typename WorldCoordType,
         typename ParametricCoordType>
VTKM_EXEC_EXPORT
vtkm::Vec<typename FieldVecType::ComponentType,3>
CellDerivative(const FieldVecType &field,
               const WorldCoordType &wCoords,
               const vtkm::Vec<ParametricCoordType,3> &pcoords,
               vtkm::CellShapeTagPyramid,
               const vtkm::exec::FunctorBase &worklet)
{
  VTKM_ASSERT_EXEC(field.GetNumberOfComponents() == 5, worklet);
  VTKM_ASSERT_EXEC(wCoords.GetNumberOfComponents() == 5, worklet);

  return detail::CellDerivativeFor3DCell(
        field, wCoords, pcoords, vtkm::CellShapeTagPyramid());
}


}
} // namespace vtkm::exec

#endif //vtk_m_exec_Derivative_h
