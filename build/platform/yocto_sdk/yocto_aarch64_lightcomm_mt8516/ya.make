RESOURCES_LIBRARY()



DECLARE_EXTERNAL_RESOURCE(YOCTO_SDK_ROOT sbr:1216642973)
CFLAGS(
    GLOBAL -isystem GLOBAL $YOCTO_SDK_ROOT_RESOURCE_GLOBAL/usr/include/c++/5.3.0/aarch64-poky-linux
    GLOBAL -isystem GLOBAL $YOCTO_SDK_ROOT_RESOURCE_GLOBAL/usr/include/c++/5.3.0
)
LDFLAGS(
    -L$YOCTO_SDK_ROOT_RESOURCE_GLOBAL/usr/lib64
)
END()
