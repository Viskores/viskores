## Consolidated Python requirements

The Viskores source now provides a `requirements.txt` file that you can use to
install any Python components that may be used during the Viskores build. All
packages can be installed by simply running

```python
pip install -r requirements.txt
```

The use of Python is optional for the Viskores build, but the `requirements.txt`
provides the proper components for the following optional features.

* Compilation of Python bindings.
* Building and checking expanded template code.
* Replicating CI with docker (Utilities/CI/reproduce_ci_env.py).
* Sphinx documentation.
