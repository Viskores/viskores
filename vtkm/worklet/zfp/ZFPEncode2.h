#ifndef vtk_m_worklet_zfp_encode2_h
#define vtk_m_worklet_zfp_encode2_h

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
VTKM_EXEC inline void GatherPartial2(Scalar* q,
                                     const PortalType& scalars,
                                     vtkm::Id offset,
                                     int nx,
                                     int ny,
                                     int sx,
                                     int sy)
{
  vtkm::Id x, y;
  for (y = 0; y < ny; y++, offset += sy - nx * sx)
  {
    for (x = 0; x < nx; x++, offset += 1)
      q[4 * y + x] = scalars.Get(offset);
    PadBlock(q + 4 * y, nx, 1);
  }
  for (x = 0; x < 4; x++)
    PadBlock(q + x, ny, 4);
}

template <typename Scalar, typename PortalType>
VTKM_EXEC inline void Gather2(Scalar* fblock,
                              const PortalType& scalars,
                              vtkm::Id offset,
                              int sx,
                              int sy)
{
  vtkm::Id counter = 0;

  for (vtkm::Id y = 0; y < 4; y++, offset += sy - 4 * sx)
    for (vtkm::Id x = 0; x < 4; x++, offset += sx)
    {
      fblock[counter] = scalars.Get(offset);
      counter++;
    }
}

struct Encode2 : public vtkm::worklet::WorkletMapField
{
protected:
  vtkm::Id2 Dims;       // field dims
  vtkm::Id2 PaddedDims; // dims padded to a multiple of zfp block size
  vtkm::Id2 ZFPDims;    // zfp block dims
  vtkm::UInt32 MaxBits; // bits per zfp block

public:
  Encode2(const vtkm::Id2 dims, const vtkm::Id2 paddedDims, const vtkm::UInt32 maxbits)
    : Dims(dims)
    , PaddedDims(paddedDims)
    , MaxBits(maxbits)
  {
    ZFPDims[0] = PaddedDims[0] / 4;
    ZFPDims[1] = PaddedDims[1] / 4;
  }
  using ControlSignature = void(FieldIn<>, WholeArrayIn<>, AtomicArrayInOut<> bitstream);
  using ExecutionSignature = void(_1, _2, _3);

  template <class InputScalarPortal, typename BitstreamPortal>
  VTKM_EXEC void operator()(const vtkm::Id blockIdx,
                            const InputScalarPortal& scalars,
                            BitstreamPortal& stream) const
  {
    using Scalar = typename InputScalarPortal::ValueType;

    //    typedef unsigned long long int ull;
    //    typedef long long int ll;
    //    const ull blockId = blockIdx.x +
    //                        blockIdx.y * gridDim.x +
    //                        gridDim.x * gridDim.y * blockIdx.z;

    //    // each thread gets a block so the block index is
    //    // the global thread index
    //    const uint block_idx = blockId * blockDim.x + threadIdx.x;

    //    if(block_idx >= tot_blocks)
    //    {
    //      // we can't launch the exact number of blocks
    //      // so just exit if this isn't real
    //      return;
    //    }

    //    uint2 block_dims;
    //    block_dims.x = padded_dims.x >> 2;
    //    block_dims.y = padded_dims.y >> 2;

    //    // logical pos in 3d array
    //    uint2 block;
    //    block.x = (block_idx % block_dims.x) * 4;
    //    block.y = ((block_idx/ block_dims.x) % block_dims.y) * 4;
    //    const ll offset = (ll)block.x * stride.x + (ll)block.y * stride.y;

    vtkm::Id2 zfpBlock;
    zfpBlock[0] = blockIdx % ZFPDims[0];
    zfpBlock[1] = (blockIdx / ZFPDims[0]) % ZFPDims[1];
    vtkm::Id2 logicalStart = zfpBlock * vtkm::Id(4);
    vtkm::Id offset = logicalStart[1] * Dims[0] + logicalStart[0];

    constexpr vtkm::Int32 BlockSize = 16;
    Scalar fblock[BlockSize];

    //    bool partial = false;
    //    if(block.x + 4 > dims.x) partial = true;
    //    if(block.y + 4 > dims.y) partial = true;

    bool partial = false;
    if (logicalStart[0] + 4 > Dims[0])
      partial = true;
    if (logicalStart[1] + 4 > Dims[1])
      partial = true;

    if (partial)
    {
      const vtkm::Int32 nx =
        logicalStart[0] + 4 > Dims[0] ? vtkm::Int32(Dims[0] - logicalStart[0]) : vtkm::Int32(4);
      const vtkm::Int32 ny =
        logicalStart[1] + 4 > Dims[1] ? vtkm::Int32(Dims[1] - logicalStart[1]) : vtkm::Int32(4);
      GatherPartial2(fblock, scalars, offset, nx, ny, 1, Dims[0]);
    }
    else
    {
      Gather2(fblock, scalars, offset, 1, Dims[0]);
    }

    for (int i = 0; i < 16; ++i)
    {
      std::cout << " " << fblock[i];
    }
    std::cout << "\n";

    //zfp_encode_block<Scalar, ZFP_2D_BLOCK_SIZE>(fblock, maxbits, block_idx, stream);
    zfp::ZFPBlockEncoder<BlockSize, Scalar, BitstreamPortal> encoder;
    encoder.encode(fblock, MaxBits, vtkm::UInt32(blockIdx), stream);
  }
};
}
}
}
#endif
