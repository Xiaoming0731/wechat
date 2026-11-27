FROM ubuntu:26.04

ENV DEBIAN_FRONTEND=noninteractive

# nginx + stream 动态模块（与本地 WSL 环境保持一致）
RUN apt-get update && apt-get install -y --no-install-recommends \
        nginx libnginx-mod-stream \
    && rm -rf /var/lib/apt/lists/* \
    && rm -f /etc/nginx/sites-enabled/default

COPY deploy/nginx.docker.conf /etc/nginx/nginx.conf

EXPOSE 8000
CMD ["nginx", "-g", "daemon off;"]
