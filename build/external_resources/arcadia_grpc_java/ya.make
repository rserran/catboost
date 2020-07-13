RESOURCES_LIBRARY()



IF (HOST_OS_DARWIN AND HOST_ARCH_X86_64 OR
    HOST_OS_LINUX AND HOST_ARCH_X86_64 OR
    HOST_OS_WINDOWS AND HOST_ARCH_X86_64)
ELSE()
    MESSAGE(FATAL_ERROR Unsupported host platform for prebuilt arcadia grpc_java)
ENDIF()

DECLARE_EXTERNAL_HOST_RESOURCES_BUNDLE(
    ARCADIA_GRPC_JAVA
    sbr:1601135957 FOR DARWIN
    sbr:1601136162 FOR LINUX
    sbr:1601136036 FOR WIN32
)

END()
