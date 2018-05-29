//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_worklet_gradient_StructuredPointGradient_h
#define vtk_m_worklet_gradient_StructuredPointGradient_h

#include <vtkm/worklet/WorkletPointNeighborhood.h>
#include <vtkm/worklet/gradient/GradientOutput.h>


namespace vtkm
{
namespace worklet
{
namespace gradient
{

template <typename T>
struct StructuredPointGradientInType : vtkm::ListTagBase<T>
{
};

template <typename T>
struct StructuredPointGradient : public vtkm::worklet::WorkletPointNeighborhood3x3x3
{

  using ControlSignature = void(CellSetIn,
                                FieldInNeighborhood<Vec3> points,
                                FieldInNeighborhood<StructuredPointGradientInType<T>>,
                                GradientOutputs outputFields);

  using ExecutionSignature = void(OnBoundary, _2, _3, _4);

  using InputDomain = _1;

  template <typename PointsIn, typename FieldIn, typename GradientOutType>
  VTKM_EXEC void operator()(const vtkm::exec::arg::BoundaryState& boundary,
                            const PointsIn& inputPoints,
                            const FieldIn& inputField,
                            GradientOutType& outputGradient) const
  {
    using CoordType = typename PointsIn::ValueType;
    using CT = typename vtkm::BaseComponent<CoordType>::Type;
    using OT = typename GradientOutType::ComponentType;

    vtkm::Vec<CT, 3> xi, eta, zeta;
    this->Jacobian(inputPoints, boundary, xi, eta, zeta); //store the metrics in xi,eta,zeta

    T dxi = inputField.Get(1, 0, 0) - inputField.Get(-1, 0, 0);
    T deta = inputField.Get(0, 1, 0) - inputField.Get(0, -1, 0);
    T dzeta = inputField.Get(0, 0, 1) - inputField.Get(0, 0, -1);

    dxi = (boundary.OnX() ? dxi : dxi * 0.5f);
    deta = (boundary.OnY() ? deta : deta * 0.5f);
    dzeta = (boundary.OnZ() ? dzeta : dzeta * 0.5f);

    outputGradient[0] = static_cast<OT>(xi[0] * dxi + eta[0] * deta + zeta[0] * dzeta);
    outputGradient[1] = static_cast<OT>(xi[1] * dxi + eta[1] * deta + zeta[1] * dzeta);
    outputGradient[2] = static_cast<OT>(xi[2] * dxi + eta[2] * deta + zeta[2] * dzeta);
  }

  template <typename FieldIn, typename GradientOutType>
  VTKM_EXEC void operator()(
    const vtkm::exec::arg::BoundaryState& boundary,
    const vtkm::exec::arg::Neighborhood<1, vtkm::internal::ArrayPortalUniformPointCoordinates>&
      inputPoints,
    const FieldIn& inputField,
    GradientOutType& outputGradient) const
  {
    //When the points and cells are both structured we can achieve even better
    //performance by not doing the Jacobian, but instead do an image gradient
    //using central differences
    using PointsIn =
      vtkm::exec::arg::Neighborhood<1, vtkm::internal::ArrayPortalUniformPointCoordinates>;
    using CoordType = typename PointsIn::ValueType;
    using OT = typename GradientOutType::ComponentType;


    CoordType r = inputPoints.Portal.GetSpacing();

    r[0] = (boundary.OnX() ? r[0] : r[0] * 0.5f);
    r[1] = (boundary.OnY() ? r[1] : r[1] * 0.5f);
    r[2] = (boundary.OnZ() ? r[2] : r[2] * 0.5f);

    const T dx = inputField.Get(1, 0, 0) - inputField.Get(-1, 0, 0);
    const T dy = inputField.Get(0, 1, 0) - inputField.Get(0, -1, 0);
    const T dz = inputField.Get(0, 0, 1) - inputField.Get(0, 0, -1);

    outputGradient[0] = static_cast<OT>(dx * r[0]);
    outputGradient[1] = static_cast<OT>(dy * r[1]);
    outputGradient[2] = static_cast<OT>(dz * r[2]);
  }

  //we need to pass the coordinates into this function, and instead
  //of the input being Vec<coordtype,3> it needs to be Vec<float,3> as the metrics
  //will be float,3 even when T is a 3 component field
  template <typename PointsIn, typename CT>
  VTKM_EXEC void Jacobian(const PointsIn& inputPoints,
                          const vtkm::exec::arg::BoundaryState& boundary,
                          vtkm::Vec<CT, 3>& m_xi,
                          vtkm::Vec<CT, 3>& m_eta,
                          vtkm::Vec<CT, 3>& m_zeta) const
  {
    using CoordType = typename PointsIn::ValueType;

    CoordType xi = inputPoints.Get(1, 0, 0) - inputPoints.Get(-1, 0, 0);
    CoordType eta = inputPoints.Get(0, 1, 0) - inputPoints.Get(0, -1, 0);
    CoordType zeta = inputPoints.Get(0, 0, 1) - inputPoints.Get(0, 0, -1);

    xi = (boundary.OnX() ? xi : xi * 0.5f);
    eta = (boundary.OnY() ? eta : eta * 0.5f);
    zeta = (boundary.OnZ() ? zeta : zeta * 0.5f);

    CT aj = xi[0] * eta[1] * zeta[2] + xi[1] * eta[2] * zeta[0] + xi[2] * eta[0] * zeta[1] -
      xi[2] * eta[1] * zeta[0] - xi[1] * eta[0] * zeta[2] - xi[0] * eta[2] * zeta[1];

    aj = (aj != 0.0) ? 1.f / aj : aj;

    //  Xi metrics.
    m_xi[0] = aj * (eta[1] * zeta[2] - eta[2] * zeta[1]);
    m_xi[1] = -aj * (eta[0] * zeta[2] - eta[2] * zeta[0]);
    m_xi[2] = aj * (eta[0] * zeta[1] - eta[1] * zeta[0]);

    //  Eta metrics.
    m_eta[0] = -aj * (xi[1] * zeta[2] - xi[2] * zeta[1]);
    m_eta[1] = aj * (xi[0] * zeta[2] - xi[2] * zeta[0]);
    m_eta[2] = -aj * (xi[0] * zeta[1] - xi[1] * zeta[0]);

    //  Zeta metrics.
    m_zeta[0] = aj * (xi[1] * eta[2] - xi[2] * eta[1]);
    m_zeta[1] = -aj * (xi[0] * eta[2] - xi[2] * eta[0]);
    m_zeta[2] = aj * (xi[0] * eta[1] - xi[1] * eta[0]);
  }
};
}
}
}

#endif
