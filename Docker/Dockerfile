FROM ubuntu:18.04

RUN apt-get update && apt-get install -y curl

CMD ["tail", "-f", "/var/log/faillog"]
