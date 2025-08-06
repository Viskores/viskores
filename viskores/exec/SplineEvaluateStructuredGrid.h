//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_exec_splineevaluatestructuredgrid_h
#define viskores_exec_splineevaluatestructuredgrid_h

#include <viskores/Bounds.h>
#include <viskores/ErrorCode.h>
#include <viskores/cont/ArrayHandleCartesianProduct.h>
#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>

namespace viskores
{
namespace exec
{

class VISKORES_ALWAYS_EXPORT SplineEvaluateUniformGrid
{
private:
  using FieldType = viskores::cont::ArrayHandle<viskores::FloatDefault>;
  using FieldPortalType = typename FieldType::ReadPortalType;

public:
  VISKORES_CONT SplineEvaluateUniformGrid() = default;

  template <typename ArrayPortalType>
  VISKORES_CONT SplineEvaluateUniformGrid(const viskores::Vec3f origin,
                                          const viskores::Vec3f spacing,
                                          const viskores::Id3 cellDims,
                                          const ArrayPortalType& field)
    : Bounds(origin[0],
             origin[0] + static_cast<viskores::FloatDefault>(cellDims[0] * spacing[0]),
             origin[1],
             origin[1] + static_cast<viskores::FloatDefault>(cellDims[1] * spacing[1]),
             origin[2],
             origin[2] + static_cast<viskores::FloatDefault>(cellDims[2] * spacing[2]))
    , Dimensions(cellDims)
    , Field(field)
    , Origin(origin)
    , Spacing(spacing)
  {
  }

  VISKORES_EXEC viskores::ErrorCode Evaluate(const viskores::Vec3f& point,
                                             viskores::FloatDefault& value) const
  {
    if (!this->Bounds.Contains(point))
      return viskores::ErrorCode::CellNotFound;

    //map world to index space.
    viskores::Vec3f pointIndex;
    pointIndex[0] = (point[0] - this->Origin[0]) / this->Spacing[0];
    pointIndex[1] = (point[1] - this->Origin[1]) / this->Spacing[1];
    pointIndex[2] = (point[2] - this->Origin[2]) / this->Spacing[2];

    //auto cellDims = cellSet.GetCellDimensions();
    std::cout << "Fix me: " << __LINE__ << std::endl;
    auto cellDims = this->Dimensions;

    return this->TriCubicEvaluate(cellDims, pointIndex, this->Field, value);
  }

private:
  template <typename FieldType>
  VISKORES_EXEC viskores::ErrorCode TriCubicEvaluate(const viskores::Id3& dims,
                                                     const viskores::Vec3f& pointIndex,
                                                     const FieldType& field,
                                                     viskores::FloatDefault& value) const
  {
    viskores::Id nx = dims[0], ny = dims[1], nz = dims[2];
    viskores::FloatDefault x = pointIndex[0], y = pointIndex[1], z = pointIndex[2];
    // base integer coords
    viskores::Id ix = static_cast<viskores::Id>(viskores::Floor(x));
    viskores::Id iy = static_cast<viskores::Id>(viskores::Floor(y));
    viskores::Id iz = static_cast<viskores::Id>(viskores::Floor(z));

    // fractional offsets
    viskores::FloatDefault tx = x - ix;
    viskores::FloatDefault ty = y - iy;
    viskores::FloatDefault tz = z - iz;

    // Coefficients for tricubic interpolation
    viskores::FloatDefault P[4 * 4 * 4];
    viskores::FloatDefault C[4 * 4];
    viskores::FloatDefault D[4];

    // 1) Gather 4×4×4 neighborhood into P
    //    P[kk][jj][ii] -> P[(kk*4 + jj)*4 + ii]
    //    data[z0][y0][x0] -> data[(z0*ny + y0)*nx + x0]
    for (viskores::Id kk = 0; kk < 4; ++kk)
    {
      viskores::Id z0 = viskores::Clamp(iz - 1 + kk, 0, nz);
      for (viskores::Id jj = 0; jj < 4; ++jj)
      {
        viskores::Id y0 = viskores::Clamp(iy - 1 + jj, 0, ny);
        for (viskores::Id ii = 0; ii < 4; ++ii)
        {
          viskores::Id x0 = viskores::Clamp(ix - 1 + ii, 0, nx);
          // flatten 3D (kk,jj,ii) to 1D:
          viskores::Id pIndex = (kk * 4 + jj) * 4 + ii;
          // flatten volume coords: (x0,y0,z0) -> 1D index
          viskores::Id dIndex = (z0 * ny + y0) * nx + x0;
          P[pIndex] = field.Get(dIndex);
        }
      }
    }

    // 2) Interpolate in X for each (kk, jj) → C[kk][jj]
    //    C[kk][jj] -> C[kk*4 + jj]
    for (viskores::Id kk = 0; kk < 4; ++kk)
    {
      for (viskores::Id jj = 0; jj < 4; ++jj)
      {
        // base offset for P row
        viskores::Id baseP = (kk * 4 + jj) * 4;
        viskores::Id cIndex = kk * 4 + jj;
        C[cIndex] =
          this->CubicInterpolate(P[baseP + 0], P[baseP + 1], P[baseP + 2], P[baseP + 3], tx);
      }
    }

    // 3) Interpolate in Y for each kk → D[kk]
    //    C[kk][0..3] -> C[kk*4 + 0..3]
    for (viskores::Id kk = 0; kk < 4; ++kk)
    {
      D[kk] =
        this->CubicInterpolate(C[kk * 4 + 0], C[kk * 4 + 1], C[kk * 4 + 2], C[kk * 4 + 3], ty);
    }

    // 4) Interpolate in Z across D[0..3]
    value = this->CubicInterpolate(D[0], D[1], D[2], D[3], tz);
    return viskores::ErrorCode::Success;
  }

  // 1D cubic‐convolution (Catmull–Rom) kernel
  // given four samples p0,p1,p2,p3 and relative t in [0,1]
  VISKORES_EXEC viskores::FloatDefault CubicInterpolate(viskores::FloatDefault p0,
                                                        viskores::FloatDefault p1,
                                                        viskores::FloatDefault p2,
                                                        viskores::FloatDefault p3,
                                                        viskores::FloatDefault t) const
  {
    // Catmull–Rom basis with no tension.
    viskores::FloatDefault t2 = t * t;
    viskores::FloatDefault t3 = t2 * t;

    viskores::FloatDefault m0 = (p2 - p0) * 0.5f;
    viskores::FloatDefault m1 = (p3 - p1) * 0.5f;
    viskores::FloatDefault d0 = p1;
    viskores::FloatDefault d1 = p2;

    // Hermite form: h00, h10, h01, h11
    viskores::FloatDefault h00 = 2 * t3 - 3 * t2 + 1;
    viskores::FloatDefault h10 = t3 - 2 * t2 + t;
    viskores::FloatDefault h01 = -2 * t3 + 3 * t2;
    viskores::FloatDefault h11 = t3 - t2;

    return h00 * d0 + h10 * m0 + h01 * d1 + h11 * m1;
  }

  viskores::Bounds Bounds;
  viskores::Id3 Dimensions;
  FieldPortalType Field;
  viskores::Vec3f Origin;
  viskores::Vec3f Spacing;
};

class VISKORES_ALWAYS_EXPORT SplineEvaluateRectilinearGrid
{
private:
  using FieldType = viskores::cont::ArrayHandle<viskores::FloatDefault>;
  using AxisType = FieldType;

  using RectilinearCoordsType =
    viskores::cont::ArrayHandleCartesianProduct<AxisType, AxisType, AxisType>;
  using FieldPortalType = typename FieldType::ReadPortalType;
  using AxisPortalType = typename AxisType::ReadPortalType;
  using RectilinearPortalType = typename RectilinearCoordsType::ReadPortalType;

public:
  VISKORES_CONT SplineEvaluateRectilinearGrid() = default;

  template <typename AxisType, typename ArrayPortalType>
  VISKORES_CONT SplineEvaluateRectilinearGrid(
    viskores::cont::ArrayHandleCartesianProduct<AxisType, AxisType, AxisType> coords,
    const ArrayPortalType& field)
    : Field(field)
  {
    this->AxisPortals[0] = coords.GetFirstArray().ReadPortal();
    this->AxisPortals[1] = coords.GetSecondArray().ReadPortal();
    this->AxisPortals[2] = coords.GetThirdArray().ReadPortal();

    this->NumX = this->AxisPortals[0].GetNumberOfValues();
    this->NumY = this->AxisPortals[1].GetNumberOfValues();
    this->NumZ = this->AxisPortals[2].GetNumberOfValues();
    this->Bounds = viskores::Bounds(this->AxisPortals[0].Get(0),
                                    this->AxisPortals[0].Get(this->NumX - 1),
                                    this->AxisPortals[1].Get(0),
                                    this->AxisPortals[1].Get(this->NumY - 1),
                                    this->AxisPortals[2].Get(0),
                                    this->AxisPortals[2].Get(this->NumZ - 1));
  }

  VISKORES_EXEC viskores::ErrorCode Evaluate(const viskores::Vec3f& point,
                                             viskores::FloatDefault& value) const
  {
    if (!this->Bounds.Contains(point))
      return viskores::ErrorCode::CellNotFound;

    auto x = point[0];
    auto y = point[1];
    auto z = point[2];

    viskores::Id iu = this->FindIndex(this->AxisPortals[0], this->NumX, point[0]);
    viskores::Id iv = this->FindIndex(this->AxisPortals[1], this->NumY, point[1]);
    viskores::Id iw = this->FindIndex(this->AxisPortals[2], this->NumZ, point[2]);

    if (this->NumZ < 4)
    {
      std::cout << "FIX ME: " << __LINE__ << std::endl;
      value = 0.0f;
      return viskores::ErrorCode::Success;
#if 0
      // --- bicubic: gather a 4×4 patch in X–Y at single k = clamp(iw,0,ny−1) ---
      double P2d[4 * 4];
      for (viskores::Id jj = 0; jj < 4; ++jj)
      {
        viskores::Id j = clamp(iv - 1 + jj, ny);
        for (viskores::Id ii = 0; ii < 4; ++ii)
        {
          viskores::Id i = clamp(iu - 1 + ii, nx);
          // flatten (i,j, 0) → data index
          P2d[jj * 4 + ii] = data.Get((j * nx) + i);
        }
      }
      // 3) bicubic along X → C2[4]
      double C2[4];
      for (int jj = 0; jj < 4; ++jj)
      {
        auto x0 = xCoords.Get(iu - 1), x1 = xCoords.Get(iu), x2 = xCoords.Get(iu + 1),
             x3 = xCoords.Get(iu + 2);
        C2[jj] = cubicInterpolateNonUniform(
          x0, x1, x2, x3, P2d[jj * 4 + 0], P2d[jj * 4 + 1], P2d[jj * 4 + 2], P2d[jj * 4 + 3], x);
      }
      // 4) bicubic along Y on C2 → final
      auto y0 = yCoords.Get(iv - 1), y1 = yCoords.Get(iv), y2 = yCoords.Get(iv + 1),
           y3 = yCoords.Get(iv + 2);
      return cubicInterpolateNonUniform(y0, y1, y2, y3, C2[0], C2[1], C2[2], C2[3], y);
#endif
    }

    viskores::FloatDefault P[4 * 4 * 4];
    for (viskores::Id kk = 0; kk < 4; ++kk)
    {
      viskores::Id k = viskores::Clamp(iw - 1 + kk, 0, this->NumZ);
      for (viskores::Id jj = 0; jj < 4; ++jj)
      {
        viskores::Id j = viskores::Clamp(iv - 1 + jj, 0, this->NumY);
        for (viskores::Id ii = 0; ii < 4; ++ii)
        {
          viskores::Id i = viskores::Clamp(iu - 1 + ii, 0, this->NumX);
          auto pIndex = (kk * 4 + jj) * 4 + ii;
          auto dIndex = (k * this->NumY + j) * this->NumX + i;
          P[pIndex] = this->Field.Get(dIndex);
        }
      }
    }

    // interpolate in X for each (kk,jj) → Cbuf[16]
    viskores::FloatDefault Cbuf[4 * 4];
    for (int kk = 0; kk < 4; ++kk)
      for (int jj = 0; jj < 4; ++jj)
      {
        auto x0 = this->AxisPortals[0].Get(iu - 1);
        auto x1 = this->AxisPortals[0].Get(iu);
        auto x2 = this->AxisPortals[0].Get(iu + 1);
        auto x3 = this->AxisPortals[0].Get(iu + 2);
        auto p0 = P[(kk * 4 + jj) * 4 + 0];
        auto p1 = P[(kk * 4 + jj) * 4 + 1];
        auto p2 = P[(kk * 4 + jj) * 4 + 2];
        auto p3 = P[(kk * 4 + jj) * 4 + 3];
        Cbuf[kk * 4 + jj] = this->CubicInterpolateNonUniform(x0, x1, x2, x3, p0, p1, p2, p3, x);
      }

    // interpolate in Y → D[4]
    viskores::FloatDefault D[4];
    for (int kk = 0; kk < 4; ++kk)
    {
      auto y0 = this->AxisPortals[1].Get(iv - 1);
      auto y1 = this->AxisPortals[1].Get(iv);
      auto y2 = this->AxisPortals[1].Get(iv + 1);
      auto y3 = this->AxisPortals[1].Get(iv + 2);
      D[kk] = this->CubicInterpolateNonUniform(
        y0, y1, y2, y3, Cbuf[kk * 4 + 0], Cbuf[kk * 4 + 1], Cbuf[kk * 4 + 2], Cbuf[kk * 4 + 3], y);
    }

    // interpolate in Z
    auto z0 = this->AxisPortals[2].Get(iw - 1);
    auto z1 = this->AxisPortals[2].Get(iw);
    auto z2 = this->AxisPortals[2].Get(iw + 1);
    auto z3 = this->AxisPortals[2].Get(iw + 2);
    value = this->CubicInterpolateNonUniform(z0, z1, z2, z3, D[0], D[1], D[2], D[3], z);

    return viskores::ErrorCode::Success;
  }

private:
  VISKORES_EXEC viskores::Id FindIndex(const AxisPortalType& axis,
                                       const viskores::Id& N,
                                       viskores::FloatDefault val) const
  {
    // 1) Binary search for the largest index i with coords[i] <= val
    viskores::Id left = 0;
    viskores::Id right = N - 1;
    while (left <= right)
    {
      viskores::Id mid = left + (right - left) / 2;
      if (axis.Get(mid) <= val)
      {
        // mid is still ≤ val, so it might be our i
        left = mid + 1;
      }
      else
      {
        // coords[mid] > val, so the index we want is below mid
        right = mid - 1;
      }
    }
    // when loop ends, `right` is the last index where coords[right] <= val
    viskores::Id i = right;

    // 2) Clamp i into [1, N-3]
    if (i < 1)
      i = 1;
    else if (i > N - 3)
      i = N - 3;

    return i;
  }

  VISKORES_EXEC
  viskores::FloatDefault CubicInterpolateNonUniform(viskores::FloatDefault x0,
                                                    viskores::FloatDefault x1,
                                                    viskores::FloatDefault x2,
                                                    viskores::FloatDefault x3,
                                                    viskores::FloatDefault p0,
                                                    viskores::FloatDefault p1,
                                                    viskores::FloatDefault p2,
                                                    viskores::FloatDefault p3,
                                                    viskores::FloatDefault x) const
  {
    // 1) Compute interval lengths
    viskores::FloatDefault h0 = x1 - x0;
    viskores::FloatDefault h1 = x2 - x1;
    viskores::FloatDefault h2 = x3 - x2;
    if (h0 <= 0 || h1 <= 0 || h2 <= 0)
      throw std::runtime_error(
        "cubicInterpolateNonUniform: coordinates must be strictly increasing");

    // 2) Compute right‐hand sides for second‐derivative system
    viskores::FloatDefault rhs1 = 6.0 * ((p2 - p1) / h1 - (p1 - p0) / h0);
    viskores::FloatDefault rhs2 = 6.0 * ((p3 - p2) / h2 - (p2 - p1) / h1);

    // 3) Build and solve the 2×2 system:
    //     [2(h0+h1)   h1      ][d2_1] = [rhs1]
    //     [  h1     2(h1+h2)  ][d2_2]   [rhs2]
    viskores::FloatDefault a11 = 2.0 * (h0 + h1);
    viskores::FloatDefault a12 = h1;
    viskores::FloatDefault a21 = h1;
    viskores::FloatDefault a22 = 2.0 * (h1 + h2);
    viskores::FloatDefault det = a11 * a22 - a12 * a21;
    if (det == 0.0)
      throw std::runtime_error("cubicInterpolateNonUniform: degenerate knot spacing");
    viskores::FloatDefault d2_1 = (rhs1 * a22 - a12 * rhs2) / det;
    viskores::FloatDefault d2_2 = (a11 * rhs2 - rhs1 * a21) / det;

    // 4) Map x into local parameter t ∈ [0,1] on [x1,x2]
    viskores::FloatDefault t = (x - x1) / h1;

    // 5) Hermite form of the natural cubic on [x1, x2]
    viskores::FloatDefault A = 1.0 - t;
    viskores::FloatDefault B = t;
    viskores::FloatDefault h1_sq = h1 * h1;
    viskores::FloatDefault term1 = (A * A * A - A) * (h1_sq / 6.0) * d2_1;
    viskores::FloatDefault term2 = (B * B * B - B) * (h1_sq / 6.0) * d2_2;

    // 6) Combine the linear and curvature parts
    return A * p1 + B * p2 + term1 + term2;
  }

  AxisPortalType AxisPortals[3];
  viskores::Bounds Bounds;
  FieldPortalType Field;
  viskores::Id NumX;
  viskores::Id NumY;
  viskores::Id NumZ;
};
} //namespace exec
} //namespace viskores

#endif //viskores_exec_splineevaluatestructuredgrid_h
