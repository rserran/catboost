RESOURCES_LIBRARY()



IF (OS_ANDROID)
    DECLARE_EXTERNAL_RESOURCE(MAPSMOBI_MAVEN_REPO sbr:2109311446)
ELSE()
    MESSAGE(FATAL_ERROR Unsupported platform)
ENDIF()

END()
