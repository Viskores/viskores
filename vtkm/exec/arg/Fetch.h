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
#ifndef vtk_m_exec_arg_Fetch_h
#define vtk_m_exec_arg_Fetch_h

#include <vtkm/Types.h>

namespace vtkm {
namespace exec {
namespace arg {

/// \brief Class for loading and storing values in thread instance.
///
/// The \c Fetch class is used within a thread in the execution environment
/// to load a value from an execution object specific for the given thread
/// instance and to store a resulting value back in the object. (Either load
/// or store can be a no-op.)
///
/// \c Fetch is a templated class with four arguments. The first argument is
/// a tag declaring the type of fetch, which is usually tied to a particular
/// type of execution object. The second argument is an aspect tag that
/// declares what type of data to pull/push. Together, these two tags determine
/// the mechanism for the fetch. The third argument as an Invocation object
/// containing the details of the call. The fourth argument identifies the
/// parameter of the input arguments that is the source of the data.
///
/// There is no generic implementaiton of \c Fetch. There are partial
/// specializations of \c Fetch for each mechanism (fetch-aspect tag
/// combination) supported. If you get a compiler error about an incomplete
/// type for \c Fetch, it means you used an invalid \c FetchTag - \c AspectTag
/// combination. Most likely this means that a parameter in an
/// ExecutionSignature with a particular aspect is pointing to the wrong
/// argument or an invalid argument in the ControlSignature.
///
template<typename FetchTag,
         typename AspectTag,
         typename Invocation,
         vtkm::IdComponent ParameterIndex>
struct Fetch
#ifdef VTKM_DOXYGEN_ONLY
{
  /// \brief The type of the execution object to load and store data (optional).
  ///
  /// This is the type of the parameter of the \c Invocation pointed to by
  /// \c ParameterIndex. Declaring this is optional, but often helpful.
  ///
  typedef typename Invocation::ParameterInterface::
      template ParameterType<ParameterIndex>::type ExecObjectType;

  /// \brief The type of value to load and store.
  ///
  /// All \c Fetch specializations are expected to declare a type named \c
  /// ValueType that is the type of object returned from \c Load and passed to
  /// \c Store.
  ///
  typedef typename ExecObjectType::ValueType ValueType;

  /// \brief Load data for a work instance.
  ///
  /// All \c Fetch specializations are expected to have a constant method named
  /// \c Load that takes a work instance index and an \c Invocation object
  /// (preferably as a constant reference) and returns the value appropriate
  /// for the work instance. If there is no actual data to load (for example
  /// for a write-only fetch), this method can be a no-op and return any value.
  ///
  VTKM_EXEC_EXPORT
  ValueType Load(vtkm::Id index, const Invocation &invocation) const;

  /// \brief Store data from a work instance.
  ///
  /// All \c Fetch specializations are expected to have a constant method named
  /// \c Store that takes a work instance index, a an \c Invocation object
  /// (preferably as a constant reference), and a value computed by the given
  /// work instance and stores that value into the execution object associated
  /// with this fetch. If the store is not applicable (for example for a
  /// read-only fetch), this method can be a no-op.
  ///
  VTKM_EXEC_EXPORT
  void Store(vtkm::Id index,
             const Invocation &invocation,
             const ValueType &value) const;
};
#else // VTKM_DOXYGEN_ONLY
    ;
#endif // VTKM_DOXYGEN_ONLY

}
}
} // namespace vtkm::exec::arg

#endif //vtk_m_exec_arg_Fetch_h
