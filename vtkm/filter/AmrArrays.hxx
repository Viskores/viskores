//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_AmrArrays_hxx
#define vtk_m_filter_AmrArrays_hxx

#include <vtkm/CellClassification.h>
#include <vtkm/RangeId.h>
#include <vtkm/RangeId2.h>
#include <vtkm/RangeId3.h>
#include <vtkm/Types.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/BoundsCompute.h>
#include <vtkm/worklet/WorkletMapTopology.h>


namespace vtkm
{
namespace worklet
{
template <vtkm::IdComponent Dim>
struct GenerateGhostTypeWorklet : vtkm::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn cellSet,
                                FieldInPoint pointArray,
                                FieldInOutCell ghostArray);
  using ExecutionSignature = void(PointCount, _2, _3);
  using InputDomain = _1;

  GenerateGhostTypeWorklet(vtkm::Bounds boundsChild)
    : BoundsChild(boundsChild)
  {
  }

  template <typename pointArrayType, typename cellArrayType>
  VTKM_EXEC void operator()(vtkm::IdComponent numPoints,
                            const pointArrayType pointArray,
                            cellArrayType& ghostArray) const
  {
    vtkm::Bounds boundsCell = vtkm::Bounds();
    for (vtkm::IdComponent pointId = 0; pointId < numPoints; pointId++)
    {
      boundsCell.Include(pointArray[pointId]);
    }
    vtkm::Bounds boundsIntersection = boundsCell.Intersection(BoundsChild);
    if ((Dim == 2 && boundsIntersection.Area() > 0.5 * boundsCell.Area()) ||
        (Dim == 3 && boundsIntersection.Volume() > 0.5 * boundsCell.Volume()))
    {
      //      std::cout<<boundsCell<<" is (partly) contained in "<<BoundsChild<<" "<<boundsIntersection<<" "<<boundsIntersection.Area()<<std::endl;
      ghostArray = ghostArray + vtkm::CellClassification::Blanked;
    }
  }

  vtkm::Bounds BoundsChild;
};
} // worklet
} // vtkm

namespace vtkm
{
namespace filter
{

inline VTKM_CONT AmrArrays::AmrArrays() {}

VTKM_CONT
void AmrArrays::GenerateParentChildInformation()
{
  vtkm::Bounds bounds = vtkm::cont::BoundsCompute(this->AmrDataSet);
  if (bounds.Z.Max - bounds.Z.Min < vtkm::Epsilon<vtkm::FloatDefault>())
  {
    ComputeGenerateParentChildInformation<2>();
  }
  else
  {
    ComputeGenerateParentChildInformation<3>();
  }
}

template void AmrArrays::ComputeGenerateParentChildInformation<2>();

template void AmrArrays::ComputeGenerateParentChildInformation<3>();

VTKM_CONT
template <vtkm::IdComponent Dim>
void AmrArrays::ComputeGenerateParentChildInformation()
{
  // read out spacings in decreasing order to infer levels
  std::set<FloatDefault, std::greater<FloatDefault>> spacings;
  for (vtkm::Id p = 0; p < this->AmrDataSet.GetNumberOfPartitions(); p++)
  {
    vtkm::cont::ArrayHandleUniformPointCoordinates uniformCoords =
      this->AmrDataSet.GetPartition(p)
        .GetCoordinateSystem()
        .GetData()
        .AsArrayHandle<vtkm::cont::ArrayHandleUniformPointCoordinates>();
    spacings.insert(uniformCoords.GetSpacing()[0]);
  }
  std::set<FloatDefault, std::greater<FloatDefault>>::iterator itr;
  //  for (itr = spacings.begin(); itr != spacings.end(); itr++)
  //  {
  //    std::cout << *itr << "\n";
  //  }

  /// contains the partitionIds of each level and blockId
  this->PartitionIds.resize(spacings.size());
  for (vtkm::Id p = 0; p < this->AmrDataSet.GetNumberOfPartitions(); p++)
  {
    vtkm::cont::ArrayHandleUniformPointCoordinates uniformCoords =
      this->AmrDataSet.GetPartition(p)
        .GetCoordinateSystem()
        .GetData()
        .AsArrayHandle<vtkm::cont::ArrayHandleUniformPointCoordinates>();
    int index = -1;
    for (itr = spacings.begin(); itr != spacings.end(); itr++)
    {
      index++;
      if (*itr == uniformCoords.GetSpacing()[0])
      {
        break;
      }
    }
    this->PartitionIds.at(index).push_back(p);
    //    std::cout <<p<<" "<< index << "\n";
  }

  // compute parent and child relations
  this->ParentsIdsVector.resize(this->AmrDataSet.GetNumberOfPartitions());
  this->ChildrenIdsVector.resize(this->AmrDataSet.GetNumberOfPartitions());
  for (unsigned int l = 0; l < this->PartitionIds.size() - 1; l++)
  {
    for (unsigned int bParent = 0; bParent < this->PartitionIds.at(l).size(); bParent++)
    {
      //        std::cout << std::endl << "level " << l << " block " << bParent << std::endl;
      vtkm::Bounds boundsParent =
        this->AmrDataSet.GetPartition(this->PartitionIds.at(l).at(bParent))
          .GetCoordinateSystem()
          .GetBounds();

      // compute size of a cell to compare overlap against
      auto coords = this->AmrDataSet.GetPartition(this->PartitionIds.at(l).at(bParent))
                      .GetCoordinateSystem()
                      .GetDataAsMultiplexer();
      vtkm::cont::CellSetStructured<Dim> cellset;
      vtkm::Id ptids[8];
      this->AmrDataSet.GetPartition(this->PartitionIds.at(l).at(bParent))
        .GetCellSet()
        .AsCellSet(cellset);
      cellset.GetCellPointIds(0, ptids);
      vtkm::Bounds boundsCell = vtkm::Bounds();
      for (vtkm::IdComponent pointId = 0; pointId < cellset.GetNumberOfPointsInCell(0); pointId++)
      {
        boundsCell.Include(coords.ReadPortal().Get(ptids[pointId]));
      }

      // see if there is overlap of at least one half of a cell
      for (unsigned int bChild = 0; bChild < this->PartitionIds.at(l + 1).size(); bChild++)
      {
        vtkm::Bounds boundsChild =
          this->AmrDataSet.GetPartition(this->PartitionIds.at(l + 1).at(bChild))
            .GetCoordinateSystem()
            .GetBounds();
        vtkm::Bounds boundsIntersection = boundsParent.Intersection(boundsChild);
        if ((Dim == 2 && boundsIntersection.Area() > 0.5 * boundsCell.Area()) ||
            (Dim == 3 && boundsIntersection.Volume() >= 0.5 * boundsCell.Volume()))
        {
          this->ParentsIdsVector.at(this->PartitionIds.at(l + 1).at(bChild))
            .push_back(this->PartitionIds.at(l).at(bParent));
          this->ChildrenIdsVector.at(this->PartitionIds.at(l).at(bParent))
            .push_back(this->PartitionIds.at(l + 1).at(bChild));
          //          std::cout << " overlaps with level " << l + 1 << " block  " << bChild << " "
          //                    << boundsParent << " " << boundsChild << " " << boundsIntersection << " "
          //          << boundsIntersection.Area() << " " << boundsIntersection.Volume() << std::endl;
        }
        //        else
        //        {
        //          std::cout << " does not overlap with level " << l + 1 << " block  " << bChild << " "
        //                    << boundsParent << " " << boundsChild << " " << boundsIntersection << " "
        //                    << boundsIntersection.Area() << " " << boundsIntersection.Volume() << std::endl;
        //        }
      }
    }
  }
}

VTKM_CONT
void AmrArrays::GenerateGhostType()
{
  vtkm::Bounds bounds = vtkm::cont::BoundsCompute(this->AmrDataSet);
  if (bounds.Z.Max - bounds.Z.Min < vtkm::Epsilon<vtkm::FloatDefault>())
  {
    ComputeGenerateGhostType<2>();
  }
  else
  {
    ComputeGenerateGhostType<3>();
  }
}

template void AmrArrays::ComputeGenerateGhostType<2>();

template void AmrArrays::ComputeGenerateGhostType<3>();

VTKM_CONT
template <vtkm::IdComponent Dim>
void AmrArrays::ComputeGenerateGhostType()
{
  for (unsigned int l = 0; l < this->PartitionIds.size(); l++)
  {
    for (unsigned int bParent = 0; bParent < this->PartitionIds.at(l).size(); bParent++)
    {
      //      std::cout<<std::endl<<"level  "<<l<<" block  "<<bParent<<" has  "<<this->ChildrenIdsVector.at(this->PartitionIds.at(l).at(bParent)).size()<<" children"<<std::endl;

      vtkm::cont::DataSet partition =
        this->AmrDataSet.GetPartition(this->PartitionIds.at(l).at(bParent));
      vtkm::cont::CellSetStructured<Dim> cellset;
      partition.GetCellSet().AsCellSet(cellset);
      vtkm::cont::ArrayHandle<vtkm::UInt8> ghostField;
      if (!partition.HasField("vtkGhostType", vtkm::cont::Field::Association::Cells))
      {
        vtkm::cont::ArrayCopy(
          vtkm::cont::ArrayHandleConstant<vtkm::UInt8>(0, partition.GetNumberOfCells()),
          ghostField);
      }
      // leave field unchanged if it is the highest level
      if (l < this->PartitionIds.size() - 1)
      {
        auto pointField = partition.GetCoordinateSystem().GetDataAsMultiplexer();

        for (unsigned int bChild = 0;
             bChild < this->ChildrenIdsVector.at(this->PartitionIds.at(l).at(bParent)).size();
             bChild++)
        {
          vtkm::Bounds boundsChild =
            this->AmrDataSet
              .GetPartition(
                this->ChildrenIdsVector.at(this->PartitionIds.at(l).at(bParent)).at(bChild))
              .GetCoordinateSystem()
              .GetBounds();
          //          std::cout<<" is (partly) contained in level "<<l + 1<<" block "<<bChild<<" which is partition "<<this->ChildrenIdsVector.at(this->PartitionIds.at(l).at(bParent)).at(bChild)<<" with bounds "<<boundsChild<<std::endl;

          vtkm::cont::Invoker invoke;
          invoke(vtkm::worklet::GenerateGhostTypeWorklet<Dim>{ boundsChild },
                 cellset,
                 pointField,
                 ghostField);
        }
      }
      partition.AddCellField("vtkGhostType", ghostField);
      this->AmrDataSet.ReplacePartition(this->PartitionIds.at(l).at(bParent), partition);
    }
  }
}

// Add helper arrays like in ParaView
VTKM_CONT
void AmrArrays::GenerateIndexArrays()
{
  for (unsigned int l = 0; l < this->PartitionIds.size(); l++)
  {
    for (unsigned int b = 0; b < this->PartitionIds.at(l).size(); b++)
    {
      vtkm::cont::DataSet partition = this->AmrDataSet.GetPartition(this->PartitionIds.at(l).at(b));

      vtkm::cont::ArrayHandle<vtkm::Id> fieldAmrLevel;
      vtkm::cont::ArrayCopy(
        vtkm::cont::ArrayHandleConstant<vtkm::Id>(l, partition.GetNumberOfCells()), fieldAmrLevel);
      partition.AddCellField("vtkAmrLevel", fieldAmrLevel);

      vtkm::cont::ArrayHandle<vtkm::Id> fieldBlockId;
      vtkm::cont::ArrayCopy(
        vtkm::cont::ArrayHandleConstant<vtkm::Id>(b, partition.GetNumberOfCells()), fieldBlockId);
      partition.AddCellField("vtkAmrIndex", fieldBlockId);

      vtkm::cont::ArrayHandle<vtkm::Id> fieldPartitionIndex;
      vtkm::cont::ArrayCopy(vtkm::cont::ArrayHandleConstant<vtkm::Id>(
                              this->PartitionIds.at(l).at(b), partition.GetNumberOfCells()),
                            fieldPartitionIndex);
      partition.AddCellField("vtkCompositeIndex", fieldPartitionIndex);

      this->AmrDataSet.ReplacePartition(this->PartitionIds.at(l).at(b), partition);
    }
  }
}

template <typename DerivedPolicy>
vtkm::cont::PartitionedDataSet AmrArrays::PrepareForExecution(
  const vtkm::cont::PartitionedDataSet& input,
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  this->AmrDataSet = input;
  this->GenerateParentChildInformation();
  this->GenerateGhostType();
  this->GenerateIndexArrays();
  return this->AmrDataSet;
}

}
}
#endif
