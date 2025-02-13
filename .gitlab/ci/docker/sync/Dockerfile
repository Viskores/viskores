from python:3.6.15

# run apt install without asking user
run debian_frontend=noninteractive apt update && \
    apt -y install --no-install-recommends ssh git && \
    pip install python-dateutil pygithub
