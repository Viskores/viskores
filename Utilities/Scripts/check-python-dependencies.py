#!/usr/bin/env python3

##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

"""Check version constraints shared by pyproject.toml and requirements.txt."""

import argparse
import ast
from pathlib import Path
import re
import sys


PACKAGE_NAME = re.compile(r"^([A-Za-z0-9][A-Za-z0-9._-]*)(.*)$")
SECTION = re.compile(r"^\s*\[([^]]+)\]\s*(?:#.*)?$")


def canonical_name(name):
    """Apply the package-name normalization defined by Python packaging."""
    return re.sub(r"[-_.]+", "-", name).lower()


def split_requirement(requirement, source):
    """Return a normalized package name and its version constraint."""
    requirement = requirement.split(";", 1)[0].strip()
    match = PACKAGE_NAME.match(requirement)
    if not match:
        raise ValueError(f"Could not parse dependency {requirement!r} in {source}")

    name, constraint = match.groups()
    constraint = re.sub(r"\s+", "", constraint)
    return canonical_name(name), constraint


def add_requirement(requirements, requirement, source):
    name, constraint = split_requirement(requirement, source)
    previous = requirements.get(name)
    if previous is not None and previous != constraint:
        raise ValueError(
            f"Conflicting constraints for {name} in {source}: "
            f"{previous!r} and {constraint!r}"
        )
    requirements[name] = constraint


def read_pyproject(path):
    """Read build and runtime dependencies without requiring a TOML package."""
    lines = path.read_text(encoding="utf-8").splitlines()
    requirements = {}
    section = None
    line_number = 0

    while line_number < len(lines):
        line = lines[line_number]
        section_match = SECTION.match(line)
        if section_match:
            section = section_match.group(1)

        key = None
        if section == "build-system":
            key = "requires"
        elif section == "project":
            key = "dependencies"

        assignment = re.match(rf"^\s*{key}\s*=\s*(.*)$", line) if key else None
        if not assignment:
            line_number += 1
            continue

        value_lines = [assignment.group(1)]
        while True:
            try:
                dependencies = ast.literal_eval("\n".join(value_lines))
                break
            except (SyntaxError, ValueError) as error:
                parse_error = error
            line_number += 1
            if line_number >= len(lines):
                raise ValueError(
                    f"Could not parse {key} in {path}: {parse_error}"
                ) from parse_error
            value_lines.append(lines[line_number])

        if not isinstance(dependencies, list) or not all(
            isinstance(dependency, str) for dependency in dependencies
        ):
            raise ValueError(f"Expected {key} in {path} to be an array of strings")
        for dependency in dependencies:
            add_requirement(requirements, dependency, path)

        line_number += 1

    return requirements


def read_requirements(path):
    requirements = {}
    for line_number, line in enumerate(
        path.read_text(encoding="utf-8").splitlines(), start=1
    ):
        requirement = line.split("#", 1)[0].strip()
        if not requirement or requirement.startswith("-"):
            continue
        add_requirement(requirements, requirement, f"{path}:{line_number}")
    return requirements


def check_dependencies(pyproject_path, requirements_path):
    pyproject = read_pyproject(pyproject_path)
    requirements = read_requirements(requirements_path)
    shared = sorted(pyproject.keys() & requirements.keys())

    mismatches = [
        (name, pyproject[name], requirements[name])
        for name in shared
        if pyproject[name] != requirements[name]
    ]
    if mismatches:
        for name, pyproject_constraint, requirements_constraint in mismatches:
            print(
                f"ERROR: {name} has constraint {pyproject_constraint!r} in "
                f"{pyproject_path}, but {requirements_constraint!r} in "
                f"{requirements_path}.",
                file=sys.stderr,
            )
        return False

    print(f"Python dependency constraints match for: {', '.join(shared)}")
    return True


def main():
    repository = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pyproject", type=Path, default=repository / "pyproject.toml")
    parser.add_argument(
        "--requirements", type=Path, default=repository / "requirements.txt"
    )
    arguments = parser.parse_args()

    try:
        matches = check_dependencies(arguments.pyproject, arguments.requirements)
    except (OSError, ValueError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1
    return 0 if matches else 1


if __name__ == "__main__":
    sys.exit(main())
