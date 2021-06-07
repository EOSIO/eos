FROM python:3

WORKDIR /usr/src/app
COPY requirements.txt ./
COPY latency_collector.py ./
RUN pip install --no-cache-dir -r requirements.txt
ENTRYPOINT [ "python" ]
CMD [ "./latency_collector.py" ]