# Copyright 2023-2024 DreamWorks Animation LLC
# SPDX-License-Identifier: Apache-2.0

function(McrtDenoise_cxx_compile_features target)
    target_compile_features(${target}
        PRIVATE
            cxx_std_17
    )
endfunction()

