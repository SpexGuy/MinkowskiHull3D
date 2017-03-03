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
    tri1.edges[0].opposite = 5;
    tri1.edges[1].vertex = 1;
    tri1.edges[1].opposite = 6;
    tri1.edges[2].vertex = 2;
    tri1.edges[2].opposite = 7;

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

}
