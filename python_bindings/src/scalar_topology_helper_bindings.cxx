//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include "pyviskores_common.h"
#include "pyviskores_bindings.h"

namespace viskores::python::bindings
{

#if VISKORES_PYTHON_ENABLE_FILTER_SCALAR_TOPOLOGY
nb::object EdgePairArrayToNumPy(
  const viskores::worklet::contourtree_augmented::EdgePairArray& saddlePeak)
{
  const auto portal = saddlePeak.ReadPortal();
  nb::list rows;
  for (viskores::Id index = 0; index < saddlePeak.GetNumberOfValues(); ++index)
  {
    const auto edge = portal.Get(index);
    rows.append(nb::make_tuple(edge.first, edge.second));
  }
  nb::module_ numpy = nb::module_::import_("numpy");
  return numpy.attr("array")(rows, nb::arg("dtype") = numpy.attr("int64"));
}

viskores::Id3 ComputeNumberOfBlocksPerAxis(viskores::Id3 globalSize, viskores::Id numberOfBlocks)
{
  viskores::Id powerOfTwoPortion = 1;
  while (numberOfBlocks % 2 == 0)
  {
    powerOfTwoPortion *= 2;
    numberOfBlocks /= 2;
  }

  auto findSplitAxis = [](viskores::Id3 size) {
    viskores::IdComponent splitAxis = 0;
    for (viskores::IdComponent d = 1; d < 3; ++d)
    {
      if (size[d] > size[splitAxis])
      {
        splitAxis = d;
      }
    }
    return splitAxis;
  };

  viskores::Id3 blocksPerAxis{ 1, 1, 1 };
  if (numberOfBlocks > 1)
  {
    viskores::IdComponent splitAxis = findSplitAxis(globalSize);
    blocksPerAxis[splitAxis] = numberOfBlocks;
    globalSize[splitAxis] /= numberOfBlocks;
  }

  while (powerOfTwoPortion > 1)
  {
    viskores::IdComponent splitAxis = findSplitAxis(globalSize);
    blocksPerAxis[splitAxis] *= 2;
    globalSize[splitAxis] /= 2;
    powerOfTwoPortion /= 2;
  }

  return blocksPerAxis;
}

std::tuple<viskores::Id3, viskores::Id3, viskores::Id3> ComputeBlockExtents(
  viskores::Id3 globalSize,
  viskores::Id3 blocksPerAxis,
  viskores::Id blockNo)
{
  viskores::Id3 blockIndex, blockOrigin, blockSize;
  for (viskores::IdComponent d = 0; d < 3; ++d)
  {
    blockIndex[d] = blockNo % blocksPerAxis[d];
    blockNo /= blocksPerAxis[d];

    float dx = float(globalSize[d] - 1) / float(blocksPerAxis[d]);
    blockOrigin[d] = viskores::Id(blockIndex[d] * dx);
    viskores::Id maxIdx = blockIndex[d] < blocksPerAxis[d] - 1
      ? viskores::Id((blockIndex[d] + 1) * dx)
      : globalSize[d] - 1;
    blockSize[d] = maxIdx - blockOrigin[d] + 1;
  }
  return std::make_tuple(blockIndex, blockOrigin, blockSize);
}

viskores::cont::DataSet CreateSubDataSet(const viskores::cont::DataSet& ds,
                                         viskores::Id3 blockOrigin,
                                         viskores::Id3 blockSize,
                                         const std::string& fieldName)
{
  viskores::Id3 globalSize;
  ds.GetCellSet().CastAndCallForTypes<VISKORES_DEFAULT_CELL_SET_LIST_STRUCTURED>(
    viskores::worklet::contourtree_augmented::GetPointDimensions(), globalSize);
  const viskores::Id nOutValues = blockSize[0] * blockSize[1] * blockSize[2];

  const auto inDataArrayHandle = ds.GetPointField(fieldName).GetData();
  viskores::cont::ArrayHandle<viskores::Id> copyIdsArray;
  copyIdsArray.Allocate(nOutValues);
  auto copyIdsPortal = copyIdsArray.WritePortal();

  viskores::Id3 outArrIdx;
  for (outArrIdx[2] = 0; outArrIdx[2] < blockSize[2]; ++outArrIdx[2])
    for (outArrIdx[1] = 0; outArrIdx[1] < blockSize[1]; ++outArrIdx[1])
      for (outArrIdx[0] = 0; outArrIdx[0] < blockSize[0]; ++outArrIdx[0])
      {
        viskores::Id3 inArrIdx = outArrIdx + blockOrigin;
        viskores::Id inIdx =
          (inArrIdx[2] * globalSize[1] + inArrIdx[1]) * globalSize[0] + inArrIdx[0];
        viskores::Id outIdx =
          (outArrIdx[2] * blockSize[1] + outArrIdx[1]) * blockSize[0] + outArrIdx[0];
        copyIdsPortal.Set(outIdx, inIdx);
      }

  viskores::cont::Field permutedField;
  bool success =
    viskores::filter::MapFieldPermutation(ds.GetPointField(fieldName), copyIdsArray, permutedField);
  if (!success)
  {
    throw std::runtime_error("Field copy failed while creating a sub-dataset.");
  }

  viskores::cont::DataSetBuilderUniform builder;
  if (globalSize[2] <= 1)
  {
    viskores::Id2 dimensions{ blockSize[0], blockSize[1] };
    auto dataSet = builder.Create(dimensions);
    viskores::cont::CellSetStructured<2> cellSet;
    cellSet.SetPointDimensions(dimensions);
    cellSet.SetGlobalPointDimensions(viskores::Id2{ globalSize[0], globalSize[1] });
    cellSet.SetGlobalPointIndexStart(viskores::Id2{ blockOrigin[0], blockOrigin[1] });
    dataSet.SetCellSet(cellSet);
    dataSet.AddField(permutedField);
    return dataSet;
  }

  auto dataSet = builder.Create(blockSize);
  viskores::cont::CellSetStructured<3> cellSet;
  cellSet.SetPointDimensions(blockSize);
  cellSet.SetGlobalPointDimensions(globalSize);
  cellSet.SetGlobalPointIndexStart(blockOrigin);
  dataSet.SetCellSet(cellSet);
  dataSet.AddField(permutedField);
  return dataSet;
}
#endif

} // namespace viskores::python::bindings
