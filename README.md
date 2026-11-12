# EvoTSP — watch a genetic algorithm evolve the shortest route

An end-to-end C++ application that solves the **Traveling Salesman Problem (TSP)**
with a genetic algorithm and lets you watch it converge in your browser. You drop
cities on a map, hit **Evolve route**, and the C++ backend evolves a population of
candidate routes — using selection, order crossover, and inversion mutation —
streaming its progress back so the frontend can animate the route untangling
itself, generation by generation.

- **Backend (C++):** the whole solver plus a small HTTP server in one file
  (`server.cpp`). Uses two header-only libraries, [cpp-httplib] and
  [nlohmann/json], already vendored in `third_party/`.
- **Frontend (HTML/Canvas/JS):** a single self-contained page (`public/index.html`)
  with the interactive map, algorithm controls, live telemetry, and a convergence plot.

The research angle: TSP is NP-hard, so exact search is infeasible as cities grow.
A genetic algorithm is a **metaheuristic** — it doesn't guarantee the optimum, but
it finds very good routes fast, and this app makes the search *visible* so you can
see how population size and mutation rate change convergence.

## Project structure

```
evotsp/
├── server.cpp            # C++ backend: GA solver + HTTP server
├── CMakeLists.txt        # optional CMake build
├── public/
│   └── index.html        # the interactive frontend
├── third_party/
│   ├── httplib.h         # cpp-httplib (MIT)
│   └── json.hpp          # nlohmann/json (MIT)
├── .gitignore
└── README.md
```

## Prerequisites

A C++17 compiler. Check with `g++ --version` (need 7+) or `clang++ --version`.
- **Linux:** `sudo apt install g++`
- **macOS:** `xcode-select --install`
- **Windows:** install [MSYS2] and `g++`, or use Visual Studio.

## Run it — step by step

**1. Open a terminal in the project root** (the folder with `server.cpp`).

**2. Compile** (one line, no build system needed):

```bash
# Linux / macOS
g++ -std=c++17 -O2 -pthread server.cpp -o evotsp
```

```bash
# Windows (MSYS2 / MinGW) — note the -lws2_32 for Winsock
g++ -std=c++17 -O2 server.cpp -o evotsp.exe -lws2_32
```

<details>
<summary>Prefer CMake?</summary>

```bash
cmake -S . -B build
cmake --build build
# binary lands in ./build/evotsp
```
</details>

**3. Start the server** (run it from the project root so it can find `public/`):

```bash
./evotsp          # Windows: .\evotsp.exe
```

You'll see: `EvoTSP running at http://localhost:8080`

**4. Open** `http://localhost:8080` in your browser.

**5. Use it:**
- Click anywhere on the map to drop a city, or press **Scatter 12** for a random batch.
- Adjust **Population**, **Generations**, and **Mutation rate** if you like.
- Press **Evolve route** and watch the amber route tighten as the telemetry and
  convergence plot update.

**6. Stop the server** with `Ctrl+C`.

## How it works (the algorithm)

Each candidate route is a permutation of the cities. The GA repeats, per generation:

1. **Evaluate** — score every route by its total round-trip distance.
2. **Elitism** — copy the best few routes into the next generation untouched.
3. **Selection** — run tournaments: pick K routes at random, keep the shortest.
4. **Order crossover (OX)** — splice a slice of one parent with the remaining
   cities from the other, in order, keeping each city exactly once.
5. **Inversion mutation** — occasionally reverse a sub-path to untangle crossings.

Over many generations the population drifts toward shorter and shorter tours — the
curve you see flattening in the convergence plot.

## Upload to GitHub — step by step

**1. Create an empty repo** at <https://github.com/new>. Name it e.g. `evotsp`.
Don't add a README or .gitignore there — this project already has them.

**2. In your terminal, from the project root:**

```bash
git init
git add .
git commit -m "EvoTSP: genetic-algorithm TSP solver with a C++ backend and web UI"
git branch -M main
git remote add origin https://github.com/<your-username>/evotsp.git
git push -u origin main
```

Replace `<your-username>` with your GitHub username. (If git asks for a password,
use a [Personal Access Token], not your account password.)

<details>
<summary>Have the GitHub CLI installed? Even shorter:</summary>

```bash
git init && git add . && git commit -m "Initial commit"
gh repo create evotsp --public --source=. --push
```
</details>

The compiled binary and build folder are ignored by `.gitignore`, so only the
source gets pushed — anyone who clones it can rebuild with the one-line compile above.

[cpp-httplib]: https://github.com/yhirose/cpp-httplib
[nlohmann/json]: https://github.com/nlohmann/json
[MSYS2]: https://www.msys2.org/
[Personal Access Token]: https://github.com/settings/tokens
