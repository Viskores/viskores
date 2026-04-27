##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

import viskores
import viskores.cont
import viskores.filter.vector_analysis


def assert_doc_contains(object_, *words):
    doc = getattr(object_, "__doc__", None)
    assert doc is not None
    for word in words:
        assert word in doc


def test_core_class_docstrings():
    assert_doc_contains(viskores.cont.DataSet, "cell-set", "fields")
    assert_doc_contains(viskores.cont.Field, "association", "values")
    assert_doc_contains(viskores.cont.CoordinateSystem, "coordinate system")
    assert_doc_contains(viskores.cont.DataSetBuilderUniform, "uniform", "origin")


def test_function_docstrings():
    assert_doc_contains(viskores.cont.Initialize, "Initialize", "runtime")
    assert_doc_contains(viskores.cont.array_from_numpy, "UnknownArrayHandle", "NumPy")
    assert_doc_contains(viskores.cont.create_uniform_dataset, "uniform", "DataSet")


def test_method_docstrings():
    assert_doc_contains(viskores.filter.vector_analysis.VectorMagnitude.Execute, "Execute", "DataSet")


def main():
    test_core_class_docstrings()
    test_function_docstrings()
    test_method_docstrings()


if __name__ == "__main__":
    main()
