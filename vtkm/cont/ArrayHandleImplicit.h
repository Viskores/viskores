//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleImplicit_h
#define vtk_m_cont_ArrayHandleImplicit_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/StorageImplicit.h>

namespace vtkm
{
namespace cont
{

namespace detail
{

template <class FunctorType_>
class VTKM_ALWAYS_EXPORT ArrayPortalImplicit;

/// A convenience class that provides a typedef to the appropriate tag for
/// a implicit array container.
template <typename FunctorType>
struct ArrayHandleImplicitTraits
{
  using ValueType = decltype(FunctorType{}(vtkm::Id{}));
  using StorageTag = vtkm::cont::StorageTagImplicit<ArrayPortalImplicit<FunctorType>>;
  using Superclass = vtkm::cont::ArrayHandle<ValueType, StorageTag>;
};

/// \brief An array portal that returns the result of a functor
///
/// This array portal is similar to an implicit array i.e an array that is
/// defined functionally rather than actually stored in memory. The array
/// comprises a functor that is called for each index.
///
/// The \c ArrayPortalImplicit is used in an ArrayHandle with an
/// \c StorageImplicit container.
///
template <class FunctorType_>
class VTKM_ALWAYS_EXPORT ArrayPortalImplicit
{
public:
  using ValueType = typename ArrayHandleImplicitTraits<FunctorType_>::ValueType;
  using FunctorType = FunctorType_;

  VTKM_EXEC_CONT
  ArrayPortalImplicit()
    : Functor()
    , NumberOfValues(0)
  {
  }

  VTKM_EXEC_CONT
  ArrayPortalImplicit(FunctorType f, vtkm::Id numValues)
    : Functor(f)
    , NumberOfValues(numValues)
  {
  }

  VTKM_EXEC_CONT
  const FunctorType& GetFunctor() const { return this->Functor; }

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const { return this->Functor(index); }

private:
  FunctorType Functor;
  vtkm::Id NumberOfValues;
};

} // namespace detail

/// \brief An \c ArrayHandle that computes values on the fly.
///
/// \c ArrayHandleImplicit is a specialization of ArrayHandle.
/// It takes a user defined functor which is called with a given index value.
/// The functor returns the result of the functor as the value of this
/// array at that position.
///
template <class FunctorType>
class VTKM_ALWAYS_EXPORT ArrayHandleImplicit
  : public detail::ArrayHandleImplicitTraits<FunctorType>::Superclass
{
private:
  using ArrayTraits = typename detail::ArrayHandleImplicitTraits<FunctorType>;

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleImplicit,
                             (ArrayHandleImplicit<FunctorType>),
                             (typename ArrayTraits::Superclass));

  VTKM_CONT
  ArrayHandleImplicit(FunctorType functor, vtkm::Id length)
    : Superclass(typename Superclass::PortalConstControl(functor, length))
  {
  }
};

/// make_ArrayHandleImplicit is convenience function to generate an
/// ArrayHandleImplicit.  It takes a functor and the virtual length of the
/// arry.

template <typename FunctorType>
VTKM_CONT vtkm::cont::ArrayHandleImplicit<FunctorType> make_ArrayHandleImplicit(FunctorType functor,
                                                                                vtkm::Id length)
{
  return ArrayHandleImplicit<FunctorType>(functor, length);
}
}
} // namespace vtkm::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace vtkm
{
namespace cont
{

template <typename Functor>
struct SerializableTypeString<vtkm::cont::ArrayHandleImplicit<Functor>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_Implicit<" + SerializableTypeString<Functor>::Get() + ">";
    return name;
  }
};

template <typename Functor>
struct SerializableTypeString<vtkm::cont::ArrayHandle<
  typename vtkm::cont::detail::ArrayHandleImplicitTraits<Functor>::ValueType,
  vtkm::cont::StorageTagImplicit<vtkm::cont::detail::ArrayPortalImplicit<Functor>>>>
  : SerializableTypeString<vtkm::cont::ArrayHandleImplicit<Functor>>
{
};
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <typename Functor>
struct Serialization<vtkm::cont::ArrayHandleImplicit<Functor>>
{
private:
  using Type = vtkm::cont::ArrayHandleImplicit<Functor>;
  using BaseType = vtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    vtkmdiy::save(bb, obj.GetNumberOfValues());
    vtkmdiy::save(bb, obj.GetPortalConstControl().GetFunctor());
  }

  static VTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    vtkm::Id count = 0;
    vtkmdiy::load(bb, count);

    Functor functor;
    vtkmdiy::load(bb, functor);

    obj = vtkm::cont::make_ArrayHandleImplicit(functor, count);
  }
};

template <typename Functor>
struct Serialization<vtkm::cont::ArrayHandle<
  typename vtkm::cont::detail::ArrayHandleImplicitTraits<Functor>::ValueType,
  vtkm::cont::StorageTagImplicit<vtkm::cont::detail::ArrayPortalImplicit<Functor>>>>
  : Serialization<vtkm::cont::ArrayHandleImplicit<Functor>>
{
};

} // diy
/// @endcond SERIALIZATION

#endif //vtk_m_cont_ArrayHandleImplicit_h
