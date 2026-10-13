"""Embed the standalone template sources in the SDK generator."""
import json
from pathlib import Path
import sys

root, imgui, output = map(Path, sys.argv[1:])
files = {}
for source, destination in [(root / "sdk-base", "sdk-base"),
                            (root / "templates/mod-template", "")]:
    for path in sorted(source.rglob("*")):
        if path.is_file():
            name = path.relative_to(source).as_posix()
            files[f"{destination}/{name}".lstrip("/")] = path
for name in ["imgui.cpp", "imgui_draw.cpp", "imgui_tables.cpp", "imgui_widgets.cpp",
             "imgui.h", "imgui_internal.h", "imconfig.h", "imstb_rectpack.h",
             "imstb_textedit.h", "imstb_truetype.h", "LICENSE.txt"]:
    files[f"imgui/{name}"] = imgui / name
files["LICENSE"] = root / "LICENSE"
output.parent.mkdir(parents=True, exist_ok=True)
with output.open("w", encoding="ascii") as out:
    out.write('#include "TemplateBundle.hpp"\nnamespace Kuru::SdkGen {\n')
    for i, path in enumerate(files.values()):
        data = path.read_bytes()
        out.write(f"static const unsigned char data{i}[] = {{")
        out.write(",".join(map(str, data)) or "0")
        out.write("};\n")
    out.write("const BundleFile templateFiles[] = {\n")
    for i, (name, path) in enumerate(files.items()):
        out.write(f"{{{json.dumps(name)}, data{i}, {path.stat().st_size}}},\n")
    out.write("};\nconst size_t templateFileCount = sizeof(templateFiles) / sizeof(BundleFile);\n}\n")
