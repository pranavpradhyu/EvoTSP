// EvoTSP — a genetic-algorithm Traveling Salesman solver with a web UI.
//
// This single file is the whole backend. It does two jobs:
//   1. Serves the interactive frontend (the ./public folder).
//   2. Exposes POST /solve, which runs a genetic algorithm over the cities
//      the user placed and streams back the convergence history so the
//      browser can animate the route improving generation by generation.
//
// The research angle: the Traveling Salesman Problem is NP-hard, so instead of
// searching for the exact optimum we evolve a population of candidate routes
// using selection, order crossover (OX), and inversion mutation — a classic
// metaheuristic you can watch converge in real time.

#include "third_party/httplib.h"
#include "third_party/json.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <vector>

using json = nlohmann::json;

// ---------- Problem representation ----------

struct City {
    double x = 0.0;
    double y = 0.0;
};

// A tour is a permutation of city indices. Distance is the length of the
// closed loop that visits every city once and returns to the start.
static double tour_distance(const std::vector<int>& tour,
                            const std::vector<City>& cities) {
    double total = 0.0;
    const std::size_t n = tour.size();
    for (std::size_t i = 0; i < n; ++i) {
        const City& a = cities[tour[i]];
        const City& b = cities[tour[(i + 1) % n]];  // wrap back to start
        total += std::hypot(a.x - b.x, a.y - b.y);
    }
    return total;
}

// ---------- Genetic algorithm ----------

struct GAParams {
    int population = 200;
    int generations = 400;
    double mutation_rate = 0.15;
    int tournament_size = 5;
    int elitism = 2;       // best N individuals copied unchanged each generation
    int snapshots = 60;    // how many intermediate routes to send back for animation
};

struct GAResult {
    std::vector<int> best_tour;
    double best_distance = 0.0;
    std::vector<double> history;                              // best distance per generation
    std::vector<std::pair<int, std::vector<int>>> snapshots;  // (generation, best tour so far)
};

class GeneticTSP {
public:
    GeneticTSP(std::vector<City> cities, GAParams params)
        : cities_(std::move(cities)), params_(params), rng_(std::random_device{}()) {}

    GAResult run() {
        const int n = static_cast<int>(cities_.size());
        GAResult result;

        // Seed the population with random permutations.
        std::vector<std::vector<int>> population;
        population.reserve(params_.population);
        std::vector<int> base(n);
        std::iota(base.begin(), base.end(), 0);
        for (int i = 0; i < params_.population; ++i) {
            std::shuffle(base.begin(), base.end(), rng_);
            population.push_back(base);
        }

        std::vector<int> best = population.front();
        double best_dist = tour_distance(best, cities_);

        const int snap_every =
            std::max(1, params_.generations / std::max(1, params_.snapshots));

        for (int gen = 0; gen < params_.generations; ++gen) {
            // Evaluate and sort the population best-first.
            std::vector<std::pair<double, int>> scored(population.size());
            for (std::size_t i = 0; i < population.size(); ++i)
                scored[i] = {tour_distance(population[i], cities_), static_cast<int>(i)};
            std::sort(scored.begin(), scored.end());

            if (scored.front().first < best_dist) {
                best_dist = scored.front().first;
                best = population[scored.front().second];
            }
            result.history.push_back(best_dist);
            if (gen % snap_every == 0) result.snapshots.push_back({gen, best});

            // Build the next generation.
            std::vector<std::vector<int>> next;
            next.reserve(population.size());

            // Elitism: carry the very best routes forward untouched.
            for (int e = 0; e < params_.elitism && e < static_cast<int>(scored.size()); ++e)
                next.push_back(population[scored[e].second]);

            while (static_cast<int>(next.size()) < params_.population) {
                const std::vector<int>& p1 = tournament(population);
                const std::vector<int>& p2 = tournament(population);
                std::vector<int> child = order_crossover(p1, p2);
                if (uniform_() < params_.mutation_rate) inversion_mutation(child);
                next.push_back(std::move(child));
            }
            population = std::move(next);
        }

        // Always record the final state.
        result.snapshots.push_back({params_.generations, best});
        result.best_tour = best;
        result.best_distance = best_dist;
        return result;
    }

private:
    // Tournament selection: pick K random routes, return the shortest.
    const std::vector<int>& tournament(const std::vector<std::vector<int>>& pop) {
        std::uniform_int_distribution<int> pick(0, static_cast<int>(pop.size()) - 1);
        int best_i = pick(rng_);
        double best_d = tour_distance(pop[best_i], cities_);
        for (int t = 1; t < params_.tournament_size; ++t) {
            int cand = pick(rng_);
            double d = tour_distance(pop[cand], cities_);
            if (d < best_d) { best_d = d; best_i = cand; }
        }
        return pop[best_i];
    }

    // Order crossover (OX): copy a slice of parent A, fill the rest from
    // parent B in order, keeping every city exactly once.
    std::vector<int> order_crossover(const std::vector<int>& a, const std::vector<int>& b) {
        const int n = static_cast<int>(a.size());
        std::uniform_int_distribution<int> pos(0, n - 1);
        int lo = pos(rng_), hi = pos(rng_);
        if (lo > hi) std::swap(lo, hi);

        std::vector<int> child(n, -1);
        std::vector<char> used(n, 0);
        for (int i = lo; i <= hi; ++i) { child[i] = a[i]; used[a[i]] = 1; }

        int write = (hi + 1) % n;
        for (int k = 0; k < n; ++k) {
            int gene = b[(hi + 1 + k) % n];
            if (!used[gene]) {
                child[write] = gene;
                used[gene] = 1;
                write = (write + 1) % n;
            }
        }
        return child;
    }

    // Inversion mutation: reverse a random sub-path. Good for TSP because it
    // untangles crossings without scrambling the whole route.
    void inversion_mutation(std::vector<int>& tour) {
        const int n = static_cast<int>(tour.size());
        std::uniform_int_distribution<int> pos(0, n - 1);
        int lo = pos(rng_), hi = pos(rng_);
        if (lo > hi) std::swap(lo, hi);
        std::reverse(tour.begin() + lo, tour.begin() + hi + 1);
    }

    double uniform_() {
        std::uniform_real_distribution<double> d(0.0, 1.0);
        return d(rng_);
    }

    std::vector<City> cities_;
    GAParams params_;
    std::mt19937 rng_;
};

// ---------- HTTP layer ----------

int main() {
    httplib::Server server;

    // Serve the frontend (index.html, etc.) from ./public.
    if (!server.set_mount_point("/", "./public")) {
        std::fprintf(stderr, "Could not find ./public folder — run from the project root.\n");
        return 1;
    }

    server.Post("/solve", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        try {
            json body = json::parse(req.body);

            std::vector<City> cities;
            for (const auto& c : body.at("cities"))
                cities.push_back({c.at("x").get<double>(), c.at("y").get<double>()});

            if (cities.size() < 3) {
                res.status = 400;
                res.set_content(R"({"error":"Place at least 3 cities."})", "application/json");
                return;
            }

            GAParams p;
            if (body.contains("population"))   p.population    = body["population"].get<int>();
            if (body.contains("generations"))  p.generations   = body["generations"].get<int>();
            if (body.contains("mutationRate")) p.mutation_rate = body["mutationRate"].get<double>();

            // Clamp to sane bounds so the browser can't ask for a 10-minute run.
            p.population   = std::clamp(p.population, 20, 1000);
            p.generations  = std::clamp(p.generations, 10, 3000);
            p.mutation_rate = std::clamp(p.mutation_rate, 0.0, 1.0);

            GeneticTSP ga(cities, p);
            GAResult r = ga.run();

            json out;
            out["bestDistance"] = r.best_distance;
            out["bestTour"] = r.best_tour;
            out["history"] = r.history;
            out["snapshots"] = json::array();
            for (const auto& [gen, tour] : r.snapshots) {
                out["snapshots"].push_back({{"generation", gen},
                                            {"distance", tour_distance(tour, cities)},
                                            {"tour", tour}});
            }
            res.set_content(out.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            json err; err["error"] = e.what();
            res.set_content(err.dump(), "application/json");
        }
    });

    const char* host = "0.0.0.0";
    const int port = 8080;
    std::printf("EvoTSP running at http://localhost:%d  (Ctrl+C to stop)\n", port);
    server.listen(host, port);
    return 0;
}
