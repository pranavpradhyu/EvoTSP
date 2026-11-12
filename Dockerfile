# ---------- Stage 1: build the C++ binary ----------
FROM ubuntu:24.04 AS build
RUN apt-get update \
    && apt-get install -y --no-install-recommends g++ \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY server.cpp .
COPY third_party ./third_party
# Static libstdc++/libgcc so the runtime image needs no extra packages.
RUN g++ -std=c++17 -O2 -pthread -static-libstdc++ -static-libgcc server.cpp -o evotsp

# ---------- Stage 2: tiny runtime image ----------
FROM ubuntu:24.04
WORKDIR /app
COPY --from=build /src/evotsp .
COPY public ./public
# Render injects its own PORT at runtime; 8080 is only the fallback.
ENV PORT=8080
EXPOSE 8080
USER ubuntu
CMD ["./evotsp"]
