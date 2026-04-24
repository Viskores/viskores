#!/usr/bin/env python3

##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

import argparse
import sys

import viskores.cont
from viskores.filter.contour import ClipWithField
from viskores.io import VTKDataSetReader, VTKDataSetWriter


def main():
    argv = list(sys.argv)
    # Let Viskores process its command line options, such as device selection.
    viskores.cont.Initialize(argv)

    parser = argparse.ArgumentParser(
        description="Python port of examples/clipping/Clipping.cxx"
    )
    parser.add_argument("in_data")
    parser.add_argument("field_name")
    parser.add_argument("clip_value", type=float)
    parser.add_argument("out_data", nargs="?", default="out_data.vtk")
    args = parser.parse_args(argv[1:])

    # Read the data set.
    reader = VTKDataSetReader(args.in_data)
    input_dataset = reader.ReadDataSet()

    # Clip the data set.
    clip_filter = ClipWithField()
    clip_filter.SetActiveField(args.field_name)
    clip_filter.SetClipValue(args.clip_value)
    output_dataset = clip_filter.Execute(input_dataset)

    # Write the result.
    writer = VTKDataSetWriter(args.out_data)
    writer.WriteDataSet(output_dataset)


if __name__ == "__main__":
    main()
