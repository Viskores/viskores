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
//  Copyright 2014 Los Alamos National Security.
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

#include <vtkm/TypeListTag.h>

#include <vtkm/exec/FunctorBase.h>
#include <vtkm/exec/arg/BasicArg.h>
#include <vtkm/exec/arg/FetchTagExecObject.h>
#include <vtkm/exec/arg/ThreadIndicesBasic.h>
#include <vtkm/exec/arg/VisitIndex.h>
#include <vtkm/exec/arg/WorkIndex.h>

#include <vtkm/cont/arg/ControlSignatureTagBase.h>
#include <vtkm/cont/arg/TransportTagExecObject.h>
#include <vtkm/cont/arg/TypeCheckTagExecObject.h>

#include <vtkm/worklet/ScatterIdentity.h>

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
  ///
  typedef vtkm::exec::arg::WorkIndex WorkIndex;

  /// \c ExecutionSignature tag for getting the visit index.
  ///
  typedef vtkm::exec::arg::VisitIndex VisitIndex;

  /// \c ControlSignature tag for execution object inputs.
  struct ExecObject : vtkm::cont::arg::ControlSignatureTagBase {
    typedef vtkm::cont::arg::TypeCheckTagExecObject TypeCheckTag;
    typedef vtkm::cont::arg::TransportTagExecObject TransportTag;
    typedef vtkm::exec::arg::FetchTagExecObject FetchTag;
  };

  /// Default input domain is the first argument. Worklet subclasses can
  /// override this by redefining this type.
  typedef _1 InputDomain;

  /// All worklets must define their scatter operation. The scatter defines
  /// what output each input contributes to. The default scatter is the
  /// identity scatter (1-to-1 input to output).
  typedef vtkm::worklet::ScatterIdentity ScatterType;

  /// In addition to defining the scatter type, the worklet must produce the
  /// scatter. The default ScatterIdentity has no state, so just return an
  /// instance.
  VTKM_CONT_EXPORT
  ScatterType GetScatter() const { return ScatterType(); }

  /// \brief A type list containing the type vtkm::Id.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  typedef vtkm::TypeListTagId IdType;

  /// \brief A type list containing the type vtkm::Id2.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  typedef vtkm::TypeListTagId2 Id2Type;

  /// \brief A type list containing the type vtkm::Id3.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  typedef vtkm::TypeListTagId3 Id3Type;

  /// \brief A list of types commonly used for indexing.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  typedef vtkm::TypeListTagIndex Index;

  /// \brief A list of types commonly used for scalar fields.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  typedef vtkm::TypeListTagFieldScalar Scalar;

  /// \brief A list of all basic types used for scalar fields.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  typedef vtkm::TypeListTagScalarAll ScalarAll;

  /// \brief A list of types commonly used for vector fields of 2 components.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  typedef vtkm::TypeListTagFieldVec2 Vec2;

  /// \brief A list of types commonly used for vector fields of 3 components.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  typedef vtkm::TypeListTagFieldVec3 Vec3;

  /// \brief A list of types commonly used for vector fields of 4 components.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  typedef vtkm::TypeListTagFieldVec4 Vec4;

  /// \brief A list of all basic types used for vector fields.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  typedef vtkm::TypeListTagVecAll VecAll;

  /// \brief A list of types (scalar and vector) commonly used in fields.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  typedef vtkm::TypeListTagField FieldCommon;

  /// \brief A list of vector types commonly used in fields.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  typedef vtkm::TypeListTagVecCommon VecCommon;

  /// \brief A list of generally common types.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  typedef vtkm::TypeListTagCommon CommonTypes;

  /// \brief A list of all basic types.
  ///
  /// This is a convenience type to use as template arguments to \c
  /// ControlSignature tags to specify the types of worklet arguments.
  typedef vtkm::TypeListTagAll AllTypes;

  /// \brief Creates a \c ThreadIndices object.
  ///
  /// Worklet types can add additional indices by returning different object
  /// types.
  ///
  template<typename Invocation>
  VTKM_EXEC_EXPORT
  vtkm::exec::arg::ThreadIndicesBasic
  GetThreadIndices(vtkm::Id threadIndex, const Invocation &invocation) const
  {
    return vtkm::exec::arg::ThreadIndicesBasic(threadIndex, invocation);
  }

  /// \brief Creates a \c ThreadIndices object.
  ///
  /// Worklet types can add additional indices by returning different object
  /// types.
  ///
  template<typename T, typename Invocation>
  VTKM_EXEC_EXPORT
  vtkm::exec::arg::ThreadIndicesBasic
  GetThreadIndices(const T& threadIndex, const Invocation &invocation) const
  {
    return vtkm::exec::arg::ThreadIndicesBasic(threadIndex, invocation);
  }
};

}
}
} // namespace vtkm::worklet::internal

#endif //vtk_m_worklet_internal_WorkletBase_h
