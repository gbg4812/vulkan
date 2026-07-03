cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug -DLOCAL_SEQ=1
ln -s ./build-debug/compile_commands.json .
