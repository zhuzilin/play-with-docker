# Version: 0.0.1
FROM ubuntu:14.04
MAINTAINER zhuzilin "zhuzilinallen@gmail.com"
RUN apt-get update && apt-get install -y nginx

COPY data/ /data
COPY beginner.conf /etc/nginx/nginx.conf

ENTRYPOINT ["/bin/bash", "service", "nginx", "start"]

EXPOSE 80
