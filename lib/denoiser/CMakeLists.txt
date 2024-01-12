# Copyright 2023-2024 DreamWorks Animation LLC
# SPDX-License-Identifier: Apache-2.0

set(component denoiser)

set(installIncludeDir ${PACKAGE_NAME}/denoiser)
set(exportGroup ${PROJECT_NAME}Targets)

add_library(${component} SHARED "")
add_library(${PROJECT_NAME}::${component} ALIAS ${component})

target_sources(${component}
    PRIVATE
        Denoiser.cc
        OIDNDenoiserImpl.cc
)

if (MOONRAY_USE_CUDA)
    target_sources(${component}
        PRIVATE
            OptixDenoiserImpl.cc
    )
endif()

set_property(TARGET ${component}
    PROPERTY PUBLIC_HEADER
        Denoiser.h
)

target_include_directories(${component}
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(${component}
    PRIVATE
        SceneRdl2::render_logging
    PUBLIC
        OpenImageDenoise
)

if (MOONRAY_USE_CUDA)
    target_link_libraries(${component}
        PUBLIC
            CUDA::cudart
            OptiX::OptiX
    )
endif()

# If at Dreamworks add a SConscript stub file so others can use this library.
SConscript_Stub(${component})

# Set standard compile/link options
McrtDenoise_cxx_compile_definitions(${component})
McrtDenoise_cxx_compile_features(${component})
McrtDenoise_cxx_compile_options(${component})
McrtDenoise_link_options(${component})

set(MOONRAY_DENOISER_TARGET_ARCHITECTURE "SSE" CACHE STRING "Target architecture for the denoiser library")
set_property(CACHE MOONRAY_DENOISER_TARGET_ARCHITECTURE PROPERTY STRINGS SSE AVX AVX2)
if(${MOONRAY_DENOISER_TARGET_ARCHITECTURE} MATCHES "^AVX2$")
    target_compile_definitions(${component}
        PUBLIC __TARGET_AVX2__ __AVX2__)
elseif(${MOONRAY_DENOISER_TARGET_ARCHITECTURE} MATCHES "^AVX$")
    target_compile_definitions(${component}
        PUBLIC __TARGET_AVX__ __AVX__)
endif()

if (MOONRAY_USE_CUDA)
    target_compile_definitions(${component}
        PRIVATE MOONRAY_USE_CUDA)
endif()

# -------------------------------------
# Install the target and the export set
# -------------------------------------
include(GNUInstallDirs)

# install the target
install(TARGETS ${component}
    COMPONENT ${component}
    EXPORT ${exportGroup}
    LIBRARY
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        NAMELINK_SKIP
    RUNTIME
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${installIncludeDir}
)

# # install the export set
# install(
#     EXPORT ${exportGroup}
#     NAMESPACE ${PROJECT_NAME}::
#     DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}-${PROJECT_VERSION}
# )
