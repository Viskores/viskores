//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtkm_filter_flow_worklet_XGCField_h
#define vtkm_filter_flow_worklet_XGCField_h

#include <vtkm/Geometry.h>
#include <vtkm/Matrix.h>
#include <vtkm/Types.h>
#include <vtkm/VectorAnalysis.h>


#include <vtkm/VecVariable.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ExecutionObjectBase.h>
#include <vtkm/exec/CellInterpolate.h>

#include <vtkm/filter/flow/worklet/XGCHelper.h>

namespace vtkm
{
namespace worklet
{
namespace flow
{

template <typename FieldComponentType, typename FieldVecType>
class ExecutionXGCField
{
public:
  using ComponentPortalType = typename FieldComponentType::ReadPortalType;
  using VecPortalType = typename FieldVecType::ReadPortalType;
  using Association = vtkm::cont::Field::Association;
  using DelegateToField = std::true_type;
  using Ray3f = vtkm::Ray<vtkm::FloatDefault, 3, true>;

  VTKM_CONT
  ExecutionXGCField(const FieldComponentType& as_ff,
                    const FieldVecType& _dAs_ff_rzp,
                    const FieldComponentType& coeff_1D,
                    const FieldComponentType& coeff_2D,
                    const FieldVecType& _B_rzp,
                    const FieldComponentType& psi,
                    const XGCParams& params,
                    vtkm::FloatDefault period,
                    bool useDeltaBScale,
                    vtkm::FloatDefault deltaBScale,
                    bool useBScale,
                    vtkm::FloatDefault bScale,
                    const Association assoc,
                    vtkm::cont::DeviceAdapterId device,
                    vtkm::cont::Token& token)
    : As_ff(as_ff.PrepareForInput(device, token))
    , dAs_ff_rzp(_dAs_ff_rzp.PrepareForInput(device, token))
    , Coeff_1D(coeff_1D.PrepareForInput(device, token))
    , Coeff_2D(coeff_2D.PrepareForInput(device, token))
    , Psi(psi.PrepareForInput(device, token))
    , B_rzp(_B_rzp.PrepareForInput(device, token))
    , Params(params)
    , Period(period)
    , UseDeltaBScale(useDeltaBScale)
    , DeltaBScale(deltaBScale)
    , UseBScale(useBScale)
    , BScale(bScale)
    , Assoc(assoc)
  {
  }

private:
  template <typename LocatorType, typename InterpolationHelper>
  VTKM_EXEC bool PtLoc(const vtkm::Vec3f& ptRZ,
                       const LocatorType& locator,
                       const InterpolationHelper& helper,
                       vtkm::Vec3f& param,
                       vtkm::Vec<vtkm::Id, 3>& vIds) const
  {
    vtkm::Id cellId;
    vtkm::ErrorCode status;
    status = locator.FindCell(ptRZ, cellId, param);
    if (status != vtkm::ErrorCode::Success)
    {
      return false;
    }
    vtkm::UInt8 cellShape;
    vtkm::IdComponent nVerts;
    vtkm::VecVariable<vtkm::Id, 8> ptIndices;
    helper.GetCellInfo(cellId, cellShape, nVerts, ptIndices);

    vIds[0] = ptIndices[0];
    vIds[1] = ptIndices[1];
    vIds[2] = ptIndices[2];

    return true;
  }

public:
  VTKM_EXEC Association GetAssociation() const { return this->Assoc; }

  VTKM_EXEC void GetValue(const vtkm::Id vtkmNotUsed(cellId),
                          vtkm::VecVariable<vtkm::Vec3f, 2>& vtkmNotUsed(value)) const
  {
    //TODO Raise Error : XGC Field should not allow this path
  }

  VTKM_EXEC void GetValue(const vtkm::VecVariable<vtkm::Id, 8>& vtkmNotUsed(indices),
                          const vtkm::Id vtkmNotUsed(vertices),
                          const vtkm::Vec3f& vtkmNotUsed(parametric),
                          const vtkm::UInt8 vtkmNotUsed(cellShape),
                          vtkm::VecVariable<vtkm::Vec3f, 2>& vtkmNotUsed(value)) const
  {
    //TODO Raise Error : XGC Field should not allow this path
  }

  template <typename Point, typename Locator, typename Helper>
  VTKM_EXEC bool GetValue(const Point& point,
                          const vtkm::FloatDefault& time,
                          vtkm::VecVariable<Point, 2>& out,
                          const Locator& locator,
                          const Helper& helper) const
  {
    // not used
    (void)time;
    auto R = point[0];
    auto Phi = point[1];
    auto Z = point[2];

    ParticleMetaData metadata;

    if (!HighOrderB(point, metadata, this->Params, this->Coeff_1D, this->Coeff_2D))
      return false;

    vtkm::Id planeIdx0, planeIdx1, numRevs;
    vtkm::FloatDefault phiN, Phi0, Phi1, T;
    GetPlaneIdx(
      Phi, this->Period, this->Params, phiN, planeIdx0, planeIdx1, Phi0, Phi1, numRevs, T);

    vtkm::Vec3f B0_rpz(metadata.B0_rzp[0], metadata.B0_rzp[2], metadata.B0_rzp[1]);

    vtkm::Vec3f ff_pt_rpz;
    CalcFieldFollowingPt(
      { R, phiN, Z }, B0_rpz, Phi0, Phi1, this->Params, this->Coeff_1D, this->Coeff_2D, ff_pt_rpz);

    //Now, interpolate between Phi_i and Phi_i+1
    vtkm::FloatDefault T01 = (phiN - Phi0) / (Phi1 - Phi0);
    vtkm::FloatDefault T10 = 1.0f - T01;

    //Get vec at Phi0 and Phi1.
    //x_ff is in rzp
    //vtkm::Vec3f x_ff_rzp(ptOnMidPlane_rpz[0], ptOnMidPlane_rpz[2], 0);
    vtkm::Vec3f x_ff_rzp(ff_pt_rpz[0], ff_pt_rpz[2], 0);

    // Offsets into As and DAs to jump to the right plane.
    int offsets[2];
    offsets[0] = planeIdx0 * this->Params.NumNodes * 2;
    offsets[1] = planeIdx0 * this->Params.NumNodes * 2 + this->Params.NumNodes;

    const vtkm::FloatDefault basis = 0.0f;
    //auto B0_R = B0_rzp[0];
    //auto B0_Z = B0_rzp[1];
    //auto x_ff_R = x_ff_rzp[0];
    //auto x_ff_Z = x_ff_rzp[1];

    //gradPsi: pt on mid plane?  (question)
    //dPsi/dR = B0_Z * R
    //dPsi/dZ = -B0_R * R;
    //vtkm::Vec3f gradPsi_rzp(B0_Z * x_ff_R, -B0_R * x_ff_R, 0);
    //use high order...
    vtkm::Vec3f gradPsi_rzp = metadata.gradPsi_rzp;
    vtkm::FloatDefault gammaPsi = 1.0f / vtkm::Magnitude(gradPsi_rzp);

    vtkm::Vec2f rvec(0, 0), zvec(0, 0);
    rvec[0] = basis + (1.0 - basis) * gammaPsi * gradPsi_rzp[0];
    rvec[1] = (1.0 - basis) * gammaPsi * gradPsi_rzp[1];
    zvec[0] = (1.0 - basis) * gammaPsi * (-gradPsi_rzp[1]);
    zvec[1] = basis + (1.0 - basis) * gammaPsi * gradPsi_rzp[0];

    //Get the vectors in the ff coordinates.
    //auto dAs_ff_rzp = EvalVector(ds, locator, {x_ff_rzp, x_ff_rzp}, "dAs_ff_rzp", offsets);
    //auto dAs_ff0_rzp = dAs_ff_rzp[0];
    //auto dAs_ff1_rzp = dAs_ff_rzp[1];

    vtkm::Vec3f x_ff_param;
    vtkm::Vec<vtkm::Id, 3> x_ff_vids;

    //Hotspot..
    if (!this->PtLoc(x_ff_rzp, locator, helper, x_ff_param, x_ff_vids))
      return false;

    // Eval actual values for DAsPhiFF from the triangular planes
    auto dAs_ff0_rzp = Eval(this->dAs_ff_rzp, offsets[0], x_ff_param, x_ff_vids);
    auto dAs_ff1_rzp = Eval(this->dAs_ff_rzp, offsets[1], x_ff_param, x_ff_vids);

    vtkm::FloatDefault wphi[2] = { T10, T01 }; //{T01, T10};
    vtkm::Vec3f gradAs_rpz;

    //vec.r = wphi[0]*( rvec[0]*V.r[0] + zvec[0]*V.z[0]) +
    //        wphi[1]*( rvec[0]*V.r[1] + zvec[0]*v.z[1]);
    //vec.p = wphi[0]*V.phi[0] +
    //        whpi[1]*V.phi[1];
    //vec.z = wphi[0]*( rvec[1]*V.r[0] + zvec[1]*V.z[0]) +
    //        wphi[1]*( rvec[1]*V.r[1] + zvec[1]*V.Z[1]);
    gradAs_rpz[0] = wphi[0] * (rvec[0] * dAs_ff0_rzp[0] + zvec[0] * dAs_ff0_rzp[1]) +
      wphi[1] * (rvec[0] * dAs_ff1_rzp[0] + zvec[0] * dAs_ff1_rzp[1]);
    gradAs_rpz[1] = wphi[0] * dAs_ff0_rzp[2] + wphi[1] * dAs_ff1_rzp[2];
    gradAs_rpz[2] = wphi[0] * (rvec[1] * dAs_ff0_rzp[0] + zvec[1] * dAs_ff0_rzp[1]) +
      wphi[1] * (rvec[1] * dAs_ff1_rzp[0] + zvec[1] * dAs_ff1_rzp[1]);

    vtkm::FloatDefault BMag = vtkm::Magnitude(metadata.B0_rzp);
    //project using bfield.
    //gradAs.Phi = (gradAs.Phi * BMag - gradAs.R*B0_pos.R - gradAs.Z*B0_pos.Z) / B0_pos.Phi
    gradAs_rpz[1] = (gradAs_rpz[1] * BMag - gradAs_rpz[0] * metadata.B0_rzp[0] -
                     gradAs_rpz[2] * metadata.B0_rzp[1]) /
      metadata.B0_rzp[2];

    //deltaB = AsCurl(bhat) + gradAs x bhat.
    //std::vector<int> off = {planeIdx0*this->NumNodes};
    //vtkm::Vec3f AsCurl_bhat_rzp = EvalVector(ds, locator, {x_ff_rzp}, "AsCurlBHat_RZP", off)[0];
    //auto AsCurl_bhat_rzp = this->EvalV(AsCurlBHat_RZP, 0, x_ff_vids, x_ff_param);
    //
    auto As_ff0 = Eval(this->As_ff, offsets[0], x_ff_param, x_ff_vids);
    auto As_ff1 = Eval(this->As_ff, offsets[1], x_ff_param, x_ff_vids);

    // Interpolated value of As on plane 10 & 1.
    // wPhi is distance of the plane from the point.
    vtkm::FloatDefault As = wphi[0] * As_ff0 + wphi[1] * As_ff1;
    auto AsCurl_bhat_rzp = As * metadata.curl_nb_rzp;

    auto bhat_rzp = vtkm::Normal(metadata.B0_rzp);

    vtkm::Vec3f gradAs_rzp(gradAs_rpz[0], gradAs_rpz[2], gradAs_rpz[1]);
    vtkm::Vec3f deltaB_rzp = AsCurl_bhat_rzp + vtkm::Cross(gradAs_rzp, bhat_rzp);

    if (this->UseDeltaBScale)
      deltaB_rzp = deltaB_rzp * this->DeltaBScale;

    vtkm::Vec3f B0_rzp = metadata.B0_rzp;
    if (this->UseBScale)
      B0_rzp = B0_rzp * this->BScale;

    deltaB_rzp[2] /= R;
    B0_rzp[2] /= R;

    vtkm::Vec3f vec_rzp = B0_rzp + deltaB_rzp;
    vtkm::Vec3f vec_rpz(vec_rzp[0], vec_rzp[2], vec_rzp[1]);
    out = vtkm::make_Vec(vec_rpz);
    return true;
  }

private:
  ComponentPortalType As_ff;
  VecPortalType dAs_ff_rzp;

  ComponentPortalType Coeff_1D;
  ComponentPortalType Coeff_2D;

  ComponentPortalType Psi;

  VecPortalType B_rzp;

  vtkm::FloatDefault Period;
  bool UseDeltaBScale;
  vtkm::FloatDefault DeltaBScale = 1.0;
  bool UseBScale;
  vtkm::FloatDefault BScale = 1.0;

  Association Assoc;
  XGCParams Params;
};

template <typename FieldComponentType, typename FieldVecType>
class XGCField : public vtkm::cont::ExecutionObjectBase
{
public:
  using ExecutionType = ExecutionXGCField<FieldComponentType, FieldVecType>;
  using Association = vtkm::cont::Field::Association;

  VTKM_CONT
  XGCField() = default;

  VTKM_CONT
  XGCField(const FieldComponentType& as_ff,
           const FieldVecType& _dAs_ff_rzp,
           const FieldComponentType& coeff_1D,
           const FieldComponentType& coeff_2D,
           const FieldVecType& _B_rzp,
           const FieldComponentType& psi,
           const XGCParams& params,
           vtkm::FloatDefault period,
           bool useDeltaBScale,
           vtkm::FloatDefault deltaBScale,
           bool useBScale,
           vtkm::FloatDefault bScale)
    : As_ff(as_ff)
    , dAs_ff_rzp(_dAs_ff_rzp)
    , Coeff_1D(coeff_1D)
    , Coeff_2D(coeff_2D)
    , Psi(psi)
    , B_rzp(_B_rzp)
    , Params(params)
    , Period(period)
    , UseDeltaBScale(useDeltaBScale)
    , DeltaBScale(deltaBScale)
    , UseBScale(useBScale)
    , BScale(bScale)
    , Assoc(vtkm::cont::Field::Association::Points)
  {
  }

  VTKM_CONT
  XGCField(const FieldComponentType& as_ff,
           const FieldVecType& _dAs_ff_rzp,
           const FieldComponentType& coeff_1D,
           const FieldComponentType& coeff_2D,
           const FieldVecType& _B_rzp,
           const FieldComponentType& psi,
           const XGCParams& params,
           vtkm::FloatDefault period,
           bool useDeltaBScale,
           vtkm::FloatDefault deltaBScale,
           bool useBScale,
           vtkm::FloatDefault bScale,
           const Association assoc)
    : As_ff(as_ff)
    , dAs_ff_rzp(_dAs_ff_rzp)
    , Coeff_1D(coeff_1D)
    , Coeff_2D(coeff_2D)
    , Psi(psi)
    , B_rzp(_B_rzp)
    , Params(params)
    , Period(period)
    , UseDeltaBScale(useDeltaBScale)
    , DeltaBScale(deltaBScale)
    , UseBScale(useBScale)
    , BScale(bScale)
    , Assoc(assoc)
  {
    if (assoc != Association::Points && assoc != Association::Cells)
      throw("Unsupported field association");
  }

  VTKM_CONT
  const ExecutionType PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                          vtkm::cont::Token& token) const
  {
    return ExecutionType(this->As_ff,
                         this->dAs_ff_rzp,
                         this->Coeff_1D,
                         this->Coeff_2D,
                         this->B_rzp,
                         this->Psi,
                         this->Params,
                         this->Period,
                         this->UseDeltaBScale,
                         this->DeltaBScale,
                         this->UseBScale,
                         this->BScale,
                         this->Assoc,
                         device,
                         token);
  }

private:
  // For turbulance in the magnetic field (B is constant, but this emulates deltaB)
  FieldComponentType As_ff;
  FieldVecType dAs_ff_rzp;

  // For Higher order interpolation
  // on RZ uniform grid, not the triangle grid.
  FieldComponentType Coeff_1D;
  FieldComponentType Coeff_2D;

  // Constant magnetic field on the triangle mesh
  FieldComponentType Psi;

  // B field at each vertex of the triangular field
  FieldVecType B_rzp;

  XGCParams Params;

  vtkm::FloatDefault Period;
  bool UseDeltaBScale;
  vtkm::FloatDefault DeltaBScale = 1.0;
  bool UseBScale;
  vtkm::FloatDefault BScale = 1.0;

  Association Assoc;
};

}
}
} //vtkm::worklet::flow

#endif //vtkm_filter_flow_worklet_XGCField_h
