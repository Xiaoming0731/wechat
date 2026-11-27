# ---------- 构建阶段 ----------
FROM ubuntu:26.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# 国内网络环境下官方源不稳定，统一换成清华镜像
RUN sed -i 's@//archive.ubuntu.com@//mirrors.tuna.tsinghua.edu.cn@g; s@//security.ubuntu.com@//mirrors.tuna.tsinghua.edu.cn@g' \
        /etc/apt/sources.list /etc/apt/sources.list.d/*.sources 2>/dev/null || true

RUN apt-get -o Acquire::ForceIPv4=true -o Acquire::Retries=10 -o Acquire::http::Dl-Limit=500 -o Acquire::Queue-Mode=access update \
    && apt-get -o Acquire::ForceIPv4=true -o Acquire::Retries=10 -o Acquire::http::Dl-Limit=500 -o Acquire::Queue-Mode=access install -y --no-install-recommends \
        build-essential cmake git ca-certificates libboost-dev \
        libhiredis-dev libmysqlclient-dev \
    && rm -rf /var/lib/apt/lists/*

# muduo 网络库不在仓库内，构建时源码编译安装（GCC 15 需要补 cstdint 头）
# GitHub 访问不畅时可通过 --build-arg MUDUO_REPO=... 指向镜像源
ARG MUDUO_REPO=https://github.com/chenshuo/muduo.git
RUN git clone --depth 1 --branch v2.0.3 "$MUDUO_REPO" /tmp/muduo \
    && sed -i 's/-Werror//g' /tmp/muduo/CMakeLists.txt \
    && cmake -S /tmp/muduo -B /tmp/muduo/build \
        -DCMAKE_BUILD_TYPE=Release \
        -DMUDUO_BUILD_EXAMPLES=OFF \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
        -DCMAKE_CXX_FLAGS="-include cstdint -Wno-error -Wno-format-truncation" \
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

RUN sed -i 's@//archive.ubuntu.com@//mirrors.tuna.tsinghua.edu.cn@g; s@//security.ubuntu.com@//mirrors.tuna.tsinghua.edu.cn@g' \
        /etc/apt/sources.list /etc/apt/sources.list.d/*.sources 2>/dev/null || true

RUN apt-get -o Acquire::ForceIPv4=true -o Acquire::Retries=10 -o Acquire::http::Dl-Limit=500 -o Acquire::Queue-Mode=access update \
    && apt-get -o Acquire::ForceIPv4=true -o Acquire::Retries=10 -o Acquire::http::Dl-Limit=500 -o Acquire::Queue-Mode=access install -y --no-install-recommends \
        libhiredis-dev libmysqlclient-dev ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/bin/ChatServer ./ChatServer

EXPOSE 6000
ENTRYPOINT ["./ChatServer"]
CMD ["6000"]
