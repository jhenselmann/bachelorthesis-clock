FROM rasa/rasa-sdk:latest

USER root

RUN pip install --no-cache-dir httpx==0.24.1

USER 1001