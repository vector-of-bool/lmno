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
            compiler_launcher: "python3 -u tools/unmangle.py ccache", \
        }' \
        | tee toolchain.yaml
    RUN --mount=type=cache,target=/root/.ccache \
        bpt build --toolchain=toolchain.yaml --tweaks-dir=conf

build-alpine-gcc-11.2:
    FROM alpine:3.16
    RUN apk add jq gcc "g++" musl-dev ccache python3
    DO +BUILD

build-alpine-gcc-12.2:
    FROM alpine:3.17
    RUN apk add jq gcc "g++" musl-dev ccache python3
    DO +BUILD
