#!/usr/bin/env python
import os
import sys

env = SConscript("godot-cpp/SConstruct")

env.Append(CPPPATH=["src/"])
sources = Glob("src/*.cpp")

if env["platform"] == "windows":
    library = env.SharedLibrary(
        "bin/libmathmatic{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )
elif env["platform"] == "macos":
    library = env.SharedLibrary(
        "bin/libmathmatic{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )
else:
    library = env.SharedLibrary(
        "bin/libmathmatic{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

Default(library)
