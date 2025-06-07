FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# Install gcc, python3, pip and other tools
RUN apt-get update && \
    apt-get install -y gcc-13 g++-13 make gdb git curl vim python3 python3-pip && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100 && \
    pip3 install --no-cache-dir pytest --break-system-packages && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

# Create a non-root user
RUN useradd -m vscode && echo "vscode ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

USER vscode
WORKDIR /workspace
