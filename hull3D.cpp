//
// Created by Martin Wickham on 3/2/17.
//

#include "hull3D.h"

using namespace std;
using namespace glm;

void SurfaceState::init() {
    current = 0;
    vec3 top = object->findSupport(vec3(0, 1, 0));
    vec3 bottom = object->findSupport(vec3(0, -1, 0));

    if (top.y == bottom.y) {
        current = 65535;
        return;
    }

    // find a vector perpendicular to our top-bottom line
    vec3 perp = vec3(top.x - bottom.x, 0, bottom.z - top.z);
    if (perp == vec3(0)) perp = vec3(0,0,1); // if top-bottom is vertical, we can pick any vector on the xz plane. Z should suffice.

    vec3 left = object->findSupport(perp);
    if (left == top || left == bottom) {
        left = object->findSupport(-perp);
        if (left == top || left == bottom) {
            current = 65535;
            return;
        }
    }

    points.push_back(top);
    points.push_back(bottom);
    points.push_back(left);

    // Make two triangles welded together.
    triangles.emplace_back();
    Triangle &tri1 = triangles.back();
    tri1.flags = 0;
    tri1.edges[0].vertex = 0;
    tri1.edges[0].opposite = 7;
    tri1.edges[1].vertex = 1;
    tri1.edges[1].opposite = 6;
    tri1.edges[2].vertex = 2;
    tri1.edges[2].opposite = 5;

    triangles.emplace_back();
    Triangle &tri2 = triangles.back();
    tri2.flags = 0;
    tri2.edges[0].vertex = 0;
    tri2.edges[0].opposite = 3;
    tri2.edges[1].vertex = 2;
    tri2.edges[1].opposite = 2;
    tri2.edges[2].vertex = 1;
    tri2.edges[2].opposite = 1;
}

void SurfaceState::step() {
    if (done()) return;

    Triangle &expand = triangles[current];
    vec3 a = points[expand.edges[0].vertex];
    vec3 b = points[expand.edges[1].vertex];
    vec3 c = points[expand.edges[2].vertex];
    vec3 normal = cross(c-b, a-b); // NOTE: not normalized

    // ignore degenerates and move to the next triangle
    if (normal == vec3(0)) {
        printf("Degenerate Triangle at %d!\n", current);
        current++;
        return;
    }

    vec3 support = object->findSupport(normal);

    // If the support is within epsilon of the surface, this face is complete. Move to the next triangle.
    if (dot(normalize(normal), support - a) <= epsilon) {
        current++;
        return;
    }


    uint16_t pointIndex = points.size();
    points.push_back(support);

    // Otherwise we need to replace the current triangle with three triangles
    // NOTE: expand may not be valid beyond this point, if the vector reallocates.
    uint16_t triAIndex = current;
    uint16_t triBIndex = triangles.size();
    triangles.emplace_back();
    uint16_t triCIndex = triangles.size();
    triangles.emplace_back();

    Triangle &triA = triangles[triAIndex];
    Triangle &triB = triangles[triBIndex];
    Triangle &triC = triangles[triCIndex];

    // Fill in triB and triC first
    // triA = ABD
    // triB = BCD
    // triC = CAD
    triB.flags = 0;
    triB.edges[0] = triA.edges[1];
    triB.edges[1].vertex = triA.edges[2].vertex;
    triB.edges[1].opposite = triCIndex * 4 + 3;
    triB.edges[2].vertex = pointIndex;
    triB.edges[2].opposite = triAIndex * 4 + 2;
    edges()[triB.edges[0].opposite].opposite = triBIndex * 4 + 1;

    triC.flags = 0;
    triC.edges[0] = triA.edges[2];
    triC.edges[1].vertex = triA.edges[0].vertex;
    triC.edges[1].opposite = triAIndex * 4 + 3;
    triC.edges[2].vertex = pointIndex;
    triC.edges[2].opposite = triBIndex * 4 + 2;
    edges()[triC.edges[0].opposite].opposite = triCIndex * 4 + 1;

    triA.edges[2].vertex = pointIndex;
    triA.edges[1].opposite = triBIndex * 4 + 3;
    triA.edges[2].opposite = triCIndex * 4 + 2;

    // Now we need to check across the edges and make sure the shape is still convex.
    maybeSwapEdge(triAIndex * 4 + 1);
    maybeSwapEdge(triBIndex * 4 + 1);
    maybeSwapEdge(triCIndex * 4 + 1);
}

void SurfaceState::maybeSwapEdge(uint16_t base) {
    uint16_t prev = prevEdge(base);
    uint16_t next = prevEdge(prev);

    uint16_t oppBase = edges()[base].opposite;
    uint16_t oppPrev = prevEdge(oppBase);
    uint16_t oppNext = prevEdge(oppPrev);

    vec3 a = points[edges()[base].vertex];
    vec3 b = points[edges()[next].vertex];
    vec3 c = points[edges()[prev].vertex];
    vec3 d = points[edges()[oppPrev].vertex];

    if (dot(cross(a - c, b - c), d - c) <= 0) return;

    // fix up vertices
    edges()[next].vertex = edges()[oppPrev].vertex;
    edges()[oppNext].vertex = edges()[prev].vertex;

    // fix up opposite links
    edges()[base].opposite = edges()[oppNext].opposite;
    edges()[edges()[base].opposite].opposite = base;

    edges()[oppBase].opposite = edges()[next].opposite;
    edges()[edges()[oppBase].opposite].opposite = oppBase;

    edges()[next].opposite = oppNext;
    edges()[oppNext].opposite = next;

    maybeSwapEdge(base);
    maybeSwapEdge(oppPrev);
}
