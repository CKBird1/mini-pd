#include "io/parser.hpp"
#include "io/bookshelf.hpp"

#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace minipd {
namespace {

std::string trim(const std::string& s) {
    std::size_t b = 0;
    while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) {
        ++b;
    }
    std::size_t e = s.size();
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) {
        --e;
    }
    return s.substr(b, e - b);
}

bool valid_name(const std::string& n) {
    if (n.empty()) {
        return false;
    }
    const unsigned char c0 = static_cast<unsigned char>(n[0]);
    if (!(std::isalpha(c0) || n[0] == '_')) {
        return false;
    }
    for (char ch : n) {
        const unsigned char c = static_cast<unsigned char>(ch);
        if (!(std::isalnum(c) || ch == '_')) {
            return false;
        }
    }
    return true;
}

bool has_ext(const std::string& path, const char* ext) {
    const std::size_t n = path.size();
    const std::size_t m = std::char_traits<char>::length(ext);
    if (n < m) {
        return false;
    }
    for (std::size_t i = 0; i < m; ++i) {
        const unsigned char a = static_cast<unsigned char>(path[n - m + i]);
        const unsigned char b = static_cast<unsigned char>(ext[i]);
        if (std::tolower(a) != std::tolower(b)) {
            return false;
        }
    }
    return true;
}

}  // namespace

Design parse_file(const std::string& path) {
    if (has_ext(path, ".aux")) {
        return parse_bookshelf(path);
    }

    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("cannot open " + path);
    }

    Design d;
    bool have_die = false;
    bool have_rows = false;
    std::unordered_map<std::string, std::size_t> cell_index;
    std::unordered_set<std::string> net_names;
    int lineno = 0;
    std::string raw;

    auto err = [&](const std::string& msg) {
        throw std::runtime_error(path + ":" + std::to_string(lineno) + ": " + msg);
    };

    while (std::getline(in, raw)) {
        ++lineno;
        const auto hash = raw.find('#');
        if (hash != std::string::npos) {
            raw = raw.substr(0, hash);
        }
        const std::string line = trim(raw);
        if (line.empty()) {
            continue;
        }

        std::istringstream ss(line);
        std::string kw;
        ss >> kw;

        if (kw == "DIE") {
            if (have_die) {
                err("duplicate DIE");
            }
            double x0 = 0, y0 = 0, x1 = 0, y1 = 0;
            if (!(ss >> x0 >> y0 >> x1 >> y1)) {
                err("DIE expects x0 y0 x1 y1");
            }
            std::string extra;
            if (ss >> extra) {
                err("unexpected token after DIE");
            }
            if (x1 <= x0 || y1 <= y0) {
                err("DIE must have x1>x0 and y1>y0");
            }
            d.die.bbox = {x0, y0, x1, y1};
            have_die = true;
        } else if (kw == "ROWS") {
            if (!have_die) {
                err("ROWS before DIE");
            }
            if (have_rows) {
                err("duplicate ROWS");
            }
            int count = 0;
            double height = 0;
            if (!(ss >> count >> height)) {
                err("ROWS expects <count> <height>");
            }
            std::string extra;
            if (ss >> extra) {
                err("unexpected token after ROWS");
            }
            if (count <= 0) {
                err("row count must be positive");
            }
            if (height <= 0) {
                err("row height must be positive");
            }
            d.rows.resize(static_cast<std::size_t>(count));
            for (int i = 0; i < count; ++i) {
                Row r;
                r.y = d.die.bbox.y0 + static_cast<double>(i) * height;
                r.height = height;
                r.x0 = d.die.bbox.x0;
                r.x1 = d.die.bbox.x1;
                d.rows[static_cast<std::size_t>(i)] = r;
            }
            have_rows = true;
        } else if (kw == "CELL") {
            if (!have_die) {
                err("CELL before DIE");
            }
            std::string name;
            double w = 0, h = 0;
            if (!(ss >> name >> w >> h)) {
                err("CELL expects name width height");
            }
            std::string extra;
            if (ss >> extra) {
                err("unexpected token after CELL");
            }
            if (!valid_name(name)) {
                err("invalid cell name");
            }
            if (w <= 0 || h <= 0) {
                err("cell size must be positive");
            }
            if (cell_index.count(name)) {
                err("duplicate cell " + name);
            }
            Cell c;
            c.name = name;
            c.width = w;
            c.height = h;
            cell_index[name] = d.cells.size();
            d.cells.push_back(std::move(c));
        } else if (kw == "NET") {
            std::string name;
            if (!(ss >> name)) {
                err("NET expects name");
            }
            if (!valid_name(name)) {
                err("invalid net name");
            }
            if (net_names.count(name)) {
                err("duplicate net " + name);
            }
            Net n;
            n.name = name;
            std::string pin;
            while (ss >> pin) {
                const auto it = cell_index.find(pin);
                if (it == cell_index.end()) {
                    err("NET " + name + " unknown cell " + pin);
                }
                n.pins.push_back(it->second);
            }
            if (n.pins.empty()) {
                err("NET " + name + " has no pins");
            }
            net_names.insert(name);
            d.nets.push_back(std::move(n));
        } else {
            err("unknown keyword " + kw);
        }
    }

    if (!have_die) {
        throw std::runtime_error(path + ": missing DIE");
    }
    if (!have_rows) {
        throw std::runtime_error(path + ": missing ROWS");
    }
    return d;
}

}  // namespace minipd
