//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_filter_resampling_testing_TestingHistSampling_h
#define viskores_filter_resampling_testing_TestingHistSampling_h

#include <viskores/CellShape.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/DataSetBuilderExplicit.h>

#include <vector>

namespace viskores
{
namespace filter
{
namespace resampling
{
namespace testing
{

inline VISKORES_CONT viskores::cont::DataSet MakePointCloudPartition(viskores::Id numberOfPoints,
                                                                     viskores::Id numberOfZeros,
                                                                     viskores::Id pointIdOffset)
{
  std::vector<viskores::Vec3f> coordinates(static_cast<std::size_t>(numberOfPoints));
  std::vector<viskores::Id> connectivity(static_cast<std::size_t>(numberOfPoints));
  for (viskores::Id index = 0; index < numberOfPoints; ++index)
  {
    coordinates[static_cast<std::size_t>(index)] =
      viskores::Vec3f{ static_cast<viskores::FloatDefault>(index), 0.0f, 0.0f };
    connectivity[static_cast<std::size_t>(index)] = index;
  }
  auto dataSet = viskores::cont::DataSetBuilderExplicit::Create(
    coordinates, viskores::CellShapeTagVertex{}, 1, connectivity);

  viskores::cont::ArrayHandle<viskores::FloatDefault> scalarArray;
  scalarArray.AllocateAndFill(numberOfPoints, 1.0f);
  auto scalarPortal = scalarArray.WritePortal();
  for (viskores::Id index = 0; index < numberOfZeros; ++index)
  {
    scalarPortal.Set(index, 0.0f);
  }
  dataSet.AddPointField("scalarField", scalarArray);

  dataSet.AddPointField(
    "pointId",
    viskores::cont::make_ArrayHandleCounting(pointIdOffset, viskores::Id{ 1 }, numberOfPoints));
  return dataSet;
}

} // namespace testing
} // namespace resampling
} // namespace filter
} // namespace viskores

#endif // viskores_filter_resampling_testing_TestingHistSampling_h
