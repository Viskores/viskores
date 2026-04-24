//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_python_bindings_pyviskores_bindings_h
#define viskores_python_bindings_pyviskores_bindings_h

#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>

#include <functional>
#include <memory>
#include <stdexcept>

namespace nb = nanobind;

namespace viskores::python::bindings
{

void SetPythonError(const std::exception& error);

nb::object UnknownArrayToNumPyArray(const viskores::cont::UnknownArrayHandle& array);
nb::object FieldToNumPyArray(const viskores::cont::Field& field);
viskores::cont::UnknownArrayHandle NumPyArrayToUnknownArray(nb::handle object);
std::vector<viskores::Float64> ParseIsoValues(nb::handle object);
nb::object IdArrayToNumPy(const viskores::cont::ArrayHandle<viskores::Id>& array);
nb::object VectorToNumPy(const std::vector<viskores::FloatDefault>& values);
viskores::cont::ArrayHandle<viskores::Id3> ParseId3Array(nb::handle object);
nb::object CreateId3ArrayObject(const viskores::cont::ArrayHandle<viskores::Id3>& array);
viskores::cont::DataSet MakeGhostCellDataSetImpl(const std::string& datasetType,
                                               viskores::Id nx,
                                               viskores::Id ny,
                                               viskores::Id nz,
                                               int numLayers,
                                               const std::string& ghostName,
                                               bool addMidGhost);
#if VISKORES_PYTHON_ENABLE_FILTER_SCALAR_TOPOLOGY
nb::object EdgePairArrayToNumPy(
  const viskores::worklet::contourtree_augmented::EdgePairArray& saddlePeak);
viskores::Id3 ComputeNumberOfBlocksPerAxis(viskores::Id3 globalSize, viskores::Id numberOfBlocks);
std::tuple<viskores::Id3, viskores::Id3, viskores::Id3> ComputeBlockExtents(
  viskores::Id3 globalSize,
  viskores::Id3 blocksPerDim,
  viskores::Id blockNo);
viskores::cont::DataSet CreateSubDataSet(const viskores::cont::DataSet& ds,
                                         viskores::Id3 blockOrigin,
                                         viskores::Id3 blockSize,
                                         const std::string& fieldName);
#endif

viskores::cont::Field::Association ParseAssociation(
  nb::handle object,
  viskores::cont::Field::Association defaultValue = viskores::cont::Field::Association::Any);
viskores::Id3 ParseDimensions(nb::handle object);
viskores::Vec3f ParseVec3(nb::handle object, const viskores::Vec3f& defaultValue);
viskores::RangeId3 ParseRangeId3(nb::handle object);
viskores::Range ParseRange(
  nb::handle object,
  const viskores::Range& defaultValue = viskores::Range());
viskores::Vec3f_32 ParseColorTableColor(nb::handle object);
viskores::rendering::Color ParseColor(
  nb::handle object,
  const viskores::rendering::Color& defaultValue = viskores::rendering::Color(0, 0, 0, 1));
#if VISKORES_PYTHON_ENABLE_FILTER_ENTITY_EXTRACTION
bool ParseImplicitFunction(nb::handle object, viskores::ImplicitFunctionGeneral& function);
#endif

viskores::cont::DataSet* RequireDataSet(nb::handle object);
viskores::cont::PartitionedDataSet* RequirePartitionedDataSet(nb::handle object);
std::shared_ptr<viskores::cont::ColorTable> RequireColorTable(nb::handle object);
nb::object WrapDataSet(const viskores::cont::DataSet& dataSet);
nb::object WrapPartitionedDataSet(const viskores::cont::PartitionedDataSet& dataSet);

template <typename FilterType>
nb::object ExecuteFilterOnPythonDataObject(FilterType& filter, nb::handle dataObject)
{
  viskores::cont::DataSet* dataSet = nullptr;
  if (nb::try_cast(dataObject, dataSet))
  {
    return WrapDataSet(filter.Execute(*dataSet));
  }

  viskores::cont::PartitionedDataSet* partitionedDataSet = nullptr;
  if (nb::try_cast(dataObject, partitionedDataSet))
  {
    return WrapPartitionedDataSet(filter.Execute(*partitionedDataSet));
  }

  throw std::runtime_error("Expected a viskores.DataSet or viskores.PartitionedDataSet instance.");
}

template <typename FilterType>
nb::object ExecuteFilterToPython(FilterType& filter, nb::handle dataObject)
{
  return ExecuteFilterOnPythonDataObject<FilterType>(filter, dataObject);
}

void RegisterNanobindSharedDataClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindTestingClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindSourceClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindIOClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindFieldConversionClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindVectorAnalysisClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindContourClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindFieldTransformClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindEntityExtractionClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindAdditionalFilterClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindDensityEstimateClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindGeometryRefinementClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindImageProcessingClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindResamplingClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindScalarTopologyClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindHelperFunctions(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindInteropClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindColorTableClass(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindCameraClass(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindRenderingClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindImplicitFunctionClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindCompatibilityFunctions(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindModuleConstants(nb::module_& m);
void RegisterNanobindModule(nb::module_& m);

#if VISKORES_PYTHON_ENABLE_RENDERING
viskores::rendering::Camera* RequireCamera(nb::handle object);
viskores::rendering::Mapper* RequireMapper(nb::handle object);
#endif

#if VISKORES_PYTHON_ENABLE_INTEROP
struct NanobindBufferStateHolder
{
  GLuint HandleStorage = 0;
  std::shared_ptr<viskores::interop::BufferState> State;

  NanobindBufferStateHolder();
  explicit NanobindBufferStateHolder(unsigned long handle);
  NanobindBufferStateHolder(unsigned long handle, unsigned long bufferType);
  unsigned long GetHandle();
  void SetHandle(unsigned long handle);
  bool HasType() const;
  unsigned long GetType() const;
  void SetType(unsigned long bufferType);
  long long GetSize() const;
  long long GetCapacity() const;
  std::string Repr() const;
};

void TransferUnknownArrayToOpenGL(const viskores::cont::UnknownArrayHandle& array,
                                  viskores::interop::BufferState& state);
#endif

} // namespace viskores::python::bindings

#endif
