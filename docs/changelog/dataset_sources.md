# Added Source class hierarchy

A new class hierarchy for dataset source was added. The intention is to
consolidate and refactor various (procedural) dataset generators for unit
tests, especially the multiple copy&past-ed implementations of the Tangle
field. As they are compiled into a library rather than as header files,
we also expect the overall compile time to decrease.

The public interface of dataset source is modeled after Filter. A new DataSet
is returned by calling the Execute() method of the dataset source, for example:

```cpp
vtkm::Id3 dims(4, 4, 4);
vtkm::source::Tangle tangle(dims);
vtkm::cont::DataSet dataSet = tangle.Execute();
```
