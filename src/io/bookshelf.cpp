#include "io/bookshelf.hpp"

#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>

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

bool to_int(const std::string& s, int& out) {
    std::istringstream ss(s);
    int v = 0;
    if (!(ss >> v)) {
        return false;
    }
    char extra;
    if (ss >> extra) {
        return false;
    }
    out = v;
    return true;
}

bool to_double(const std::string& s, double& out) {
    std::istringstream ss(s);
    double v = 0;
    if (!(ss >> v)) {
        return false;
    }
    char extra;
    if (ss >> extra) {
        return false;
    }
    out = v;
    return true;
}

std::vector<std::string> tokenize(const std::string& s) {
    std::string spaced;
    spaced.reserve(s.size() + 8);
    for (char c : s) {
        if (c == ':') {
            spaced += " : ";
        } else {
            spaced += c;
        }
    }
    std::istringstream ss(spaced);
    std::vector<std::string> tok;
    std::string t;
    while (ss >> t) {
        tok.push_back(t);
    }
    return tok;
}

std::string parent_dir(const std::string& path) {
    const std::size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) {
        return ".";
    }
    if (pos == 0) {
        return path.substr(0, 1);
    }
    return path.substr(0, pos);
}

std::string resolve_sibling(const std::string& dir, const std::string& file) {
    if (file.empty() || file[0] == '/') {
        return file;
    }
    if (dir == ".") {
        return file;
    }
    return dir + "/" + file;
}

struct File {
    std::string path;
    std::ifstream in;
    int lineno = 0;
    std::string line;

    explicit File(std::string p) : path(std::move(p)), in(path) {
        if (!in) {
            throw std::runtime_error("cannot open " + path);
        }
    }

    void err(const std::string& msg) const {
        throw std::runtime_error(path + ":" + std::to_string(lineno) + ": " + msg);
    }

    bool next() {
        std::string raw;
        while (std::getline(in, raw)) {
            ++lineno;
            const auto hash = raw.find('#');
            if (hash != std::string::npos) {
                raw = raw.substr(0, hash);
            }
            line = trim(raw);
            if (line.empty()) {
                continue;
            }
            std::istringstream ss(line);
            std::string tok;
            ss >> tok;
            if (tok == "UCLA") {
                continue;
            }
            return true;
        }
        return false;
    }
};

struct AuxPaths {
    std::string nodes;
    std::string nets;
    std::string scl;
};

AuxPaths parse_aux(const std::string& path) {
    File f(path);
    bool have_rbp = false;
    std::vector<std::string> files;
    while (f.next()) {
        const auto tok = tokenize(f.line);
        if (tok.empty()) {
            continue;
        }
        if (tok[0] != "RowBasedPlacement") {
            f.err("expected RowBasedPlacement, got " + tok[0]);
        }
        if (have_rbp) {
            f.err("duplicate RowBasedPlacement");
        }
        have_rbp = true;
        std::size_t i = 1;
        if (i < tok.size() && tok[i] == ":") {
            ++i;
        }
        if (i >= tok.size()) {
            f.err("RowBasedPlacement has no files");
        }
        for (; i < tok.size(); ++i) {
            if (tok[i] == ":") {
                f.err("unexpected ':'");
            }
            files.push_back(tok[i]);
        }
    }
    if (!have_rbp) {
        throw std::runtime_error(path + ": missing RowBasedPlacement");
    }

    const std::string dir = parent_dir(path);
    AuxPaths out;
    for (const std::string& name : files) {
        const std::string resolved = resolve_sibling(dir, name);
        if (has_ext(name, ".nodes")) {
            if (!out.nodes.empty()) {
                throw std::runtime_error(path + ": duplicate .nodes");
            }
            out.nodes = resolved;
        } else if (has_ext(name, ".nets")) {
            if (!out.nets.empty()) {
                throw std::runtime_error(path + ": duplicate .nets");
            }
            out.nets = resolved;
        } else if (has_ext(name, ".scl")) {
            if (!out.scl.empty()) {
                throw std::runtime_error(path + ": duplicate .scl");
            }
            out.scl = resolved;
        } else if (has_ext(name, ".wts") || has_ext(name, ".pl")) {
            continue;
        } else {
            throw std::runtime_error(path + ": unsupported file " + name);
        }
    }
    if (out.nodes.empty()) {
        throw std::runtime_error(path + ": missing .nodes");
    }
    if (out.nets.empty()) {
        throw std::runtime_error(path + ": missing .nets");
    }
    if (out.scl.empty()) {
        throw std::runtime_error(path + ": missing .scl");
    }
    return out;
}

void expect_colon_value(File& f, const std::vector<std::string>& tok,
                        const char* key, std::string& value) {
    if (tok.size() != 3 || tok[1] != ":") {
        f.err(std::string(key) + " expects ': <value>'");
    }
    value = tok[2];
}

void parse_nodes(const std::string& path, Design& d,
                 std::unordered_map<std::string, std::size_t>& cell_index) {
    File f(path);
    bool have_nn = false;
    bool have_nt = false;
    int num_nodes = 0;
    int num_terminals = 0;

    while (f.next()) {
        const auto tok = tokenize(f.line);
        if (tok.empty()) {
            continue;
        }
        if (tok[0] == "NumNodes" || tok[0] == "NumTerminals") {
            std::string val;
            expect_colon_value(f, tok, tok[0].c_str(), val);
            int n = 0;
            if (!to_int(val, n) || n < 0) {
                f.err(tok[0] + " must be a non-negative integer");
            }
            if (tok[0] == "NumNodes") {
                if (have_nn) {
                    f.err("duplicate NumNodes");
                }
                num_nodes = n;
                have_nn = true;
            } else {
                if (have_nt) {
                    f.err("duplicate NumTerminals");
                }
                num_terminals = n;
                have_nt = true;
                if (num_terminals != 0) {
                    f.err("NumTerminals must be 0 (v1 has no terminals)");
                }
            }
            continue;
        }

        if (!have_nn || !have_nt) {
            f.err("cell record before NumNodes/NumTerminals");
        }
        if (tok.size() < 3) {
            f.err("cell expects name width height");
        }
        if (tok.size() > 3) {
            if (tok[3] == "terminal" || tok[3] == "terminal_NI") {
                f.err("terminals not supported in v1");
            }
            f.err("unexpected token after cell " + tok[0]);
        }
        const std::string& name = tok[0];
        if (!valid_name(name)) {
            f.err("invalid cell name");
        }
        double w = 0, h = 0;
        if (!to_double(tok[1], w) || !to_double(tok[2], h)) {
            f.err("cell size must be numeric");
        }
        if (w <= 0 || h <= 0) {
            f.err("cell size must be positive");
        }
        if (cell_index.count(name)) {
            f.err("duplicate cell " + name);
        }
        Cell c;
        c.name = name;
        c.width = w;
        c.height = h;
        cell_index[name] = d.cells.size();
        d.cells.push_back(std::move(c));
    }

    if (!have_nn) {
        throw std::runtime_error(path + ": missing NumNodes");
    }
    if (!have_nt) {
        throw std::runtime_error(path + ": missing NumTerminals");
    }
    if (num_terminals != 0) {
        throw std::runtime_error(path + ": NumTerminals must be 0 (v1 has no terminals)");
    }
    if (static_cast<int>(d.cells.size()) != num_nodes) {
        throw std::runtime_error(path + ": NumNodes is " + std::to_string(num_nodes) +
                                 " but parsed " + std::to_string(d.cells.size()) + " cells");
    }
}

void parse_nets(const std::string& path, Design& d,
                const std::unordered_map<std::string, std::size_t>& cell_index) {
    File f(path);
    bool have_nn = false;
    bool have_np = false;
    int num_nets = 0;
    int num_pins = 0;
    int pins_seen = 0;
    std::unordered_set<std::string> net_names;

    auto read_pin_line = [&](Net& net) {
        const auto tok = tokenize(f.line);
        if (tok.empty()) {
            f.err("empty pin line");
        }
        if (tok[0] == "NetDegree" || tok[0] == "NumNets" || tok[0] == "NumPins") {
            f.err("expected pin, got " + tok[0]);
        }
        const std::string& cell = tok[0];
        const auto it = cell_index.find(cell);
        if (it == cell_index.end()) {
            f.err("unknown cell " + cell);
        }
        std::size_t i = 1;
        if (i < tok.size() && (tok[i] == "I" || tok[i] == "O" || tok[i] == "B")) {
            ++i;
        }
        if (i < tok.size()) {
            if (tok[i] != ":") {
                f.err("unexpected token on pin " + cell);
            }
            ++i;
            if (i + 1 >= tok.size()) {
                f.err("pin offset expects dx dy");
            }
            double dx = 0, dy = 0;
            if (!to_double(tok[i], dx) || !to_double(tok[i + 1], dy)) {
                f.err("pin offset must be numeric");
            }
            i += 2;
            // v1: pin is cell lower-left; offsets ignored
        }
        if (i != tok.size()) {
            f.err("unexpected token after pin " + cell);
        }
        net.pins.push_back(it->second);
        ++pins_seen;
    };

    while (f.next()) {
        const auto tok = tokenize(f.line);
        if (tok.empty()) {
            continue;
        }
        if (tok[0] == "NumNets" || tok[0] == "NumPins") {
            std::string val;
            expect_colon_value(f, tok, tok[0].c_str(), val);
            int n = 0;
            if (!to_int(val, n) || n < 0) {
                f.err(tok[0] + " must be a non-negative integer");
            }
            if (tok[0] == "NumNets") {
                if (have_nn) {
                    f.err("duplicate NumNets");
                }
                num_nets = n;
                have_nn = true;
            } else {
                if (have_np) {
                    f.err("duplicate NumPins");
                }
                num_pins = n;
                have_np = true;
            }
            continue;
        }
        if (tok[0] != "NetDegree") {
            f.err("expected NetDegree, got " + tok[0]);
        }
        if (!have_nn || !have_np) {
            f.err("NetDegree before NumNets/NumPins");
        }
        if (tok.size() != 4 || tok[1] != ":") {
            f.err("NetDegree expects ': <degree> <name>'");
        }
        int degree = 0;
        if (!to_int(tok[2], degree) || degree <= 0) {
            f.err("net degree must be a positive integer");
        }
        const std::string& name = tok[3];
        if (!valid_name(name)) {
            f.err("invalid net name");
        }
        if (net_names.count(name)) {
            f.err("duplicate net " + name);
        }

        Net net;
        net.name = name;
        for (int p = 0; p < degree; ++p) {
            if (!f.next()) {
                f.err("NetDegree " + name + " missing pins");
            }
            read_pin_line(net);
        }
        if (static_cast<int>(net.pins.size()) != degree) {
            f.err("NET " + name + " pin count mismatch");
        }
        net_names.insert(name);
        d.nets.push_back(std::move(net));
    }

    if (!have_nn) {
        throw std::runtime_error(path + ": missing NumNets");
    }
    if (!have_np) {
        throw std::runtime_error(path + ": missing NumPins");
    }
    if (static_cast<int>(d.nets.size()) != num_nets) {
        throw std::runtime_error(path + ": NumNets is " + std::to_string(num_nets) +
                                 " but parsed " + std::to_string(d.nets.size()) + " nets");
    }
    if (pins_seen != num_pins) {
        throw std::runtime_error(path + ": NumPins is " + std::to_string(num_pins) +
                                 " but parsed " + std::to_string(pins_seen) + " pins");
    }
}

void parse_one_row(File& f, Design& d) {
    const auto head = tokenize(f.line);
    if (head.size() != 2 || head[1] != "Horizontal") {
        f.err("expected 'CoreRow Horizontal'");
    }

    bool have_y = false, have_h = false, have_sw = false, have_x0 = false, have_ns = false;
    double y = 0, h = 0, sw = 0, x0 = 0;
    int ns = 0;

    auto take_field = [&](const std::string& key, const std::string& val) {
        if (key == "Coordinate") {
            if (have_y) {
                f.err("duplicate Coordinate");
            }
            if (!to_double(val, y)) {
                f.err("Coordinate must be numeric");
            }
            have_y = true;
        } else if (key == "Height") {
            if (have_h) {
                f.err("duplicate Height");
            }
            if (!to_double(val, h) || h <= 0) {
                f.err("Height must be positive");
            }
            have_h = true;
        } else if (key == "Sitewidth") {
            if (have_sw) {
                f.err("duplicate Sitewidth");
            }
            if (!to_double(val, sw) || sw <= 0) {
                f.err("Sitewidth must be positive");
            }
            have_sw = true;
        } else if (key == "SubrowOrigin") {
            if (have_x0) {
                f.err("duplicate SubrowOrigin (v1: one subrow per CoreRow)");
            }
            if (!to_double(val, x0)) {
                f.err("SubrowOrigin must be numeric");
            }
            have_x0 = true;
        } else if (key == "NumSites") {
            if (have_ns) {
                f.err("duplicate NumSites");
            }
            if (!to_int(val, ns) || ns <= 0) {
                f.err("NumSites must be a positive integer");
            }
            have_ns = true;
        } else if (key == "Sitespacing" || key == "Siteorient" || key == "Sitesymmetry") {
            return;
        } else {
            f.err("unknown row field " + key);
        }
    };

    while (f.next()) {
        const auto tok = tokenize(f.line);
        if (tok.size() == 1 && tok[0] == "End") {
            if (!have_y || !have_h || !have_sw || !have_x0 || !have_ns) {
                f.err("CoreRow missing Coordinate/Height/Sitewidth/SubrowOrigin/NumSites");
            }
            Row r;
            r.y = y;
            r.height = h;
            r.x0 = x0;
            r.x1 = x0 + static_cast<double>(ns) * sw;
            if (r.x1 <= r.x0) {
                f.err("row x1 must be > x0");
            }
            d.rows.push_back(r);
            return;
        }
        if (tok.empty() || tok[0] == "CoreRow") {
            f.err("missing End");
        }
        if (tok.size() % 3 != 0) {
            f.err("row field expects 'Key : value'");
        }
        for (std::size_t i = 0; i < tok.size(); i += 3) {
            if (tok[i + 1] != ":") {
                f.err("row field expects 'Key : value'");
            }
            take_field(tok[i], tok[i + 2]);
        }
    }
    f.err("missing End");
}

void parse_scl(const std::string& path, Design& d) {
    File f(path);
    bool have_nr = false;
    int num_rows = 0;

    while (f.next()) {
        const auto tok = tokenize(f.line);
        if (tok.empty()) {
            continue;
        }
        if (tok[0] == "NumRows") {
            if (have_nr) {
                f.err("duplicate NumRows");
            }
            std::string val;
            expect_colon_value(f, tok, "NumRows", val);
            if (!to_int(val, num_rows) || num_rows <= 0) {
                f.err("NumRows must be a positive integer");
            }
            have_nr = true;
            continue;
        }
        if (tok[0] == "CoreRow") {
            if (!have_nr) {
                f.err("CoreRow before NumRows");
            }
            parse_one_row(f, d);
            continue;
        }
        f.err("unknown keyword " + tok[0]);
    }

    if (!have_nr) {
        throw std::runtime_error(path + ": missing NumRows");
    }
    if (static_cast<int>(d.rows.size()) != num_rows) {
        throw std::runtime_error(path + ": NumRows is " + std::to_string(num_rows) +
                                 " but parsed " + std::to_string(d.rows.size()) + " rows");
    }
}

void die_from_rows(Design& d, const std::string& scl_path) {
    if (d.rows.empty()) {
        throw std::runtime_error(scl_path + ": no rows");
    }
    std::sort(d.rows.begin(), d.rows.end(),
              [](const Row& a, const Row& b) { return a.y < b.y; });

    Rect b;
    b.x0 = d.rows[0].x0;
    b.y0 = d.rows[0].y;
    b.x1 = d.rows[0].x1;
    b.y1 = d.rows[0].y + d.rows[0].height;
    for (std::size_t i = 1; i < d.rows.size(); ++i) {
        const Row& r = d.rows[i];
        if (r.x0 < b.x0) {
            b.x0 = r.x0;
        }
        if (r.y < b.y0) {
            b.y0 = r.y;
        }
        if (r.x1 > b.x1) {
            b.x1 = r.x1;
        }
        const double top = r.y + r.height;
        if (top > b.y1) {
            b.y1 = top;
        }
    }
    if (b.x1 <= b.x0 || b.y1 <= b.y0) {
        throw std::runtime_error(scl_path + ": row union is not a valid die");
    }
    d.die.bbox = b;
}

}  // namespace

Design parse_bookshelf(const std::string& path) {
    const AuxPaths files = parse_aux(path);
    Design d;
    std::unordered_map<std::string, std::size_t> cell_index;
    parse_nodes(files.nodes, d, cell_index);
    parse_nets(files.nets, d, cell_index);
    parse_scl(files.scl, d);
    die_from_rows(d, files.scl);
    return d;
}

}  // namespace minipd
