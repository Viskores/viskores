#ifndef vtk_m_worklet_zfp_encode3_h
#define vtk_m_worklet_zfp_encode3_h

#include <vtkm/Types.h>
#include <vtkm/internal/ExportMacros.h>

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/zfp/ZFPBlockWriter.h>
#include <vtkm/worklet/zfp/ZFPEncode.h>
#include <vtkm/worklet/zfp/ZFPFunctions.h>
#include <vtkm/worklet/zfp/ZFPStructs.h>
#include <vtkm/worklet/zfp/ZFPTypeInfo.h>

namespace vtkm
{
namespace worklet
{
namespace zfp
{

template <typename Scalar, typename PortalType>
VTKM_EXEC inline void GatherPartial3(Scalar* q,
                                     const PortalType& scalars,
                                     const vtkm::Id3 dims,
                                     vtkm::Id offset,
                                     vtkm::Int32 nx,
                                     vtkm::Int32 ny,
                                     vtkm::Int32 nz)
{
  vtkm::Id x, y, z;

  for (z = 0; z < nz; z++, offset += dims[0] * dims[1] - ny * dims[0])
  {
    for (y = 0; y < ny; y++, offset += dims[0] - nx)
    {
      for (x = 0; x < nx; x++, offset += 1)
      {
        q[16 * z + 4 * y + x] = scalars.Get(offset);
      }
      PadBlock(q + 16 * z + 4 * y, nx, 1);
    }

    for (x = 0; x < 4; x++)
    {
      PadBlock(q + 16 * z + x, ny, 4);
    }
  }

  for (y = 0; y < 4; y++)
  {
    for (x = 0; x < 4; x++)
    {
      PadBlock(q + 4 * y + x, nz, 16);
    }
  }
}

template <typename Scalar, typename PortalType>
VTKM_EXEC inline void Gather3(Scalar* fblock,
                              const PortalType& scalars,
                              const vtkm::Id3 dims,
                              vtkm::Id offset)
{
  // TODO: gather partial
  vtkm::Id counter = 0;
  for (vtkm::Id z = 0; z < 4; z++, offset += dims[0] * dims[1] - 4 * dims[0])
  {
    for (vtkm::Id y = 0; y < 4; y++, offset += dims[0] - 4)
    {
      for (vtkm::Id x = 0; x < 4; x++, ++offset)
      {
        fblock[counter] = scalars.Get(offset);
        counter++;
      } // x
    }   // y
  }     // z
}

struct Encode3 : public vtkm::worklet::WorkletMapField
{
protected:
  vtkm::Id3 Dims;       // field dims
  vtkm::Id3 PaddedDims; // dims padded to a multiple of zfp block size
  vtkm::Id3 ZFPDims;    // zfp block dims
  vtkm::UInt32 MaxBits; // bits per zfp block
public:
  Encode3(const vtkm::Id3 dims, const vtkm::Id3 paddedDims, const vtkm::UInt32 maxbits)
    : Dims(dims)
    , PaddedDims(paddedDims)
    , MaxBits(maxbits)
  {
    ZFPDims[0] = PaddedDims[0] / 4;
    ZFPDims[1] = PaddedDims[1] / 4;
    ZFPDims[2] = PaddedDims[2] / 4;
  }
  using ControlSignature = void(FieldIn<>, WholeArrayIn<>, AtomicArrayInOut<> bitstream);
  using ExecutionSignature = void(_1, _2, _3);

  template <typename InputScalarPortal, typename BitstreamPortal>
  VTKM_EXEC void operator()(const vtkm::Id blockIdx,
                            const InputScalarPortal& scalars,
                            BitstreamPortal& stream) const
  {
    using Scalar = typename InputScalarPortal::ValueType;
    constexpr vtkm::Int32 BlockSize = 64;
    Scalar fblock[BlockSize];

    vtkm::Id3 zfpBlock;
    zfpBlock[0] = blockIdx % ZFPDims[0];
    zfpBlock[1] = (blockIdx / ZFPDims[0]) % ZFPDims[1];
    zfpBlock[2] = blockIdx / (ZFPDims[0] * ZFPDims[1]);
    vtkm::Id3 logicalStart = zfpBlock * vtkm::Id(4);

    //std::cout<<"Block ID "<<blockIdx<<"\n";
    //std::cout<<"ZFP Block "<<zfpBlock<<"\n";
    //std::cout<<"logicalStart Start "<<logicalStart<<"\n";
    // get the offset into the field
    //vtkm::Id offset = (zfpBlock[2]*4*ZFPDims[1] + zfpBlock[1] * 4)*ZFPDims[0] * 4 + zfpBlock[0] * 4;
    vtkm::Id offset = (logicalStart[2] * Dims[1] + logicalStart[1]) * Dims[0] + logicalStart[0];
    //std::cout<<"ZFP block offset "<<offset<<"\n";

    bool partial = false;
    if (logicalStart[0] + 4 > Dims[0])
      partial = true;
    if (logicalStart[1] + 4 > Dims[1])
      partial = true;
    if (logicalStart[2] + 4 > Dims[2])
      partial = true;
    //std::cout<<"Dims "<<Dims<<"\n";
    if (partial)
    {
      const vtkm::Int32 nx =
        logicalStart[0] + 4 > Dims[0] ? vtkm::Int32(Dims[0] - logicalStart[0]) : vtkm::Int32(4);
      const vtkm::Int32 ny =
        logicalStart[1] + 4 > Dims[1] ? vtkm::Int32(Dims[1] - logicalStart[1]) : vtkm::Int32(4);
      const vtkm::Int32 nz =
        logicalStart[2] + 4 > Dims[2] ? vtkm::Int32(Dims[2] - logicalStart[2]) : vtkm::Int32(4);
      //std::cout<<"Partial block "<<logicalStart<<" offset "<<offset<<"\n";
      //std::cout<<"Nx "<<nx<<" "<<ny<<" "<<nz<<"\n";
      GatherPartial3(fblock, scalars, Dims, offset, nx, ny, nz);
    }
    else
    {
      //std::cout<<"FULL block "<<zfpBlock<<"\n";
      Gather3(fblock, scalars, Dims, offset);
    }

    for (int i = 0; i < 64; ++i)
    {
      std::cout << " " << fblock[i];
    }
    std::cout << "\n";
    zfp::ZFPBlockEncoder<BlockSize, Scalar, BitstreamPortal> encoder;

    encoder.encode(fblock, MaxBits, vtkm::UInt32(blockIdx), stream);
    //blockWriter.print(0);
    //blockWriter.print(1);
  }
};
}
}
} // namespace vtkm::worklet::zfp
#endif
