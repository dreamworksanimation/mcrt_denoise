# Copyright 2023-2024 DreamWorks Animation LLC
# SPDX-License-Identifier: Apache-2.0

function(McrtDenoise_cxx_compile_options target)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${target}
            PRIVATE
                $<$<BOOL:${ABI_SET_VERSION}>:
                    -fabi-version=${ABI_VERSION} # corrects the promotion behavior of C++11 scoped enums and the mangling of template argument packs.
                >
                -fexceptions                    # Enable exception handling.
                -fno-omit-frame-pointer         # TODO: add a note
                -fno-strict-aliasing            # TODO: add a note
                -march=core-avx2                # Specify the name of the target architecture
                -pipe                           # Use pipes rather than intermediate files.
                -pthread                        # Define additional macros required for using the POSIX threads library.
                -w                              # Inhibit all warning messages.
                -Wall                           # Enable most warning messages.
                -Wcast-align                    # Warn about pointer casts which increase alignment.
                -Wcast-qual                     # Warn about casts which discard qualifiers.
                -Wdisabled-optimization         # Warn when an optimization pass is disabled.
                -Wextra                         # This enables some extra warning flags that are not enabled by -Wall
                -Woverloaded-virtual            # Warn about overloaded virtual function names.
                -Wno-conversion                 # Disable certain warnings that are enabled by -Wall
                -Wno-switch                     # Disable certain warnings that are enabled by -Wall
                -Wno-system-headers             # Disable certain warnings that are enabled by -Wall
                -Wno-unknown-pragmas            # Disable certain warnings that are enabled by -Wall
                -Wno-unused-parameter           # Disable certain warnings that are enabled by -Wall

                $<$<CONFIG:RELWITHDEBINFO>:
                    -O3                         # the default is -O2 for RELWITHDEBINFO
                >
        )
    endif()
endfunction()

