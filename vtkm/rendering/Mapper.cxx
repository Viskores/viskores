//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/BoundsCompute.h>
#include <vtkm/rendering/Mapper.h>

namespace vtkm
{
namespace rendering
{

Mapper::~Mapper() {}

void Mapper::RenderCells(const vtkm::cont::UnknownCellSet& cellset,
                         const vtkm::cont::CoordinateSystem& coords,
                         const vtkm::cont::Field& scalarField,
                         const vtkm::cont::ColorTable& colorTable,
                         const vtkm::rendering::Camera& camera,
                         const vtkm::Range& scalarRange)
{
  RenderCells(cellset,
              coords,
              scalarField,
              colorTable,
              camera,
              scalarRange,
              make_FieldCell(
                vtkm::cont::GetGlobalGhostCellFieldName(),
                vtkm::cont::ArrayHandleConstant<vtkm::UInt8>(0, scalarField.GetNumberOfValues())));
};

struct CompareIndices
{
  vtkm::Vec3f CameraDirection;
  vtkm::Vec3f* Centers;
  CompareIndices(vtkm::Vec3f* centers, vtkm::Vec3f cameraDirection)
    : CameraDirection(cameraDirection)
    , Centers(centers)
  {
  }

  bool operator()(int i, int j) const
  {
    return (vtkm::Dot(Centers[i], CameraDirection) > vtkm::Dot(Centers[j], CameraDirection));
  }
};

void Mapper::RenderCellsPartitioned(const vtkm::cont::PartitionedDataSet partitionedData,
                                    const std::string fieldName,
                                    const vtkm::cont::ColorTable& colorTable,
                                    const vtkm::rendering::Camera& camera,
                                    const vtkm::Range& scalarRange)
{
  // sort partitions back to front for best rendering with the volume renderer
  vtkm::Vec3f centers[partitionedData.GetNumberOfPartitions()];
  std::vector<int> indices(partitionedData.GetNumberOfPartitions());
  for (unsigned int p = 0; p < partitionedData.GetNumberOfPartitions(); p++)
  {
    indices[p] = p;
    centers[p] = vtkm::cont::BoundsCompute(partitionedData.GetPartition(p)).Center();
  }
  CompareIndices comparator(centers, camera.GetLookAt() - camera.GetPosition());
  std::sort(indices.begin(), indices.end(), comparator);

  for (unsigned int p = 0; p < partitionedData.GetNumberOfPartitions(); p++)
  {
    auto partition = partitionedData.GetPartition(indices[p]);
    this->RenderCells(partition.GetCellSet(),
                      partition.GetCoordinateSystem(),
                      partition.GetField(fieldName.c_str()),
                      colorTable,
                      camera,
                      scalarRange,
                      partition.GetGhostCellField());
  }
}

void Mapper::SetActiveColorTable(const vtkm::cont::ColorTable& colorTable)
{

  constexpr vtkm::Float32 conversionToFloatSpace = (1.0f / 255.0f);

  vtkm::cont::ArrayHandle<vtkm::Vec4ui_8> temp;

  {
    vtkm::cont::ScopedRuntimeDeviceTracker tracker(vtkm::cont::DeviceAdapterTagSerial{});
    colorTable.Sample(1024, temp);
  }

  this->ColorMap.Allocate(1024);
  auto portal = this->ColorMap.WritePortal();
  auto colorPortal = temp.ReadPortal();
  for (vtkm::Id i = 0; i < 1024; ++i)
  {
    auto color = colorPortal.Get(i);
    vtkm::Vec4f_32 t(color[0] * conversionToFloatSpace,
                     color[1] * conversionToFloatSpace,
                     color[2] * conversionToFloatSpace,
                     color[3] * conversionToFloatSpace);
    portal.Set(i, t);
  }
}

void Mapper::SetLogarithmX(bool l)
{
  this->LogarithmX = l;
}

void Mapper::SetLogarithmY(bool l)
{
  this->LogarithmY = l;
}
}
}
