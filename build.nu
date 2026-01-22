#!/usr/bin/env nu

def main [--test (-t)] {
  cmake -B build -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
  cmake --build build

  if $test {
    ./build/interp_tests
  } else {
    ./build/interpreter evaluate test.lox
  }
}
