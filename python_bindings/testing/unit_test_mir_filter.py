##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

import numpy as np
from pathlib import Path

from viskores.cont import make_FieldCell, make_FieldWholeDataSet
from viskores.filter.contour import MIRFilter
from viskores.io import VTKDataSetReader


def main():
    data_path = Path(__file__).resolve().parents[2] / "data" / "data" / "uniform" / "venn250.vtk"
    reader = VTKDataSetReader(str(data_path))
    dataset = reader.ReadDataSet()

    background = dataset.GetField("mesh_topo/background")
    circle_a = dataset.GetField("mesh_topo/circle_a")
    circle_b = dataset.GetField("mesh_topo/circle_b")
    circle_c = dataset.GetField("mesh_topo/circle_c")

    length = (
        (background > 0).astype(np.int64)
        + (circle_a > 0).astype(np.int64)
        + (circle_b > 0).astype(np.int64)
        + (circle_c > 0).astype(np.int64)
    )
    offset = np.zeros_like(length)
    offset[1:] = np.cumsum(length[:-1], dtype=np.int64)

    total = int(length.sum())
    mat_ids = np.empty(total, dtype=np.int64)
    mat_vfs = np.empty(total, dtype=np.float32)

    for cell_index in range(length.shape[0]):
        write_index = int(offset[cell_index])
        if background[cell_index] > 0:
            mat_ids[write_index] = 1
            mat_vfs[write_index] = background[cell_index]
            write_index += 1
        if circle_a[cell_index] > 0:
            mat_ids[write_index] = 2
            mat_vfs[write_index] = circle_a[cell_index]
            write_index += 1
        if circle_b[cell_index] > 0:
            mat_ids[write_index] = 3
            mat_vfs[write_index] = circle_b[cell_index]
            write_index += 1
        if circle_c[cell_index] > 0:
            mat_ids[write_index] = 4
            mat_vfs[write_index] = circle_c[cell_index]

    dataset.AddField(make_FieldCell("scatter_pos", offset))
    dataset.AddField(make_FieldCell("scatter_len", length))
    dataset.AddField(make_FieldWholeDataSet("scatter_ids", mat_ids))
    dataset.AddField(make_FieldWholeDataSet("scatter_vfs", mat_vfs))

    mir = MIRFilter()
    mir.SetIDWholeSetName("scatter_ids")
    mir.SetPositionCellSetName("scatter_pos")
    mir.SetLengthCellSetName("scatter_len")
    mir.SetVFWholeSetName("scatter_vfs")
    mir.SetErrorScaling(0.2)
    mir.SetScalingDecay(1.0)
    mir.SetMaxIterations(0)
    mir.SetMaxPercentError(0.00001)

    output = mir.Execute(dataset)
    assert output.GetNumberOfCells() == 66086


if __name__ == "__main__":
    main()
