//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_VariantArrayHandleContainer_h
#define vtk_m_cont_VariantArrayHandleContainer_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleVirtual.h>
#include <vtkm/cont/ArrayHandleVirtual.hxx>
#include <vtkm/cont/ErrorBadType.h>

#include <vtkm/VecTraits.h>

#include <memory>
#include <sstream>
#include <typeindex>

namespace vtkm
{
namespace cont
{

// Forward declaration needed for GetContainer
template <typename TypeList>
class VariantArrayHandleBase;

namespace internal
{

/// \brief Base class for VariantArrayHandleContainer
///
struct VTKM_CONT_EXPORT VariantArrayHandleContainerBase
{
  std::type_index TypeIndex;

  VariantArrayHandleContainerBase();
  explicit VariantArrayHandleContainerBase(const std::type_info& hash);

  // This must exist so that subclasses are destroyed correctly.
  virtual ~VariantArrayHandleContainerBase();

  virtual vtkm::Id GetNumberOfValues() const = 0;
  virtual vtkm::IdComponent GetNumberOfComponents() const = 0;

  virtual void ReleaseResourcesExecution() = 0;
  virtual void ReleaseResources() = 0;

  virtual void PrintSummary(std::ostream& out) const = 0;

  virtual std::shared_ptr<VariantArrayHandleContainerBase> NewInstance() const = 0;
};

/// \brief ArrayHandle container that can use C++ run-time type information.
///
/// The \c VariantArrayHandleContainer holds ArrayHandle objects
/// (with different template parameters) so that it can polymorphically answer
/// simple questions about the object.
///
template <typename T>
struct VTKM_ALWAYS_EXPORT VariantArrayHandleContainer final : public VariantArrayHandleContainerBase
{
  vtkm::cont::ArrayHandleVirtual<T> Array;
  mutable vtkm::IdComponent NumberOfComponents = 0;

  VariantArrayHandleContainer()
    : VariantArrayHandleContainerBase(typeid(T))
    , Array()
  {
  }

  VariantArrayHandleContainer(const vtkm::cont::ArrayHandleVirtual<T>& array)
    : VariantArrayHandleContainerBase(typeid(T))
    , Array(array)
  {
  }

  ~VariantArrayHandleContainer<T>() override = default;

  vtkm::Id GetNumberOfValues() const override { return this->Array.GetNumberOfValues(); }

  vtkm::IdComponent GetNumberOfComponents() const override
  {
    // Cache number of components to avoid unnecessary device to host transfers of the array.
    // Also assumes that the number of components is constant accross all elements and
    // throughout the life of the array.
    if (this->NumberOfComponents == 0)
    {
      this->NumberOfComponents =
        this->GetNumberOfComponents(typename vtkm::VecTraits<T>::IsSizeStatic{});
    }
    return this->NumberOfComponents;
  }

  void ReleaseResourcesExecution() override { this->Array.ReleaseResourcesExecution(); }
  void ReleaseResources() override { this->Array.ReleaseResources(); }

  void PrintSummary(std::ostream& out) const override
  {
    vtkm::cont::printSummary_ArrayHandle(this->Array, out);
  }

  std::shared_ptr<VariantArrayHandleContainerBase> NewInstance() const override
  {
    return std::make_shared<VariantArrayHandleContainer<T>>(this->Array.NewInstance());
  }

private:
  vtkm::IdComponent GetNumberOfComponents(VecTraitsTagSizeStatic) const
  {
    return vtkm::VecTraits<T>::NUM_COMPONENTS;
  }

  vtkm::IdComponent GetNumberOfComponents(VecTraitsTagSizeVariable) const
  {
    return (this->Array.GetNumberOfValues() == 0)
      ? 0
      : vtkm::VecTraits<T>::GetNumberOfComponents(this->Array.GetPortalConstControl().Get(0));
  }
};

namespace variant
{

// One instance of a template class cannot access the private members of
// another instance of a template class. However, I want to be able to copy
// construct a VariantArrayHandle from another VariantArrayHandle of any other
// type. Since you cannot partially specialize friendship, use this accessor
// class to get at the internals for the copy constructor.
struct GetContainer
{
  template <typename TypeList>
  VTKM_CONT static const std::shared_ptr<VariantArrayHandleContainerBase>& Extract(
    const vtkm::cont::VariantArrayHandleBase<TypeList>& src)
  {
    return src.ArrayContainer;
  }
};

template <typename T>
VTKM_CONT bool IsValueType(const VariantArrayHandleContainerBase* container)
{
  if (container == nullptr)
  { //you can't use typeid on nullptr of polymorphic types
    return false;
  }

  //needs optimizations based on platform. !OSX can use typeid
  return container->TypeIndex == std::type_index(typeid(T));
  // return (nullptr != dynamic_cast<const VariantArrayHandleContainer<T>*>(container));
}

template <typename ArrayHandleType>
VTKM_CONT inline bool IsType(const VariantArrayHandleContainerBase* container)
{ //container could be nullptr
  using T = typename ArrayHandleType::ValueType;
  if (!IsValueType<T>(container))
  {
    return false;
  }

  const auto* derived = static_cast<const VariantArrayHandleContainer<T>*>(container);
  return vtkm::cont::IsType<ArrayHandleType>(derived->Array);
}

template <typename T, typename S>
struct VTKM_ALWAYS_EXPORT Caster
{
  vtkm::cont::ArrayHandle<T, S> operator()(const VariantArrayHandleContainerBase* container) const
  {
    //This needs to be reworked
    using ArrayHandleType = vtkm::cont::ArrayHandle<T, S>;
    if (!IsValueType<T>(container))
    {
      VTKM_LOG_CAST_FAIL(container, ArrayHandleType);
      throwFailedDynamicCast(vtkm::cont::TypeToString(container),
                             vtkm::cont::TypeToString<ArrayHandleType>());
    }

    const auto* derived = static_cast<const VariantArrayHandleContainer<T>*>(container);
    return vtkm::cont::Cast<vtkm::cont::ArrayHandle<T, S>>(derived->Array);
  }
};

template <typename T>
struct VTKM_ALWAYS_EXPORT Caster<T, vtkm::cont::StorageTagVirtual>
{
  vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagVirtual> operator()(
    const VariantArrayHandleContainerBase* container) const
  {
    if (!IsValueType<T>(container))
    {
      VTKM_LOG_CAST_FAIL(container, vtkm::cont::ArrayHandleVirtual<T>);
      throwFailedDynamicCast(vtkm::cont::TypeToString(container),
                             vtkm::cont::TypeToString<vtkm::cont::ArrayHandleVirtual<T>>());
    }

    // Technically, this method returns a copy of the \c ArrayHandle. But
    // because \c ArrayHandle acts like a shared pointer, it is valid to
    // do the copy.
    const auto* derived = static_cast<const VariantArrayHandleContainer<T>*>(container);
    VTKM_LOG_CAST_SUCC(container, derived->Array);
    return derived->Array;
  }
};

template <typename ArrayHandleType>
VTKM_CONT ArrayHandleType Cast(const VariantArrayHandleContainerBase* container)
{ //container could be nullptr
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType);
  using Type = typename ArrayHandleType::ValueType;
  using Storage = typename ArrayHandleType::StorageTag;
  auto ret = Caster<Type, Storage>{}(container);
  return ArrayHandleType(std::move(ret));
}

struct ForceCastToVirtual
{
  template <typename SrcValueType, typename Storage, typename DstValueType>
  VTKM_CONT typename std::enable_if<std::is_same<SrcValueType, DstValueType>::value>::type
  operator()(const vtkm::cont::ArrayHandle<SrcValueType, Storage>& input,
             vtkm::cont::ArrayHandleVirtual<DstValueType>& output) const
  { // ValueTypes match
    output = vtkm::cont::make_ArrayHandleVirtual<DstValueType>(input);
  }

  template <typename SrcValueType, typename Storage, typename DstValueType>
  VTKM_CONT typename std::enable_if<!std::is_same<SrcValueType, DstValueType>::value>::type
  operator()(const vtkm::cont::ArrayHandle<SrcValueType, Storage>& input,
             vtkm::cont::ArrayHandleVirtual<DstValueType>& output) const
  { // ValueTypes do not match
    this->ValidateWidthAndCast<SrcValueType, DstValueType>(input, output);
  }

private:
  template <typename S,
            typename D,
            typename InputType,
            vtkm::IdComponent SSize = vtkm::VecTraits<S>::NUM_COMPONENTS,
            vtkm::IdComponent DSize = vtkm::VecTraits<D>::NUM_COMPONENTS>
  VTKM_CONT typename std::enable_if<SSize == DSize>::type ValidateWidthAndCast(
    const InputType& input,
    vtkm::cont::ArrayHandleVirtual<D>& output) const
  { // number of components match
    auto casted = vtkm::cont::make_ArrayHandleCast<D>(input);
    output = vtkm::cont::make_ArrayHandleVirtual<D>(casted);
  }

  template <typename S,
            typename D,
            vtkm::IdComponent SSize = vtkm::VecTraits<S>::NUM_COMPONENTS,
            vtkm::IdComponent DSize = vtkm::VecTraits<D>::NUM_COMPONENTS>
  VTKM_CONT typename std::enable_if<SSize != DSize>::type ValidateWidthAndCast(
    const ArrayHandleBase&,
    ArrayHandleBase&) const
  { // number of components do not match
    std::ostringstream str;
    str << "VariantArrayHandle::AsVirtual: Cannot cast from " << vtkm::cont::TypeToString<S>()
        << " to " << vtkm::cont::TypeToString<D>() << "; "
                                                      "number of components must match exactly.";
    throw vtkm::cont::ErrorBadType(str.str());
  }
};
}
}
}
} //namespace vtkm::cont::internal::variant

#endif
