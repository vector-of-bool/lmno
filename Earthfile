VERSION 0.6

bpt-linux:
    FROM alpine:3.16
    RUN apk add curl && \
        curl -sL https://github.com/vector-of-bool/bpt/releases/download/1.0.0-beta.1/bpt-linux-x64 \
            -o bpt && \
        chmod +x bpt
    SAVE ARTIFACT bpt

BUILD:
    COMMAND
    COPY --dir src/ tools/ bpt.yaml /s
    COPY +bpt-linux/bpt /usr/local/bin/bpt
    ARG compiler_id=gnu
    ARG compiler="g++"
    ARG cxx_flags
    WORKDIR /s
    RUN jq -n \
        --arg compiler_id "$compiler_id" \
        --arg compiler "$compiler" \
        --arg cxx_flags "$cxx_flags" \
        '{ \
            compiler_id: $compiler_id, \
            cxx_compiler: $compiler, \
            cxx_version: "c++20", \
            cxx_flags: $cxx_flags, \
            debug: true, \
            runtime: {debug: true}, \
            warning_flags: ["-Werror", "-Wsign-compare"], \
            compiler_launcher: "ccache", \
        }' \
        | tee toolchain.yaml
    RUN --mount=type=cache,target=/root/.ccache \
        bpt build --toolchain=toolchain.yaml --tweaks-dir=conf

build-alpine-gcc-11.2:
    FROM alpine:3.16
    RUN apk add jq gcc "g++" musl-dev ccache
    DO +BUILD


build-u22-gcc-12:
    FROM ubuntu:22.04
    RUN apt-get update && apt-get -y install gcc-12 g++-12 ccache jq
    DO +BUILD --compiler="g++-12"
