FROM --platform=linux/amd64 debian:bookworm-slim@sha256:3783cc01769c7b2b1b83a5c5ad96c815348e28ed7da68e2e3687004faa906251
RUN apt-get update -qq && apt-get install -y --no-install-recommends build-essential cmake=3.25.1-1 ninja-build=1.11.1-2~deb12u1 pkg-config libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxfixes-dev libxss-dev libxtst-dev libwayland-dev libxkbcommon-dev libasound2-dev libpulse-dev libudev-dev libgl1-mesa-dev xvfb xauth python3 curl ca-certificates && rm -rf /var/lib/apt/lists/*
WORKDIR /work
ENV SDL_AUDIODRIVER=dummy XDG_RUNTIME_DIR=/tmp
