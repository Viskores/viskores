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

Entries without a binding key are generated. Explicit binding states are manual
for classes with custom binding requirements and excluded for classes outside
the Python API surface.

Generated entries use fieldInputs for field-selection helpers, properties for
Set/Get pairs, directMethods for direct C++ method pointer bindings, and
methods for explicit typed method overloads.
"""

from __future__ import annotations

import argparse
from collections import Counter
from pathlib import Path
import re
import sys
from typing import Any


DEFAULT_BINDING = "generated"
VALID_BINDINGS = {DEFAULT_BINDING, "manual", "excluded"}
VALID_CONSTRUCTORS = {"default", "fileName"}
VALID_REGISTRATION_GROUPS = {"default", "early"}
VALID_BASES = {
    "viskores::cont::Field",
    "viskores::cont::UnknownArrayHandle",
    "viskores::cont::UnknownCellSet",
    "viskores::filter::Filter",
    "viskores::filter::contour::Contour",
    "viskores::io::ImageWriterBase",
    "viskores::rendering::Canvas",
}
GENERATED_LIST_KEYS = {
    "constructors",
    "directMethods",
    "enumOptions",
    "enums",
    "fieldInputs",
    "implicitConstructors",
    "methods",
    "properties",
    "staticMethods",
    "values",
}
KNOWN_KEYS = {
    "base",
    "binding",
    "class",
    "constructor",
    "constructors",
    "directMethods",
    "enum",
    "enumOptions",
    "enums",
    "fieldInputs",
    "guard",
    "header",
    "implicitConstructors",
    "methods",
    "properties",
    "pythonModule",
    "pythonName",
    "reason",
    "registrationGroup",
    "repr",
    "staticMethods",
    "values",
}

FIELD_INPUT_BINDERS = {
    "ActiveField": "BindFilterActiveFieldMethods",
    "ActiveFieldAssociation": "BindFilterActiveFieldAssociationMethod",
    "ActiveFieldName": "BindFilterActiveFieldNameMethods",
    "PrimaryField": "BindFilterPrimaryFieldMethods",
    "SecondaryField": "BindFilterSecondaryFieldMethods",
}

PROPERTY_BINDERS = {
    "OutputFieldName": "BindFilterOutputFieldMethods",
}

ARGUMENT_CONVERTER_EXPRESSIONS = {
    "Bounds": "ParseBounds({name})",
    "Dimensions": "ParseDimensions({name})",
    "Float64List": "ParseFloat64List({name})",
    "IdList": "ParseIdSequence({name})",
    "NumPyId3Array": "ParseId3Array({name})",
    "NumPyVec3fArray": "ParseArrayHandleVec3f({name})",
    "Range": "ParseRange({name})",
    "RangeId3": "ParseRangeId3({name})",
    "StringList": "ParseStringList({name})",
    "UnknownArray": "PythonObjectToUnknownArray({name})",
    "Vec3fDefaultOne": "ParseVec3({name}, viskores::Vec3f(1.0f, 1.0f, 1.0f))",
    "Vec3fDefaultZero": "ParseVec3({name}, viskores::Vec3f(0.0f, 0.0f, 0.0f))",
}
ARGUMENT_CONVERTERS = set(ARGUMENT_CONVERTER_EXPRESSIONS)

RETURN_CONVERTER_EXPRESSIONS = {
    "CoordinateSystemData": (
        "viskores::cont::UncertainArrayHandle<"
        "VISKORES_DEFAULT_TYPE_LIST, VISKORES_DEFAULT_STORAGE_LIST>({result})"
    ),
}
RETURN_CONVERTERS = set(RETURN_CONVERTER_EXPRESSIONS)

TYPE_ARGUMENT_CONVERTER_EXPRESSIONS = {
    "viskores::Id2": "ParseId2({name})",
    "viskores::Id3": "ParseId3({name})",
    "viskores::Vec3f": "ParseVec3({name}, viskores::Vec3f(0.0f, 0.0f, 0.0f))",
    "viskores::Vec3f_32": "ParseVec3f32({name})",
}

TYPE_RETURN_CONVERTER_EXPRESSIONS = {
    "viskores::Id2": "Vec2ToTuple({result})",
    "viskores::Id3": "Vec3ToTuple({result})",
    "viskores::Vec3f": "Vec3ToTuple({result})",
    "viskores::Vec3f_32": "Vec3ToTuple({result})",
    "viskores::Vec<viskores::Range, 3>": "Vec3ToTuple({result})",
}

ENUM_OPTION_EXPRESSIONS = {
    "arithmetic": "nb::is_arithmetic()",
    "flag": "nb::is_flag()",
}

PYTHON_TYPE_OVERRIDES = {
    "viskores::Float32": "double",
    "viskores::Float64": "double",
    "viskores::FloatDefault": "double",
    "viskores::Int32": "long",
    "viskores::Id": "long long",
    "viskores::IdComponent": "long long",
    "viskores::UInt8": "unsigned long",
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


def constructor_type(entry: dict[str, Any]) -> str:
    value = entry.get("constructor", "default")
    return value if isinstance(value, str) else ""


def base_type(entry: dict[str, Any]) -> str | None:
    value = entry.get("base")
    return value if isinstance(value, str) and value else None


def registration_group(entry: dict[str, Any]) -> str:
    value = entry.get("registrationGroup", "default")
    return value if isinstance(value, str) else ""


def split_top_level(text: str, separator: str = ",") -> list[str]:
    parts: list[str] = []
    start = 0
    angle_depth = 0
    bracket_depth = 0
    for index, char in enumerate(text):
        if char == "<":
            angle_depth += 1
        elif char == ">" and angle_depth:
            angle_depth -= 1
        elif char == "[":
            bracket_depth += 1
        elif char == "]" and bracket_depth:
            bracket_depth -= 1
        elif char == separator and angle_depth == 0 and bracket_depth == 0:
            part = text[start:index].strip()
            if part:
                parts.append(part)
            start = index + 1
    tail = text[start:].strip()
    if tail:
        parts.append(tail)
    return parts


def split_default(text: str) -> tuple[str, str | None]:
    angle_depth = 0
    bracket_depth = 0
    for index, char in enumerate(text):
        if char == "<":
            angle_depth += 1
        elif char == ">" and angle_depth:
            angle_depth -= 1
        elif char == "[":
            bracket_depth += 1
        elif char == "]" and bracket_depth:
            bracket_depth -= 1
        elif char == "=" and angle_depth == 0 and bracket_depth == 0:
            return text[:index].strip(), text[index + 1 :].strip()
    return text.strip(), None


def parse_argument_list(args_text: str) -> list[dict[str, str | None]]:
    args: list[dict[str, str | None]] = []
    args_text = args_text.strip()
    if args_text:
        for arg_text in split_top_level(args_text):
            converter = None
            arg_text, default = split_default(arg_text)
            converter_match = re.fullmatch(r"(?P<body>.*)\[(?P<converter>\w+)\]\s*", arg_text)
            if converter_match:
                arg_text = converter_match.group("body").strip()
                converter = converter_match.group("converter")

            if ":" not in arg_text:
                raise ValueError(f"argument '{arg_text}' must be written as name: type")
            arg_name, arg_type = arg_text.split(":", 1)
            arg_name = arg_name.strip()
            arg_type = arg_type.strip()
            if not re.fullmatch(r"\w+", arg_name):
                raise ValueError(f"invalid argument name '{arg_name}'")
            if not arg_type:
                raise ValueError(f"argument '{arg_name}' is missing a type")
            args.append(
                {
                    "name": arg_name,
                    "type": arg_type,
                    "default": default,
                    "converter": converter,
                }
            )
    return args


def parse_method_signature(signature: str) -> dict[str, Any]:
    signature = signature.strip()
    return_type = None
    return_converter = None
    if "->" in signature:
        signature, return_text = signature.split("->", 1)
        return_text = return_text.strip()
        converter_match = re.fullmatch(r"(?P<type>.*)\[(?P<converter>\w+)\]\s*", return_text)
        if converter_match:
            return_type = converter_match.group("type").strip()
            return_converter = converter_match.group("converter")
        else:
            return_type = return_text

    match = re.fullmatch(r"(?P<name>\w+)\((?P<args>.*)\)", signature.strip())
    if not match:
        raise ValueError("expected MethodName(arg: Type, ...) signature")

    return {
        "name": match.group("name"),
        "args": parse_argument_list(match.group("args")),
        "returnType": return_type,
        "returnConverter": return_converter,
    }


def parse_constructor_signature(signature: str) -> dict[str, Any]:
    match = re.fullmatch(r"\((?P<args>.*)\)", signature.strip())
    if not match:
        raise ValueError("expected (arg: Type, ...) constructor signature")
    return {"args": parse_argument_list(match.group("args"))}


def parse_enum_signature(signature: str, class_name: str) -> dict[str, Any]:
    match = re.fullmatch(r"(?P<enum>.*):\s+(?P<values>.*)", signature.strip())
    if not match:
        raise ValueError("expected EnumName: Value, ...")
    enum_text = match.group("enum").strip()
    values_text = match.group("values").strip()
    if not enum_text:
        raise ValueError("missing enum name")

    python_name_value = None
    if " as " in enum_text:
        enum_text, python_name_value = (part.strip() for part in enum_text.split(" as ", 1))
    if not enum_text:
        raise ValueError("missing enum type")

    enum_type = enum_text if "::" in enum_text else f"{class_name}::{enum_text}"
    if python_name_value is None:
        python_name_value = enum_text.rsplit("::", 1)[-1]
    if not re.fullmatch(r"\w+", python_name_value):
        raise ValueError(f"invalid Python enum name '{python_name_value}'")

    values = []
    for value_text in split_top_level(values_text):
        value_text = value_text.strip()
        if not value_text:
            continue
        if "=" in value_text:
            python_value, cpp_value = (part.strip() for part in value_text.split("=", 1))
        else:
            python_value = value_text.rsplit("::", 1)[-1]
            cpp_value = value_text
        if not re.fullmatch(r"\w+", python_value):
            raise ValueError(f"invalid Python enum value '{python_value}'")
        if not cpp_value:
            raise ValueError(f"missing C++ enum value for '{python_value}'")
        if "::" not in cpp_value:
            cpp_value = f"{enum_type}::{cpp_value}"
        values.append({"pythonName": python_value, "cppName": cpp_value})

    if not values:
        raise ValueError("enum must list at least one value")

    return {"type": enum_type, "pythonName": python_name_value, "values": values}


def enum_signature_from_entry(entry: dict[str, Any]) -> str:
    enum_type = entry["enum"]
    enum_text = enum_type
    python_name_value = entry.get("pythonName")
    if python_name_value:
        enum_text = f"{enum_text} as {python_name_value}"
    return f"{enum_text}: {', '.join(entry['values'])}"


def python_argument_type(arg: dict[str, str | None]) -> str:
    converter = arg.get("converter")
    if converter in ARGUMENT_CONVERTER_EXPRESSIONS:
        return "nb::handle"
    arg_type = arg["type"]
    assert arg_type is not None
    if arg_type in TYPE_ARGUMENT_CONVERTER_EXPRESSIONS:
        return "nb::handle"
    return PYTHON_TYPE_OVERRIDES.get(arg_type, arg_type)


def cpp_argument_expression(arg: dict[str, str | None]) -> str:
    name = arg["name"]
    arg_type = arg["type"]
    converter = arg.get("converter")
    assert name is not None
    assert arg_type is not None
    if converter in ARGUMENT_CONVERTER_EXPRESSIONS:
        return ARGUMENT_CONVERTER_EXPRESSIONS[converter].format(name=name)
    if arg_type in TYPE_ARGUMENT_CONVERTER_EXPRESSIONS:
        return TYPE_ARGUMENT_CONVERTER_EXPRESSIONS[arg_type].format(name=name)
    python_type = python_argument_type(arg)
    if python_type != arg_type:
        return f"static_cast<{arg_type}>({name})"
    return name


def argument_needs_lambda(arg: dict[str, str | None]) -> bool:
    return cpp_argument_expression(arg) != arg["name"]


def cpp_result_expression(result: str, return_type: str | None, converter: str | None) -> str:
    if converter in RETURN_CONVERTER_EXPRESSIONS:
        return RETURN_CONVERTER_EXPRESSIONS[converter].format(result=result)
    if return_type in TYPE_RETURN_CONVERTER_EXPRESSIONS:
        return TYPE_RETURN_CONVERTER_EXPRESSIONS[return_type].format(result=result)
    return result


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
    if len(parts) == 2 and parts[0] == "viskores":
        return f"viskores/{parts[1]}.h"
    if len(parts) == 3 and parts[0] == "viskores" and parts[1] == "cont":
        return f"viskores/cont/{parts[2]}.h"
    if len(parts) == 4 and parts[0] == "viskores" and parts[1] == "filter":
        return f"viskores/filter/{parts[2]}/{parts[3]}.h"
    if len(parts) == 3 and parts[0] == "viskores" and parts[1] == "io":
        return f"viskores/io/{parts[2]}.h"
    if len(parts) == 3 and parts[0] == "viskores" and parts[1] == "rendering":
        return f"viskores/rendering/{parts[2]}.h"
    if len(parts) == 3 and parts[0] == "viskores" and parts[1] == "source":
        return f"viskores/source/{parts[2]}.h"
    return None


def resolve_header(entry: dict[str, Any]) -> str | None:
    header = entry.get("header")
    if isinstance(header, str) and header:
        return header
    class_name = entry.get("class")
    if not isinstance(class_name, str):
        class_name = entry.get("enum")
    if not isinstance(class_name, str):
        return None
    return infer_header(class_name)


def infer_guard(class_name: str) -> str | None:
    parts = class_name.split("::")
    if len(parts) == 4 and parts[0] == "viskores" and parts[1] == "filter":
        return f"VISKORES_PYTHON_ENABLE_FILTER_{parts[2].upper()}"
    if len(parts) == 3 and parts[0] == "viskores" and parts[1] == "io":
        return "VISKORES_PYTHON_ENABLE_IO"
    if len(parts) == 3 and parts[0] == "viskores" and parts[1] == "rendering":
        return "VISKORES_PYTHON_ENABLE_RENDERING"
    if len(parts) == 3 and parts[0] == "viskores" and parts[1] == "source":
        return "VISKORES_PYTHON_ENABLE_SOURCE"
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

        binding = binding_type(entry)
        if binding not in VALID_BINDINGS:
            errors.append(
                f"Entry {index}: binding must be one of {', '.join(sorted(VALID_BINDINGS))}"
            )

        enum_type = entry.get("enum")
        class_name = entry.get("class")
        if enum_type:
            if class_name:
                errors.append(f"Entry {index}: use either class or enum, not both")
            if binding != DEFAULT_BINDING:
                errors.append(f"{enum_type}: enum entries cannot set binding")
            if not isinstance(enum_type, str):
                errors.append(f"Entry {index}: enum must be a string")
                continue
            enum_options = entry.get("enumOptions", [])
            if not isinstance(enum_options, list):
                errors.append(f"{enum_type}: enumOptions must be a list")
                enum_options = []
            for option in enum_options:
                if option not in ENUM_OPTION_EXPRESSIONS:
                    errors.append(f"{enum_type}: unsupported enum option {option}")
            values = entry.get("values")
            if not isinstance(values, list):
                errors.append(f"{enum_type}: values must be a list")
            elif not values:
                errors.append(f"{enum_type}: enum entries require values")
            else:
                try:
                    parse_enum_signature(enum_signature_from_entry(entry), "")
                except ValueError as error:
                    errors.append(f"{enum_type}: invalid enum: {error}")
            header = resolve_header(entry)
            if not header:
                errors.append(f"{enum_type}: missing header and could not infer one")
            elif not (source_root / header).is_file():
                errors.append(f"{enum_type}: header does not exist: {header}")
            continue

        if not isinstance(class_name, str) or not class_name:
            errors.append(f"Entry {index}: missing class or enum")
            continue
        if class_name in seen:
            errors.append(f"Entry {index}: duplicate class {class_name}")
        seen.add(class_name)
        constructor = constructor_type(entry)
        if constructor not in VALID_CONSTRUCTORS:
            errors.append(
                f"{class_name}: constructor must be one of {', '.join(sorted(VALID_CONSTRUCTORS))}"
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
            group = registration_group(entry)
            if group not in VALID_REGISTRATION_GROUPS:
                errors.append(
                    f"{class_name}: registrationGroup must be one of {', '.join(sorted(VALID_REGISTRATION_GROUPS))}"
                )
            base = base_type(entry)
            if base and base not in VALID_BASES:
                errors.append(f"{class_name}: base must be one of {', '.join(sorted(VALID_BASES))}")
            if not python_name(entry):
                errors.append(
                    f"{class_name}: generated entries require pythonName or inferable class"
                )
            if entry.get("constructors") and entry.get("constructor"):
                errors.append(f"{class_name}: use either constructors or constructor, not both")
            for value in entry.get("fieldInputs", []):
                if value not in FIELD_INPUT_BINDERS:
                    errors.append(f"{class_name}: unsupported field input {value}")
            for value in entry.get("constructors", []):
                try:
                    parse_constructor_signature(value)
                except ValueError as error:
                    errors.append(f"{class_name}: invalid constructor '{value}': {error}")
            for value in entry.get("implicitConstructors", []):
                if not isinstance(value, str) or not value:
                    errors.append(f"{class_name}: implicit constructor entries must be types")
            for value in entry.get("enums", []):
                try:
                    parse_enum_signature(value, class_name)
                except ValueError as error:
                    errors.append(f"{class_name}: invalid enum '{value}': {error}")
            for value in entry.get("methods", []):
                try:
                    method = parse_method_signature(value)
                except ValueError as error:
                    errors.append(f"{class_name}: invalid method '{value}': {error}")
                    continue
                for arg in method["args"]:
                    converter = arg.get("converter")
                    if converter and converter not in ARGUMENT_CONVERTERS:
                        errors.append(
                            f"{class_name}: unsupported converter {converter} in method {value}"
                        )
                return_converter = method.get("returnConverter")
                if return_converter and return_converter not in RETURN_CONVERTERS:
                    errors.append(
                        f"{class_name}: unsupported return converter {return_converter} in method {value}"
                    )
            for value in entry.get("staticMethods", []):
                try:
                    method = parse_method_signature(value)
                except ValueError as error:
                    errors.append(f"{class_name}: invalid static method '{value}': {error}")
                    continue
                for arg in method["args"]:
                    converter = arg.get("converter")
                    if converter and converter not in ARGUMENT_CONVERTERS:
                        errors.append(
                            f"{class_name}: unsupported converter {converter} in static method {value}"
                        )
                return_converter = method.get("returnConverter")
                if return_converter and return_converter not in RETURN_CONVERTERS:
                    errors.append(
                        f"{class_name}: unsupported return converter {return_converter} in static method {value}"
                    )
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
        name = entry.get("class", entry.get("enum"))
        print(f"  {name}{suffix}")


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
    manual = sorted((entry for entry in entries if binding_type(entry) == "manual"),
                    key=lambda item: item["class"])
    excluded = sorted((entry for entry in entries if binding_type(entry) == "excluded"),
                      key=lambda item: item["class"])

    if args.list == "excluded":
        print_entries(excluded, "Excluded classes:")
    elif args.list == "manual":
        print_entries(manual, "Manual classes:")
    elif args.list == "missing":
        print_entries(missing, "Classes without manifest entries:")
    elif args.list == "all":
        print_entries(sorted(entries, key=lambda item: item.get("class", item.get("enum", ""))),
                      "Manifest entries:")
    else:
        counts = Counter(binding_type(entry) for entry in entries)
        print(f"Manifest entries: {len(entries)}")
        for binding in sorted(counts):
            print(f"  {binding}: {counts[binding]}")
        print(f"Scanned filter/IO candidate classes: {len(candidates)}")
        print(f"Classes without manifest entries: {len(missing)}")
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


def generated_method_call(class_name: str, signature: str, variable: str) -> list[str]:
    method = parse_method_signature(signature)
    method_name = method["name"]
    args = method["args"]
    return_converter = method.get("returnConverter")
    return_type = method.get("returnType")
    returns_reference = return_type is not None and return_type.endswith("&")
    has_implicit_return_converter = return_type in TYPE_RETURN_CONVERTER_EXPRESSIONS
    if not args and not return_type and not return_converter and not has_implicit_return_converter:
        return [f'    {variable}.def("{method_name}", &{class_name}::{method_name});']

    lambda_args = [
        f"{python_argument_type(arg)} {arg['name']}"
        for arg in args
    ]
    converted_args: list[str] = []
    conversion_lines: list[str] = []
    for arg in args:
        name = arg["name"]
        arg_type = arg["type"]
        assert name is not None
        assert arg_type is not None
        expression = cpp_argument_expression(arg)
        if expression == name:
            converted_args.append(name)
            continue
        converted_name = f"{name}Value"
        conversion_lines.append(f"        auto {converted_name} = {expression};")
        converted_args.append(converted_name)
    call_args = ", ".join(converted_args)
    result_expression = f"self.{method_name}({call_args})"
    result_expression = cpp_result_expression(result_expression, return_type, return_converter)
    lines = [
        f'    {variable}.def(',
        f'      "{method_name}",',
        f"      []({class_name}& self{', ' if lambda_args else ''}{', '.join(lambda_args)})"
        f"{f' -> {return_type}' if returns_reference else ''}",
        "      {",
    ]
    lines.extend(conversion_lines)
    lines.append(f"        return {result_expression};")
    lines.append("      },")
    if not args:
        if returns_reference:
            lines[-1] = "      }, nb::rv_policy::reference_internal);"
        else:
            lines[-1] = "      });"
        return lines
    if returns_reference:
        lines.append("      nb::rv_policy::reference_internal,")
    for index, arg in enumerate(args):
        suffix = "," if index + 1 < len(args) else ");"
        default = arg.get("default")
        if default is None:
            lines.append(f'      nb::arg("{arg["name"]}"){suffix}')
        else:
            lines.append(f'      nb::arg("{arg["name"]}") = {default}{suffix}')
    return lines


def generated_static_method_call(class_name: str, signature: str, variable: str) -> list[str]:
    method = parse_method_signature(signature)
    method_name = method["name"]
    args = method["args"]
    return_converter = method.get("returnConverter")
    return_type = method.get("returnType")
    returns_reference = return_type is not None and return_type.endswith("&")
    has_implicit_return_converter = return_type in TYPE_RETURN_CONVERTER_EXPRESSIONS
    if not args and not return_type and not return_converter and not has_implicit_return_converter:
        return [f'    {variable}.def_static("{method_name}", &{class_name}::{method_name});']

    lambda_args = [
        f"{python_argument_type(arg)} {arg['name']}"
        for arg in args
    ]
    converted_args: list[str] = []
    conversion_lines: list[str] = []
    for arg in args:
        name = arg["name"]
        arg_type = arg["type"]
        assert name is not None
        assert arg_type is not None
        expression = cpp_argument_expression(arg)
        if expression == name:
            converted_args.append(name)
            continue
        converted_name = f"{name}Value"
        conversion_lines.append(f"        auto {converted_name} = {expression};")
        converted_args.append(converted_name)
    call_args = ", ".join(converted_args)
    result_expression = f"{class_name}::{method_name}({call_args})"
    result_expression = cpp_result_expression(result_expression, return_type, return_converter)
    lines = [
        f'    {variable}.def_static(',
        f'      "{method_name}",',
        f"      []({', '.join(lambda_args)})"
        f"{f' -> {return_type}' if returns_reference else ''}",
        "      {",
    ]
    lines.extend(conversion_lines)
    lines.append(f"        return {result_expression};")
    lines.append("      },")
    if not args:
        if returns_reference:
            lines[-1] = "      }, nb::rv_policy::reference_internal);"
        else:
            lines[-1] = "      });"
        return lines
    if returns_reference:
        lines.append("      nb::rv_policy::reference_internal,")
    for index, arg in enumerate(args):
        suffix = "," if index + 1 < len(args) else ");"
        default = arg.get("default")
        if default is None:
            lines.append(f'      nb::arg("{arg["name"]}"){suffix}')
        else:
            lines.append(f'      nb::arg("{arg["name"]}") = {default}{suffix}')
    return lines


def constructor_def_call(class_name: str, signature: str, variable: str) -> list[str]:
    constructor = parse_constructor_signature(signature)
    args = constructor["args"]
    if not args:
        return [f"    {variable}.def(nb::init<>());"]

    needs_lambda = any(argument_needs_lambda(arg) for arg in args)
    if needs_lambda:
        lambda_args = [
            f"{python_argument_type(arg)} {arg['name']}"
            for arg in args
        ]
        converted_args: list[str] = []
        conversion_lines: list[str] = []
        for arg in args:
            name = arg["name"]
            assert name is not None
            expression = cpp_argument_expression(arg)
            if expression == name:
                converted_args.append(name)
                continue
            converted_name = f"{name}Value"
            conversion_lines.append(f"        auto {converted_name} = {expression};")
            converted_args.append(converted_name)
        call_args = ", ".join(converted_args)
        lines = [
            f"    {variable}.def(",
            '      "__init__",',
            f"      []({class_name}* self{', ' if lambda_args else ''}{', '.join(lambda_args)})",
            "      {",
        ]
        lines.extend(conversion_lines)
        lines.append(f"        new (self) {class_name}({call_args});")
        lines.append("      },")
        for index, arg in enumerate(args):
            suffix = "," if index + 1 < len(args) else ");"
            default = arg.get("default")
            if default is None:
                lines.append(f'      nb::arg("{arg["name"]}"){suffix}')
            else:
                lines.append(f'      nb::arg("{arg["name"]}") = {default}{suffix}')
        return lines

    cxx_types = ", ".join(arg["type"] for arg in args)
    lines = [f"    {variable}.def(nb::init<{cxx_types}>(),"]
    for index, arg in enumerate(args):
        suffix = "," if index + 1 < len(args) else ");"
        default = arg.get("default")
        if default is None:
            lines.append(f'             nb::arg("{arg["name"]}"){suffix}')
        else:
            lines.append(f'             nb::arg("{arg["name"]}") = {default}{suffix}')
    return lines


def enum_def_call(
    class_name: str, signature: str, variable: str, enum_options: list[str] | None = None
) -> list[str]:
    enum = parse_enum_signature(signature, class_name)
    option_expressions = [
        ENUM_OPTION_EXPRESSIONS[option] for option in (enum_options or [])
    ]
    enum_args = ", ".join([variable, f"\"{enum['pythonName']}\"", *option_expressions])
    lines = [
        f"    nb::enum_<{enum['type']}>({enum_args})",
    ]
    for value in enum["values"]:
        lines.append(f"      .value(\"{value['pythonName']}\", {value['cppName']})")
    lines[-1] += ";"
    return lines


def class_constructor_call(entry: dict[str, Any], variable: str) -> list[str]:
    class_name = entry["class"]
    template_args = class_name
    base = base_type(entry)
    if base:
        template_args = f"{class_name}, {base}"
    if entry.get("constructors"):
        return [
            f"    auto {variable} = BindClass<",
            f'      {template_args}>(m, erase_existing_name, "{python_name(entry)}");',
        ]
    binder = {
        "default": "BindClassWithDefaultConstructor",
        "fileName": "BindClassWithFileNameConstructor",
    }[constructor_type(entry)]
    return [
        f"    auto {variable} = {binder}<",
        f'      {template_args}>(m, erase_existing_name, "{python_name(entry)}");',
    ]


def append_generated_classes(
    lines: list[str],
    function_name: str,
    generated_classes: list[dict[str, Any]],
) -> None:
    lines.extend(
        [
            f"void {function_name}(",
            "  nb::module_& m,",
            "  const std::function<void(const char*)>& erase_existing_name)",
            "{",
        ]
    )

    for index, entry in enumerate(generated_classes):
        class_name = entry["class"]
        variable = f"cls{index}"
        guard = effective_guard(entry)
        if guard:
            lines.append(f"#if {guard}")
        lines.append("  {")
        lines.extend(class_constructor_call(entry, variable))
        for enum_signature in entry.get("enums", []):
            lines.extend(enum_def_call(class_name, enum_signature, variable))
        for signature in entry.get("constructors", []):
            lines.extend(constructor_def_call(class_name, signature, variable))
        for constructor_type_name in entry.get("implicitConstructors", []):
            lines.append(
                f"    {variable}.def(nb::init_implicit<{constructor_type_name}>(), nb::arg(\"function\"));"
            )
        for name in entry.get("fieldInputs", []):
            lines.append(binding_call(FIELD_INPUT_BINDERS[name], class_name, variable))
        for name in entry.get("properties", []):
            if name in PROPERTY_BINDERS:
                lines.append(binding_call(PROPERTY_BINDERS[name], class_name, variable))
            else:
                lines.extend(direct_property_calls(class_name, name, variable))
        for name in entry.get("directMethods", []):
            lines.append(direct_method_call(class_name, name, variable))
        for signature in entry.get("methods", []):
            lines.extend(generated_method_call(class_name, signature, variable))
        for signature in entry.get("staticMethods", []):
            lines.extend(generated_static_method_call(class_name, signature, variable))
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

    lines.append("}")
    lines.append("")


def command_generate(args: argparse.Namespace) -> int:
    entries = load_manifest(args.manifest)
    errors = validate_manifest(entries, args.source_root)
    if errors:
        for error in errors:
            print(error, file=sys.stderr)
        return 1

    generated = [entry for entry in entries if binding_type(entry) == DEFAULT_BINDING]
    generated_enums = [entry for entry in generated if "enum" in entry]
    generated_early_classes = [
        entry for entry in generated if "class" in entry and registration_group(entry) == "early"
    ]
    generated_classes = [
        entry for entry in generated if "class" in entry and registration_group(entry) == "default"
    ]
    lines: list[str] = [
        "// This file is generated by python_bindings/tools/binding_manifest.py.",
        "// Do not edit by hand.",
        "",
        '#include "pyviskores_common.h"',
        '#include "pyviskores_bindings.h"',
        "",
        "#include <nanobind/stl/shared_ptr.h>",
        "#include <nanobind/stl/string.h>",
        "",
    ]
    headers: dict[str, str | None] = {}
    for entry in generated:
        header = resolve_header(entry)
        if not header:
            continue
        guard = effective_guard(entry)
        if header not in headers:
            headers[header] = guard
        elif headers[header] != guard:
            headers[header] = None
    for header, guard in sorted(headers.items()):
        if guard:
            lines.append(f"#if {guard}")
        lines.append(f"#include <{header}>")
        if guard:
            lines.append("#endif")

    lines.extend(
        [
            "",
            "namespace viskores::python::bindings",
            "{",
            "",
            "void RegisterNanobindGeneratedEnums(nb::module_& m)",
            "{",
        ]
    )

    for enum_entry in generated_enums:
        guard = effective_guard(enum_entry)
        if guard:
            lines.append(f"#if {guard}")
        lines.extend(
            enum_def_call(
                "", enum_signature_from_entry(enum_entry), "m", enum_entry.get("enumOptions")
            )
        )
        if guard:
            lines.append("#endif")

    lines.extend(["}", ""])
    append_generated_classes(lines, "RegisterNanobindGeneratedEarlyClasses", generated_early_classes)
    append_generated_classes(lines, "RegisterNanobindGeneratedClasses", generated_classes)
    lines.extend(["} // namespace viskores::python::bindings", ""])

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
        choices=("summary", "excluded", "manual", "missing", "all"),
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
