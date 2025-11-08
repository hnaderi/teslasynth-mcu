#!/usr/bin/env python

import json
from pathlib import Path

Import("env")

home = Path.home()
board_name = env.get("BOARD")
board_path = Path(env.subst("$BUILD_DIR"))
db_path = str(board_path / "compile_commands.json")

print(f"Generating .clangd files for board: {board_name}")

with open(db_path) as f:
    db = json.load(f)

espidf_includes = list(
    set(
        flag
        for entry in db
        for flag in entry["command"].split(" ")
        if "esp" in flag and "-I" in flag
    )
)

packages = home / ".platformio" / "packages"

xtensa_query = [
    f"--query-driver={packages / 'toolchain-xtensa-esp-elf' / 'bin' / 'xtensa-esp32-elf-g++'}"
]

std_flags = ["-std=gnu++17", "-Wall", "-Wextra"]

test_includes = [
    f"-I{packages / 'framework-espidf' / 'components' / 'unity' / 'include'}",
    f"-I{packages / 'framework-espidf' / 'components' / 'unity' / 'unity' / 'src'}",
    "-Itest",
]

espidf_defines = [
    "-D__XTENSA__=1",
    "-D__XTENSA_WINDOWED_ABI__=1",
    "-D__XTENSA_SOFT_FLOAT__=1",
    "-DCORE_DEBUG_LEVEL=0",  # Common ESP32 define
    "-DESP_PLATFORM",  # Enables ESP-IDF checks
    "-DCONFIG_NIMBLE_CPP_IDF=1",
]

xtensa_includes = [
    f"-I{board_path / 'config'}",
    f"-I{packages / 'toolchain-xtensa-esp32' / 'xtensa-esp32-elf' / 'include' / 'c++' / '8.4.0'}",
    f"-I{packages / 'toolchain-xtensa-esp-elf' / 'xtensa-esp-elf' / 'include'}",
    f"-I{packages / 'toolchain-xtensa-esp32' / 'xtensa-esp32-elf' / 'sys-include'}",
]

flags_to_remove = [
    "-fno-tree-switch-conversion",
    "-fno-shrink-wrap",
    "-mtext-section-literals",
    "-mlongcalls",
    "-fstrict-volatile-bitfields",
    "-free",
    "-fipa-pta",
]


def write(path):
    def obj(config):
        with open(path, "w") as f:
            json.dump(config, f, indent=2)

    return obj


write("src/.clangd")(
    {
        "CompileFlags": {
            "CompilationDatabase": "..",
            "Add": xtensa_query
            + espidf_defines
            + std_flags
            + espidf_includes
            + xtensa_includes,
            "Remove": flags_to_remove,
        },
        "Diagnostics": {
            "Suppress": [
                "pp_including_mainfile_in_preamble",
                "pp_file_not_found",
                "unknown_pragma",  # ESP-IDF pragmas
            ]
        },
    }
)

write("lib/.clangd")(
    {
        "CompileFlags": {
            "CompilationDatabase": db_path,
            "Add": std_flags,
            "Remove": flags_to_remove + ["-D__XTENSA__*"],
        },
        "Diagnostics": {"Suppress": ["unknown_type_orthogonal"]},
    }
)

write("test/.clangd")(
    {
        "CompileFlags": {
            "Add": std_flags + test_includes,
            "Remove": flags_to_remove + ["-D__XTENSA__*"],
        },
        "Diagnostics": {"Suppress": ["unknown_type_orthogonal"]},
    }
)
