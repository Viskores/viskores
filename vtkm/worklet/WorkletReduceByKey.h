//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 Sandia Corporation.
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_worklet_WorkletReduceByKey_h
#define vtk_m_worklet_WorkletReduceByKey_h

#include <vtkm/worklet/internal/WorkletBase.h>

#include <vtkm/cont/arg/TransportTagKeysIn.h>
#include <vtkm/cont/arg/TypeCheckTagKeys.h>

#include <vtkm/exec/internal/ReduceByKeyLookup.h>

#include <vtkm/exec/arg/FetchTagKeysIn.h>
#include <vtkm/exec/arg/ThreadIndicesReduceByKey.h>

namespace vtkm {
namespace worklet {

class WorkletReduceByKey : public vtkm::worklet::internal::WorkletBase
{
public:
  /// \brief A control signature tag for input keys.
  ///
  /// A \c WorkletReduceByKey operates by collected all identical keys and
  /// then executing the worklet on each unique key. This tag specifies a
  /// \c Keys object that defines and manages these keys.
  ///
  /// A \c WorkletReduceByKey should have exactly one \c KeysIn tag in its \c
  /// ControlSignature, and the \c InputDomain should point to it.
  ///
  struct KeysIn : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagKeys;
    using TransportTag = vtkm::cont::arg::TransportTagKeysIn;
    using FetchTag = vtkm::exec::arg::FetchTagKeysIn;
  };

  /// Reduce by key worklets use the related thread indices class.
  ///
  VTKM_SUPPRESS_EXEC_WARNINGS
  template<typename T,
           typename OutToInArrayType,
           typename VisitArrayType,
           typename InputDomainType>
  VTKM_EXEC
  vtkm::exec::arg::ThreadIndicesReduceByKey
  GetThreadIndices(const T& threadIndex,
                   const OutToInArrayType& outToIn,
                   const VisitArrayType& visit,
                   const InputDomainType &inputDomain,
                   const T& globalThreadIndexOffset=0) const
  {
    return vtkm::exec::arg::ThreadIndicesReduceByKey(
          threadIndex,
          outToIn.Get(threadIndex),
          visit.Get(threadIndex),
          inputDomain,
          globalThreadIndexOffset);
  }
};

}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_WorkletReduceByKey_h
