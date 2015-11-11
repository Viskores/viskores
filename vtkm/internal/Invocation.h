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
#ifndef vtk_m_internal_Invocation_h
#define vtk_m_internal_Invocation_h

#include <vtkm/Types.h>

namespace vtkm {
namespace internal {

/// \brief Container for types when dispatching worklets.
///
/// When a dispatcher and associated class invoke a worklet, they need to keep
/// track of the types of all parameters and the associated features of the
/// worklet. \c Invocation is a class that manages all these types.
///
template<typename _ParameterInterface,
         typename _ControlInterface,
         typename _ExecutionInterface,
         vtkm::IdComponent _InputDomainIndex,
         typename _OutputToInputMapType = vtkm::internal::NullType,
         typename _VisitArrayType = vtkm::internal::NullType>
struct Invocation
{
  /// \brief The types of the parameters
  ///
  /// \c ParameterInterface is (expected to be) a \c FunctionInterface class
  /// that lists the types of the parameters for the invocation.
  ///
  typedef _ParameterInterface ParameterInterface;

  /// \brief The tags of the \c ControlSignature.
  ///
  /// \c ControlInterface is (expected to be) a \c FunctionInterface class that
  /// represents the \c ControlSignature of a worklet (although dispatchers
  /// might modify the control signature to provide auxiliary information).
  ///
  typedef _ControlInterface ControlInterface;

  /// \brief The tags of the \c ExecutionSignature.
  ///
  /// \c ExecutionInterface is (expected to be) a \c FunctionInterface class that
  /// represents the \c ExecutionSignature of a worklet (although dispatchers
  /// might modify the execution signature to provide auxiliary information).
  ///
  typedef _ExecutionInterface ExecutionInterface;

  /// \brief The index of the input domain.
  ///
  /// When a worklet is invoked, the pool of working threads is based of some
  /// constituent element of the input (such as the points or cells). This
  /// index points to the parameter that defines this input domain.
  ///
  static const vtkm::IdComponent InputDomainIndex = _InputDomainIndex;

  /// \brief An array representing the output to input map.
  ///
  /// When a worklet is invoked, there is an optional scatter operation that
  /// allows you to vary the number of outputs each input affects. This is
  /// represented with a map where each output points to an input that creates
  /// it.
  ///
  typedef _OutputToInputMapType OutputToInputMapType;

  /// \brief An array containing visit indices.
  ///
  /// When a worklet is invoked, there is an optinonal scatter operation that
  /// allows you to vary the number of outputs each input affects. Thus,
  /// multiple outputs may point to the same input. The visit index uniquely
  /// identifies which instance each is.
  ///
  typedef _VisitArrayType VisitArrayType;

  VTKM_CONT_EXPORT
  Invocation(ParameterInterface parameters,
             OutputToInputMapType outputToInputMap = OutputToInputMapType(),
             VisitArrayType visitArray = VisitArrayType())
    : Parameters(parameters),
      OutputToInputMap(outputToInputMap),
      VisitArray(visitArray)
  {  }

  /// Defines a new \c Invocation type that is the same as this type except
  /// with the \c Parameters replaced.
  ///
  template<typename NewParameterInterface>
  struct ChangeParametersType {
    typedef Invocation<NewParameterInterface,
                       ControlInterface,
                       ExecutionInterface,
                       InputDomainIndex,
                       OutputToInputMapType,
                       VisitArrayType> type;
  };

  /// Returns a new \c Invocation that is the same as this one except that the
  /// \c Parameters are replaced with those provided.
  ///
  template<typename NewParameterInterface>
  VTKM_CONT_EXPORT
  typename ChangeParametersType<NewParameterInterface>::type
  ChangeParameters(NewParameterInterface newParameters) const {
    return typename ChangeParametersType<NewParameterInterface>::type(
          newParameters, this->OutputToInputMap, this->VisitArray);
  }

  /// Defines a new \c Invocation type that is the same as this type except
  /// with the \c ControlInterface replaced.
  ///
  template<typename NewControlInterface>
  struct ChangeControlInterfaceType {
    typedef Invocation<ParameterInterface,
                       NewControlInterface,
                       ExecutionInterface,
                       InputDomainIndex> type;
  };

  /// Returns a new \c Invocation that is the same as this one except that the
  /// \c ControlInterface type is changed to the type given.
  ///
  template<typename NewControlInterface>
  typename ChangeControlInterfaceType<NewControlInterface>::type
  ChangeControlInterface(NewControlInterface) const {
    return typename ChangeControlInterfaceType<NewControlInterface>::type(
          this->Parameters, this->OutputToInputMap, this->VisitArray);
  }

  /// Defines a new \c Invocation type that is the same as this type except
  /// with the \c ExecutionInterface replaced.
  ///
  template<typename NewExecutionInterface>
  struct ChangeExecutionInterfaceType {
    typedef Invocation<ParameterInterface,
                       NewExecutionInterface,
                       ExecutionInterface,
                       InputDomainIndex> type;
  };

  /// Returns a new \c Invocation that is the same as this one except that the
  /// \c ExecutionInterface type is changed to the type given.
  ///
  template<typename NewExecutionInterface>
  typename ChangeExecutionInterfaceType<NewExecutionInterface>::type
  ChangeExecutionInterface(NewExecutionInterface) const {
    return typename ChangeExecutionInterfaceType<NewExecutionInterface>::type(
          this->Parameters, this->OutputToInputMap, this->VisitArray);
  }

  /// Defines a new \c Invocation type that is the same as this type except
  /// with the \c InputDomainIndex replaced.
  ///
  template<vtkm::IdComponent NewInputDomainIndex>
  struct ChangeInputDomainIndexType {
    typedef Invocation<ParameterInterface,
                       ControlInterface,
                       ExecutionInterface,
                       NewInputDomainIndex> type;
  };

  /// Returns a new \c Invocation that is the same as this one except that the
  /// \c InputDomainIndex is changed to the static number given.
  ///
  template<vtkm::IdComponent NewInputDomainIndex>
  VTKM_EXEC_CONT_EXPORT
  typename ChangeInputDomainIndexType<NewInputDomainIndex>::type
  ChangeInputDomainIndex() const {
    return typename ChangeInputDomainIndexType<NewInputDomainIndex>::type(
          this->Parameters, this->OutputToInputMap, this->VisitArray);
  }

  /// Defines a new \c Invocation type that is the same as this type except
  /// with the \c OutputToInputMapType replaced.
  ///
  template<typename NewOutputToInputMapType>
  struct ChangeOutputToInputMapType {
    typedef Invocation<ParameterInterface,
                       ControlInterface,
                       ExecutionInterface,
                       InputDomainIndex,
                       NewOutputToInputMapType,
                       VisitArrayType> type;
  };

  /// Returns a new \c Invocation that is the same as this one except that the
  /// \c OutputToInputMap is replaced with that provided.
  ///
  template<typename NewOutputToInputMapType>
  VTKM_CONT_EXPORT
  typename ChangeOutputToInputMapType<NewOutputToInputMapType>::type
  ChangeOutputToInputMap(NewOutputToInputMapType newOutputToInputMap) const {
    return typename ChangeOutputToInputMapType<NewOutputToInputMapType>::type(
          this->Parameters, newOutputToInputMap, this->VisitArray);
  }

  /// Defines a new \c Invocation type that is the same as this type except
  /// with the \c VisitArrayType replaced.
  ///
  template<typename NewVisitArrayType>
  struct ChangeVisitArrayType {
    typedef Invocation<ParameterInterface,
                       ControlInterface,
                       ExecutionInterface,
                       InputDomainIndex,
                       OutputToInputMapType,
                       NewVisitArrayType> type;
  };

  /// Returns a new \c Invocation that is the same as this one except that the
  /// \c VisitArray is replaced with that provided.
  ///
  template<typename NewVisitArrayType>
  VTKM_CONT_EXPORT
  typename ChangeVisitArrayType<NewVisitArrayType>::type
  ChangeVisitArray(NewVisitArrayType newVisitArray) const {
    return typename ChangeVisitArrayType<NewVisitArrayType>::type(
          this->Parameters, this->OutputToInputMap, newVisitArray);
  }

  /// A convenience typedef for the input domain type.
  ///
  typedef typename ParameterInterface::
      template ParameterType<InputDomainIndex>::type InputDomainType;

  /// A convenience method to get the input domain object.
  ///
  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT_EXPORT
  InputDomainType GetInputDomain() const
  {
    return this->Parameters.template GetParameter<InputDomainIndex>();
  }

  /// The state of an \c Invocation object holds the parameters of the
  /// invocation. As well as the output to input map and the visit array.
  ///
  ParameterInterface Parameters;
  OutputToInputMapType OutputToInputMap;
  VisitArrayType VisitArray;
};

/// Convenience function for creating an Invocation object.
///
template<vtkm::IdComponent InputDomainIndex,
         typename ControlInterface,
         typename ExecutionInterface,
         typename ParameterInterface,
         typename OutputToInputMapType,
         typename VisitArrayType>
VTKM_CONT_EXPORT
vtkm::internal::Invocation<ParameterInterface,
                           ControlInterface,
                           ExecutionInterface,
                           InputDomainIndex,
                           OutputToInputMapType,
                           VisitArrayType>
make_Invocation(const ParameterInterface &params,
                ControlInterface,
                ExecutionInterface,
                OutputToInputMapType outputToInputMap,
                VisitArrayType visitArray)
{
  return vtkm::internal::Invocation<ParameterInterface,
                                    ControlInterface,
                                    ExecutionInterface,
                                    InputDomainIndex,
                                    OutputToInputMapType,
                                    VisitArrayType>(params,
                                                    outputToInputMap,
                                                    visitArray);
}
template<vtkm::IdComponent InputDomainIndex,
         typename ControlInterface,
         typename ExecutionInterface,
         typename ParameterInterface>
VTKM_CONT_EXPORT
vtkm::internal::Invocation<ParameterInterface,
                           ControlInterface,
                           ExecutionInterface,
                           InputDomainIndex>
make_Invocation(const ParameterInterface &params,
                ControlInterface = ControlInterface(),
                ExecutionInterface = ExecutionInterface())
{
  return vtkm::internal::make_Invocation<InputDomainIndex>(
        params,
        ControlInterface(),
        ExecutionInterface(),
        vtkm::internal::NullType(),
        vtkm::internal::NullType());
}

}
} // namespace vtkm::internal

#endif //vtk_m_internal_Invocation_h
