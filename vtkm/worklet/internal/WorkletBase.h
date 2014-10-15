//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_worklet_internal_WorkletBase_h
#define vtk_m_worklet_internal_WorkletBase_h

#include <vtkm/exec/FunctorBase.h>
#include <vtkm/exec/arg/BasicArg.h>
#include <vtkm/exec/arg/FetchTagExecObject.h>
#include <vtkm/exec/arg/WorkIndex.h>

#include <vtkm/cont/arg/TransportTagExecObject.h>

namespace vtkm {
namespace worklet {
namespace internal {

/// Base class for all worklet classes. Worklet classes are subclasses and a
/// operator() const is added to implement an algorithm in VTK-m. Different
/// worklets have different calling semantics.
///
class WorkletBase : public vtkm::exec::FunctorBase
{
public:
  template<int ControlSignatureIndex>
  struct Arg : vtkm::exec::arg::BasicArg<ControlSignatureIndex> {  };

  /// Basic execution argument tags
  struct _1 : Arg<1> {  };
  struct _2 : Arg<2> {  };
  struct _3 : Arg<3> {  };
  struct _4 : Arg<4> {  };
  struct _5 : Arg<5> {  };
  struct _6 : Arg<6> {  };
  struct _7 : Arg<7> {  };
  struct _8 : Arg<8> {  };
  struct _9 : Arg<9> {  };

  /// \c ExecutionSignature tag for getting the work index.
  typedef vtkm::exec::arg::WorkIndex WorkIndex;

  /// \c ControlSignature tag for execution object inputs.
  struct ExecObject {
    typedef vtkm::cont::arg::TransportTagExecObject TransportTag;
    typedef vtkm::exec::arg::FetchTagExecObject FetchTag;
  };

  /// Default input domain is the first argument. Worklet subclasses can
  /// override this by redefining this type.
  typedef _1 InputDomain;
};

}
}
} // namespace vtkm::worklet::internal

#endif //vtk_m_worklet_internal_WorkletBase_h
