# update-CellLocatorTwoLevelUniformGrid

`CellLocatorTwoLevelUniformGrid` has been renamed to `CellLocatorUniformBins`
for brevity. It has been modified to be a subclass of `vtkm::cont::CellLocator`
and can be used wherever a `CellLocator` is accepted.

`CellLocatorUniformBins` can work with all kinds of datasets, but there are cell
locators that are more efficient for specific data sets. Therefore, a new cell
locator - `CellLocatorGeneral` has been implemented that can be configured to use
specialized cell locators based on its input data. A "configurator" function object
can be specified using the `SetConfigurator` function. The configurator should
have the following signature:

```c++
void (std::unique_ptr<vtkm::cont::CellLocator>&,
     const vtkm::cont::DynamicCellSet&,
     const vtkm::cont::CoordinateSystem&);
```

The configurator is invoked whenever the `Update` method is called and the input
has changed. The current cell locator is passed in a `std::unique_ptr`. Based on
the types of the input cellset and coordinates, and possibly some heuristics on
their values, the current cell locator's parameters can be updated, or a different
cell-locator can be instantiated and transferred to the `unique_ptr`. The default
configurator configures a `CellLocatorUniformGrid` for uniform grid datasets,
a `CellLocatorRecitlinearGrid` for rectilinear datasets, and `CellLocatorUniformBins`
for all other dataset types.

The class `CellLocatorHelper` that implemented similar functionality to
`CellLocatorGeneral` has been removed.

