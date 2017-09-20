//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================
#ifndef vtk_m_cont_ArrayHandleSwizzle_h
#define vtk_m_cont_ArrayHandleSwizzle_h

#include <vtkm/VecTraits.h>
#include <vtkm/cont/ArrayHandle.h>

#include <array>

namespace vtkm
{
namespace cont
{
namespace internal
{

// If TestValue appears more than once in ComponentMap, IsUnique will be false,
// but true if TestValue is unique.
template <vtkm::IdComponent TestValue, vtkm::IdComponent... ComponentMap>
struct ComponentIsUnique;

// Terminal case:
template <vtkm::IdComponent TestValue, vtkm::IdComponent Head>
struct ComponentIsUnique<TestValue, Head>
{
  const static bool IsUnique = TestValue != Head;
};

// Recursive case:
template <vtkm::IdComponent TestValue, vtkm::IdComponent Head, vtkm::IdComponent... Tail>
struct ComponentIsUnique<TestValue, Head, Tail...>
{
  using Next = ComponentIsUnique<TestValue, Tail...>;
  const static bool IsUnique = TestValue != Head && Next::IsUnique;
};

// Validate the component map.
// All elements must be (1) unique, (2) >= 0, and (3) < InputSize
template <vtkm::IdComponent InputSize, vtkm::IdComponent... ComponentMap>
struct ValidateComponentMap;

// Terminal impl:
template <vtkm::IdComponent InputSize, vtkm::IdComponent Head>
struct ValidateComponentMap<InputSize, Head>
{
  static const bool Valid = Head >= 0 && Head < InputSize;
};

// Recursive impl:
template <vtkm::IdComponent InputSize, vtkm::IdComponent Head, vtkm::IdComponent... Tail>
struct ValidateComponentMap<InputSize, Head, Tail...>
{
  using Next = ValidateComponentMap<InputSize, Tail...>;
  static const bool IsUnique = ComponentIsUnique<Head, Tail...>::IsUnique;
  static const bool Valid = Head >= 0 && Head < InputSize && IsUnique && Next::Valid;
};

} // end namespace internal

/// This class collects metadata for an ArrayHandleSwizzle.
template <typename InputValueType, vtkm::IdComponent... ComponentMap>
struct ArrayHandleSwizzleTraits
{
  /// The number of elements in the ComponentMap.
  static const vtkm::IdComponent COUNT = static_cast<vtkm::IdComponent>(sizeof...(ComponentMap));
  VTKM_STATIC_ASSERT_MSG(COUNT > 0, "Invalid ComponentMap: Cannot swizzle zero components.");

  /// A std::array containing the ComponentMap for runtime querying.
  using RuntimeComponentMapType = std::array<vtkm::IdComponent, COUNT>;
  static VTKM_CONSTEXPR RuntimeComponentMapType GenerateRuntimeComponentMap()
  {
    return RuntimeComponentMapType{ { ComponentMap... } };
  }

  /// The ValueType of the ArrayHandleSwizzle's internal ArrayHandle.
  using InputType = InputValueType;

  /// The VecTraits for InputType.
  using InputTraits = VecTraits<InputType>;

  using Validator = internal::ValidateComponentMap<InputTraits::NUM_COMPONENTS, ComponentMap...>;
  VTKM_STATIC_ASSERT_MSG(Validator::Valid,
                         "Invalid ComponentMap: Ids in ComponentMap must be unique, positive, and "
                         "less than the number of input components.");

  /// The ComponentType of the ArrayHandleSwizzle.
  using ComponentType = typename InputTraits::ComponentType;

  /// The ValueType of the ArrayHandleSwizzle.
  using OutputType = vtkm::Vec<ComponentType, COUNT>;

  // The VecTraits for OutputType.
  using OutputTraits = VecTraits<OutputType>;

  /// If true, we use all components in the input vector. If false, we'll need
  /// to make sure to preserve existing values on write.
  static const bool ALL_COMPS_USED = InputTraits::NUM_COMPONENTS == COUNT;

private:
  template <vtkm::IdComponent OutputIndex, vtkm::IdComponent... Map>
  struct GetImpl;

  // Terminal case:
  template <vtkm::IdComponent OutputIndex, vtkm::IdComponent Head>
  struct GetImpl<OutputIndex, Head>
  {
    VTKM_CONSTEXPR vtkm::IdComponent operator()() const { return OutputIndex == 0 ? Head : -1; }
  };

  // Recursive case:
  template <vtkm::IdComponent OutputIndex, vtkm::IdComponent Head, vtkm::IdComponent... Tail>
  struct GetImpl<OutputIndex, Head, Tail...>
  {
    using Next = GetImpl<OutputIndex - 1, Tail...>;

    VTKM_CONSTEXPR vtkm::IdComponent operator()() const
    {
      return OutputIndex == 0 ? Head : Next()();
    }
  };

public:
  /// Get the component from ComponentMap at the specified index as a
  /// compile-time constant:
  template <vtkm::IdComponent OutputIndex>
  static VTKM_CONSTEXPR vtkm::IdComponent Get()
  {
    return GetImpl<OutputIndex, ComponentMap...>()();
  }

private:
  template <vtkm::IdComponent OutputIndex, vtkm::IdComponent... Map>
  struct SwizzleImpl;

  // Terminal case:
  template <vtkm::IdComponent OutputIndex, vtkm::IdComponent Head>
  struct SwizzleImpl<OutputIndex, Head>
  {
    static const vtkm::IdComponent InputIndex = Head;

    void operator()(const InputType& in, OutputType& out) const
    {
      OutputTraits::SetComponent(out, OutputIndex, InputTraits::GetComponent(in, InputIndex));
    }
  };

  // Recursive case:
  template <vtkm::IdComponent OutputIndex, vtkm::IdComponent Head, vtkm::IdComponent... Tail>
  struct SwizzleImpl<OutputIndex, Head, Tail...>
  {
    using Next = SwizzleImpl<OutputIndex + 1, Tail...>;
    static const vtkm::IdComponent InputIndex = Head;

    void operator()(const InputType& in, OutputType& out) const
    {
      OutputTraits::SetComponent(out, OutputIndex, InputTraits::GetComponent(in, InputIndex));
      Next()(in, out);
    }
  };

public:
  /// Swizzle the input type into the output type.
  static void Swizzle(const InputType& in, OutputType& out)
  {
    SwizzleImpl<0, ComponentMap...>()(in, out);
  }

  // UnSwizzle output type --> input type
private:
  template <vtkm::IdComponent OutputIndex, vtkm::IdComponent... Map>
  struct UnSwizzleImpl;

  // Terminal case:
  template <vtkm::IdComponent OutputIndex, vtkm::IdComponent Head>
  struct UnSwizzleImpl<OutputIndex, Head>
  {
    static const vtkm::IdComponent InputIndex = Head;

    void operator()(const OutputType& out, InputType& in) const
    {
      InputTraits::SetComponent(in, InputIndex, OutputTraits::GetComponent(out, OutputIndex));
    }
  };

  // Recursive case:
  template <vtkm::IdComponent OutputIndex, vtkm::IdComponent Head, vtkm::IdComponent... Tail>
  struct UnSwizzleImpl<OutputIndex, Head, Tail...>
  {
    using Next = UnSwizzleImpl<OutputIndex + 1, Tail...>;
    static const vtkm::IdComponent InputIndex = Head;

    void operator()(const OutputType& out, InputType& in) const
    {
      InputTraits::SetComponent(in, InputIndex, OutputTraits::GetComponent(out, OutputIndex));
      Next()(out, in);
    }
  };

  // Entry point:
public:
  /// Unswizzle the output type back into the input type.
  /// @warning If the entire vector is not used, there may be uninitialized
  /// data in the resulting InputType vector. See ALL_COMPS_USED flag.
  static void UnSwizzle(const OutputType& out, InputType& in)
  {
    UnSwizzleImpl<0, ComponentMap...>()(out, in);
  }
};

namespace internal
{

template <typename PortalType, vtkm::IdComponent... ComponentMap>
class VTKM_ALWAYS_EXPORT ArrayPortalSwizzle
{
  using Traits = ArrayHandleSwizzleTraits<typename PortalType::ValueType, ComponentMap...>;

public:
  using ValueType = typename Traits::OutputType;

  VTKM_EXEC_CONT
  ArrayPortalSwizzle()
    : Portal()
  {
  }

  VTKM_EXEC_CONT
  ArrayPortalSwizzle(const PortalType& portal)
    : Portal(portal)
  {
  }

  // Copy constructor
  VTKM_EXEC_CONT ArrayPortalSwizzle(const ArrayPortalSwizzle<PortalType, ComponentMap...>& src)
    : Portal(src.GetPortal())
  {
  }

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return this->Portal.GetNumberOfValues(); }

  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const
  {
    typename Traits::OutputType result;
    Traits::Swizzle(this->Portal.Get(index), result);
    return result;
  }

  VTKM_EXEC_CONT
  void Set(vtkm::Id index, const ValueType& value) const
  {
    SetImpl<!Traits::ALL_COMPS_USED>(this->Portal)(index, value);
  }

private:
  // If NeedsRead is true, we need to initialize the InputType vector we write
  // with the current values at @a index to avoid overwriting unused components.
  template <bool NeedsRead>
  struct SetImpl
  {
    const PortalType& Portal;

    SetImpl(const PortalType& portal)
      : Portal(portal)
    {
    }

    void operator()(const vtkm::Id& index, const ValueType& value)
    {
      typename Traits::InputType in;
      if (NeedsRead)
      {
        in = this->Portal.Get(index);
      }
      Traits::UnSwizzle(value, in);
      this->Portal.Set(index, in);
    }
  };

public:
  VTKM_EXEC_CONT
  const PortalType& GetPortal() const { return this->Portal; }

private:
  PortalType Portal;
}; // class ArrayPortalSwizzle

} // namespace internal

template <typename ArrayHandleType, vtkm::IdComponent... ComponentMap>
class StorageTagSwizzle
{
};

namespace internal
{

template <typename ArrayHandleType, vtkm::IdComponent... ComponentMap>
class Storage<typename ArrayHandleSwizzleTraits<typename ArrayHandleType::ValueType,
                                                ComponentMap...>::OutputType,
              StorageTagSwizzle<ArrayHandleType, ComponentMap...>>
{
public:
  using PortalType = ArrayPortalSwizzle<typename ArrayHandleType::PortalControl, ComponentMap...>;
  using PortalConstType =
    ArrayPortalSwizzle<typename ArrayHandleType::PortalConstControl, ComponentMap...>;
  using ValueType = typename PortalType::ValueType;

  VTKM_CONT
  Storage()
    : Valid(false)
  {
  }

  VTKM_CONT
  Storage(const ArrayHandleType& array)
    : Array(array)
    , Valid(true)
  {
  }

  VTKM_CONT
  PortalConstType GetPortalConst() const
  {
    VTKM_ASSERT(this->Valid);
    return PortalConstType(this->Array.GetPortalConstControl());
  }

  VTKM_CONT
  PortalType GetPortal()
  {
    VTKM_ASSERT(this->Valid);
    return PortalType(this->Array.GetPortalControl());
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const
  {
    VTKM_ASSERT(this->Valid);
    return this->Array.GetNumberOfValues();
  }

  VTKM_CONT
  void Allocate(vtkm::Id numberOfValues)
  {
    VTKM_ASSERT(this->Valid);
    this->Array.Allocate(numberOfValues);
  }

  VTKM_CONT
  void Shrink(vtkm::Id numberOfValues)
  {
    VTKM_ASSERT(this->Valid);
    this->Array.Shrink(numberOfValues);
  }

  VTKM_CONT
  void ReleaseResources()
  {
    VTKM_ASSERT(this->Valid);
    this->Array.ReleaseResources();
  }

  VTKM_CONT
  const ArrayHandleType& GetArray() const
  {
    VTKM_ASSERT(this->Valid);
    return this->Array;
  }

private:
  ArrayHandleType Array;
  bool Valid;
}; // class Storage

template <typename ArrayHandleType, vtkm::IdComponent... ComponentMap, typename Device>
class ArrayTransfer<typename ArrayHandleSwizzleTraits<typename ArrayHandleType::ValueType,
                                                      ComponentMap...>::OutputType,
                    StorageTagSwizzle<ArrayHandleType, ComponentMap...>,
                    Device>
{
  using ArrayExecutionTypes = typename ArrayHandleType::template ExecutionTypes<Device>;
  using StorageTag = StorageTagSwizzle<ArrayHandleType, ComponentMap...>;

public:
  using SwizzleTraits =
    ArrayHandleSwizzleTraits<typename ArrayHandleType::ValueType, ComponentMap...>;
  using ValueType = typename SwizzleTraits::OutputType;

private:
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  using PortalExecution = ArrayPortalSwizzle<typename ArrayExecutionTypes::Portal, ComponentMap...>;
  using PortalConstExecution =
    ArrayPortalSwizzle<typename ArrayExecutionTypes::PortalConst, ComponentMap...>;

  VTKM_CONT
  ArrayTransfer(StorageType* storage)
    : Array(storage->GetArray())
  {
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return this->Array.GetNumberOfValues(); }

  VTKM_CONT
  PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData))
  {
    return PortalConstExecution(this->Array.PrepareForInput(Device()));
  }

  VTKM_CONT
  PortalExecution PrepareForInPlace(bool vtkmNotUsed(updateData))
  {
    return PortalExecution(this->Array.PrepareForInPlace(Device()));
  }

  VTKM_CONT
  PortalExecution PrepareForOutput(vtkm::Id numberOfValues)
  {
    return PortalExecution(this->Array.PrepareForOutput(numberOfValues, Device()));
  }

  VTKM_CONT
  void RetrieveOutputData(StorageType* vtkmNotUsed(storage)) const
  {
    // Implementation of this method should be unnecessary. The internal
    // array handle should automatically retrieve the output data as
    // necessary.
  }

  VTKM_CONT
  void Shrink(vtkm::Id numberOfValues) { this->Array.Shrink(numberOfValues); }

  VTKM_CONT
  void ReleaseResources() { this->Array.ReleaseResourcesExecution(); }

private:
  ArrayHandleType Array;
};
}
}
} // namespace vtkm::cont::internal

namespace vtkm
{
namespace cont
{

/// \brief A fancy ArrayHandle that rearranges and/or removes components of an
/// ArrayHandle with a vtkm::Vec ValueType.
///
/// ArrayHandleSwizzle is a specialization of ArrayHandle. It takes an
/// input ArrayHandle with a vtkm::Vec ValueType and a compile-time component
/// map and uses this information to create a new array consisting of the
/// specified components of the input ArrayHandle in the specified order. So for
/// a given index i, ArrayHandleSwizzle looks up the i-th vtkm::Vec in
/// the index array and reads or writes to the specified components, leaving all
/// other components unmodified. This is done on the fly rather than creating a
/// copy of the array.
template <typename ArrayHandleType, vtkm::IdComponent... ComponentMap>
class ArrayHandleSwizzle : public vtkm::cont::ArrayHandle<
                             typename ArrayHandleSwizzleTraits<typename ArrayHandleType::ValueType,
                                                               ComponentMap...>::OutputType,
                             StorageTagSwizzle<ArrayHandleType, ComponentMap...>>
{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleSwizzle,
    (ArrayHandleSwizzle<ArrayHandleType, ComponentMap...>),
    (vtkm::cont::ArrayHandle<typename ArrayHandleSwizzleTraits<typename ArrayHandleType::ValueType,
                                                               ComponentMap...>::OutputType,
                             StorageTagSwizzle<ArrayHandleType, ComponentMap...>>));

  using SwizzleTraits =
    ArrayHandleSwizzleTraits<typename ArrayHandleType::ValueType, ComponentMap...>;

protected:
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  VTKM_CONT
  ArrayHandleSwizzle(const ArrayHandleType& array)
    : Superclass(StorageType(array))
  {
  }
};

/// make_ArrayHandleSwizzle is convenience function to generate an
/// ArrayHandleSwizzle.
template <vtkm::IdComponent... ComponentMap, typename ArrayHandleType>
VTKM_CONT ArrayHandleSwizzle<ArrayHandleType, ComponentMap...> make_ArrayHandleSwizzle(
  const ArrayHandleType& array)
{
  return ArrayHandleSwizzle<ArrayHandleType, ComponentMap...>(array);
}
}
} // namespace vtkm::cont

#endif // vtk_m_cont_ArrayHandleSwizzle_h
