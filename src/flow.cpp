#include "io/parser.hpp"
#include "legalize/snap.hpp"
#include "metrics/hpwl.hpp"
#include "place/random.hpp"
#include "viz/qor.hpp"
#include "viz/svg.hpp"

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

static void usage(const char* argv0) {
    std::cerr << "usage: " << argv0 << " <input.bench> -o <outdir> [--seed N]\n";
}

int main(int argc, char** argv) {
    std::string input;
    std::string outdir;
    std::uint32_t seed = 1;

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

        minipd::RandomPlacer placer(seed);
        placer.place(design);

        minipd::SnapLegalizer legalizer;
        legalizer.legalize(design);

        const double wirelength = minipd::hpwl(design);

        fs::create_directories(outdir);
        const fs::path out(outdir);
        minipd::write_placed_svg(design, (out / "placed.svg").string());
        minipd::write_qor(design, input, placer.name(), legalizer.name(), wirelength,
                         (out / "qor.txt").string());
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
