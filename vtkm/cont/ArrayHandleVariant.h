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
#ifndef vtk_m_cont_ArrayHandleVariant_h
#define vtk_m_cont_ArrayHandleVariant_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/TypeListTag.h>
#include <vtkm/VecTraits.h>

#include <vtkm/cont/ArrayHandleVirtual.h>

#include <vtkm/cont/ErrorBadType.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/internal/DynamicTransform.h>

#include <vtkm/cont/internal/ArrayHandleVariantContainer.h>

namespace vtkm
{
namespace cont
{
/// \brief Holds an array handle without having to specify template parameters.
///
/// \c ArrayHandleVariant holds an \c ArrayHandle or \c ArrayHandleVirtual
/// object using runtime polymorphism to manage different value types and
/// storage rather than compile-time templates. This adds a programming
/// convenience that helps avoid a proliferation of templates. It also provides
/// the management necessary to interface VTK-m with data sources where types
/// will not be known until runtime.
///
/// To interface between the runtime polymorphism and the templated algorithms
/// in VTK-m, \c ArrayHandleVariant contains a method named \c CastAndCall that
/// will determine the correct type from some known list of types. It returns
/// an ArrayHandleVirtual which type erases the storage type by using polymorphism.
/// This mechanism is used internally by VTK-m's worklet invocation
/// mechanism to determine the type when running algorithms.
///
/// By default, \c ArrayHandleVariant will assume that the value type in the
/// array matches one of the types specified by \c VTKM_DEFAULT_TYPE_LIST_TAG
/// This list can be changed by using the \c ResetTypes. It is
/// worthwhile to match these lists closely to the possible types that might be
/// used. If a type is missing you will get a runtime error. If there are more
/// types than necessary, then the template mechanism will create a lot of
/// object code that is never used, and keep in mind that the number of
/// combinations grows exponentially when using multiple \c ArrayHandleVariant
/// objects.
///
/// The actual implementation of \c ArrayHandleVariant is in a templated class
/// named \c ArrayHandleVariantBase, which is templated on the list of
/// component types.
///
template <typename TypeList>
class VTKM_ALWAYS_EXPORT ArrayHandleVariantBase
{
public:
  VTKM_CONT
  ArrayHandleVariantBase() = default;

  template <typename T, typename Storage>
  VTKM_CONT ArrayHandleVariantBase(const vtkm::cont::ArrayHandle<T, Storage>& array)
    : ArrayContainer(std::make_shared<internal::ArrayHandleVariantContainer<T>>(
        vtkm::cont::ArrayHandleAny<T>{ array }))
  {
  }

  template <typename T>
  explicit VTKM_CONT ArrayHandleVariantBase(
    const vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagVirtual>& array)
    : ArrayContainer(std::make_shared<internal::ArrayHandleVariantContainer<T>>(array))
  {
  }

  template <typename OtherTypeList>
  VTKM_CONT explicit ArrayHandleVariantBase(const ArrayHandleVariantBase<OtherTypeList>& src)
    : ArrayContainer(internal::variant::GetContainer::Extract(src))
  {
  }

  VTKM_CONT ArrayHandleVariantBase(const ArrayHandleVariantBase& src) = default;
  VTKM_CONT ArrayHandleVariantBase(ArrayHandleVariantBase&& src) noexcept = default;

  VTKM_CONT
  ~ArrayHandleVariantBase() {}

  VTKM_CONT
  ArrayHandleVariantBase<TypeList>& operator=(const ArrayHandleVariantBase<TypeList>& src) =
    default;

  VTKM_CONT
  ArrayHandleVariantBase<TypeList>& operator=(ArrayHandleVariantBase<TypeList>&& src) noexcept =
    default;


  /// Returns true if this array matches the array handle type passed in.
  ///
  template <typename ArrayHandleType>
  VTKM_CONT bool IsType()
  {
    return internal::variant::IsType<ArrayHandleType>(this->ArrayContainer.get());
  }

  /// Returns true if this array matches the ValueType type passed in.
  ///
  template <typename T>
  VTKM_CONT bool IsVirtualType()
  {
    return internal::variant::IsVirtualType<T>(this->ArrayContainer.get());
  }

  /// Returns this array cast to the given \c ArrayHandle type. Throws \c
  /// ErrorBadType if the cast does not work. Use \c IsType
  /// to check if the cast can happen.
  ///
  template <typename ArrayHandleType>
  VTKM_CONT ArrayHandleType Cast() const
  {
    return internal::variant::Cast<ArrayHandleType>(this->ArrayContainer.get());
  }

  /// Returns this array cast to a \c ArrayHandleVirtual of the given type. Throws \c
  /// ErrorBadType if the cast does not work.
  ///
  template <typename T>
  VTKM_CONT vtkm::cont::ArrayHandleVirtual<T> AsVirtual() const
  {
    return internal::variant::Cast<vtkm::cont::ArrayHandleVirtual<T>>(this->ArrayContainer.get());
  }

  /// Given a references to an ArrayHandle object, casts this array to the
  /// ArrayHandle's type and sets the given ArrayHandle to this array. Throws
  /// \c ErrorBadType if the cast does not work. Use \c
  /// ArrayHandleType to check if the cast can happen.
  ///
  /// Note that this is a shallow copy. The data are not copied and a change
  /// in the data in one array will be reflected in the other.
  ///
  template <typename ArrayHandleType>
  VTKM_CONT void CopyTo(ArrayHandleType& array) const
  {
    VTKM_IS_ARRAY_HANDLE(ArrayHandleType);
    array = this->Cast<ArrayHandleType>();
  }

  /// Changes the types to try casting to when resolving this variant array,
  /// which is specified with a list tag like those in TypeListTag.h. Since C++
  /// does not allow you to actually change the template arguments, this method
  /// returns a new variant array object. This method is particularly useful to
  /// narrow down (or expand) the types when using an array of particular
  /// constraints.
  ///
  template <typename NewTypeList>
  VTKM_CONT ArrayHandleVariantBase<NewTypeList> ResetTypes(NewTypeList = NewTypeList()) const
  {
    VTKM_IS_LIST_TAG(NewTypeList);
    return ArrayHandleVariantBase<NewTypeList>(*this);
  }

  /// Attempts to cast the held array to a specific value type,
  /// then call the given functor with the cast array. The types
  /// tried in the cast are those in the lists defined by the TypeList.
  /// By default \c ArrayHandleVariant set this to VTKM_DEFAULT_TYPE_LIST_TAG.
  ///
  template <typename Functor, typename... Args>
  VTKM_CONT void CastAndCall(Functor&& f, Args&&...) const;

  /// \brief Create a new array of the same type as this array.
  ///
  /// This method creates a new array that is the same type as this one and
  /// returns a new variant array handle for it. This method is convenient when
  /// creating output arrays that should be the same type as some input array.
  ///
  VTKM_CONT
  ArrayHandleVariantBase<TypeList> NewInstance() const
  {
    ArrayHandleVariantBase<TypeList> instance;
    instance.ArrayContainer = this->ArrayContainer->NewInstance();
    return instance;
  }

  /// Releases any resources being used in the execution environment (that are
  /// not being shared by the control environment).
  ///
  void ReleaseResourcesExecution() { return this->ArrayContainer->ReleaseResourcesExecution(); }


  /// Releases all resources in both the control and execution environments.
  ///
  void ReleaseResources() { return this->ArrayContainer->ReleaseResources(); }

  /// \brief Get the number of components in each array value.
  ///
  /// This method will query the array type for the number of components in
  /// each value of the array. The number of components is determined by
  /// the \c VecTraits::NUM_COMPONENTS trait class.
  ///
  VTKM_CONT
  vtkm::IdComponent GetNumberOfComponents() const
  {
    return this->ArrayContainer->GetNumberOfComponents();
  }

  /// \brief Get the number of values in the array.
  ///
  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return this->ArrayContainer->GetNumberOfValues(); }

  VTKM_CONT
  void PrintSummary(std::ostream& out) const { this->ArrayContainer->PrintSummary(out); }

private:
  friend struct internal::variant::GetContainer;
  std::shared_ptr<vtkm::cont::internal::ArrayHandleVariantContainerBase> ArrayContainer;
};

using ArrayHandleVariant = vtkm::cont::ArrayHandleVariantBase<VTKM_DEFAULT_TYPE_LIST_TAG>;

namespace detail
{

struct ArrayHandleVariantTry
{
  template <typename T, typename Functor, typename... Args>
  void operator()(T,
                  Functor&& f,
                  bool& called,
                  const vtkm::cont::internal::ArrayHandleVariantContainerBase& container,
                  Args&&... args) const
  {
    if (!called && vtkm::cont::internal::variant::IsVirtualType<T>(&container))
    {
      called = true;
      const auto* derived =
        static_cast<const vtkm::cont::internal::ArrayHandleVariantContainer<T>*>(&container);
      VTKM_LOG_CAST_SUCC(container, derived);
      f(derived->Array, std::forward<Args>(args)...);
    }
  }
};

VTKM_CONT_EXPORT void ThrowCastAndCallException(
  const vtkm::cont::internal::ArrayHandleVariantContainerBase&,
  const std::type_info&);
} // namespace detail



template <typename TypeList>
template <typename Functor, typename... Args>
VTKM_CONT void ArrayHandleVariantBase<TypeList>::CastAndCall(Functor&& f, Args&&... args) const
{
  bool called = false;
  const auto& ref = *this->ArrayContainer;
  vtkm::ListForEach(detail::ArrayHandleVariantTry{},
                    TypeList{},
                    std::forward<Functor>(f),
                    called,
                    ref,
                    std::forward<Args>(args)...);
  if (!called)
  {
    // throw an exception
    VTKM_LOG_CAST_FAIL(*this, TypeList);
    detail::ThrowCastAndCallException(ref, typeid(TypeList));
  }
}

namespace internal
{

template <typename TypeList>
struct DynamicTransformTraits<vtkm::cont::ArrayHandleVariantBase<TypeList>>
{
  using DynamicTag = vtkm::cont::internal::DynamicTransformTagCastAndCall;
};

} // namespace internal
} // namespace cont
} // namespace vtkm

//=============================================================================
// Specializations of serialization related classes
namespace diy
{

namespace internal
{

struct ArrayHandleVariantSerializeFunctor
{
  template <typename ArrayHandleType>
  void operator()(const ArrayHandleType& ah, BinaryBuffer& bb) const
  {
    diy::save(bb, vtkm::cont::TypeString<ArrayHandleType>::Get());
    diy::save(bb, ah);
  }
};

struct ArrayHandleVariantDeserializeFunctor
{
  template <typename T, typename TypeList>
  void operator()(T,
                  vtkm::cont::ArrayHandleVariantBase<TypeList>& dh,
                  const std::string& typeString,
                  bool& success,
                  BinaryBuffer& bb) const
  {
    using ArrayHandleType = vtkm::cont::ArrayHandleVirtual<T>;

    if (!success && (typeString == vtkm::cont::TypeString<ArrayHandleType>::Get()))
    {
      ArrayHandleType ah;
      diy::load(bb, ah);
      dh = vtkm::cont::ArrayHandleVariantBase<TypeList>(ah);
      success = true;
    }
  }
};

} // internal

template <typename TypeList>
struct Serialization<vtkm::cont::ArrayHandleVariantBase<TypeList>>
{
private:
  using Type = vtkm::cont::ArrayHandleVariantBase<TypeList>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const Type& obj)
  {
    vtkm::cont::CastAndCall(obj, internal::ArrayHandleVariantSerializeFunctor{}, bb);
  }

  static VTKM_CONT void load(BinaryBuffer& bb, Type& obj)
  {
    std::string typeString;
    diy::load(bb, typeString);

    bool success = false;
    vtkm::ListForEach(
      internal::ArrayHandleVariantDeserializeFunctor{}, TypeList{}, obj, typeString, success, bb);

    if (!success)
    {
      throw vtkm::cont::ErrorBadType(
        "Error deserializing ArrayHandleVariant. Message TypeString: " + typeString);
    }
  }
};

} // diy


#endif //vtk_m_virts_ArrayHandleVariant_h
