#!/usr/bin/env python3
"""Writes THIRD_PARTY_LICENSES.txt: the license texts of everything linked into
the plugin, read from a configured build tree (vcpkg's copyright files and the
FetchContent sources).

    python3 ci/collect-licenses.py build/linux-release > THIRD_PARTY_LICENSES.txt
"""
import html
import pathlib
import re
import sys

MIT_TEXT = """Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
"""


def main() -> None:
    build = pathlib.Path(sys.argv[1])
    share = next(build.glob("vcpkg_installed/*/share"))
    deps = build / "_deps"

    def read(path: pathlib.Path) -> str:
        data = path.read_bytes().replace(b"\r\n", b"\n")
        try:
            return data.decode("utf-8")
        except UnicodeDecodeError:
            return data.decode("cp1252")  # e.g. Lua's copyright file

    def vcpkg(port: str) -> str:
        return read(share / port / "copyright")

    def source(path: str) -> str:
        return read(deps / path)

    def luasql() -> str:
        page = source("luasql-src/doc/us/license.html")
        page = re.sub(r"(?s).*?(Copyright)", r"\1", page, count=1)
        page = re.sub(r"(?s)</div>.*", "", page)
        text = html.unescape(re.sub(r"<[^>]+>", "", page))
        return re.sub(r"\n{3,}", "\n\n", text).strip() + "\n"

    def apache_note(what: str, holder: str) -> str:
        return (f"{holder}\n\n{what} is licensed under the Apache License, Version 2.0;\n"
                "the full text is in the OpenSSL section above.\n")

    sections = [
        ("Lua 5.4", vcpkg("lua")),
        ("sol2", vcpkg("sol2")),
        ("spdlog", vcpkg("spdlog")),
        ("{fmt}", vcpkg("fmt")),
        ("SQLite", vcpkg("sqlite3")),
        ("OpenSSL", vcpkg("openssl")),
        ("libpq (PostgreSQL)", vcpkg("libpq")),
        ("MariaDB Connector/C", vcpkg("libmariadb")),
        ("libcurl", vcpkg("curl")),
        ("zlib", vcpkg("zlib")),
        ("LuaFileSystem", source("luafilesystem-src/LICENSE")),
        ("lua-cjson (OpenResty)", source("lua_cjson-src/LICENSE")),
        ("LuaSocket", source("luasocket-src/LICENSE")),
        ("LuaSQL", luasql()),
        ("Copas", source("copas-src/LICENSE")),
        ("binaryheap.lua", "binaryheap.lua by Thijs Schreijer, MIT/X11 license.\n\n" + MIT_TEXT),
        ("timerwheel.lua", source("timerwheel-src/LICENSE")),
        ("inspect.lua", source("inspect-src/MIT-LICENSE.txt")),
        ("digestpp", source("digestpp-src/LICENSE")),
        ("Mozilla CA certificates (Linux build only)",
         "The built-in CA bundle is Mozilla's root certificate list as extracted by the\n"
         "curl project (https://curl.se/docs/caextract.html). It is subject to the\n"
         "Mozilla Public License, version 2.0 (https://mozilla.org/MPL/2.0/). Its\n"
         "source form is available at https://curl.se/docs/caextract.html.\n"),
        ("VC:MP plugin SDK header",
         apache_note("The header", "Copyright 2011-2019 Ago Allikmaa (maxorator)")),
    ]

    out = sys.stdout
    out.write("License texts of the third-party software in the VCMP-Lua plugin.\n"
              "See THIRD_PARTY_NOTICES.md for versions and sources.\n")
    for name, text in sections:
        out.write("\n" + "=" * 79 + "\n" + name + "\n" + "=" * 79 + "\n\n")
        out.write(text.rstrip() + "\n")


if __name__ == "__main__":
    main()
