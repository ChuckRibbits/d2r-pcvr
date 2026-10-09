include("${CMAKE_CURRENT_LIST_DIR}/proton-toolchain.cmake")
# Older MinGW headers omit the desktop-compatible SYSTEM family partition.
string(APPEND CMAKE_CXX_FLAGS_INIT " -DWINAPI_PARTITION_SYSTEM=4")
set(CMAKE_RC_FLAGS_INIT "-I${MINGW_ROOT}/share/mingw-w64/include -D__MINGW32__ -D_WIN32")
string(APPEND CMAKE_SHARED_LINKER_FLAGS_INIT " ${MINGW_ROOT}/x86_64-w64-mingw32/lib/libwinpthread.a")
