//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtkm_filter_flow_worklet_XGCHelper_h
#define vtkm_filter_flow_worklet_XGCHelper_h

#include <vtkm/Geometry.h>
#include <vtkm/Matrix.h>

namespace vtkm
{
namespace worklet
{
namespace flow
{

struct XGCParams
{
  vtkm::Id NumPlanes = -1;
  vtkm::Id NumNodes = -1;
  vtkm::Id NumTri = -1;

  vtkm::FloatDefault Period;
  vtkm::FloatDefault dPhi;

  int ncoeff_r, ncoeff_z, ncoeff_psi;
  vtkm::FloatDefault min_psi, max_psi;
  vtkm::FloatDefault sml_bp_sign = -1.0f;

  vtkm::FloatDefault eq_axis_r = -1, eq_axis_z = -1, eq_x_psi = -1, eq_x_r = -1, eq_x_z = -1;
  vtkm::FloatDefault eq_min_r = -1, eq_max_r = -1;
  vtkm::FloatDefault eq_min_z = -1, eq_max_z = 1;
  vtkm::FloatDefault psi_min = -1, psi_max = -1;
  vtkm::Id eq_mr = -1, eq_mz = -1;
  vtkm::Id eq_mpsi = -1;
  vtkm::Id sml_wedge_n = -1;
  vtkm::FloatDefault itp_min_psi = -1, itp_max_psi = -1;
  vtkm::FloatDefault one_d_cub_dpsi_inv;

  int nr, nz;
  vtkm::FloatDefault rmin, zmin, rmax, zmax;
  vtkm::FloatDefault dr, dz, dr_inv, dz_inv;
  vtkm::FloatDefault eq_x2_z, eq_x2_r, eq_x2_psi;
  vtkm::FloatDefault eq_x_slope, eq_x2_slope;
};

struct ParticleMetaData
{
  ParticleMetaData() = default;
  ParticleMetaData(const ParticleMetaData&) = default;
  ParticleMetaData& operator=(const ParticleMetaData&) = default;

  vtkm::Id3 PrevCell = { -1, -1, -1 };
  vtkm::FloatDefault Psi = 0;
  vtkm::FloatDefault dpsi_dr = 0, dpsi_dz = 0, d2psi_drdz = 0, d2psi_d2r = 0, d2psi_d2z = 0;
  vtkm::Vec3f gradPsi_rzp, B0_rzp, curlB_rzp, curl_nb_rzp;
};

VTKM_EXEC
inline vtkm::Id GetIndex(const vtkm::FloatDefault& x,
                         const vtkm::Id& nx,
                         const vtkm::FloatDefault& xmin,
                         const vtkm::FloatDefault& dx_inv)
{
  vtkm::Id idx = vtkm::Max(vtkm::Id(0), vtkm::Min(nx - 1, vtkm::Id((x - xmin) * dx_inv)));
  return idx;
}

template <typename ScalarPortal>
VTKM_EXEC void EvalBicub2(const vtkm::FloatDefault& x,
                          const vtkm::FloatDefault& y,
                          const vtkm::FloatDefault& xc,
                          const vtkm::FloatDefault& yc,
                          const vtkm::Id& offset,
                          const ScalarPortal& Coeff_2D,
                          vtkm::FloatDefault& f00,
                          vtkm::FloatDefault& f10,
                          vtkm::FloatDefault& f01,
                          vtkm::FloatDefault& f11,
                          vtkm::FloatDefault& f20,
                          vtkm::FloatDefault& f02)
{
  vtkm::FloatDefault dx = x - xc;
  vtkm::FloatDefault dy = y - yc;

  //fortran code.
  f00 = f01 = f10 = f11 = f20 = f02 = 0.0f;
  vtkm::FloatDefault xv[4] = { 1, dx, dx * dx, dx * dx * dx };
  vtkm::FloatDefault yv[4] = { 1, dy, dy * dy, dy * dy * dy };
  vtkm::FloatDefault fx[4] = { 0, 0, 0, 0 };
  vtkm::FloatDefault dfx[4] = { 0, 0, 0, 0 };
  vtkm::FloatDefault dfy[4] = { 0, 0, 0, 0 };
  vtkm::FloatDefault dfx2[4] = { 0, 0, 0, 0 };
  vtkm::FloatDefault dfy2[4] = { 0, 0, 0, 0 };

  for (int j = 0; j < 4; j++)
  {
    for (int i = 0; i < 4; i++)
      fx[j] = fx[j] + xv[i] * Coeff_2D.Get(offset + j * 4 + i); //acoeff[i][j];
    for (int i = 1; i < 4; i++)
      dfx[j] = dfx[j] +
        vtkm::FloatDefault(i) * xv[i - 1] * Coeff_2D.Get(offset + j * 4 + i); //acoeff[i][j];
    for (int i = 2; i < 4; i++)
      dfx2[j] = dfx2[j] +
        vtkm::FloatDefault(i * (i - 1)) * xv[i - 2] *
          Coeff_2D.Get(offset + j * 4 + i); //acoeff[i][j];
  }

  for (int j = 0; j < 4; j++)
  {
    f00 = f00 + fx[j] * yv[j];
    f10 = f10 + dfx[j] * yv[j];
    f20 = f20 + dfx2[j] * yv[j];
  }

  for (int j = 1; j < 4; j++)
  {
    dfy[j] = vtkm::FloatDefault(j) * yv[j - 1];
    f01 = f01 + fx[j] * dfy[j];
    f11 = f11 + dfx[j] * dfy[j];
  }

  for (int j = 2; j < 4; j++)
  {
    dfy2[j] = vtkm::FloatDefault(j * (j - 1)) * yv[j - 2];
    f02 = f02 + fx[j] * dfy2[j];
  }
}

template <typename ScalarPortal>
VTKM_EXEC vtkm::FloatDefault I_interpol(const vtkm::FloatDefault& in_psi,
                                        const vtkm::Id& ideriv,
                                        const vtkm::Id& region,
                                        const XGCParams& Params,
                                        const ScalarPortal& Coeff_1D)
{
  vtkm::FloatDefault psi;
  if (region == 3)
  {
    psi = vtkm::Min(Params.eq_x_psi, Params.itp_max_psi);
    if (ideriv != 0)
      return 0.0;
  }
  else
  {
    psi = vtkm::Min(in_psi, Params.itp_max_psi);
    psi = vtkm::Max(psi, Params.itp_min_psi);
  }

  vtkm::FloatDefault pn = psi * Params.one_d_cub_dpsi_inv;
  vtkm::Id ip = floor(pn);
  ip = vtkm::Min(vtkm::Max(ip, vtkm::Id(0)), vtkm::Id(Params.ncoeff_psi - 1));
  vtkm::FloatDefault wp = pn - (vtkm::FloatDefault)(ip);

  int idx = ip * 4;

  //vtkm::FloatDefault acoef[4];
  //acoef[0] = one_d_cub_acoef(ip).coeff[0];
  //acoef[1] = one_d_cub_acoef(ip).coeff[1];
  //acoef[2] = one_d_cub_acoef(ip).coeff[2];
  //acoef[3] = one_d_cub_acoef(ip).coeff[3];

  const vtkm::FloatDefault acoef[4] = {
    Coeff_1D.Get(idx + 0), Coeff_1D.Get(idx + 1), Coeff_1D.Get(idx + 2), Coeff_1D.Get(idx + 3)
  };

  vtkm::FloatDefault iVal = 0.0;
  if (ideriv == 0)
    iVal = acoef[0] + (acoef[1] + (acoef[2] + acoef[3] * wp) * wp) * wp;
  else if (ideriv == 1)
    iVal = (acoef[1] + (2.0 * acoef[2] + 3.0 * acoef[3] * wp) * wp) * Params.one_d_cub_dpsi_inv;

  return iVal * Params.sml_bp_sign;
}

VTKM_EXEC
inline bool IsRegion12(const vtkm::FloatDefault& R,
                       const vtkm::FloatDefault& Z,
                       const vtkm::FloatDefault& psi,
                       const XGCParams& Params)
{
  constexpr vtkm::FloatDefault epsil_psi = 1e-5;

  if ((psi <= (Params.eq_x_psi - epsil_psi) &&
       -(R - Params.eq_x_r) * Params.eq_x_slope + (Z - Params.eq_x_z) > 0 &&
       -(R - Params.eq_x2_r) * Params.eq_x2_slope + (Z - Params.eq_x2_z) < 0) ||
      (psi > Params.eq_x_psi - epsil_psi && psi <= Params.eq_x2_psi - epsil_psi &&
       -(R - Params.eq_x2_r) * Params.eq_x2_slope + (Z - Params.eq_x2_z) < 0) ||
      psi > Params.eq_x2_psi - epsil_psi)
  {
    return true;
  }

  return false;
}

template <typename ScalarPortal>
VTKM_EXEC bool HighOrderB(const vtkm::Vec3f& pointRPZ,
                          ParticleMetaData& metadata,
                          const XGCParams& Params,
                          const ScalarPortal& Coeff_1D,
                          const ScalarPortal& Coeff_2D)
{
  vtkm::FloatDefault R = pointRPZ[0], Z = pointRPZ[2];

  vtkm::Id r_i = GetIndex(R, Params.nr, Params.eq_min_r, Params.dr_inv);
  vtkm::Id z_i = GetIndex(Z, Params.nz, Params.zmin, Params.dz_inv);

  vtkm::FloatDefault Rc = Params.eq_min_r + (vtkm::FloatDefault)(r_i)*Params.dr;
  vtkm::FloatDefault Zc = Params.eq_min_z + (vtkm::FloatDefault)(z_i)*Params.dz;

  auto Rc_1 = Rc + Params.dr;
  auto Zc_1 = Zc + Params.dz;
  Rc = (Rc + Rc_1) * 0.5;
  Zc = (Zc + Zc_1) * 0.5;

  //Get the coeffcients (z,r,4,4)
  vtkm::Matrix<vtkm::FloatDefault, 4, 4> acoeff;
  //offset = ri * nz + zi
  //vtkm::Id offset = (r_i * ncoeff + z_i) * 16; //DRP
  vtkm::Id offset = (z_i * Params.ncoeff_r + r_i) * 16;

  vtkm::FloatDefault psi, dpsi_dr, dpsi_dz, d2psi_d2r, d2psi_drdz, d2psi_d2z;
  EvalBicub2(
    R, Z, Rc, Zc, offset, Coeff_2D, psi, dpsi_dr, dpsi_dz, d2psi_drdz, d2psi_d2r, d2psi_d2z);

  if (psi < 0)
    psi = 0;

  metadata.Psi = psi;
  //PSI = psi;
  metadata.gradPsi_rzp[0] = dpsi_dr;
  metadata.gradPsi_rzp[1] = dpsi_dz;
  metadata.gradPsi_rzp[2] = 0;
  metadata.dpsi_dr = dpsi_dr;
  metadata.dpsi_dz = dpsi_dz;
  metadata.d2psi_drdz = d2psi_drdz;
  metadata.d2psi_d2r = d2psi_d2r;
  metadata.d2psi_d2z = d2psi_d2z;

  vtkm::FloatDefault fld_I, fld_dIdpsi;

  bool isR12 = IsRegion12(R, Z, psi, Params);
  if (!isR12)
  {
    fld_I = I_interpol(psi, 0, 3, Params, Coeff_1D);
    fld_dIdpsi = I_interpol(psi, 1, 3, Params, Coeff_1D);
  }
  else
  {
    fld_I = I_interpol(psi, 0, 1, Params, Coeff_1D);
    fld_dIdpsi = I_interpol(psi, 1, 1, Params, Coeff_1D);
  }

  vtkm::FloatDefault over_r = 1 / R;
  vtkm::FloatDefault over_r2 = over_r * over_r;
  vtkm::FloatDefault Br = -dpsi_dz * over_r;
  vtkm::FloatDefault Bz = dpsi_dr * over_r;
  vtkm::FloatDefault Bp = fld_I * over_r;

  metadata.B0_rzp = vtkm::Vec3f(Br, Bz, Bp);

  const vtkm::FloatDefault bp_sign = 1.0;

  auto dBr_dr = (dpsi_dz * over_r2 - d2psi_drdz * over_r) * bp_sign;
  auto dBr_dz = -d2psi_d2z * over_r * bp_sign;
  auto dBr_dp = 0.0 * bp_sign;

  auto dBz_dr = (-dpsi_dr * over_r2 + d2psi_d2r * over_r) * bp_sign;
  auto dBz_dz = d2psi_drdz * over_r * bp_sign;
  auto dBz_dp = 0.0 * bp_sign;

  auto dBp_dr = dpsi_dr * fld_dIdpsi * over_r - fld_I * over_r2;
  auto dBp_dz = fld_dIdpsi * dpsi_dz * over_r;
  //auto dBp_dp = jacobian_rzp[2][2];

  //calculate curl_B
  //vtkm::Vec3f curlB_rzp;
  metadata.curlB_rzp[0] = dBz_dp * over_r - dBp_dz;
  metadata.curlB_rzp[1] = Bp * over_r + dBp_dr - dBr_dp * over_r;
  metadata.curlB_rzp[2] = dBr_dz - dBz_dr;
  //std::cout<<"curl_B_rzp= "<<curlB_rzp<<std::endl;

  //calculate curl_nb
  vtkm::FloatDefault Bmag = vtkm::Magnitude(metadata.B0_rzp);
  vtkm::FloatDefault over_B = 1. / Bmag, over_B2 = over_B * over_B;

  vtkm::FloatDefault dBdr, dBdz;
  dBdr = (Br * dBr_dr + Bp * dBp_dr + Bz * dBz_dr) * over_B;
  dBdz = (Br * dBr_dz + Bp * dBp_dz + Bz * dBz_dz) * over_B;

  //vtkm::Vec3f curl_nb_rzp;
  metadata.curl_nb_rzp[0] = metadata.curlB_rzp[0] * over_B + (Bp * dBdz) * over_B2;
  metadata.curl_nb_rzp[1] = metadata.curlB_rzp[1] * over_B + (-Bp * dBdr) * over_B2;
  metadata.curl_nb_rzp[2] = metadata.curlB_rzp[2] * over_B + (Bz * dBdr - Br * dBdz) * over_B2;

  return true;
}

template <typename ScalarPortal>
VTKM_EXEC vtkm::Vec3f HighOrderBOnly(const vtkm::Vec3f& ptRPZ,
                                     const XGCParams& Params,
                                     const ScalarPortal& Coeff_1D,
                                     const ScalarPortal& Coeff_2D)
{
  vtkm::FloatDefault R = ptRPZ[0], Z = ptRPZ[2];
  vtkm::Vec3f ptRZ(R, Z, 0);

  int r_i = GetIndex(R, Params.nr, Params.rmin, Params.dr_inv);
  int z_i = GetIndex(Z, Params.nz, Params.zmin, Params.dz_inv);

  // rc(i), zc(j)
  vtkm::FloatDefault Rc = Params.rmin + (vtkm::FloatDefault)(r_i)*Params.dr;
  vtkm::FloatDefault Zc = Params.zmin + (vtkm::FloatDefault)(z_i)*Params.dz;
  auto Rc_1 = Rc + Params.dr;
  auto Zc_1 = Zc + Params.dz;
  Rc = (Rc + Rc_1) * 0.5;
  Zc = (Zc + Zc_1) * 0.5;

  //Get the coeffcients (z,r,4,4)
  vtkm::Matrix<vtkm::FloatDefault, 4, 4> acoeff;
  //vtkm::Id offset = (r_i * ncoeff + z_i) * 16; //DRP
  vtkm::Id offset = (z_i * Params.ncoeff_r + r_i) * 16;

  /*
  std::cout<<"InterpolatePsi: "<<vtkm::Vec2f(R,Z)<<std::endl;
  std::cout<<"  i/j= "<<r_i<<" "<<z_i<<std::endl;
  std::cout<<"  ncoeff= "<<ncoeff<<std::endl;
  std::cout<<"  offset= "<<offset<<std::endl;
  std::cout<<"  Rc/Zc= "<<Rc<<" "<<Zc<<std::endl;
  */

  vtkm::FloatDefault psi, dpsi_dr, dpsi_dz, d2psi_d2r, d2psi_drdz, d2psi_d2z;
  EvalBicub2(
    R, Z, Rc, Zc, offset, Coeff_2D, psi, dpsi_dr, dpsi_dz, d2psi_drdz, d2psi_d2r, d2psi_d2z);
  if (psi < 0)
    psi = 0;

  bool isR12 = IsRegion12(R, Z, psi, Params);
  vtkm::FloatDefault fld_I;
  if (!isR12)
    fld_I = I_interpol(psi, 0, 3, Params, Coeff_1D);
  else
    fld_I = I_interpol(psi, 0, 0, Params, Coeff_1D);

  vtkm::FloatDefault over_r = 1 / R;
  vtkm::FloatDefault Br = -dpsi_dz * over_r;
  vtkm::FloatDefault Bz = dpsi_dr * over_r;
  vtkm::FloatDefault Bp = fld_I * over_r;

  return vtkm::Vec3f(Br, Bz, Bp);
}

template <typename ScalarPortal>
VTKM_EXEC vtkm::Vec3f GetB(vtkm::Vec3f& pt_rpz,
                           const XGCParams& Params,
                           const ScalarPortal& Coeff_1D,
                           const ScalarPortal& Coeff_2D)
{
  auto B0_rzp = HighOrderBOnly(pt_rpz, Params, Coeff_1D, Coeff_2D);
  vtkm::Vec3f B0_rpz(B0_rzp[0], B0_rzp[2], B0_rzp[1]);
  // This gives is the time derivative: Br = dR/dt, Bz= dZ/dt, B_phi/R = dphi/dt
  // We need with respect to phi:
  // dR/dphi = dR/dt / (dphi/dt) = Br / B_phi * R
  // same for z;
  B0_rpz[0] /= B0_rzp[2];
  B0_rpz[2] /= B0_rzp[2];
  B0_rpz[0] *= pt_rpz[0];
  B0_rpz[2] *= pt_rpz[0];
  return B0_rpz;
}

template <typename ScalarPortal>
VTKM_EXEC bool CalcFieldFollowingPt(const vtkm::Vec3f& pt_rpz,
                                    const vtkm::Vec3f& B0_rpz,
                                    const vtkm::FloatDefault& Phi0,
                                    const vtkm::FloatDefault& Phi1,
                                    const XGCParams& Params,
                                    const ScalarPortal& coeff_1D,
                                    const ScalarPortal& coeff_2D,
                                    vtkm::Vec3f& x_ff_rpz)
{
  using Ray3f = vtkm::Ray<vtkm::FloatDefault, 3, true>;

  vtkm::FloatDefault R = pt_rpz[0];
  vtkm::FloatDefault Phi = pt_rpz[1];
  vtkm::FloatDefault Z = pt_rpz[2];

  vtkm::FloatDefault PhiMid = Phi0 + (Phi1 - Phi0) / 2.0;
  vtkm::Plane<> midPlane({ 0, PhiMid, 0 }, { 0, 1, 0 });
  Ray3f ray_rpz({ R, Phi, Z }, B0_rpz);

  //Get point on mid plane.  Use the R,Z for this point for triangle finds.
  vtkm::FloatDefault RP_T;
  vtkm::Vec3f ptOnMidPlane_rpz;
  bool b;
  midPlane.Intersect(ray_rpz, RP_T, ptOnMidPlane_rpz, b);

  //Now, do it using RK4 and two steps.
  vtkm::FloatDefault h = (PhiMid - Phi) / 2.0;
  //h = -h;
  vtkm::FloatDefault h_2 = h / 2.0;

  //k1 = F(p)
  //k2 = F(p+hk1/2)
  //k3 = F(p+hk2/2)
  //k4 = F(p+hk3)
  //Yn+1 = Yn + 1/6 h (k1+2k2+2k3+k4)
  vtkm::Vec3f p0 = { R, Phi, Z }; //pt_rpz;
  vtkm::Vec3f tmp, k1, k2, k3, k4;
  //std::cout<<"     p0 = "<<p0<<std::endl;
  for (int i = 0; i < 2; i++)
  {
    k1 = GetB(p0, Params, coeff_1D, coeff_2D);
    tmp = p0 + k1 * h_2;

    k2 = GetB(tmp, Params, coeff_1D, coeff_2D);
    tmp = p0 + k2 * h_2;

    k3 = GetB(tmp, Params, coeff_1D, coeff_2D);
    tmp = p0 + k3 * h;

    k4 = GetB(tmp, Params, coeff_1D, coeff_2D);

    vtkm::Vec3f vec = (k1 + 2 * k2 + 2 * k3 + k4) / 6.0;
    p0 = p0 + h * vec;
  }

  x_ff_rpz = p0;

  return true;
}

template <typename PortalType>
VTKM_EXEC typename PortalType::ValueType Eval(const PortalType& portal,
                                              const vtkm::Id& offset,
                                              const vtkm::Vec3f& param,
                                              const vtkm::Vec<vtkm::Id, 3>& vId)
{
#if 1
  //Hard code...
  const auto& v0 = portal.Get(vId[0] + offset);
  const auto& v1 = portal.Get(vId[1] + offset);
  const auto& v2 = portal.Get(vId[2] + offset);

  const auto& w1 = param[0];
  const auto& w2 = param[1];
  const auto w0 = 1 - (w1 + w2);
  auto ret = v0 * w0 + v1 * w1 + v2 * w2;
#else
  T ret;
  vtkm::VecVariable<T, 3> vals;
  vals.Append(portal.Get(vId[0] + offset));
  vals.Append(portal.Get(vId[1] + offset));
  vals.Append(portal.Get(vId[2] + offset));
  vtkm::exec::CellInterpolate(vals, param, vtkm::CellShapeTagTriangle(), ret);
#endif
  return ret;
}

VTKM_EXEC inline void GetPlaneIdx(const vtkm::FloatDefault& phi,
                                  const vtkm::FloatDefault& Period,
                                  const XGCParams& Params,
                                  vtkm::FloatDefault& phiN,
                                  vtkm::Id& plane0,
                                  vtkm::Id& plane1,
                                  vtkm::FloatDefault& phi0,
                                  vtkm::FloatDefault& phi1,
                                  vtkm::Id& numRevs,
                                  vtkm::FloatDefault& T)
{
  numRevs = vtkm::Floor(vtkm::Abs(phi / Period));
  phiN = phi;
  if (phi < 0)
  {
    phiN += ((1 + numRevs) * Period);
  }
  else if (phi > Period)
  {
    phiN -= (numRevs * Period);
  }

  plane0 = vtkm::Floor(phiN / Params.dPhi);
  if (plane0 == Params.NumPlanes)
    plane0 = 0;

  plane1 = plane0 + 1;
  phi0 = static_cast<vtkm::FloatDefault>(plane0) * Params.dPhi;
  phi1 = static_cast<vtkm::FloatDefault>(plane1) * Params.dPhi;

  if (plane1 == Params.NumPlanes)
    plane1 = 0;
  T = (phiN - phi0) / (phi1 - phi0);
}

}
}
} //vtkm::worklet::flow

#endif //vtkm_filter_flow_worklet_XGCHelper_h
