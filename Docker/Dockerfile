FROM ubuntu:18.04

ENV HTTP_PROXY=http://proxy.service:3128
ENV HTTPS_PROXY=http://proxy.service:3128
ENV http_proxy=http://proxy.service:3128
ENV https_proxy=http://proxy.service:3128

RUN apt-get update && apt-get install -y curl

CMD ["tail", "-f", "/var/log/faillog"]
