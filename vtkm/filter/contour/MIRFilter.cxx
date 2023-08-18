//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/Types.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/ErrorFilterExecution.h>

#include <vtkm/filter/MapFieldPermutation.h>
#include <vtkm/filter/contour/MIRFilter.h>
#include <vtkm/filter/contour/worklet/MIR.h>

#include <vtkm/filter/mesh_info/CellMeasures.h>

#include <vtkm/worklet/Keys.h>
#include <vtkm/worklet/ScatterCounting.h>

namespace vtkm
{
namespace filter
{
namespace contour
{
VTKM_CONT bool MIRFilter::DoMapField(
  vtkm::cont::DataSet& result,
  const vtkm::cont::Field& field,
  const vtkm::cont::ArrayHandle<vtkm::Id>& filterCellInterp,
  const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 8>>& MIRWeights,
  const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 8>> MIRIDs)
{
  if (field.GetName().compare(this->pos_name) == 0 ||
      field.GetName().compare(this->len_name) == 0 || field.GetName().compare(this->id_name) == 0 ||
      field.GetName().compare(this->vf_name) == 0)
  {
    // Remember, we will map the field manually...
    // Technically, this will be for all of them...thus ignore it
    return false;
  }

  if (field.IsPointField())
  {
    vtkm::cont::UnknownArrayHandle output = field.GetData().NewInstanceBasic();
    auto resolve = [&](const auto& concrete) {
      using T = typename std::decay_t<decltype(concrete)>::ValueType::ComponentType;
      auto outputArray = output.ExtractArrayFromComponents<T>(vtkm::CopyFlag::Off);
      vtkm::worklet::DestructPointWeightList destructWeightList;
      this->Invoke(destructWeightList, MIRIDs, MIRWeights, concrete, outputArray);
    };
    field.GetData().CastAndCallWithExtractedArray(resolve);
    result.AddPointField(field.GetName(), output);
    return true;
  }
  else if (field.IsCellField())
  {
    return vtkm::filter::MapFieldPermutation(field, filterCellInterp, result);
  }
  else
  {
    return false;
  }
}

//-----------------------------------------------------------------------------
VTKM_CONT vtkm::cont::DataSet MIRFilter::DoExecute(const vtkm::cont::DataSet& input)
{
  const vtkm::cont::CoordinateSystem inputCoords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> avgSizeTot;
  vtkm::filter::mesh_info::CellMeasures getSize(
    vtkm::filter::mesh_info::IntegrationType::AllMeasures);
  getSize.SetCellMeasureName("size");
  vtkm::cont::ArrayCopyShallowIfPossible(getSize.Execute(input).GetCellField("size").GetData(),
                                         avgSizeTot);
  // First, load up all fields...
  vtkm::cont::Field or_pos = input.GetField(this->pos_name);
  vtkm::cont::Field or_len = input.GetField(this->len_name);
  vtkm::cont::Field or_ids = input.GetField(this->id_name);
  vtkm::cont::Field or_vfs = input.GetField(this->vf_name);
  // TODO: Check all fields for 'IsCellField'
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> vfsdata_or, vfsdata;
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

  vtkm::cont::ArrayHandle<vtkm::Id> filterCellInterp;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 8>> MIRWeights;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 8>> MIRIDs;

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
    vtkm::cont::ArrayHandle<vtkm::FloatDefault> avgSize;
    vtkm::cont::ArrayCopyShallowIfPossible(getSize.Execute(saved).GetCellField("size").GetData(),
                                           avgSize);

    vtkm::worklet::CalcError_C calcErrC;
    vtkm::worklet::Keys<vtkm::Id> cellKeys(cellLookback);
    vtkm::cont::ArrayCopy(cellLookback, filterCellInterp);
    vtkm::cont::ArrayHandle<vtkm::Id> lenOut, posOut, idsOut;
    vtkm::cont::ArrayHandle<vtkm::FloatDefault> vfsOut, totalErrorOut;

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
    totalError = totalError / vtkm::Float64(input.GetCellSet().GetNumberOfCells());
    this->error_scaling *= this->scaling_decay;

    VTKM_LOG_S(vtkm::cont::LogLevel::Info,
               "Mir iteration " << currentIterationNum + 1 << "/" << this->max_iter
                                << "\t Total error: " << totalError);

    saved.AddField(vtkm::cont::Field(
      this->GetOutputFieldName(), vtkm::cont::Field::Association::Cells, prevMat));

    vtkm::cont::ArrayCopy(pointIDs, MIRIDs);
    vtkm::cont::ArrayCopy(pointWeights, MIRWeights);
  } while ((++currentIterationNum <= this->max_iter) && totalError >= this->max_error);

  auto mapper = [&](auto& outDataSet, const auto& f) {
    this->DoMapField(outDataSet, f, filterCellInterp, MIRWeights, MIRIDs);
  };
  auto output = this->CreateResultCoordinateSystem(
    input, saved.GetCellSet(), saved.GetCoordinateSystem(), mapper);
  output.AddField(saved.GetField(this->GetOutputFieldName()));

  return output;
}
} // namespace contour
} // namespace filter
} // namespace vtkm
