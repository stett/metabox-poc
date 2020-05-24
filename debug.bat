mkdir out
cd out
cmake -DCMAKE_GENERATOR_PLATFORM=x64 ..
cmake --build . --config Debug
.\Debug\metabox
cd ..
