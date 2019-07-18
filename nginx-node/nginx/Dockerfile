# Version: 0.0.1
FROM ubuntu:14.04
MAINTAINER zhuzilin "zhuzilinallen@gmail.com"
RUN apt-get update && apt-get install -y nginx

COPY nginx.conf /etc/nginx/nginx.conf

CMD service nginx start

EXPOSE 80
EXPOSE 443
