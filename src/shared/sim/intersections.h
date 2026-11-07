/**
 * @file
 * Share bounding-box, ray/triangle and triangle/triangle intersection queries.
 *
 * Provides dual-targeted geometric intersection tests (host and CUDA device)
 * used for broad-phase and narrow-phase collision detection.
 */

#pragma once

#include "settings/collisions/constants.h"
#include "shared/sim/vector.h"


namespace sim {

constexpr Real EPSILON_SQ = settings::Collisions::Geometry::EPSILON * settings::Collisions::Geometry::EPSILON;

/* Triangle vertices in world-space metres. */
struct Triangle { Vec3 a_m, b_m, c_m; };
/* Axis-aligned world-space bounds in metres. */
struct Aabb { Vec3 min_m, max_m; };


/**
 * Test inclusive overlap of two axis-aligned boxes.
 *
 * @param a First world-axis bounding box to test.
 * @param b Second world-axis bounding box to test.
 * @return True when the two axis-aligned boxes overlap on every axis.
 */
SIM_HD inline bool intersects(Aabb a, Aabb b) {
    return (
        a.min_m.x <= b.max_m.x && 
        a.max_m.x >= b.min_m.x && 
        a.min_m.y <= b.max_m.y &&
        a.max_m.y >= b.min_m.y && 
        a.min_m.z <= b.max_m.z && 
        a.max_m.z >= b.min_m.z
    );
}


/**
 * Perform a two-sided Moller-Trumbore ray/triangle intersection.
 *
 * @param origin_m   Ray origin in metres.
 * @param direction  Ray direction; a unit vector gives distance in metres.
 * @param t          Triangle vertices in metres.
 * @param distance_m Receives the nonnegative ray parameter.
 * @return True when the ray intersects the triangle.
 */
SIM_HD inline bool rayTriangle(Vec3 origin_m, Vec3 direction, Triangle t, Real& distance_m) {
    Vec3 e1 = t.b_m - t.a_m, e2 = t.c_m - t.a_m, h = cross(direction, e2);
    Real det = dot(e1, h);
    
    if (absolute(det) < settings::Collisions::Geometry::EPSILON)
        return false;

    Real inv = 1 / det;
    Vec3 s = origin_m - t.a_m;
    Real u = dot(s, h) * inv;
    
    if (u < 0 || u > 1)
        return false;

    Vec3 q = cross(s, e1);
    Real v = dot(direction, q) * inv;
    
    if (v < 0 || u + v > 1)
        return false;

    distance_m = dot(e2, q) * inv;
    return distance_m >= 0;
}


/**
 * Project both triangles onto an axis and test for a separating interval.
 *
 * @param axis World-space projection/separation direction used by this calculation.
 * @param a    First triangle in the common coordinate frame.
 * @param b    Second triangle in the common coordinate frame.
 * @return True when the supplied axis separates the two projected triangles.
 */
SIM_HD inline bool separatesTriangles(Vec3 axis, Triangle a, Triangle b) {
    if (dot(axis, axis) < EPSILON_SQ)
        return false;

    Real av[3] = {dot(a.a_m, axis), dot(a.b_m, axis), dot(a.c_m, axis)};
    Real bv[3] = {dot(b.a_m, axis), dot(b.b_m, axis), dot(b.c_m, axis)};

    Real alo = minimum(av[0], minimum(av[1], av[2]));
    Real ahi = maximum(av[0], maximum(av[1], av[2]));
    Real blo = minimum(bv[0], minimum(bv[1], bv[2]));
    Real bhi = maximum(bv[0], maximum(bv[1], bv[2]));

    return ahi < blo - settings::Collisions::Geometry::EPSILON || bhi < alo - settings::Collisions::Geometry::EPSILON;
}


/**
 * Test face, edge-edge and in-plane separating axes, including coplanar triangles.
 *
 * @param a First triangle in the common coordinate frame.
 * @param b Second triangle in the common coordinate frame.
 * @return True when the supported triangle intersection test succeeds.
 */
SIM_HD inline bool triangleTriangle(Triangle a, Triangle b) {
    Vec3 ae[3] = {a.b_m - a.a_m, a.c_m - a.b_m, a.a_m - a.c_m};
    Vec3 be[3] = {b.b_m - b.a_m, b.c_m - b.b_m, b.a_m - b.c_m};
    Vec3 an = cross(ae[0], ae[1]);
    Vec3 bn = cross(be[0], be[1]);

    if (dot(an, an) < EPSILON_SQ || dot(bn, bn) < EPSILON_SQ)
        return false;
        
    if (separatesTriangles(an, a, b) || separatesTriangles(bn, a, b))
        return false;

    for (int i = 0; i < 3; ++i) {
        if (separatesTriangles(cross(an, ae[i]), a, b) || separatesTriangles(cross(bn, be[i]), a, b)) {
            return false;
        }
        
        for (int j = 0; j < 3; ++j) {
            if (separatesTriangles(cross(ae[i], be[j]), a, b))
                return false;
        }
    }
    
    return true;
}

} // namespace sim
