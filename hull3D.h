//
// Created by Martin Wickham on 3/2/17.
//

#ifndef MINKOWSKIHULL3D_HULL3D_H
#define MINKOWSKIHULL3D_HULL3D_H

#include <glm/glm.hpp>
#include <vector>

struct Collider3D {
    virtual glm::vec3 findSupport(glm::vec3 direction) = 0;
};

struct AddCollider3D : public Collider3D {
    Collider3D *a;
    Collider3D *b;

    glm::vec3 findSupport(glm::vec3 direction) override {
        return a->findSupport(direction) + b->findSupport(direction);
    }
};

struct SubCollider3D : public Collider3D {
    Collider3D *a;
    Collider3D *b;

    glm::vec3 findSupport(glm::vec3 direction) override {
        return a->findSupport(direction) - b->findSupport(-direction);
    }
};

struct PointCollider3D : public Collider3D {
    glm::vec3 point;

    glm::vec3 findSupport(glm::vec3 direction) override { return point; }
};

struct SphereCollider3D : public Collider3D {
    float radius;

    glm::vec3 findSupport(glm::vec3 direction) override {
        return radius * glm::normalize(direction);
    }
};

struct PointHullCollider3D : public Collider3D {
    std::vector<glm::vec3> points;

    glm::vec3 findSupport(glm::vec3 direction) override {
        glm::vec3 best;
        float bestDot = -std::numeric_limits<float>::infinity();
        for (glm::vec3 &vec : points) {
            float d = glm::dot(direction, vec);
            if (d > bestDot) {
                bestDot = d;
                best = vec;
            }
        }
        return best;
    }
};

struct HalfEdge {
    uint16_t vertex;
    uint16_t opposite;
};

struct Triangle {
    uint32_t flags;
    HalfEdge edges[3];
};

struct SurfaceState {
    Collider3D *object;
    float epsilon;
    std::vector<glm::vec3> points;
    std::vector<Triangle> triangles;
    uint16_t current;

    void init();
    void step();
    inline bool done() {
        return current == 65535;
    }
    inline HalfEdge *edges() {
        static_assert(sizeof(Triangle) == 4 * sizeof(HalfEdge), "Four HalfEdges must be the same size as a Triangle."); // this is necessary for the indexing scheme
        return reinterpret_cast<HalfEdge *>(&triangles[0]);
    }
};

#endif //MINKOWSKIHULL3D_HULL3D_H
