//
// Created by Martin Wickham on 3/6/17.
//

#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#include "loader.h"
#include "hull3D.h"

using namespace std;
using namespace glm;

struct Symbol {
    string name;
    Collider3D *value = nullptr;
};

struct Loader {
    virtual bool load(istringstream &line, Symbol &symbol, const vector<Symbol> &symbols, int lineNum) = 0;
};

const Symbol *findSymbol(const vector<Symbol> &symbols, const string &name) {
    for (const Symbol &sym : symbols) {
        if (sym.name == name) return &sym;
    }
    return nullptr;
}

struct SphereLoader : public Loader {
    bool load(istringstream &line, Symbol &symbol, const vector<Symbol> &symbols, int lineNum) override {
        SphereCollider3D *collider = new SphereCollider3D();
        if (!(line >> collider->radius)) {
            printf("Error: Failed to load sphere, line %d.\n", lineNum);
            delete collider;
            return false;
        }
        symbol.value = collider;
        return true;
    }
};

struct PointLoader : public Loader {
    bool load(istringstream &line, Symbol &symbol, const vector<Symbol> &symbols, int lineNum) override {
        PointCollider3D *collider = new PointCollider3D();
        if (!(line >> collider->point.x >> collider->point.y >> collider->point.z)) {
            printf("Error: Failed to load point, line %d.\n", lineNum);
            delete collider;
            return false;
        }
        symbol.value = collider;
        return true;
    }
};

struct AddLoader : public Loader {
    bool load(istringstream &line, Symbol &symbol, const vector<Symbol> &symbols, int lineNum) override {
        std::string a, b;
        if (!(line >> a >> b)) {
            printf("Error: Not enough tokens for add, line %d.\n", lineNum);
            return false;
        }
        const Symbol *symA = findSymbol(symbols, a);
        const Symbol *symB = findSymbol(symbols, b);
        if (!symA) {
            printf("Error: Unknown symbol %s, line %d.\n", a.c_str(), lineNum);
            return false;
        }
        if (!symB) {
            printf("Error: Unknown symbol %s, line %d.\n", b.c_str(), lineNum);
            return false;
        }
        AddCollider3D *add = new AddCollider3D();
        add->a = symA->value;
        add->b = symB->value;
        symbol.value = add;
        return true;
    }
};

struct SubLoader : public Loader {
    bool load(istringstream &line, Symbol &symbol, const vector<Symbol> &symbols, int lineNum) override {
        std::string a, b;
        if (!(line >> a >> b)) {
            printf("Error: Not enough tokens for sub, line %d.\n", lineNum);
            return false;
        }
        const Symbol *symA = findSymbol(symbols, a);
        const Symbol *symB = findSymbol(symbols, b);
        if (!symA) {
            printf("Error: Unknown symbol %s, line %d.\n", a.c_str(), lineNum);
            return false;
        }
        if (!symB) {
            printf("Error: Unknown symbol %s, line %d.\n", b.c_str(), lineNum);
            return false;
        }
        SubCollider3D *sub = new SubCollider3D();
        sub->a = symA->value;
        sub->b = symB->value;
        symbol.value = sub;
        return true;
    }
};

struct PointsLoader : public Loader {
    bool load(istringstream &line, Symbol &symbol, const vector<Symbol> &symbols, int lineNum) override {
        vec3 pt;
        PointHullCollider3D *collider = new PointHullCollider3D();
        while (line >> pt.x >> pt.y >> pt.z) {
            collider->points.push_back(pt);
        }
        if (collider->points.size() == 0) {
            printf("Error: Empty point collider, line %d.\n", lineNum);
            return false;
        }
        symbol.value = collider;
        return true;
    }
};

static SphereLoader sphereLoader;
static PointLoader pointLoader;
static AddLoader addLoader;
static SubLoader subLoader;
static PointsLoader pointsLoader;

Loader *findLoader(const string &type) {
    if (type == "sphere") return &sphereLoader;
    if (type == "point") return &pointLoader;
    if (type == "add") return &addLoader;
    if (type == "sub") return &subLoader;
    if (type == "points") return &pointsLoader;
    return nullptr;
}

bool load(const char *filename, SurfaceState *state) {
    ifstream file(filename);
    if (!file) {
        printf("Failed to open file '%s'.\n", filename);
        return false;
    }

    vector<Symbol> symbols;
    bool hasEpsilon = false;
    bool hasObject = false;

    int lineNum = 0;
    string line;
    string token;
    while (getline(file, line)) {
        lineNum++;
        if (line.empty() || line[0] == '#') continue;

        istringstream tokens(line);
        tokens >> token;

        if (token == "epsilon") {
            if (!(tokens >> state->epsilon)) {
                printf("Error: Failed to parse epsilon, line %d.\n", lineNum);
            } else {
                hasEpsilon = true;
            }
            continue;
        }

        if (findSymbol(symbols, token)) {
            printf("Error: Duplicate token '%s' on line %d.\n", token.c_str(), lineNum);
            continue;
        }

        symbols.emplace_back();
        Symbol &symbol = symbols.back();
        symbol.name = token;
        if (!(tokens >> token)) {
            printf("Error: No type on line %d.\n", lineNum);
            symbols.pop_back();
            continue;
        }

        Loader *loader = findLoader(token);
        if (!loader) {
            printf("Error: No loader found for type %s, line %d.\n", token.c_str(), lineNum);
            symbols.pop_back();
            continue;
        }

        if (!loader->load(tokens, symbol, symbols, lineNum)) {
            symbols.pop_back();
            continue;
        }

        if (symbol.name == "object") {
            state->object = symbol.value;
            hasObject = true;
        }
    }

    if (!hasEpsilon) {
        printf("Error: No epsilon specified.\n");
        return false;
    }

    if (!hasObject) {
        printf("Error: No object specified.\n");
        return false;
    }

    return true;
}
