//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/FieldMetadata.h>
#include <vtkm/filter/PolicyDefault.h>

#include <vtkm/filter/internal/ResolveFieldTypeAndExecute.h>
#include <vtkm/filter/internal/ResolveFieldTypeAndMap.h>

#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/Logging.h>

namespace vtkm
{
namespace filter
{

namespace internal
{

template <typename T, typename InputType, typename DerivedPolicy>
struct SupportsPreExecute
{
  template <typename U,
            typename S = decltype(std::declval<U>().PreExecute(
              std::declval<InputType>(),
              std::declval<vtkm::filter::PolicyBase<DerivedPolicy>>()))>
  static std::true_type has(int);
  template <typename U>
  static std::false_type has(...);
  using type = decltype(has<T>(0));
};

template <typename T, typename InputType, typename DerivedPolicy>
struct SupportsPostExecute
{
  template <typename U,
            typename S = decltype(std::declval<U>().PostExecute(
              std::declval<InputType>(),
              std::declval<InputType&>(),
              std::declval<vtkm::filter::PolicyBase<DerivedPolicy>>()))>
  static std::true_type has(int);
  template <typename U>
  static std::false_type has(...);
  using type = decltype(has<T>(0));
};


template <typename T, typename InputType, typename DerivedPolicy>
struct SupportsPrepareForExecution
{
  template <typename U,
            typename S = decltype(std::declval<U>().PrepareForExecution(
              std::declval<InputType>(),
              std::declval<vtkm::filter::PolicyBase<DerivedPolicy>>()))>
  static std::true_type has(int);
  template <typename U>
  static std::false_type has(...);
  using type = decltype(has<T>(0));
};

template <typename T, typename DerivedPolicy>
struct SupportsMapFieldOntoOutput
{
  template <typename U,
            typename S = decltype(std::declval<U>().MapFieldOntoOutput(
              std::declval<vtkm::cont::DataSet&>(),
              std::declval<vtkm::cont::Field>(),
              std::declval<vtkm::filter::PolicyBase<DerivedPolicy>>()))>
  static std::true_type has(int);
  template <typename U>
  static std::false_type has(...);
  using type = decltype(has<T>(0));
};

//--------------------------------------------------------------------------------
template <typename Derived, typename... Args>
void CallPreExecuteInternal(std::true_type, Derived* self, Args&&... args)
{
  return self->PreExecute(std::forward<Args>(args)...);
}

//--------------------------------------------------------------------------------
template <typename Derived, typename... Args>
void CallPreExecuteInternal(std::false_type, Derived*, Args&&...)
{
}

//--------------------------------------------------------------------------------
template <typename Derived, typename InputType, typename DerivedPolicy>
void CallPreExecute(Derived* self,
                    const InputType& input,
                    const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  using call_supported_t = typename SupportsPreExecute<Derived, InputType, DerivedPolicy>::type;
  CallPreExecuteInternal(call_supported_t(), self, input, policy);
}

//--------------------------------------------------------------------------------
template <typename Derived, typename DerivedPolicy>
void CallMapFieldOntoOutputInternal(std::true_type,
                                    Derived* self,
                                    const vtkm::cont::DataSet& input,
                                    vtkm::cont::DataSet& output,
                                    const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  for (vtkm::IdComponent cc = 0; cc < input.GetNumberOfFields(); ++cc)
  {
    auto field = input.GetField(cc);
    if (self->GetFieldsToPass().IsFieldSelected(field))
    {
      self->MapFieldOntoOutput(output, field, policy);
    }
  }
}

//--------------------------------------------------------------------------------
template <typename Derived, typename DerivedPolicy>
void CallMapFieldOntoOutputInternal(std::false_type,
                                    Derived* self,
                                    const vtkm::cont::DataSet& input,
                                    vtkm::cont::DataSet& output,
                                    const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  // no MapFieldOntoOutput method is present. In that case, we simply copy the
  // requested input fields to the output.
  for (vtkm::IdComponent cc = 0; cc < input.GetNumberOfFields(); ++cc)
  {
    auto field = input.GetField(cc);
    if (self->GetFieldsToPass().IsFieldSelected(field))
    {
      output.AddField(field);
    }
  }
}

//--------------------------------------------------------------------------------
template <typename Derived, typename DerivedPolicy>
void CallMapFieldOntoOutput(Derived* self,
                            const vtkm::cont::DataSet& input,
                            vtkm::cont::DataSet& output,
                            vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  using call_supported_t = typename SupportsMapFieldOntoOutput<Derived, DerivedPolicy>::type;
  CallMapFieldOntoOutputInternal(call_supported_t(), self, input, output, policy);
}

//--------------------------------------------------------------------------------
// forward declare.
template <typename Derived, typename InputType, typename DerivedPolicy>
InputType CallPrepareForExecution(Derived* self,
                                  const InputType& input,
                                  const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

//--------------------------------------------------------------------------------
template <typename Derived, typename InputType, typename DerivedPolicy>
InputType CallPrepareForExecutionInternal(std::true_type,
                                          Derived* self,
                                          const InputType& input,
                                          const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  return self->PrepareForExecution(input, policy);
}

//--------------------------------------------------------------------------------
// specialization for PartitionedDataSet input when `PrepareForExecution` is not provided
// by the subclass. we iterate over blocks and execute for each block
// individually.
template <typename Derived, typename DerivedPolicy>
vtkm::cont::PartitionedDataSet CallPrepareForExecutionInternal(
  std::false_type,
  Derived* self,
  const vtkm::cont::PartitionedDataSet& input,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  vtkm::cont::PartitionedDataSet output;
  for (const auto& inBlock : input)
  {
    vtkm::cont::DataSet outBlock = CallPrepareForExecution(self, inBlock, policy);
    CallMapFieldOntoOutput(self, inBlock, outBlock, policy);
    output.AppendPartition(outBlock);
  }
  return output;
}

//--------------------------------------------------------------------------------
template <typename Derived, typename InputType, typename DerivedPolicy>
InputType CallPrepareForExecution(Derived* self,
                                  const InputType& input,
                                  const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  using call_supported_t =
    typename SupportsPrepareForExecution<Derived, InputType, DerivedPolicy>::type;
  return CallPrepareForExecutionInternal(call_supported_t(), self, input, policy);
}

//--------------------------------------------------------------------------------
template <typename Derived, typename InputType, typename DerivedPolicy>
void CallPostExecuteInternal(std::true_type,
                             Derived* self,
                             const InputType& input,
                             InputType& output,
                             const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  self->PostExecute(input, output, policy);
}

//--------------------------------------------------------------------------------
template <typename Derived, typename... Args>
void CallPostExecuteInternal(std::false_type, Derived*, Args&&...)
{
}

//--------------------------------------------------------------------------------
template <typename Derived, typename InputType, typename DerivedPolicy>
void CallPostExecute(Derived* self,
                     const InputType& input,
                     InputType& output,
                     const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  using call_supported_t = typename SupportsPostExecute<Derived, InputType, DerivedPolicy>::type;
  CallPostExecuteInternal(call_supported_t(), self, input, output, policy);
}
}

//----------------------------------------------------------------------------
template <typename Derived>
inline VTKM_CONT Filter<Derived>::Filter()
  : Invoke()
  , FieldsToPass(vtkm::filter::FieldSelection::MODE_ALL)
{
}

//----------------------------------------------------------------------------
template <typename Derived>
inline VTKM_CONT Filter<Derived>::~Filter()
{
}

//----------------------------------------------------------------------------
template <typename Derived>
inline VTKM_CONT vtkm::cont::DataSet Filter<Derived>::Execute(const vtkm::cont::DataSet& input)
{
  return this->Execute(input, vtkm::filter::PolicyDefault());
}

//----------------------------------------------------------------------------
template <typename Derived>
inline VTKM_CONT vtkm::cont::PartitionedDataSet Filter<Derived>::Execute(
  const vtkm::cont::PartitionedDataSet& input)
{
  return this->Execute(input, vtkm::filter::PolicyDefault());
}


//----------------------------------------------------------------------------
template <typename Derived>
template <typename DerivedPolicy>
VTKM_CONT vtkm::cont::DataSet Filter<Derived>::Execute(
  const vtkm::cont::DataSet& input,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  VTKM_LOG_SCOPE(
    vtkm::cont::LogLevel::Perf, "Filter: '%s'", vtkm::cont::TypeToString<Derived>().c_str());

  Derived* self = static_cast<Derived*>(this);
  vtkm::cont::PartitionedDataSet output =
    self->Execute(vtkm::cont::PartitionedDataSet(input), policy);
  if (output.GetNumberOfPartitions() > 1)
  {
    throw vtkm::cont::ErrorFilterExecution("Expecting at most 1 block.");
  }
  return output.GetNumberOfPartitions() == 1 ? output.GetPartition(0) : vtkm::cont::DataSet();
}

//----------------------------------------------------------------------------
template <typename Derived>
template <typename DerivedPolicy>
VTKM_CONT vtkm::cont::PartitionedDataSet Filter<Derived>::Execute(
  const vtkm::cont::PartitionedDataSet& input,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  VTKM_LOG_SCOPE(vtkm::cont::LogLevel::Perf,
                 "Filter (PartitionedDataSet): '%s'",
                 vtkm::cont::TypeToString<Derived>().c_str());

  Derived* self = static_cast<Derived*>(this);

  // Call `void Derived::PreExecute<DerivedPolicy>(input, policy)`, if defined.
  internal::CallPreExecute(self, input, policy);

  // Call `PrepareForExecution` (which should probably be renamed at some point)
  vtkm::cont::PartitionedDataSet output = internal::CallPrepareForExecution(self, input, policy);

  // Call `Derived::PostExecute<DerivedPolicy>(input, output, policy)` if defined.
  internal::CallPostExecute(self, input, output, policy);
  return output;
}

//----------------------------------------------------------------------------
template <typename Derived>
template <typename DerivedPolicy>
inline VTKM_CONT void Filter<Derived>::MapFieldsToPass(
  const vtkm::cont::DataSet& input,
  vtkm::cont::DataSet& output,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  Derived* self = static_cast<Derived*>(this);
  for (vtkm::IdComponent cc = 0; cc < input.GetNumberOfFields(); ++cc)
  {
    auto field = input.GetField(cc);
    if (this->GetFieldsToPass().IsFieldSelected(field))
    {
      internal::CallMapFieldOntoOutput(self, output, field, policy);
    }
  }
}
}
}
