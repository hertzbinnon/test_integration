D:
cd gtk4
ls
cd openssl
set PATH=%PATH%;C:\MinGW\Strawberry\perl\bin;C:\MinGW\Strawberry\perl\site\bin;C:\MinGW\Strawberry\c\bin
perl Configure VC-WIN64A --prefix=C:/MinGW/OpenSSL
set PATH=C:\MinGW\Strawberry\perl\bin;C:\MinGW\Strawberry\perl\site\bin;C:\MinGW\Strawberry\c\bin;%PATH%
perl Configure VC-WIN64A --prefix=C:/MinGW/OpenSSL
set PATH=C:\MinGW\NASM;C:\MinGW\Strawberry\perl\bin;C:\MinGW\Strawberry\perl\site\bin;C:\MinGW\Strawberry\c\bin;%PATH%
perl Configure VC-WIN64A --prefix=C:/MinGW/OpenSSL
nmake
cd ../
git clone https://gitlab.collabora.com/gstreameronwindows/openssl-pkgconfig.git
cd openssl
nmake test
nmake install
cd ..
git clone https://github.com/abseil/abseil-cpp.git
cd abseil-cpp
 mkdir build && cd build
cmake -DBUILD_TESTING=ON -DABSL_USE_GOOGLETEST_HEAD=ON -DCMAKE_CXX_STANDARD=11 ..
cmake --build . --target all
ls
cmake --build . --target all
cmake --build . 
ctest
ctes--rerun-failed --output-on-failuret
ctest --rerun-failed --output-on-failuret
nmake install
cmake --install
cmake --install --prefix=C:\MinGW\absl
cmake --install absl  --prefix=C:\MinGW
cmake --install lib  --prefix=C:/MinGW/absl
cmake --install .  --prefix=C:/MinGW/absl
cmake --install --prefix=C:\MinGW\absl
cmake -DBUILD_TESTING=ON -DABSL_USE_GOOGLETEST_HEAD=ON -DCMAKE_CXX_STANDARD=11 -DCMAKE_INSTALL_PREFIX=C:\MinGW\absl ..
cmake --build . 
cmake --build . --target all
cmake --install .  --prefix=C:/MinGW/absl
cmake --build . --config Release
cmake --install .  
doskey /h > c:\absl_openssl_build.cmd
