# Generated by devtools/yamaker from nixpkgs 92c884dfd7140a6c3e6c717cf8990f7a78524331.

LIBRARY()



VERSION(1.1.8)

LICENSE(BSD-3-Clause)

ADDINCL(
    GLOBAL contrib/libs/snappy/include
)

NO_COMPILER_WARNINGS()

CFLAGS(
    -DHAVE_CONFIG_H
)

SRCS(
    snappy-c.cc
    snappy-sinksource.cc
    snappy-stubs-internal.cc
    snappy.cc
)

END()
