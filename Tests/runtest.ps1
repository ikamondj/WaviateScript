cmake -S . -B build-cmake-vs
cmake --build build-cmake-vs --config Release --target WaviateScriptTests
ctest --test-dir build-cmake-vs -C Release --output-on-failure