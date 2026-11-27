# syntax=docker/dockerfile:1

# ---------- 构建阶段 ----------
FROM ubuntu:26.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake git ca-certificates \
        libhiredis-dev libmysqlclient-dev \
    && rm -rf /var/lib/apt/lists/*

# muduo 网络库不在仓库内，构建时源码编译安装（GCC 15 需要补 cstdint 头）
RUN git clone --depth 1 --branch v2.9.0 https://github.com/chenshuo/muduo.git /tmp/muduo \
    && cmake -S /tmp/muduo -B /tmp/muduo/build \
        -DCMAKE_BUILD_TYPE=Release \
        -DMUDUO_BUILD_EXAMPLES=OFF \
        -DCMAKE_CXX_FLAGS="-include cstdint" \
    && cmake --build /tmp/muduo/build -j"$(nproc)" \
    && cmake --install /tmp/muduo/build \
    && rm -rf /tmp/muduo

WORKDIR /app
COPY CMakeLists.txt ./
COPY include ./include
COPY src ./src
COPY thirdparty ./thirdparty

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -j"$(nproc)"

# ---------- 运行阶段 ----------
FROM ubuntu:26.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        libhiredis-dev libmysqlclient-dev ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/bin/ChatServer ./ChatServer

EXPOSE 6000
ENTRYPOINT ["./ChatServer"]
CMD ["6000"]
