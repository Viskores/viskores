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

"""Utilities for the Python binding manifest.

The manifest intentionally uses a small YAML subset so this script does not add
PyYAML as a build dependency. Supported values are scalar strings and lists of
scalar strings.

Entries without a binding key are generated. Explicit binding states are:
deferred for classes to convert in a later generated pass, manual for classes
with custom binding requirements, and excluded for classes outside the Python
API surface.

Generated entries use fieldInputs for field-selection helpers, properties for
Set/Get pairs, directMethods for direct C++ method pointer bindings, and
methodAdapters for reusable helper-backed bindings such as Execute and
coordinate-system selection.
"""

from __future__ import annotations

import argparse
from collections import Counter
from pathlib import Path
import re
import sys
from typing import Any


DEFAULT_BINDING = "generated"
VALID_BINDINGS = {DEFAULT_BINDING, "deferred", "manual", "excluded"}
GENERATED_LIST_KEYS = {"directMethods", "fieldInputs", "methodAdapters", "properties"}
KNOWN_KEYS = {
    "binding",
    "class",
    "directMethods",
    "fieldInputs",
    "guard",
    "header",
    "methodAdapters",
    "properties",
    "pythonModule",
    "pythonName",
    "reason",
    "repr",
}

FIELD_INPUT_BINDERS = {
    "ActiveField": "BindFilterActiveFieldMethods",
    "ActiveFieldName": "BindFilterActiveFieldNameMethods",
    "PrimaryField": "BindFilterPrimaryFieldMethods",
    "SecondaryField": "BindFilterSecondaryFieldMethods",
}

PROPERTY_BINDERS = {
    "OutputFieldName": "BindFilterOutputFieldMethods",
}

METHOD_ADAPTER_BINDERS = {
    "Execute": "BindFilterExecuteMethod",
    "FieldsToPass": "BindFilterFieldsToPassMethod",
}

COORDINATE_SYSTEM_ADAPTERS = {
    "CoordinateSystem": {
        "setUse": "SetUseCoordinateSystemAsField",
        "getUse": "GetUseCoordinateSystemAsField",
        "setCoordinateSystem": "SetActiveCoordinateSystem",
        "getCoordinateSystemIndex": "GetActiveCoordinateSystemIndex",
    },
    "PrimaryCoordinateSystem": {
        "setUse": "SetUseCoordinateSystemAsPrimaryField",
        "getUse": "GetUseCoordinateSystemAsPrimaryField",
        "setCoordinateSystem": "SetPrimaryCoordinateSystem",
        "getCoordinateSystemIndex": "GetPrimaryCoordinateSystemIndex",
    },
    "SecondaryCoordinateSystem": {
        "setUse": "SetUseCoordinateSystemAsSecondaryField",
        "getUse": "GetUseCoordinateSystemAsSecondaryField",
        "setCoordinateSystem": "SetSecondaryCoordinateSystem",
        "getCoordinateSystemIndex": "GetSecondaryCoordinateSystemIndex",
    },
}

REPR_BINDERS = {
    "ActiveField": "BindActiveFieldRepr",
}

FILTER_CLASS_RE = re.compile(
    r"class\s+(?:\w+_EXPORT\s+)?(?P<class>\w+)\s*(?::\s*public\s+(?P<base>[^{]+?))?\s*\{",
    re.DOTALL,
)
IO_CLASS_RE = re.compile(r"class\s+VISKORES_IO_EXPORT\s+(?P<class>\w+)")


def strip_yaml_comment(line: str) -> str:
    quote: str | None = None
    for index, char in enumerate(line):
        if quote:
            if char == quote:
                quote = None
        elif char in ("'", '"'):
            quote = char
        elif char == "#":
            return line[:index]
    return line


def parse_scalar(value: str) -> str:
    value = value.strip()
    if len(value) >= 2 and value[0] == value[-1] and value[0] in ("'", '"'):
        return value[1:-1]
    return value


def split_key_value(text: str, line_number: int) -> tuple[str, str]:
    if ":" not in text:
        raise ValueError(f"Line {line_number}: expected 'key: value'.")
    key, value = text.split(":", 1)
    key = key.strip()
    if not key:
        raise ValueError(f"Line {line_number}: empty key.")
    return key, parse_scalar(value)


def load_manifest_file(path: Path) -> list[dict[str, Any]]:
    entries: list[dict[str, Any]] = []
    current: dict[str, Any] | None = None
    list_key: str | None = None

    for line_number, raw_line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = strip_yaml_comment(raw_line).rstrip()
        if not line.strip():
            continue

        stripped = line.strip()
        indent = len(line) - len(line.lstrip(" "))

        if stripped.startswith("- "):
            value = stripped[2:].strip()
            if indent == 0:
                current = {}
                entries.append(current)
                list_key = None
                if value:
                    key, scalar = split_key_value(value, line_number)
                    current[key] = scalar
                continue

            if current is None or list_key is None:
                raise ValueError(f"{path}:{line_number}: list item without a list key.")
            current[list_key].append(parse_scalar(value))
            continue

        if current is None:
            raise ValueError(f"{path}:{line_number}: manifest must start with a list item.")

        key, value = split_key_value(stripped, line_number)
        if value == "":
            current[key] = []
            list_key = key
        else:
            current[key] = value
            list_key = None

    return entries


def load_manifest(path: Path) -> list[dict[str, Any]]:
    if path.is_dir():
        entries: list[dict[str, Any]] = []
        for manifest_file in sorted(path.glob("*.yml")):
            entries.extend(load_manifest_file(manifest_file))
        return entries
    return load_manifest_file(path)


def manifest_by_class(entries: list[dict[str, Any]]) -> dict[str, dict[str, Any]]:
    result: dict[str, dict[str, Any]] = {}
    for entry in entries:
        class_name = entry.get("class")
        if not isinstance(class_name, str):
            continue
        result[class_name] = entry
    return result


def binding_type(entry: dict[str, Any]) -> str:
    value = entry.get("binding", DEFAULT_BINDING)
    return value if isinstance(value, str) else ""


def class_component(class_name: str) -> str:
    return class_name.rsplit("::", 1)[-1]


def python_name(entry: dict[str, Any]) -> str | None:
    explicit = entry.get("pythonName")
    if isinstance(explicit, str) and explicit:
        return explicit
    class_name = entry.get("class")
    if isinstance(class_name, str) and class_name:
        return class_component(class_name)
    return None


def infer_header(class_name: str) -> str | None:
    parts = class_name.split("::")
    if len(parts) == 4 and parts[0] == "viskores" and parts[1] == "filter":
        return f"viskores/filter/{parts[2]}/{parts[3]}.h"
    if len(parts) == 3 and parts[0] == "viskores" and parts[1] == "io":
        return f"viskores/io/{parts[2]}.h"
    return None


def resolve_header(entry: dict[str, Any]) -> str | None:
    header = entry.get("header")
    if isinstance(header, str) and header:
        return header
    class_name = entry.get("class")
    if not isinstance(class_name, str):
        return None
    return infer_header(class_name)


def infer_guard(class_name: str) -> str | None:
    parts = class_name.split("::")
    if len(parts) == 4 and parts[0] == "viskores" and parts[1] == "filter":
        return f"VISKORES_PYTHON_ENABLE_FILTER_{parts[2].upper()}"
    if len(parts) == 3 and parts[0] == "viskores" and parts[1] == "io":
        return "VISKORES_PYTHON_ENABLE_IO"
    return None


def effective_guard(entry: dict[str, Any]) -> str | None:
    guard = entry.get("guard")
    if isinstance(guard, str) and guard:
        return guard
    class_name = entry.get("class")
    if isinstance(class_name, str):
        return infer_guard(class_name)
    return None


def validate_manifest(entries: list[dict[str, Any]], source_root: Path) -> list[str]:
    errors: list[str] = []
    seen: set[str] = set()
    for index, entry in enumerate(entries, 1):
        unknown = sorted(set(entry) - KNOWN_KEYS)
        if unknown:
            errors.append(f"Entry {index}: unknown keys: {', '.join(unknown)}")

        class_name = entry.get("class")
        if not isinstance(class_name, str) or not class_name:
            errors.append(f"Entry {index}: missing class")
            continue
        if class_name in seen:
            errors.append(f"Entry {index}: duplicate class {class_name}")
        seen.add(class_name)

        binding = binding_type(entry)
        if binding not in VALID_BINDINGS:
            errors.append(
                f"{class_name}: binding must be one of {', '.join(sorted(VALID_BINDINGS))}"
            )

        header = resolve_header(entry)
        if not header:
            errors.append(f"{class_name}: missing header and could not infer one")
        elif not (source_root / header).is_file():
            errors.append(f"{class_name}: header does not exist: {header}")

        for key in GENERATED_LIST_KEYS:
            value = entry.get(key, [])
            if not isinstance(value, list):
                errors.append(f"{class_name}: {key} must be a list")

        if binding in {"manual", "excluded"} and not entry.get("reason"):
            errors.append(f"{class_name}: {binding} entries require a reason")

        if binding == DEFAULT_BINDING:
            if not python_name(entry):
                errors.append(
                    f"{class_name}: generated entries require pythonName or inferable class"
                )
            for value in entry.get("fieldInputs", []):
                if value not in FIELD_INPUT_BINDERS:
                    errors.append(f"{class_name}: unsupported field input {value}")
            for value in entry.get("methodAdapters", []):
                if value not in METHOD_ADAPTER_BINDERS and value not in COORDINATE_SYSTEM_ADAPTERS:
                    errors.append(f"{class_name}: unsupported method adapter {value}")
            repr_value = entry.get("repr")
            if repr_value and repr_value not in REPR_BINDERS:
                errors.append(f"{class_name}: unsupported repr {repr_value}")

    return errors


def filter_namespace_from_header(header: Path) -> str | None:
    parts = header.parts
    try:
        filter_index = parts.index("filter")
    except ValueError:
        return None
    if filter_index + 1 >= len(parts) - 1:
        return None
    group = parts[filter_index + 1]
    return f"viskores::filter::{group}"


def scan_filter_candidates(source_root: Path) -> dict[str, str]:
    candidates: dict[str, str] = {}
    filter_root = source_root / "viskores" / "filter"
    if not filter_root.is_dir():
        return candidates

    skip_parts = {"internal", "testing", "worklet"}
    for header in filter_root.rglob("*.h"):
        relative = header.relative_to(source_root)
        if skip_parts.intersection(relative.parts):
            continue
        namespace = filter_namespace_from_header(relative)
        if namespace is None:
            continue

        text = header.read_text(encoding="utf-8", errors="ignore")
        for match in FILTER_CLASS_RE.finditer(text):
            base = " ".join((match.group("base") or "").split())
            if "viskores::filter::" not in base:
                continue
            class_name = match.group("class")
            candidates[f"{namespace}::{class_name}"] = str(relative)
    return candidates


def scan_io_candidates(source_root: Path) -> dict[str, str]:
    candidates: dict[str, str] = {}
    io_root = source_root / "viskores" / "io"
    if not io_root.is_dir():
        return candidates

    for header in io_root.glob("*.h"):
        relative = header.relative_to(source_root)
        text = header.read_text(encoding="utf-8", errors="ignore")
        for match in IO_CLASS_RE.finditer(text):
            class_name = match.group("class")
            if class_name.endswith(("Reader", "Writer", "ReaderBase", "WriterBase")):
                candidates[f"viskores::io::{class_name}"] = str(relative)
    return candidates


def scan_candidates(source_root: Path) -> dict[str, str]:
    candidates = scan_filter_candidates(source_root)
    candidates.update(scan_io_candidates(source_root))
    return candidates


def print_entries(entries: list[dict[str, Any]], heading: str) -> None:
    print(heading)
    if not entries:
        print("  none")
        return
    for entry in entries:
        reason = entry.get("reason")
        suffix = f" -- {reason}" if reason else ""
        print(f"  {entry['class']}{suffix}")


def command_report(args: argparse.Namespace) -> int:
    entries = load_manifest(args.manifest)
    manifest = manifest_by_class(entries)
    candidates = scan_candidates(args.source_root)

    errors = validate_manifest(entries, args.source_root)
    if errors:
        for error in errors:
            print(error, file=sys.stderr)
        return 1

    missing = [
        {"class": class_name, "header": header}
        for class_name, header in sorted(candidates.items())
        if class_name not in manifest
    ]
    deferred = sorted(
        (entry for entry in entries if binding_type(entry) == "deferred"),
        key=lambda item: item["class"],
    )
    manual = sorted((entry for entry in entries if binding_type(entry) == "manual"),
                    key=lambda item: item["class"])
    excluded = sorted((entry for entry in entries if binding_type(entry) == "excluded"),
                      key=lambda item: item["class"])

    if args.list == "excluded":
        print_entries(excluded, "Excluded classes:")
    elif args.list == "deferred":
        print_entries(deferred, "Deferred classes:")
    elif args.list == "manual":
        print_entries(manual, "Manual classes:")
    elif args.list == "missing":
        print_entries(missing, "Classes without manifest entries:")
    elif args.list == "all":
        print_entries(sorted(entries, key=lambda item: item["class"]), "Manifest entries:")
    else:
        counts = Counter(binding_type(entry) for entry in entries)
        print(f"Manifest entries: {len(entries)}")
        for binding in sorted(counts):
            print(f"  {binding}: {counts[binding]}")
        print(f"Candidate filter/IO classes found: {len(candidates)}")
        print(f"Classes without manifest entries: {len(missing)}")
        print(f"Deferred classes: {len(deferred)}")
        print(f"Excluded classes: {len(excluded)}")

    return 1 if args.fail_on_missing and missing else 0


def binding_call(binder: str, class_name: str, variable: str) -> str:
    return f"    {binder}<{class_name}>({variable});"


def direct_property_calls(class_name: str, property_name: str, variable: str) -> list[str]:
    return [
        f'    {variable}.def("Set{property_name}",',
        f"             &{class_name}::Set{property_name},",
        '             nb::arg("value"));',
        f'    {variable}.def("Get{property_name}",',
        f"             &{class_name}::Get{property_name});",
    ]


def direct_method_call(class_name: str, method_name: str, variable: str) -> str:
    return f'    {variable}.def("{method_name}", &{class_name}::{method_name});'


def coordinate_system_adapter_call(
    class_name: str, adapter_name: str, variable: str
) -> list[str]:
    adapter = COORDINATE_SYSTEM_ADAPTERS[adapter_name]
    lines: list[str] = []
    set_use = adapter.get("setUse")
    get_use = adapter.get("getUse")
    if set_use and get_use:
        lines.extend(
            [
                "    BindFilterUseCoordinateSystemAsFieldMethods(",
                f"      {variable},",
                f'      "{set_use}",',
                f"      []({class_name}& self, bool enabled) {{ self.{set_use}(enabled); }},",
                f'      "{get_use}",',
                f"      [](const {class_name}& self) {{ return self.{get_use}(); }});",
            ]
        )

    set_coordinate_system = adapter.get("setCoordinateSystem")
    get_coordinate_system_index = adapter.get("getCoordinateSystemIndex")
    if set_coordinate_system and get_coordinate_system_index:
        lines.extend(
            [
                "    BindFilterCoordinateSystemIndexMethods(",
                f"      {variable},",
                f'      "{set_coordinate_system}",',
                f"      []({class_name}& self, viskores::Id index)",
                f"      {{ self.{set_coordinate_system}(index); }},",
                f'      "{get_coordinate_system_index}",',
                f"      [](const {class_name}& self)",
                f"      {{ return self.{get_coordinate_system_index}(); }});",
            ]
        )
    return lines


def method_adapter_calls(class_name: str, adapter_name: str, variable: str) -> list[str]:
    binder = METHOD_ADAPTER_BINDERS.get(adapter_name)
    if binder:
        return [binding_call(binder, class_name, variable)]
    return coordinate_system_adapter_call(class_name, adapter_name, variable)


def command_generate(args: argparse.Namespace) -> int:
    entries = load_manifest(args.manifest)
    errors = validate_manifest(entries, args.source_root)
    if errors:
        for error in errors:
            print(error, file=sys.stderr)
        return 1

    generated = [entry for entry in entries if binding_type(entry) == DEFAULT_BINDING]
    lines: list[str] = [
        "// This file is generated by python_bindings/tools/binding_manifest.py.",
        "// Do not edit by hand.",
        "",
        '#include "pyviskores_common.h"',
        '#include "pyviskores_bindings.h"',
        "",
        "#include <nanobind/stl/string.h>",
        "",
    ]
    for header in sorted({resolve_header(entry) for entry in generated}):
        lines.append(f"#include <{header}>")

    lines.extend(
        [
            "",
            "namespace viskores::python::bindings",
            "{",
            "",
            "void RegisterNanobindGeneratedClasses(",
            "  nb::module_& m,",
            "  const std::function<void(const char*)>& erase_existing_name)",
            "{",
        ]
    )

    for index, entry in enumerate(generated):
        class_name = entry["class"]
        variable = f"cls{index}"
        guard = effective_guard(entry)
        if guard:
            lines.append(f"#if {guard}")
        lines.append("  {")
        lines.append(f"    auto {variable} = BindClassWithDefaultConstructor<")
        lines.append(f"      {class_name}>(m, erase_existing_name, \"{python_name(entry)}\");")
        for name in entry.get("fieldInputs", []):
            lines.append(binding_call(FIELD_INPUT_BINDERS[name], class_name, variable))
        for name in entry.get("properties", []):
            if name in PROPERTY_BINDERS:
                lines.append(binding_call(PROPERTY_BINDERS[name], class_name, variable))
            else:
                lines.extend(direct_property_calls(class_name, name, variable))
        for name in entry.get("directMethods", []):
            lines.append(direct_method_call(class_name, name, variable))
        for name in entry.get("methodAdapters", []):
            lines.extend(method_adapter_calls(class_name, name, variable))
        repr_name = entry.get("repr")
        if repr_name:
            qualified_name = f"{entry.get('pythonModule', 'viskores')}.{python_name(entry)}"
            lines.append(
                f'  {REPR_BINDERS[repr_name]}<{class_name}>({variable}, "{qualified_name}");'
            )
        lines.append("  }")
        if guard:
            lines.append("#endif")
        lines.append("")

    lines.extend(
        [
            "}",
            "",
            "} // namespace viskores::python::bindings",
            "",
        ]
    )

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(lines), encoding="utf-8")
    return 0


def make_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.set_defaults(func=None)
    parser.add_argument(
        "--manifest",
        type=Path,
        default=Path(__file__).resolve().parents[1] / "binding_manifest",
    )
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[2])

    subparsers = parser.add_subparsers(dest="command")
    report = subparsers.add_parser("report", help="Report manifest coverage.")
    report.add_argument(
        "--list",
        choices=("summary", "deferred", "excluded", "manual", "missing", "all"),
        default="summary",
    )
    report.add_argument("--fail-on-missing", action="store_true")
    report.set_defaults(func=command_report)

    generate = subparsers.add_parser(
        "generate", help="Generate nanobind code from generated entries."
    )
    generate.add_argument("--output", type=Path, required=True)
    generate.set_defaults(func=command_generate)
    return parser


def main() -> int:
    parser = make_parser()
    args = parser.parse_args()
    if args.func is None:
        args.func = command_report
        args.list = "summary"
        args.fail_on_missing = False
    args.manifest = args.manifest.resolve()
    args.source_root = args.source_root.resolve()
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
