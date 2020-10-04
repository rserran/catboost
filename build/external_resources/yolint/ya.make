RESOURCES_LIBRARY()



IF (NOT HOST_OS_DARWIN AND NOT HOST_OS_LINUX AND NOT HOST_OS_WINDOWS)
    MESSAGE(FATAL_ERROR Unsupported host platform for yolint)
ENDIF()

DECLARE_EXTERNAL_HOST_RESOURCES_BUNDLE(
    YOLINT
    sbr:1727102211 FOR DARWIN
    sbr:1727102805 FOR LINUX
    sbr:1727102622 FOR WIN32
)

DECLARE_EXTERNAL_HOST_RESOURCES_BUNDLE(
    YOLINT_NEXT
    sbr:1727102211 FOR DARWIN
    sbr:1727102805 FOR LINUX
    sbr:1727102622 FOR WIN32
)

END()
