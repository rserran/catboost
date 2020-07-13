# Copy of contrib/libs/grpc/src/compiler/grpc_cpp_plugin
# Generated by devtools/yamaker.



IF (USE_PREBUILT_TOOLS AND VALID_HOST_PLATFORM_FOR_COMMON_PREBUILT_TOOLS)
    PREBUILT_PROGRAM()

    PEERDIR(build/external_resources/arcadia_grpc_cpp)

    PRIMARY_OUTPUT(${ARCADIA_GRPC_CPP_RESOURCE_GLOBAL}/grpc_cpp${MODULE_SUFFIX})

    END()
ELSE()
    PROGRAM()

    LICENSE(Apache-2.0)

    PEERDIR(
        contrib/libs/grpc/src/compiler/grpc_plugin_support
    )

    ADDINCL(
        contrib/libs/grpc
        contrib/libs/grpc/include
    )

    NO_COMPILER_WARNINGS()

    CFLAGS(
        -DGRPC_USE_ABSL=0
    )

    SRCDIR(contrib/libs/grpc/src/compiler)

    IF (OS_LINUX AND NOT MUSL)
        CFLAGS(
            -DGPR_BACKWARDS_COMPATIBILITY_MODE=1
        )
    ENDIF()

    SRCS(
        cpp_plugin.cc
    )

    END()
ENDIF()
