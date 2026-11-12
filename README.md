# EvoTSP

**Live demo: https://evotsp.onrender.com**

An end-to-end C++ application that solves the **Traveling Salesman Problem** with a
genetic algorithm and makes the search visible. Click to place cities on a map, press
**Evolve route**, and watch the route tighten generation by generation while a live
convergence plot tracks the best distance.

> The demo runs on a free Render instance. If nobody has visited for about 15 minutes
> it goes to sleep, so the first load can take 30 to 60 seconds. After that it is fast.

## Why it is interesting

The Traveling Salesman Problem is NP-hard: the number of possible routes grows
factorially, so exact search stops being practical after a few dozen cities. A genetic
algorithm is a *metaheuristic*. It cannot promise the optimum, but it finds very good
routes quickly. EvoTSP lets you see how population size, generation count, and mutation
rate change how fast and how well it converges.

## Features

- Interactive map: click to add cities, or scatter a random batch
- Adjustable population size, generations, and mutation rate
- Animated route that improves as the algorithm evolves
- Live telemetry: best distance, generation counter, convergence plot
- The solver and web server are one C++17 file; the frontend is one HTML file
- No package manager needed: the two libraries are header-only and included

## How it works

Each candidate route is a permutation of the cities. Every generation the algorithm:

1. **Evaluates** every route by its total round-trip distance.
2. **Keeps the elite**: the best few routes pass into the next generation unchanged.
3. **Selects parents** with tournaments: pick a few routes at random, keep the shortest.
4. **Crosses over** with order crossover (OX): take a slice of one parent and fill the
   remaining cities in the other parent's order, so every city appears exactly once.
5. **Mutates** occasionally by reversing a sub-path, which untangles crossing edges.

The browser sends the city coordinates to `POST /solve`. The C++ backend runs the
algorithm and returns snapshots of the best route over time, which the frontend animates.

To keep the public demo responsive, the server accepts up to 150 cities and trims the
number of generations on very large runs (the page tells you when it does).

## Project structure

```
evotsp/
├── server.cpp        C++ backend: genetic algorithm + HTTP server
├── public/
│   └── index.html    Frontend: map, controls, telemetry, plot
├── third_party/
│   ├── httplib.h     cpp-httplib (MIT)
│   └── json.hpp      nlohmann/json (MIT)
├── Dockerfile        Container build, used by Render
├── render.yaml       Optional Render Blueprint
├── CMakeLists.txt    Optional CMake build
└── README.md
```

## Run it locally (optional)

You only need this if you want to modify the code. To just use the app, open the live demo.

You need a C++17 compiler (`g++` 7 or newer).

**Linux / macOS**

```bash
g++ -std=c++17 -O2 -pthread server.cpp -o evotsp
./evotsp
```

**Windows** (MSYS2 UCRT64 toolchain, with `C:\msys64\ucrt64\bin` on your PATH)

```powershell
g++ -std=c++17 -O2 server.cpp -o evotsp.exe -lws2_32
.\evotsp.exe
```

Then open http://localhost:8080. Press `Ctrl+C` to stop. Run it from the project root so
the server can find the `public/` folder. Set the `PORT` environment variable to use a
different port.

## Deploy your own copy on Render

1. Push this repository to your GitHub account.
2. In the [Render dashboard](https://dashboard.render.com), choose **New > Web Service**
   and connect the repository.
3. Set **Language** to **Docker** and **Instance type** to **Free**. Leave the build and
   start commands empty, since the `Dockerfile` handles both.
4. Click **Create Web Service**. The first build takes a few minutes.
5. When the status shows **Live**, open the `onrender.com` URL at the top of the page.

Every `git push` to the connected branch redeploys automatically.

## Tech

C++17 · [cpp-httplib](https://github.com/yhirose/cpp-httplib) ·
[nlohmann/json](https://github.com/nlohmann/json) · HTML Canvas · Docker

## License

MIT. See the licenses of the bundled libraries in `third_party/`.
