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
  nb::capsule owner(heapValues, [](void* ptr) noexcept {
    delete static_cast<std::vector<ComponentType>*>(ptr);
  });

  nb::ndarray<nb::numpy, ComponentType, nb::c_contig> array(
    heapValues->data(), shape, owner);
  return array.cast();
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
    return CreateOwnedNumPyArray(std::move(outputData),
                                 { static_cast<size_t>(numberOfValues) });
  }

  std::vector<ComponentType> outputData(
    static_cast<size_t>(numberOfValues * numberOfComponents));
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

nb::object CreateId3ArrayObject(const viskores::cont::ArrayHandle<viskores::Id3>& array)
{
  viskores::cont::UnknownArrayHandle unknown(array);
  return CopyUnknownArrayToNumPy<viskores::Id>(unknown);
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
  if (!nb::isinstance<nb::sequence>(object) || nb::isinstance<nb::str>(object))
  {
    throw std::runtime_error("dimensions must be a sequence of 1, 2, or 3 integers.");
  }

  nb::sequence sequence = nb::borrow<nb::sequence>(object);
  const size_t size = static_cast<size_t>(nb::len(sequence));
  if ((size < 1) || (size > 3))
  {
    throw std::runtime_error("dimensions must contain 1, 2, or 3 integers.");
  }

  viskores::Id3 dims(1, 1, 1);
  for (size_t index = 0; index < size; ++index)
  {
    dims[static_cast<viskores::IdComponent>(index)] =
      static_cast<viskores::Id>(nb::cast<long long>(sequence[index]));
  }
  return dims;
}

viskores::Vec3f ParseVec3(nb::handle object, const viskores::Vec3f& defaultValue)
{
  if (!object.is_valid() || object.is_none())
  {
    return defaultValue;
  }

  if (!nb::isinstance<nb::sequence>(object) || nb::isinstance<nb::str>(object))
  {
    throw std::runtime_error("Expected a sequence of 1, 2, or 3 floats.");
  }

  nb::sequence sequence = nb::borrow<nb::sequence>(object);
  const size_t size = static_cast<size_t>(nb::len(sequence));
  if ((size < 1) || (size > 3))
  {
    throw std::runtime_error("Expected a sequence of 1, 2, or 3 floats.");
  }

  viskores::Vec3f value = defaultValue;
  for (size_t index = 0; index < size; ++index)
  {
    value[static_cast<viskores::IdComponent>(index)] =
      static_cast<viskores::FloatDefault>(nb::cast<double>(sequence[index]));
  }
  return value;
}

viskores::RangeId3 ParseRangeId3(nb::handle object)
{
  if (!nb::isinstance<nb::sequence>(object) || nb::isinstance<nb::str>(object))
  {
    throw std::runtime_error("Expected a sequence of 6 integers.");
  }

  nb::sequence sequence = nb::borrow<nb::sequence>(object);
  if (nb::len(sequence) != 6)
  {
    throw std::runtime_error("Expected a sequence of 6 integers.");
  }

  viskores::Id values[6];
  for (size_t index = 0; index < 6; ++index)
  {
    values[index] = static_cast<viskores::Id>(nb::cast<long long>(sequence[index]));
  }
  return viskores::RangeId3(values);
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

  if (!nb::isinstance<nb::sequence>(object) || nb::isinstance<nb::str>(object))
  {
    throw std::runtime_error("Expected a sequence of 2 floats.");
  }

  nb::sequence sequence = nb::borrow<nb::sequence>(object);
  if (nb::len(sequence) != 2)
  {
    throw std::runtime_error("Expected a sequence of 2 floats.");
  }

  const auto minValue = nb::cast<double>(sequence[0]);
  const auto maxValue = nb::cast<double>(sequence[1]);
  return viskores::Range(minValue, maxValue);
}

#if VISKORES_PYTHON_ENABLE_FILTER_ENTITY_EXTRACTION
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

viskores::Id2 ParseSize2D(nb::handle object, const viskores::Id2& defaultValue = viskores::Id2(1024, 1024))
{
  if (!object.is_valid() || object.is_none())
  {
    return defaultValue;
  }

  if (!nb::isinstance<nb::sequence>(object) || nb::isinstance<nb::str>(object))
  {
    throw std::runtime_error("Expected a sequence of 2 integers.");
  }

  nb::sequence sequence = nb::borrow<nb::sequence>(object);
  if (nb::len(sequence) != 2)
  {
    throw std::runtime_error("Expected a sequence of 2 integers.");
  }

  viskores::Id2 dims;
  for (size_t index = 0; index < 2; ++index)
  {
    dims[static_cast<viskores::IdComponent>(index)] =
      static_cast<viskores::Id>(nb::cast<long long>(sequence[index]));
  }
  return dims;
}

template <typename VecType, typename ComponentType>
viskores::cont::UnknownArrayHandle BuildVectorArray(const ComponentType* rawData,
                                                    size_t numberOfTuples,
                                                    size_t numberOfComponents)
{
  std::vector<VecType> values(numberOfTuples);
  for (size_t tupleIndex = 0; tupleIndex < numberOfTuples; ++tupleIndex)
  {
    for (size_t componentIndex = 0; componentIndex < numberOfComponents; ++componentIndex)
    {
      values[static_cast<std::size_t>(tupleIndex)][static_cast<viskores::IdComponent>(componentIndex)] =
        static_cast<ComponentType>(rawData[(tupleIndex * numberOfComponents) + componentIndex]);
    }
  }
  return viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On);
}

viskores::cont::UnknownArrayHandle NumPyArrayToUnknownArray(nb::handle object)
{
  using AnyArray = nb::ndarray<nb::ro, nb::numpy, nb::c_contig, nb::device::cpu>;
  AnyArray array = nb::cast<AnyArray>(object);

  const auto dtype = array.dtype();
  const auto code = static_cast<nb::dlpack::dtype_code>(dtype.code);
  const auto bits = dtype.bits;
  const auto lanes = dtype.lanes;
  const bool isFloat32 = (code == nb::dlpack::dtype_code::Float) && (bits == 32) && (lanes == 1);
  const bool isFloat64 = (code == nb::dlpack::dtype_code::Float) && (bits == 64) && (lanes == 1);
  const bool isInt32 = (code == nb::dlpack::dtype_code::Int) && (bits == 32) && (lanes == 1);
  const bool isUInt32 = (code == nb::dlpack::dtype_code::UInt) && (bits == 32) && (lanes == 1);
  const bool isInt64 = (code == nb::dlpack::dtype_code::Int) && (bits == 64) && (lanes == 1);
  const bool isUInt64 = (code == nb::dlpack::dtype_code::UInt) && (bits == 64) && (lanes == 1);
  const bool isUInt8 = (code == nb::dlpack::dtype_code::UInt) && (bits == 8) && (lanes == 1);

  const auto loadValue = [&](size_t linearIndex) -> viskores::FloatDefault {
    if (isFloat32)
    {
      return static_cast<viskores::FloatDefault>(
        reinterpret_cast<const float*>(array.data())[linearIndex]);
    }
    return static_cast<viskores::FloatDefault>(
      reinterpret_cast<const double*>(array.data())[linearIndex]);
  };

  viskores::cont::UnknownArrayHandle result;
  if (array.ndim() == 1)
  {
    const auto size = array.shape(0);
    if (isFloat32 || isFloat64)
    {
      std::vector<viskores::FloatDefault> values(size);
      for (size_t index = 0; index < size; ++index)
      {
        values[index] = loadValue(index);
      }
      result = viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On);
    }
    else if (isUInt8)
    {
      const auto* begin = reinterpret_cast<const viskores::UInt8*>(array.data());
      std::vector<viskores::UInt8> values(begin, begin + size);
      result = viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On);
    }
    else if (isInt32)
    {
      const auto* begin = reinterpret_cast<const viskores::Int32*>(array.data());
      std::vector<viskores::Int32> values(begin, begin + size);
      result = viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On);
    }
    else if (isUInt32)
    {
      const auto* begin = reinterpret_cast<const viskores::UInt32*>(array.data());
      std::vector<viskores::UInt32> values(begin, begin + size);
      result = viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On);
    }
    else if (isInt64)
    {
      const auto* begin = reinterpret_cast<const viskores::Int64*>(array.data());
      std::vector<viskores::Int64> values(begin, begin + size);
      result = viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On);
    }
    else if (isUInt64)
    {
      const auto* begin = reinterpret_cast<const viskores::UInt64*>(array.data());
      std::vector<viskores::UInt64> values(begin, begin + size);
      result = viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On);
    }
    else
    {
      throw std::runtime_error(
        "Only float32, float64, uint8, int32, uint32, int64, and uint64 1D arrays are currently supported.");
    }
  }
  else if (array.ndim() == 2)
  {
    const size_t numberOfTuples = array.shape(0);
    const size_t numberOfComponents = array.shape(1);
    if (isFloat32 || isFloat64)
    {
      std::vector<viskores::FloatDefault> flatValues(
        numberOfTuples * numberOfComponents);
      for (size_t index = 0; index < (numberOfTuples * numberOfComponents); ++index)
      {
        flatValues[index] = loadValue(index);
      }
      switch (numberOfComponents)
      {
        case 2:
          result = BuildVectorArray<viskores::Vec2f, viskores::FloatDefault>(
            flatValues.data(), numberOfTuples, numberOfComponents);
          break;
        case 3:
          result = BuildVectorArray<viskores::Vec3f, viskores::FloatDefault>(
            flatValues.data(), numberOfTuples, numberOfComponents);
          break;
        case 4:
          result = BuildVectorArray<viskores::Vec4f, viskores::FloatDefault>(
            flatValues.data(), numberOfTuples, numberOfComponents);
          break;
        default:
          throw std::runtime_error("Only 1D arrays or Nx{2,3,4} arrays are currently supported.");
      }
    }
    else if (isUInt8)
    {
      const auto* rawData = reinterpret_cast<const viskores::UInt8*>(array.data());
      switch (numberOfComponents)
      {
        case 2:
          result = BuildVectorArray<viskores::Vec2ui_8, viskores::UInt8>(
            rawData, numberOfTuples, numberOfComponents);
          break;
        case 3:
          result = BuildVectorArray<viskores::Vec3ui_8, viskores::UInt8>(
            rawData, numberOfTuples, numberOfComponents);
          break;
        case 4:
          result = BuildVectorArray<viskores::Vec4ui_8, viskores::UInt8>(
            rawData, numberOfTuples, numberOfComponents);
          break;
        default:
          throw std::runtime_error("Only 1D arrays or Nx{2,3,4} arrays are currently supported.");
      }
    }
    else
    {
      throw std::runtime_error(
        "Only float32, float64, and uint8 Nx{2,3,4} arrays are currently supported.");
    }
  }
  else
  {
    throw std::runtime_error("Only 1D arrays or 2D arrays can be added as fields.");
  }
  return result;
}

nb::object UnknownArrayToNumPyArray(const viskores::cont::UnknownArrayHandle& defaultArray)
{
  if (defaultArray.IsType<viskores::cont::ArrayHandle<viskores::Pair<viskores::Id, viskores::Id>>>())
  {
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
    return CreateOwnedNumPyArray(
      std::move(outputData),
      { static_cast<size_t>(pairArray.GetNumberOfValues()), 2 });
  }
  if (defaultArray.IsBaseComponentType<viskores::Float64>())
  {
    return CopyUnknownArrayToNumPy<viskores::Float64>(defaultArray);
  }
  if (defaultArray.IsBaseComponentType<viskores::Float32>())
  {
    return CopyUnknownArrayToNumPy<viskores::Float32>(defaultArray);
  }
  if (defaultArray.IsBaseComponentType<viskores::Id>())
  {
    return CopyUnknownArrayToNumPy<viskores::Id>(defaultArray);
  }
  if (defaultArray.IsBaseComponentType<viskores::Int32>())
  {
    return CopyUnknownArrayToNumPy<viskores::Int32>(defaultArray);
  }
  if (defaultArray.IsBaseComponentType<viskores::UInt32>())
  {
    return CopyUnknownArrayToNumPy<viskores::UInt32>(defaultArray);
  }
  if (defaultArray.IsBaseComponentType<viskores::UInt64>())
  {
    return CopyUnknownArrayToNumPy<viskores::UInt64>(defaultArray);
  }
  if (defaultArray.IsBaseComponentType<viskores::UInt8>())
  {
    return CopyUnknownArrayToNumPy<viskores::UInt8>(defaultArray);
  }
  throw std::runtime_error("Unsupported field component type for NumPy export.");
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
  return CreateOwnedNumPyArray(
    std::vector<viskores::FloatDefault>(values.begin(), values.end()),
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
      values[row][static_cast<viskores::IdComponent>(col)] =
        isInt64
          ? static_cast<viskores::Id>(
              reinterpret_cast<const long long*>(array.data())[linearIndex])
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
  if (!nb::isinstance<nb::sequence>(object) || nb::isinstance<nb::str>(object))
  {
    throw std::runtime_error("Expected a sequence of 3 or 4 numeric values.");
  }

  nb::sequence sequence = nb::borrow<nb::sequence>(object);
  const size_t size = static_cast<size_t>(nb::len(sequence));
  if ((size < 3) || (size > 4))
  {
    throw std::runtime_error("Expected a sequence of 3 or 4 numeric values.");
  }

  viskores::Vec3f_32 value(0.0f, 0.0f, 0.0f);
  for (size_t index = 0; index < 3; ++index)
  {
    value[static_cast<viskores::IdComponent>(index)] =
      static_cast<viskores::Float32>(nb::cast<double>(sequence[index]));
  }
  return value;
}

#endif

#if VISKORES_PYTHON_ENABLE_TESTING_UTILS
viskores::cont::ArrayHandle<viskores::UInt8> MakeStructuredGhostCellArray(viskores::Id nx,
                                                                        viskores::Id ny,
                                                                        viskores::Id nz,
                                                                        int numLayers,
                                                                        bool addMidGhost = false)
{
  viskores::Id numCells = nx * ny;
  if (nz > 0)
  {
    numCells *= nz;
  }

  constexpr viskores::UInt8 normalCell = viskores::CellClassification::Normal;
  constexpr viskores::UInt8 duplicateCell = viskores::CellClassification::Ghost;

  viskores::cont::ArrayHandle<viskores::UInt8> ghosts;
  ghosts.Allocate(numCells);
  auto portal = ghosts.WritePortal();
  for (viskores::Id i = 0; i < numCells; ++i)
  {
    portal.Set(i, (numLayers == 0) ? normalCell : duplicateCell);
  }

  if (numLayers > 0)
  {
    if (nz == 0)
    {
      for (viskores::Id i = numLayers; i < nx - numLayers; ++i)
      {
        for (viskores::Id j = numLayers; j < ny - numLayers; ++j)
        {
          portal.Set(j * nx + i, normalCell);
        }
      }
    }
    else
    {
      for (viskores::Id i = numLayers; i < nx - numLayers; ++i)
      {
        for (viskores::Id j = numLayers; j < ny - numLayers; ++j)
        {
          for (viskores::Id k = numLayers; k < nz - numLayers; ++k)
          {
            portal.Set(k * nx * ny + j * nx + i, normalCell);
          }
        }
      }
    }
  }

  if (addMidGhost)
  {
    if (nz == 0)
    {
      viskores::Id mi = numLayers + (nx - numLayers) / 2;
      viskores::Id mj = numLayers + (ny - numLayers) / 2;
      portal.Set(mj * nx + mi, duplicateCell);
    }
    else
    {
      viskores::Id mi = numLayers + (nx - numLayers) / 2;
      viskores::Id mj = numLayers + (ny - numLayers) / 2;
      viskores::Id mk = numLayers + (nz - numLayers) / 2;
      portal.Set(mk * nx * ny + mj * nx + mi, duplicateCell);
    }
  }
  return ghosts;
}

template <class CellSetType, viskores::IdComponent NDIM>
void MakeExplicitCells(const CellSetType& cellSet,
                         viskores::Vec<viskores::Id, NDIM>& dims,
                         viskores::cont::ArrayHandle<viskores::IdComponent>& numIndices,
                         viskores::cont::ArrayHandle<viskores::UInt8>& shapes,
                         viskores::cont::ArrayHandle<viskores::Id>& conn)
{
  using Connectivity = viskores::internal::ConnectivityStructuredInternals<NDIM>;

  viskores::Id numberOfCells = cellSet.GetNumberOfCells();
  viskores::Id connectivityLength = (NDIM == 2 ? numberOfCells * 4 : numberOfCells * 8);

  conn.Allocate(connectivityLength);
  shapes.Allocate(numberOfCells);
  numIndices.Allocate(numberOfCells);

  Connectivity structured;
  structured.SetPointDimensions(dims);

  viskores::Id index = 0;
  for (viskores::Id cellIndex = 0; cellIndex < numberOfCells; ++cellIndex)
  {
    auto pointIds = structured.GetPointsOfCell(cellIndex);
    for (viskores::IdComponent component = 0; component < pointIds.GetNumberOfComponents(); ++component, ++index)
    {
      conn.WritePortal().Set(index, pointIds[component]);
    }

    shapes.WritePortal().Set(
      cellIndex, (NDIM == 2 ? viskores::CELL_SHAPE_QUAD : viskores::CELL_SHAPE_HEXAHEDRON));
    numIndices.WritePortal().Set(cellIndex, (NDIM == 2 ? 4 : 8));
  }
}

viskores::cont::DataSet MakeGhostCellDataSetImpl(const std::string& datasetType,
                                               viskores::Id nx,
                                               viskores::Id ny,
                                               viskores::Id nz,
                                               int numLayers,
                                               const std::string& ghostName,
                                               bool addMidGhost)
{
  auto applyGhostField = [&](viskores::cont::DataSet& dataSet) {
    auto ghosts = MakeStructuredGhostCellArray(nx, ny, nz, numLayers, addMidGhost);
    if (ghostName == "default")
    {
      dataSet.SetGhostCellField(ghosts);
    }
    else
    {
      dataSet.SetGhostCellField(ghostName, ghosts);
    }
  };

  if (datasetType == "uniform")
  {
    viskores::cont::DataSet dataSet =
      (nz == 0) ? viskores::cont::DataSetBuilderUniform::Create(viskores::Id2(nx + 1, ny + 1))
                : viskores::cont::DataSetBuilderUniform::Create(viskores::Id3(nx + 1, ny + 1, nz + 1));
    applyGhostField(dataSet);
    return dataSet;
  }

  if (datasetType == "rectilinear")
  {
    std::vector<float> x(static_cast<std::size_t>(nx + 1));
    std::vector<float> y(static_cast<std::size_t>(ny + 1));
    for (std::size_t index = 0; index < x.size(); ++index)
    {
      x[index] = static_cast<float>(index);
    }
    for (std::size_t index = 0; index < y.size(); ++index)
    {
      y[index] = static_cast<float>(index);
    }

    viskores::cont::DataSet dataSet;
    if (nz == 0)
    {
      dataSet = viskores::cont::DataSetBuilderRectilinear::Create(x, y);
    }
    else
    {
      std::vector<float> z(static_cast<std::size_t>(nz + 1));
      for (std::size_t index = 0; index < z.size(); ++index)
      {
        z[index] = static_cast<float>(index);
      }
      dataSet = viskores::cont::DataSetBuilderRectilinear::Create(x, y, z);
    }
    applyGhostField(dataSet);
    return dataSet;
  }

  if (datasetType == "explicit")
  {
    viskores::cont::DataSet uniformDataSet =
      MakeGhostCellDataSetImpl("uniform", nx, ny, nz, numLayers, ghostName, false);

    using CoordType = viskores::Vec3f_32;
    auto coordData = uniformDataSet.GetCoordinateSystem(0).GetDataAsMultiplexer();
    viskores::Id numberOfPoints = coordData.GetNumberOfValues();
    viskores::cont::ArrayHandle<CoordType> explicitCoords;
    explicitCoords.Allocate(numberOfPoints);
    auto explicitPortal = explicitCoords.WritePortal();
    auto coordPortal = coordData.ReadPortal();
    for (viskores::Id index = 0; index < numberOfPoints; ++index)
    {
      explicitPortal.Set(index, coordPortal.Get(index));
    }

    viskores::cont::UnknownCellSet cellSet = uniformDataSet.GetCellSet();
    viskores::cont::ArrayHandle<viskores::Id> conn;
    viskores::cont::ArrayHandle<viskores::IdComponent> numIndices;
    viskores::cont::ArrayHandle<viskores::UInt8> shapes;
    viskores::cont::DataSet dataSet;

    if (cellSet.IsType<viskores::cont::CellSetStructured<2>>())
    {
      viskores::Id2 dims(nx, ny);
      MakeExplicitCells(
        cellSet.AsCellSet<viskores::cont::CellSetStructured<2>>(), dims, numIndices, shapes, conn);
      dataSet = viskores::cont::DataSetBuilderExplicit::Create(
        explicitCoords, viskores::CellShapeTagQuad(), 4, conn, "coordinates");
    }
    else if (cellSet.IsType<viskores::cont::CellSetStructured<3>>())
    {
      viskores::Id3 dims(nx, ny, nz);
      MakeExplicitCells(
        cellSet.AsCellSet<viskores::cont::CellSetStructured<3>>(), dims, numIndices, shapes, conn);
      dataSet = viskores::cont::DataSetBuilderExplicit::Create(
        explicitCoords, viskores::CellShapeTagHexahedron(), 8, conn, "coordinates");
    }
    else
    {
      throw std::runtime_error("Unable to create explicit ghost test dataset from non-structured input.");
    }

    applyGhostField(dataSet);
    return dataSet;
  }

  throw std::runtime_error("dataset_type must be one of: uniform, rectilinear, explicit.");
}

#endif
