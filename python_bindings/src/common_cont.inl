//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

using Association = viskores::cont::Field::Association;

void SetPythonError(const std::exception& error)
{
  throw std::runtime_error(error.what());
}

template <typename ComponentType>
nb::object CreateOwnedNumPyArray(std::vector<ComponentType>&& values,
                                 std::initializer_list<size_t> shape)
{
  auto* heapValues = new std::vector<ComponentType>(std::move(values));
  nb::capsule owner(
    heapValues, [](void* ptr) noexcept { delete static_cast<std::vector<ComponentType>*>(ptr); });

  nb::ndarray<nb::numpy, ComponentType, nb::c_contig> array(heapValues->data(), shape, owner);
  return array.cast();
}

template <typename ValueType, typename ComponentType = ValueType>
struct BasicArrayViewOwner
{
  viskores::cont::ArrayHandleBasic<ValueType> Array;
  viskores::cont::Token Token;
  const ComponentType* Data = nullptr;

  explicit BasicArrayViewOwner(const viskores::cont::ArrayHandleBasic<ValueType>& array)
    : Array(array)
    , Data(reinterpret_cast<const ComponentType*>(this->Array.GetReadPointer(this->Token)))
  {
  }
};

template <typename ValueType, typename ComponentType = ValueType>
nb::object CreateBasicNumPyView(const viskores::cont::ArrayHandleBasic<ValueType>& array,
                                std::initializer_list<size_t> shape)
{
  auto* owner = new BasicArrayViewOwner<ValueType, ComponentType>(array);
  nb::capsule capsule(owner,
                      [](void* ptr) noexcept
                      { delete static_cast<BasicArrayViewOwner<ValueType, ComponentType>*>(ptr); });
  nb::ndarray<nb::numpy, const ComponentType, nb::c_contig> view(owner->Data, shape, capsule);
  return view.cast();
}

template <typename ComponentType, viskores::IdComponent NumberOfComponents>
bool TryBasicVecUnknownArrayToNumPyView(const viskores::cont::UnknownArrayHandle& array,
                                        nb::object& output)
{
  using VecType = viskores::Vec<ComponentType, NumberOfComponents>;
  using ArrayType = viskores::cont::ArrayHandleBasic<VecType>;
  if (!array.IsType<ArrayType>())
  {
    return false;
  }

  ArrayType basicArray;
  array.AsArrayHandle(basicArray);
  output = CreateBasicNumPyView<VecType, ComponentType>(
    basicArray,
    { static_cast<size_t>(array.GetNumberOfValues()),
      static_cast<size_t>(array.GetNumberOfComponentsFlat()) });
  return true;
}

template <typename ComponentType>
struct TryBasicVecUnknownArrayToNumPyViewFunctor
{
  const viskores::cont::UnknownArrayHandle& Array;
  nb::object& Output;

  template <viskores::IdComponent NumberOfComponents>
  bool operator()() const
  {
    return TryBasicVecUnknownArrayToNumPyView<ComponentType, NumberOfComponents>(
      this->Array, this->Output);
  }
};

template <typename ComponentType>
bool TryBasicUnknownArrayToNumPyView(const viskores::cont::UnknownArrayHandle& array,
                                     nb::object& output)
{
  const viskores::Id numberOfValues = array.GetNumberOfValues();
  const viskores::IdComponent numberOfComponents = array.GetNumberOfComponentsFlat();
  if (numberOfComponents <= 0)
  {
    throw std::runtime_error("Cannot export arrays with variable component counts to NumPy.");
  }

  if (numberOfComponents == 1)
  {
    using ArrayType = viskores::cont::ArrayHandleBasic<ComponentType>;
    if (array.IsType<ArrayType>())
    {
      ArrayType basicArray;
      array.AsArrayHandle(basicArray);
      output =
        CreateBasicNumPyView<ComponentType>(basicArray, { static_cast<size_t>(numberOfValues) });
      return true;
    }
    return false;
  }

  return TryCompiledVecComponentCount(
    numberOfComponents, TryBasicVecUnknownArrayToNumPyViewFunctor<ComponentType>{ array, output });
}

template <typename ComponentType>
nb::object CopyUnknownArrayToNumPy(const viskores::cont::UnknownArrayHandle& array)
{
  const viskores::Id numberOfValues = array.GetNumberOfValues();
  const viskores::IdComponent numberOfComponents = array.GetNumberOfComponentsFlat();
  if (numberOfComponents <= 0)
  {
    throw std::runtime_error("Cannot export arrays with variable component counts to NumPy.");
  }

  if (numberOfComponents == 1)
  {
    auto component = array.ExtractComponent<ComponentType>(0);
    auto portal = component.ReadPortal();
    std::vector<ComponentType> outputData(static_cast<size_t>(numberOfValues));
    for (viskores::Id index = 0; index < numberOfValues; ++index)
    {
      outputData[index] = portal.Get(index);
    }
    return CreateOwnedNumPyArray(std::move(outputData), { static_cast<size_t>(numberOfValues) });
  }

  std::vector<ComponentType> outputData(static_cast<size_t>(numberOfValues * numberOfComponents));
  for (viskores::IdComponent componentIndex = 0; componentIndex < numberOfComponents;
       ++componentIndex)
  {
    auto component = array.ExtractComponent<ComponentType>(componentIndex);
    auto portal = component.ReadPortal();
    for (viskores::Id tupleIndex = 0; tupleIndex < numberOfValues; ++tupleIndex)
    {
      outputData[(tupleIndex * numberOfComponents) + componentIndex] = portal.Get(tupleIndex);
    }
  }
  return CreateOwnedNumPyArray(
    std::move(outputData),
    { static_cast<size_t>(numberOfValues), static_cast<size_t>(numberOfComponents) });
}

template <typename ComponentType>
using BasicGroupVecVariableArray = viskores::cont::ArrayHandleGroupVecVariable<
  viskores::cont::ArrayHandle<ComponentType>,
  viskores::cont::ArrayHandle<viskores::Id>>;

template <typename ArrayType>
nb::object GroupVecVariableValueToNumPy(const ArrayType& array, viskores::Id index)
{
  using ComponentType = typename ArrayType::ComponentType;
  const viskores::Id numberOfValues = array.GetNumberOfValues();
  if (index < 0)
  {
    index += numberOfValues;
  }
  if (index < 0 || index >= numberOfValues)
  {
    throw nb::index_error("ArrayHandleGroupVecVariable index out of range.");
  }

  const auto componentsPortal = array.GetComponentsArray().ReadPortal();
  const auto offsetsPortal = array.GetOffsetsArray().ReadPortal();
  const viskores::Id begin = offsetsPortal.Get(index);
  const viskores::Id end = offsetsPortal.Get(index + 1);
  if (end < begin)
  {
    throw std::runtime_error("ArrayHandleGroupVecVariable offsets are not nondecreasing.");
  }

  std::vector<ComponentType> values(static_cast<std::size_t>(end - begin));
  for (viskores::Id component = begin; component < end; ++component)
  {
    values[static_cast<std::size_t>(component - begin)] = componentsPortal.Get(component);
  }
  return CreateOwnedNumPyArray(std::move(values), { static_cast<std::size_t>(end - begin) });
}

template <typename ArrayType>
nb::list GroupVecVariableToList(const ArrayType& array)
{
  nb::list output;
  const viskores::Id numberOfValues = array.GetNumberOfValues();
  for (viskores::Id index = 0; index < numberOfValues; ++index)
  {
    output.append(GroupVecVariableValueToNumPy(array, index));
  }
  return output;
}

template <typename ComponentType>
bool TryGroupVecVariableToPythonList(const viskores::cont::UnknownArrayHandle& array,
                                     bool copy,
                                     nb::object& output)
{
  using ArrayType = BasicGroupVecVariableArray<ComponentType>;
  if (!array.IsType<ArrayType>())
  {
    return false;
  }
  if (!copy)
  {
    throw std::runtime_error("copy=False is not supported for ArrayHandleGroupVecVariable.");
  }

  ArrayType typedArray;
  array.AsArrayHandle(typedArray);
  output = GroupVecVariableToList(typedArray);
  return true;
}

template <typename ComponentType>
bool TryGroupVecVariableValueToNumPyArray(const viskores::cont::UnknownArrayHandle& array,
                                          viskores::Id index,
                                          bool copy,
                                          nb::object& output)
{
  using ArrayType = BasicGroupVecVariableArray<ComponentType>;
  if (!array.IsType<ArrayType>())
  {
    return false;
  }
  if (!copy)
  {
    throw std::runtime_error("copy=False is not supported for ArrayHandleGroupVecVariable.");
  }

  ArrayType typedArray;
  array.AsArrayHandle(typedArray);
  output = GroupVecVariableValueToNumPy(typedArray, index);
  return true;
}

template <typename ComponentType>
bool TryUnknownArrayToNumPy(const viskores::cont::UnknownArrayHandle& array,
                            bool copy,
                            nb::object& output)
{
  if (!array.IsBaseComponentType<ComponentType>())
  {
    return false;
  }
  if (!copy)
  {
    if (TryBasicUnknownArrayToNumPyView<ComponentType>(array, output))
    {
      return true;
    }
    throw std::runtime_error(
      "copy=False requires an ArrayHandleBasic scalar or fixed-size vector array.");
  }
  output = CopyUnknownArrayToNumPy<ComponentType>(array);
  return true;
}

struct TryGroupVecVariableToPythonListFunctor
{
  const viskores::cont::UnknownArrayHandle& Array;
  bool Copy;
  nb::object& Output;

  template <typename ComponentType>
  bool operator()() const
  {
    return TryGroupVecVariableToPythonList<ComponentType>(this->Array, this->Copy, this->Output);
  }
};

struct TryGroupVecVariableValueToNumPyArrayFunctor
{
  const viskores::cont::UnknownArrayHandle& Array;
  viskores::Id Index;
  bool Copy;
  nb::object& Output;

  template <typename ComponentType>
  bool operator()() const
  {
    return TryGroupVecVariableValueToNumPyArray<ComponentType>(
      this->Array, this->Index, this->Copy, this->Output);
  }
};

struct TryUnknownArrayToNumPyFunctor
{
  const viskores::cont::UnknownArrayHandle& Array;
  bool Copy;
  nb::object& Output;

  template <typename ComponentType>
  bool operator()() const
  {
    return TryUnknownArrayToNumPy<ComponentType>(this->Array, this->Copy, this->Output);
  }
};

nb::object CreateId3ArrayObject(const viskores::cont::ArrayHandle<viskores::Id3>& array)
{
  viskores::cont::UnknownArrayHandle unknown(array);
  return CopyUnknownArrayToNumPy<viskores::Id>(unknown);
}

nb::sequence RequireSequence(nb::handle object, const char* message)
{
  if (!nb::isinstance<nb::sequence>(object) || nb::isinstance<nb::str>(object))
  {
    throw std::runtime_error(message);
  }
  return nb::borrow<nb::sequence>(object);
}

size_t RequireSequenceSize(nb::sequence sequence,
                           size_t minSize,
                           size_t maxSize,
                           const char* message)
{
  const size_t size = static_cast<size_t>(nb::len(sequence));
  if ((size < minSize) || (size > maxSize))
  {
    throw std::runtime_error(message);
  }
  return size;
}

template <typename VecType, typename ValueType, typename CastType>
std::pair<VecType, viskores::IdComponent> ParsePartialVec(nb::handle object,
                                                          const VecType& defaultValue,
                                                          size_t minSize,
                                                          size_t maxSize,
                                                          const char* message)
{
  nb::sequence sequence = RequireSequence(object, message);
  const size_t size = RequireSequenceSize(sequence, minSize, maxSize, message);

  VecType value = defaultValue;
  const size_t vectorSize = static_cast<size_t>(value.GetNumberOfComponents());
  const size_t componentsToCopy = (size < vectorSize) ? size : vectorSize;
  for (size_t index = 0; index < componentsToCopy; ++index)
  {
    value[static_cast<viskores::IdComponent>(index)] =
      static_cast<ValueType>(nb::cast<CastType>(sequence[index]));
  }
  return { value, static_cast<viskores::IdComponent>(size) };
}

template <typename ValueType, typename CastType, std::size_t Size>
std::array<ValueType, Size> ParseFixedNumericSequence(nb::handle object, const char* message)
{
  nb::sequence sequence = RequireSequence(object, message);
  RequireSequenceSize(sequence, Size, Size, message);

  std::array<ValueType, Size> values{};
  for (size_t index = 0; index < Size; ++index)
  {
    values[index] = static_cast<ValueType>(nb::cast<CastType>(sequence[index]));
  }
  return values;
}

Association ParseAssociation(nb::handle object, Association defaultValue)
{
  if (!object.is_valid() || object.is_none())
  {
    return defaultValue;
  }
  if (nb::isinstance<nb::int_>(object))
  {
    return static_cast<Association>(nb::cast<long>(object));
  }
  if (nb::isinstance<nb::str>(object))
  {
    std::string value = nb::cast<std::string>(object);
    if ((value == "any") || (value == "ANY"))
    {
      return Association::Any;
    }
    if ((value == "whole_dataset") || (value == "whole-dataset") || (value == "whole"))
    {
      return Association::WholeDataSet;
    }
    if ((value == "points") || (value == "point"))
    {
      return Association::Points;
    }
    if ((value == "cells") || (value == "cell"))
    {
      return Association::Cells;
    }
    if ((value == "partitions") || (value == "partition"))
    {
      return Association::Partitions;
    }
    if (value == "global")
    {
      return Association::Global;
    }
  }
  throw std::runtime_error("Association must be an int enum value or one of: any, points, cells.");
}

viskores::Id3 ParseDimensions(nb::handle object)
{
  return ParseDimensionsAndRank(object).first;
}

std::pair<viskores::Id3, viskores::IdComponent> ParseDimensionsAndRank(nb::handle object)
{
  return ParsePartialVec<viskores::Id3, viskores::Id, long long>(
    object, viskores::Id3(1, 1, 1), 1, 3, "dimensions must contain 1, 2, or 3 integers.");
}

viskores::Vec3f ParseVec3(nb::handle object, const viskores::Vec3f& defaultValue)
{
  if (!object.is_valid() || object.is_none())
  {
    return defaultValue;
  }

  return ParsePartialVec<viskores::Vec3f, viskores::FloatDefault, double>(
           object, defaultValue, 1, 3, "Expected a sequence of 1, 2, or 3 floats.")
    .first;
}

viskores::RangeId3 ParseRangeId3(nb::handle object)
{
  const auto values = ParseFixedNumericSequence<viskores::Id, long long, 6>(
    object, "Expected a sequence of 6 integers.");
  return viskores::RangeId3(values.data());
}

viskores::Range ParseRange(nb::handle object, const viskores::Range& defaultValue)
{
  if (!object.is_valid() || object.is_none())
  {
    return defaultValue;
  }

  viskores::Range rangeValue;
  if (nb::try_cast(object, rangeValue))
  {
    return rangeValue;
  }

  const auto values =
    ParseFixedNumericSequence<double, double, 2>(object, "Expected a sequence of 2 floats.");
  return viskores::Range(values[0], values[1]);
}

viskores::Bounds ParseBounds(nb::handle object)
{
  const auto values =
    ParseFixedNumericSequence<double, double, 6>(object, "Expected a sequence of 6 floats.");
  return viskores::Bounds(values[0], values[1], values[2], values[3], values[4], values[5]);
}

#if VISKORES_PYTHON_ENABLE_FILTER_CONTOUR || VISKORES_PYTHON_ENABLE_FILTER_ENTITY_EXTRACTION
bool ParseImplicitFunction(nb::handle object, viskores::ImplicitFunctionGeneral& function)
{
  viskores::Box box;
  if (nb::try_cast(object, box))
  {
    function = box;
    return true;
  }

  viskores::Cylinder cylinder;
  if (nb::try_cast(object, cylinder))
  {
    function = cylinder;
    return true;
  }

  viskores::Sphere sphere;
  if (nb::try_cast(object, sphere))
  {
    function = sphere;
    return true;
  }

  viskores::Plane plane;
  if (nb::try_cast(object, plane))
  {
    function = plane;
    return true;
  }

  return false;
}
#endif

viskores::Id2 ParseSize2D(nb::handle object,
                          const viskores::Id2& defaultValue = viskores::Id2(1024, 1024))
{
  if (!object.is_valid() || object.is_none())
  {
    return defaultValue;
  }

  return ParsePartialVec<viskores::Id2, viskores::Id, long long>(
           object, defaultValue, 2, 2, "Expected a sequence of 2 integers.")
    .first;
}

template <typename ValueType>
void CheckNumPyArrayAlignment(const void* data)
{
  const auto address = reinterpret_cast<std::uintptr_t>(data);
  if ((address % alignof(ValueType)) != 0)
  {
    throw std::runtime_error(
      "NumPy array data is not aligned for the requested Viskores value type.");
  }
}

template <typename ValueType>
viskores::cont::UnknownArrayHandle BuildArrayHandleFromNumPyBytes(
  const nb::ndarray<nb::ro, nb::numpy, nb::c_contig, nb::device::cpu>& array,
  nb::handle ownerObject,
  size_t numberOfValues,
  bool copy)
{
  CheckNumPyArrayAlignment<ValueType>(array.data());

  if (copy)
  {
    std::vector<ValueType> values(numberOfValues);
    const size_t numberOfBytes = sizeof(ValueType) * numberOfValues;
    if (numberOfBytes > 0)
    {
      std::memcpy(values.data(), array.data(), numberOfBytes);
    }
    return viskores::cont::make_ArrayHandleMove(std::move(values));
  }

  auto* owner = new nb::object(nb::borrow(ownerObject));
  auto* data = reinterpret_cast<ValueType*>(const_cast<void*>(array.data()));
  return viskores::cont::ArrayHandleBasic<ValueType>(
    data,
    owner,
    static_cast<viskores::Id>(numberOfValues),
    [](void* ptr) { delete static_cast<nb::object*>(ptr); },
    viskores::cont::internal::InvalidRealloc);
}

template <typename ComponentType, viskores::IdComponent NumberOfComponents>
viskores::cont::UnknownArrayHandle BuildVectorArrayFromNumPyBytes(
  const nb::ndarray<nb::ro, nb::numpy, nb::c_contig, nb::device::cpu>& array,
  nb::handle ownerObject,
  size_t numberOfTuples,
  bool copy)
{
  using VecType = viskores::Vec<ComponentType, NumberOfComponents>;
  static_assert(sizeof(VecType) == (sizeof(ComponentType) * NumberOfComponents),
                "Viskores Vec must be tightly packed for NumPy interop.");
  return BuildArrayHandleFromNumPyBytes<VecType>(array, ownerObject, numberOfTuples, copy);
}

template <typename ComponentType>
struct TryBuildVectorArrayFromNumPyBytesFunctor
{
  const nb::ndarray<nb::ro, nb::numpy, nb::c_contig, nb::device::cpu>& Array;
  nb::handle OwnerObject;
  size_t NumberOfTuples;
  bool Copy;
  viskores::cont::UnknownArrayHandle& Result;

  template <viskores::IdComponent NumberOfComponents>
  bool operator()() const
  {
    this->Result = BuildVectorArrayFromNumPyBytes<ComponentType, NumberOfComponents>(
      this->Array, this->OwnerObject, this->NumberOfTuples, this->Copy);
    return true;
  }
};

template <typename ComponentType>
bool TryNumPyArrayToUnknownArray(
  const nb::ndarray<nb::ro, nb::numpy, nb::c_contig, nb::device::cpu>& array,
  nb::handle ownerObject,
  bool copy,
  viskores::cont::UnknownArrayHandle& result)
{
  if (array.dtype() != nb::dtype<ComponentType>())
  {
    return false;
  }

  if (array.ndim() == 1)
  {
    result =
      BuildArrayHandleFromNumPyBytes<ComponentType>(array, ownerObject, array.shape(0), copy);
    return true;
  }
  if (array.ndim() == 2)
  {
    const size_t numberOfTuples = array.shape(0);
    const size_t numberOfComponents = array.shape(1);
    if (numberOfComponents == 1)
    {
      result =
        BuildArrayHandleFromNumPyBytes<ComponentType>(array, ownerObject, numberOfTuples, copy);
      return true;
    }
    if (TryCompiledVecComponentCount(
          static_cast<viskores::IdComponent>(numberOfComponents),
          TryBuildVectorArrayFromNumPyBytesFunctor<ComponentType>{
            array, ownerObject, numberOfTuples, copy, result }))
    {
      return true;
    }
    throw std::runtime_error("Only 1D arrays or Nx{1,2,3,4} arrays are currently supported.");
  }
  throw std::runtime_error("Only 1D arrays or 2D arrays can be added as fields.");
}

struct TryNumPyArrayToUnknownArrayFunctor
{
  const nb::ndarray<nb::ro, nb::numpy, nb::c_contig, nb::device::cpu>& Array;
  nb::handle OwnerObject;
  bool Copy;
  viskores::cont::UnknownArrayHandle& Result;

  template <typename ComponentType>
  bool operator()() const
  {
    return TryNumPyArrayToUnknownArray<ComponentType>(
      this->Array, this->OwnerObject, this->Copy, this->Result);
  }
};

viskores::cont::UnknownArrayHandle NumPyArrayToUnknownArray(nb::handle object, bool copy)
{
  using AnyArray = nb::ndarray<nb::ro, nb::numpy, nb::c_contig, nb::device::cpu>;
  AnyArray array = nb::cast<AnyArray>(object);

  viskores::cont::UnknownArrayHandle result;
  if (TryRegisteredScalarTypes(TryNumPyArrayToUnknownArrayFunctor{ array, object, copy, result }))
  {
    return result;
  }

  throw std::runtime_error("Only int8, uint8, int16, uint16, int32, uint32, int64, uint64, "
                           "float32, and float64 NumPy arrays are currently supported.");
}

nb::object UnknownArrayToNumPyArray(const viskores::cont::UnknownArrayHandle& defaultArray,
                                    bool copy)
{
  if (defaultArray
        .IsType<viskores::cont::ArrayHandle<viskores::Pair<viskores::Id, viskores::Id>>>())
  {
    if (!copy)
    {
      throw std::runtime_error("copy=False is not supported for viskores::Pair arrays.");
    }
    viskores::cont::ArrayHandle<viskores::Pair<viskores::Id, viskores::Id>> pairArray;
    defaultArray.AsArrayHandle(pairArray);
    const auto portal = pairArray.ReadPortal();
    std::vector<viskores::Id> outputData(static_cast<size_t>(pairArray.GetNumberOfValues() * 2));
    for (viskores::Id index = 0; index < pairArray.GetNumberOfValues(); ++index)
    {
      const auto value = portal.Get(index);
      outputData[(index * 2) + 0] = value.first;
      outputData[(index * 2) + 1] = value.second;
    }
    return CreateOwnedNumPyArray(std::move(outputData),
                                 { static_cast<size_t>(pairArray.GetNumberOfValues()), 2 });
  }

  nb::object variableComponentOutput;
  if (TryRegisteredScalarTypes(
        TryGroupVecVariableToPythonListFunctor{ defaultArray, copy, variableComponentOutput }))
  {
    return variableComponentOutput;
  }

  nb::object output;
  if (TryRegisteredScalarTypes(TryUnknownArrayToNumPyFunctor{ defaultArray, copy, output }))
  {
    return output;
  }
  throw std::runtime_error("Unsupported field component type for NumPy export.");
}

nb::object GroupVecVariableToPythonList(const viskores::cont::UnknownArrayHandle& array, bool copy)
{
  nb::object output;
  if (TryRegisteredScalarTypes(TryGroupVecVariableToPythonListFunctor{ array, copy, output }))
  {
    return output;
  }
  throw std::runtime_error("Unsupported ArrayHandleGroupVecVariable component type.");
}

nb::object GroupVecVariableValueToNumPyArray(const viskores::cont::UnknownArrayHandle& array,
                                             viskores::Id index,
                                             bool copy)
{
  nb::object output;
  if (TryRegisteredScalarTypes(
        TryGroupVecVariableValueToNumPyArrayFunctor{ array, index, copy, output }))
  {
    return output;
  }
  throw std::runtime_error("Unsupported ArrayHandleGroupVecVariable component type.");
}

nb::object FieldToNumPyArray(const viskores::cont::Field& field)
{
  return UnknownArrayToNumPyArray(field.GetData());
}

nb::object IdArrayToNumPy(const viskores::cont::ArrayHandle<viskores::Id>& array)
{
  viskores::cont::UnknownArrayHandle unknown(array);
  return UnknownArrayToNumPyArray(unknown);
}

nb::object FloatArrayToNumPy(const viskores::cont::ArrayHandle<viskores::FloatDefault>& array)
{
  viskores::cont::UnknownArrayHandle unknown(array);
  return UnknownArrayToNumPyArray(unknown);
}

nb::object VectorToNumPy(const std::vector<viskores::FloatDefault>& values)
{
  return CreateOwnedNumPyArray(std::vector<viskores::FloatDefault>(values.begin(), values.end()),
                               { values.size() });
}

viskores::cont::ArrayHandle<viskores::Id3> ParseId3Array(nb::handle object)
{
  using AnyArray = nb::ndarray<nb::ro, nb::numpy, nb::c_contig, nb::device::cpu>;
  AnyArray array = nb::cast<AnyArray>(object);

  const auto dtype = array.dtype();
  const auto code = static_cast<nb::dlpack::dtype_code>(dtype.code);
  const auto bits = dtype.bits;
  const bool isInt64 = (code == nb::dlpack::dtype_code::Int) && (bits == 64) && (dtype.lanes == 1);
  const bool isInt32 = (code == nb::dlpack::dtype_code::Int) && (bits == 32) && (dtype.lanes == 1);
  if ((!isInt64 && !isInt32) || (array.ndim() != 2) || (array.shape(1) != 3))
  {
    throw std::runtime_error("Expected a C-contiguous integer array with shape (N, 3).");
  }

  std::vector<viskores::Id3> values(array.shape(0));
  for (size_t row = 0; row < array.shape(0); ++row)
  {
    for (size_t col = 0; col < 3; ++col)
    {
      const size_t linearIndex = (row * 3) + col;
      values[row][static_cast<viskores::IdComponent>(col)] = isInt64
        ? static_cast<viskores::Id>(reinterpret_cast<const long long*>(array.data())[linearIndex])
        : static_cast<viskores::Id>(reinterpret_cast<const int*>(array.data())[linearIndex]);
    }
  }
  return viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On);
}

viskores::cont::DataSet* RequireDataSet(nb::handle object)
{
  viskores::cont::DataSet* dataSet = nullptr;
  if (nb::try_cast(object, dataSet))
  {
    return dataSet;
  }
  throw std::runtime_error("Expected a viskores.DataSet instance.");
}

viskores::cont::PartitionedDataSet* RequirePartitionedDataSet(nb::handle object)
{
  viskores::cont::PartitionedDataSet* dataSet = nullptr;
  if (nb::try_cast(object, dataSet))
  {
    return dataSet;
  }
  throw std::runtime_error("Expected a viskores.PartitionedDataSet instance.");
}

nb::object WrapDataSet(const viskores::cont::DataSet& dataSet)
{
  return nb::cast(dataSet);
}

nb::object WrapPartitionedDataSet(const viskores::cont::PartitionedDataSet& dataSet)
{
  return nb::cast(dataSet);
}

#if VISKORES_PYTHON_ENABLE_RENDERING
std::shared_ptr<viskores::cont::ColorTable> RequireColorTable(nb::handle object)
{
  std::shared_ptr<viskores::cont::ColorTable> colorTable;
  if (nb::try_cast(object, colorTable))
  {
    return colorTable;
  }
  throw std::runtime_error("Expected a viskores.cont.ColorTable instance.");
}

viskores::Vec3f_32 ParseColorTableColor(nb::handle object)
{
  return ParsePartialVec<viskores::Vec3f_32, viskores::Float32, double>(
           object,
           viskores::Vec3f_32(0.0f, 0.0f, 0.0f),
           3,
           4,
           "Expected a sequence of 3 or 4 numeric values.")
    .first;
}

#endif
