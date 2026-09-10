#include "io/parser.hpp"
#include "legalize/abacus.hpp"
#include "legalize/snap.hpp"
#include "metrics/hpwl.hpp"
#include "place/placer.hpp"
#include "place/quadratic.hpp"
#include "place/random.hpp"
#include "route/gcell.hpp"
#include "viz/qor.hpp"
#include "viz/svg.hpp"

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

static void usage(const char* argv0) {
    std::cerr << "usage: " << argv0
              << " <input.bench|.aux> -o <outdir> [--seed N]"
              << " [--placer quadratic|random] [--legalizer abacus|snap]\n";
}

int main(int argc, char** argv) {
    std::string input;
    std::string outdir;
    std::uint32_t seed = 1;
    std::string placer_name = "quadratic";
    std::string legalizer_name = "abacus";

    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "-o") {
            if (i + 1 >= argc) {
                usage(argv[0]);
                return 2;
            }
            outdir = argv[++i];
        } else if (a == "--seed") {
            if (i + 1 >= argc) {
                usage(argv[0]);
                return 2;
            }
            seed = static_cast<std::uint32_t>(std::stoul(argv[++i]));
        } else if (a == "--placer") {
            if (i + 1 >= argc) {
                usage(argv[0]);
                return 2;
            }
            placer_name = argv[++i];
        } else if (a == "--legalizer") {
            if (i + 1 >= argc) {
                usage(argv[0]);
                return 2;
            }
            legalizer_name = argv[++i];
        } else if (a == "-h" || a == "--help") {
            usage(argv[0]);
            return 0;
        } else if (!a.empty() && a[0] == '-') {
            std::cerr << "unknown flag: " << a << "\n";
            usage(argv[0]);
            return 2;
        } else if (!input.empty()) {
            usage(argv[0]);
            return 2;
        } else {
            input = a;
        }
    }

    if (input.empty() || outdir.empty()) {
        usage(argv[0]);
        return 2;
    }

    try {
        minipd::Design design = minipd::parse_file(input);

        std::unique_ptr<minipd::IPlacer> placer;
        if (placer_name == "random") {
            placer = std::make_unique<minipd::RandomPlacer>(seed);
        } else if (placer_name == "quadratic") {
            placer = std::make_unique<minipd::QuadraticPlacer>(seed);
        } else {
            std::cerr << "unknown placer: " << placer_name << "\n";
            usage(argv[0]);
            return 2;
        }
        placer->place(design);

        std::unique_ptr<minipd::ILegalizer> legalizer;
        if (legalizer_name == "abacus") {
            legalizer = std::make_unique<minipd::AbacusLegalizer>();
        } else if (legalizer_name == "snap") {
            legalizer = std::make_unique<minipd::SnapLegalizer>();
        } else {
            std::cerr << "unknown legalizer: " << legalizer_name << "\n";
            usage(argv[0]);
            return 2;
        }
        legalizer->legalize(design);

        minipd::GcellRouter router;
        router.route(design);

        const double wirelength = minipd::hpwl(design);
        const minipd::GcellGrid& grid = router.grid();

        fs::create_directories(outdir);
        const fs::path out(outdir);
        minipd::write_placed_svg(design, (out / "placed.svg").string());
        minipd::write_qor(design, input, placer->name(), legalizer->name(), wirelength,
                         router.name(), grid.nx, grid.ny, grid.capacity,
                         router.overflow(), router.max_overflow(), router.wirelength(),
                         (out / "qor.txt").string());
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
