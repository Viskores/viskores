//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_MIRFilter_hxx
#define vtk_m_filter_MIRFilter_hxx

#include <vtkm/CellShape.h>
#include <vtkm/Types.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ArrayHandleView.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/cont/ExecutionObjectBase.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/exec/FunctorBase.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/DispatcherReduceByKey.h>
#include <vtkm/worklet/Keys.h>
#include <vtkm/worklet/MIR.h>
#include <vtkm/worklet/ScatterCounting.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>
#include <vtkm/worklet/WorkletReduceByKey.h>
#include <vtkm/worklet/clip/ClipTables.h>

#include <vtkm/filter/MeshQuality.h>

namespace vtkm
{
/*
    Todos:
        Enable some sort of cell culling, so if a cell doesn't have any work to do, it doesn't get called in future invocations of MIR

 */
namespace filter
{

template <typename T, typename StorageType, typename StorageType2>
inline VTKM_CONT void MIRFilter::ProcessPointField(
  const vtkm::cont::ArrayHandle<T, StorageType>& input,
  vtkm::cont::ArrayHandle<T, StorageType2>& output)
{
  vtkm::worklet::DestructPointWeightList destructWeightList;
  this->Invoke(destructWeightList, this->MIRIDs, this->MIRWeights, input, output);
}
//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet MIRFilter::DoExecute(
  const vtkm::cont::DataSet& input,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{


  //{
  //(void)input;
  //(void)policy;
  vtkm::worklet::CheckFor2D cellCheck;
  vtkm::cont::ArrayHandle<vtkm::Id> count2D, count3D, countBad;
  this->Invoke(cellCheck,
               vtkm::filter::ApplyPolicyCellSet(input.GetCellSet(), policy, *this),
               count2D,
               count3D,
               countBad);
  vtkm::Id c2 = vtkm::cont::Algorithm::Reduce(count2D, vtkm::Id(0));
  vtkm::Id c3 = vtkm::cont::Algorithm::Reduce(count3D, vtkm::Id(0));
  vtkm::Id cB = vtkm::cont::Algorithm::Reduce(countBad, vtkm::Id(0));
  if (cB > vtkm::Id(0))
  {
    VTKM_LOG_S(
      vtkm::cont::LogLevel::Fatal,
      "Bad cell found in MIR filter input! Strictly only 2D -xor- 3D cell sets are permitted!");
  }
  if (c2 > vtkm::Id(0) && c3 > vtkm::Id(0))
  {
    VTKM_LOG_S(
      vtkm::cont::LogLevel::Fatal,
      "Bad cell mix found in MIR filter input! Input is not allowed to have both 2D and 3D cells.");
  }
  if (c2 == vtkm::Id(0) && c3 == vtkm::Id(0))
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Fatal,
               "No cells found for MIR filter! Please don't call me with nothing!");
  }

  const vtkm::cont::CoordinateSystem inputCoords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());
  vtkm::cont::ArrayHandle<vtkm::Float64> avgSizeTot;
  vtkm::worklet::MeshQuality<vtkm::filter::CellMetric> getVol;
  getVol.SetMetric(c3 > 0 ? vtkm::filter::CellMetric::VOLUME : vtkm::filter::CellMetric::AREA);
  this->Invoke(getVol,
               vtkm::filter::ApplyPolicyCellSet(input.GetCellSet(), policy, *this),
               inputCoords.GetData(),
               avgSizeTot);
  // First, load up all fields...
  vtkm::cont::Field or_pos = input.GetField(this->pos_name);
  vtkm::cont::Field or_len = input.GetField(this->len_name);
  vtkm::cont::Field or_ids = input.GetField(this->id_name);
  vtkm::cont::Field or_vfs = input.GetField(this->vf_name);
  // TODO: Check all fields for 'IsFieldCell'
  vtkm::cont::ArrayHandle<vtkm::Float32> vfsdata_or, vfsdata;
  vtkm::cont::ArrayHandle<vtkm::Id> idsdata_or, idsdata, lendata_or, lendata, posdata_or, posdata,
    allids;
  or_pos.GetData().AsArrayHandle(posdata_or);
  or_len.GetData().AsArrayHandle(lendata_or);
  or_ids.GetData().AsArrayHandle(idsdata_or);
  or_vfs.GetData().AsArrayHandle(vfsdata_or);
  vtkm::cont::ArrayCopy(idsdata_or, allids);
  vtkm::cont::Algorithm::Sort(allids);
  vtkm::cont::Algorithm::Unique(allids);
  vtkm::IdComponent numIDs = static_cast<vtkm::IdComponent>(allids.GetNumberOfValues());
  //using PortalConstType = vtkm::cont::ArrayHandle<vtkm::Id>::PortalConstControl;
  //PortalConstType readPortal = allids.GetPortalConstControl();
  using PortalConstType = vtkm::cont::ArrayHandle<vtkm::Id>::ReadPortalType;
  PortalConstType readPortal = allids.ReadPortal();
  vtkm::cont::ArrayCopy(idsdata_or, idsdata);
  vtkm::cont::ArrayCopy(lendata_or, lendata);
  vtkm::cont::ArrayCopy(posdata_or, posdata);
  vtkm::cont::ArrayCopy(vfsdata_or, vfsdata);
  //}

  vtkm::cont::DataSet saved;
  // % error of the whole system, multiplied by the number of cells
  vtkm::Float64 totalError = this->max_error + vtkm::Float64(1.1); // Dummy value
  vtkm::IdComponent currentIterationNum = 0;

  vtkm::worklet::MIRCases::MIRTables faceTableArray;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 8>> pointWeights;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 8>> pointIDs;
  vtkm::worklet::ConstructCellWeightList constructReverseInformation;
  vtkm::cont::ArrayHandleIndex pointCounter(input.GetNumberOfPoints());
  this->Invoke(constructReverseInformation, pointCounter, pointIDs, pointWeights);
  do
  {
    saved = vtkm::cont::DataSet();
    saved.AddCoordinateSystem(inputCoords);
    saved.SetCellSet(input.GetCellSet());

    vtkm::cont::ArrayHandle<vtkm::Id> currentcellIDs;
    vtkm::cont::ArrayHandle<vtkm::Id> pointlen, pointpos, pointid;
    vtkm::cont::ArrayHandle<vtkm::Float64> pointvf;
    vtkm::worklet::CombineVFsForPoints_C convertOrigCellTo;
    vtkm::worklet::CombineVFsForPoints convertOrigCellTo_Full;


    this->Invoke(convertOrigCellTo, saved.GetCellSet(), lendata, posdata, idsdata, pointlen);
    vtkm::Id pointcount = vtkm::cont::Algorithm::ScanExclusive(pointlen, pointpos);
    pointvf.Allocate(pointcount);
    pointid.Allocate(pointcount);
    this->Invoke(convertOrigCellTo_Full,
                 saved.GetCellSet(),
                 lendata,
                 posdata,
                 idsdata,
                 vfsdata,
                 pointpos,
                 pointid,
                 pointvf);

    vtkm::worklet::MIRObject<vtkm::Id, vtkm::Float64> mirobj(
      pointlen, pointpos, pointid, pointvf); // This is point VF data...
    vtkm::cont::ArrayHandle<vtkm::Id> prevMat;
    vtkm::cont::ArrayCopy(
      vtkm::cont::make_ArrayHandleConstant<vtkm::Id>(-1, saved.GetCellSet().GetNumberOfCells()),
      prevMat);
    vtkm::cont::ArrayHandle<vtkm::Id> cellLookback;
    vtkm::cont::ArrayHandleIndex tmp_ind(saved.GetCellSet().GetNumberOfCells());
    vtkm::cont::ArrayCopy(tmp_ind, cellLookback);
    vtkm::IdComponent currentMatLoc = 0;

    while (currentMatLoc < numIDs)
    {
      vtkm::IdComponent currentMatID =
        static_cast<vtkm::IdComponent>(readPortal.Get(currentMatLoc++));
      if (currentMatID < 1)
      {
        VTKM_LOG_S(
          vtkm::cont::LogLevel::Fatal,
          "MIR filter does not accept materials with an non-positive ID! Material id in offense: "
            << currentMatID
            << ". Please remap all ID values to only positive numbers to avoid this issue.");
      }
      // First go through and pick out the previous and current material VFs for each cell.
      //{
      vtkm::worklet::ExtractVFsForMIR_C extractCurrentMatVF;
      vtkm::cont::ArrayHandle<vtkm::Id> currentCellPointCounts;
      this->Invoke(extractCurrentMatVF, saved.GetCellSet(), currentCellPointCounts);
      vtkm::worklet::ExtractVFsForMIR extractCurrentMatVF_SC(currentMatID);
      vtkm::worklet::ScatterCounting extractCurrentMatVF_SC_scatter =
        extractCurrentMatVF_SC.MakeScatter(currentCellPointCounts);
      vtkm::cont::ArrayHandle<vtkm::Float64> currentMatVF;
      vtkm::cont::ArrayHandle<vtkm::Float64> previousMatVF;
      this->Invoke(extractCurrentMatVF_SC,
                   extractCurrentMatVF_SC_scatter,
                   saved.GetCellSet(),
                   mirobj,
                   prevMat,
                   currentMatVF,
                   previousMatVF);
      //}
      // Next see if we need to perform any work at all...
      if (currentMatLoc != 0)
      {
        // Run MIR, possibly changing colors...
        vtkm::cont::ArrayHandle<vtkm::Id> cellVFPointOffsets;
        vtkm::cont::Algorithm::ScanExclusive(currentCellPointCounts, cellVFPointOffsets);
        vtkm::worklet::MIR mir;
        vtkm::cont::ArrayHandle<vtkm::Id> newCellLookback, newCellID;


        vtkm::cont::CellSetExplicit<> out = mir.Run(saved.GetCellSet(),
                                                    previousMatVF,
                                                    currentMatVF,
                                                    cellVFPointOffsets,
                                                    prevMat,
                                                    currentMatID,
                                                    cellLookback,
                                                    newCellID,
                                                    newCellLookback);
        vtkm::cont::ArrayCopy(newCellLookback, cellLookback);
        vtkm::cont::ArrayCopy(newCellID, prevMat);
        auto data = saved.GetCoordinateSystem(0).GetDataAsMultiplexer();
        auto coords = mir.ProcessPointField(data);
        // Now convert the point VFs...
        vtkm::cont::ArrayHandle<vtkm::Id> plen, ppos, pids;
        vtkm::cont::ArrayHandle<vtkm::Float64> pvf;
        mir.ProcessMIRField(mirobj.getPointLenArr(),
                            mirobj.getPointPosArr(),
                            mirobj.getPointIDArr(),
                            mirobj.getPointVFArr(),
                            plen,
                            ppos,
                            pids,
                            pvf);
        vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 8>> tmppointWeights;
        vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 8>> tmppointIDs;
        mir.ProcessSimpleMIRField(pointIDs, pointWeights, tmppointIDs, tmppointWeights);
        vtkm::cont::ArrayCopy(tmppointIDs, pointIDs);
        vtkm::cont::ArrayCopy(tmppointWeights, pointWeights);
        //FileSaver fs;
        //fs(("pID" + std::to_string(currentMatID) + ".txt").c_str(), pointIDs);
        //fs(("wID" + std::to_string(currentMatID) + ".txt").c_str(), pointWeights);
        mirobj = vtkm::worklet::MIRObject<vtkm::Id, vtkm::Float64>(plen, ppos, pids, pvf);
        saved = vtkm::cont::DataSet();
        saved.SetCellSet(out);
        vtkm::cont::CoordinateSystem outCo2(inputCoords.GetName(), coords);
        saved.AddCoordinateSystem(outCo2);
      }
    }


    // Hacking workaround to not clone an entire dataset.
    vtkm::cont::ArrayHandle<vtkm::Float64> avgSize;
    this->Invoke(getVol, saved.GetCellSet(), saved.GetCoordinateSystem(0).GetData(), avgSize);

    vtkm::worklet::CalcError_C calcErrC;
    vtkm::worklet::Keys<vtkm::Id> cellKeys(cellLookback);
    vtkm::cont::ArrayCopy(cellLookback, filterCellInterp);
    vtkm::cont::ArrayHandle<vtkm::Id> lenOut, posOut, idsOut;
    vtkm::cont::ArrayHandle<vtkm::Float64> vfsOut, totalErrorOut;

    lenOut.Allocate(cellKeys.GetUniqueKeys().GetNumberOfValues());
    this->Invoke(calcErrC, cellKeys, prevMat, lendata_or, posdata_or, idsdata_or, lenOut);

    vtkm::Id numIDsOut = vtkm::cont::Algorithm::ScanExclusive(lenOut, posOut);
    idsOut.Allocate(numIDsOut);
    vfsOut.Allocate(numIDsOut);
    vtkm::worklet::CalcError calcErr(this->error_scaling);
    this->Invoke(calcErr,
                 cellKeys,
                 prevMat,
                 avgSize,
                 lendata_or,
                 posdata_or,
                 idsdata_or,
                 vfsdata_or,
                 lendata,
                 posdata,
                 idsdata,
                 vfsdata,
                 lenOut,
                 posOut,
                 idsOut,
                 vfsOut,
                 avgSizeTot,
                 totalErrorOut);
    totalError = vtkm::cont::Algorithm::Reduce(totalErrorOut, vtkm::Float64(0));
    vtkm::cont::ArrayCopy(lenOut, lendata);
    vtkm::cont::ArrayCopy(posOut, posdata);
    vtkm::cont::ArrayCopy(idsOut, idsdata);
    vtkm::cont::ArrayCopy(vfsOut, vfsdata);
    // Clean up the cells by calculating their volumes, and then calculate the relative error for each cell.
    // Note that the total error needs to be rescaled by the number of cells to get the % error.
    totalError =
      totalError /
      vtkm::Float64(
        vtkm::filter::ApplyPolicyCellSet(input.GetCellSet(), policy, *this).GetNumberOfCells());
    this->error_scaling *= this->scaling_decay;

    VTKM_LOG_S(vtkm::cont::LogLevel::Info,
               "Mir iteration " << currentIterationNum + 1 << "/" << this->max_iter
                                << "\t Total error: " << totalError);

    saved.AddField(vtkm::cont::Field(
      this->GetOutputFieldName(), vtkm::cont::Field::Association::CELL_SET, prevMat));

    vtkm::cont::ArrayCopy(pointIDs, this->MIRIDs);
    vtkm::cont::ArrayCopy(pointWeights, this->MIRWeights);
  } while ((++currentIterationNum <= this->max_iter) && totalError >= this->max_error);


  return saved;
}
}
}


#endif
