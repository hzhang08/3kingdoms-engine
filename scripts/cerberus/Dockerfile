FROM eclipse-temurin:21-jdk-noble@sha256:b468c3fc688b14450571494f588bd939378e7fd542ed5a73f8efc13f17872a87
RUN apt-get update && apt-get install -y --no-install-recommends python3 python3-venv unzip && rm -rf /var/lib/apt/lists/*
COPY ghidra_12.0.4_PUBLIC_20260303.zip /tmp/ghidra.zip
RUN echo 'c3b458661d69e26e203d739c0c82d143cc8a4a29d9e571f099c2cf4bda62a120  /tmp/ghidra.zip' | sha256sum -c - && unzip -q /tmp/ghidra.zip -d /opt && mv /opt/ghidra_12.0.4_PUBLIC /opt/ghidra && rm /tmp/ghidra.zip
COPY cerberus /opt/cerberus
COPY requirements.lock.txt /opt/requirements.lock.txt
RUN python3 -m venv /opt/venv && /opt/venv/bin/pip install -r /opt/requirements.lock.txt && /opt/venv/bin/pip install --no-build-isolation --no-deps /opt/cerberus && /opt/venv/bin/pip freeze --all > /opt/dependencies.lock.txt
ENV PATH="/opt/venv/bin:${PATH}" GHIDRA_INSTALL_DIR=/opt/ghidra GHIDRA_JDK=/opt/java/openjdk GHIDRA_RE_ROOT=/opt/cerberus GHIDRA_WORKSPACE=/workspace GHIDRA_RE_CONFIG_HOME=/workspace/config GHIDRA_MAX_CPU=4 GHIDRA_ANALYSIS_TIMEOUT_PER_FILE=900
ENV GHIDRA_NOTES_ENABLE_SHARED=0 GHIDRA_NOTES_AUTO_SYNC=0 GHIDRA_NOTES_REPO="" CERBERUS_MCP_ALLOW_UNSAFE_RUN=0 CERBERUS_MCP_BRIDGE_WRITE_OK=0 CERBERUS_MCP_RUNTIME_NAMES="" CERBERUS_MCP_RUNTIME_PIDS=""
RUN useradd -m -u 1001 analyst
USER analyst
WORKDIR /opt/cerberus
ENTRYPOINT ["cerberus-re"]
