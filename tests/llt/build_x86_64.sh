#!/bin/bash
# Copyright (c) 2026 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Build LLT test for x86_64 host execution
# Usage: bash build_x86_64.sh [clean]

set -e

script_dir="$(cd "$(dirname "$0")" && pwd)"
workspace="$(cd "$script_dir/../../../../.." && pwd)"
build_dir="$script_dir/build_x86_64"

if [ "$1" = "clean" ]; then
    [ -d "$build_dir" ] && find "$build_dir" -type f -delete && find "$build_dir" -type d -empty -delete
    echo "Cleaned build directory"; exit 0
fi

project="$workspace/foundation/ai/intelligent_voice_framework"
mock_dir="$project/tests/llt/mock"; stub_dir="$project/tests/llt/mock/x86_64"
jsoncpp_dir="$workspace/third_party/jsoncpp/jsoncpp-1.9.6"
securec_src="$workspace/third_party/bounds_checking_function"
gtest_src="$workspace/third_party/googletest/googletest"
cxx="${CXX:-g++}"
cxxflags="-std=c++17 -g -O0 -fPIC -fexceptions -frtti -include $stub_dir/force_includes.h"
cxxflags+=" -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-sign-compare"
cxxflags+=" -Wno-deprecated-declarations -Wno-overloaded-virtual -Wno-reorder"
cxxflags+=" -Wno-unused-but-set-variable -Wno-missing-field-initializers -Wno-format"
cxxflags+=" -Wno-tautological-constant-out-of-range-compare -Wno-invalid-offsetof"
cxxflags+=" -Wno-unused-private-field"
cxxflags+=" -DHILOG_ENABLE -DENABLE_DEBUG -DINTELL_VOICE_BUILD_VARIANT_ROOT -DENGINE_ENABLE"
cxxflags+=" -DHILOG_RAWFORMAT -D__GNU_SOURCE=1 -DCONFIG_STANDARD_SYSTEM -DBUILD_PUBLIC_VERSION"
cxxflags+=" -include securec.h"

# Load include paths and source file list (expand $variables)
includes=""
while IFS= read -r line; do
    line="$(echo "$line" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')"
    [ -z "$line" ] || [[ "$line" == \#* ]] && continue
    includes+=" $(eval echo "$line")"
done < "$script_dir/build_x86_64.includes"
sources=()
while IFS= read -r line; do
    line="$(echo "$line" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')"
    [ -z "$line" ] || [[ "$line" == \#* ]] && continue
    sources+=("$(eval echo "$line")")
done < "$script_dir/build_x86_64.sources"

mkdir -p "$build_dir"

# Extract jsoncpp if needed
[ ! -f "$jsoncpp_dir/src/lib_json/json_reader.cpp" ] && \
    echo "Extracting jsoncpp..." && cd "$workspace/third_party/jsoncpp" && tar xzf jsoncpp-1.9.6.tar.gz

# Build gtest for host if needed
gtest_libs="$build_dir/libgtest.a"; gtest_main="$build_dir/libgtest_main.a"
if [ ! -f "$gtest_libs" ]; then
    echo "Building gtest for host..."
    $cxx -std=c++17 -fPIC -isystem "$gtest_src/include" -I"$gtest_src" -I"$gtest_src/include" \
        -c "$gtest_src/src/gtest-all.cc" -o "$build_dir/gtest-all.o"
    $cxx -std=c++17 -fPIC -isystem "$gtest_src/include" -I"$gtest_src" -I"$gtest_src/include" \
        -c "$gtest_src/src/gtest_main.cc" -o "$build_dir/gtest_main.o"
    ar rcs "$gtest_libs" "$build_dir/gtest-all.o"
    ar rcs "$gtest_main" "$build_dir/gtest_main.o"
fi

# Build securec for host if needed
securec_lib="$build_dir/libsecurec.a"
if [ ! -f "$securec_lib" ]; then
    echo "Building securec for host..."
    securec_objs=()
    for src in "$securec_src"/src/*.c; do
        obj_name=$(basename "$src" .c).o
        gcc -c -O2 -fPIC -I"$securec_src/include" "$src" -o "$build_dir/$obj_name"
        securec_objs+=("$build_dir/$obj_name")
    done
    ar rcs "$securec_lib" "${securec_objs[@]}"
fi

# Compute object file names, remove stale .o files
objects=(); obj_names=()
for src in "${sources[@]}"; do
    obj_name=$(echo "$src" | sed "s|$workspace/||" | tr '/' '_' | sed 's/\.cpp$/.o/')
    objects+=("$build_dir/$obj_name"); obj_names+=("$obj_name")
done
for obj in "$build_dir"/*.o; do
    [ -f "$obj" ] || continue; found=0
    for name in "${obj_names[@]}"; do [ "$name" = "$(basename "$obj")" ] && found=1 && break; done
    [ $found -eq 0 ] && echo "  RM stale $(basename "$obj")" && rm -f "$obj"
done

# Recompile all if flags changed
flags_hash_file="$build_dir/.cxxflags_hash"
current_hash=$(echo "$cxxflags $includes" | md5sum | awk "{print \$1}")
[ -f "$flags_hash_file" ] && [ "$(cat "$flags_hash_file")" != "$current_hash" ] && \
    echo "Compiler flags changed, forcing full rebuild..." && for obj in "${objects[@]}"; do [ -f "$obj" ] && rm "$obj"; done
echo "$current_hash" > "$flags_hash_file"

# Compile sources
echo "=== Compiling x86_64 LLT ==="
for src in "${sources[@]}"; do
    obj_name=$(echo "$src" | sed "s|$workspace/||" | tr '/' '_' | sed 's/\.cpp$/.o/')
    obj="$build_dir/$obj_name"; need_compile=false
    if [ ! -f "$obj" ] || [ "$src" -nt "$obj" ]; then
        need_compile=true
    else
        dep_file="$build_dir/$(echo "$obj_name" | sed 's/\.o$/.d/')"
        if [ -f "$dep_file" ]; then
            for header in $(sed -e 's/\\$//' -e 's/^[^:]*://' "$dep_file" | tr ' ' '\n' | grep '\.h$' | sort -u); do
                [ -f "$header" ] && [ "$header" -nt "$obj" ] && need_compile=true && break
            done
        fi
    fi
    if [ "$need_compile" = true ]; then
        echo "  CXX $obj_name"
        $cxx $cxxflags $includes -MD -MP -c "$src" -o "$obj" 2>&1 | while IFS= read -r line; do echo "    $line"; done
        [ ${PIPESTATUS[0]} -ne 0 ] && echo "FAILED: $src" && exit 1
    fi
done

# Link
output="$build_dir/intell_voice_service_mock_llt"
echo "=== Linking x86_64 LLT ==="
$cxx -o "$output" "${objects[@]}" "$gtest_libs" "$gtest_main" "$securec_lib" -lpthread -ldl
echo "=== Build complete: $output ==="; echo "Run with: $output"
