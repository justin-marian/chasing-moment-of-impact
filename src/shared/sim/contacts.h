/**
 * @file
 * Shared deterministic host/device contacts operations.
 *
 * Inline definitions are required to allow CUDA device compilation while
 * sharing the exact same implementation logic with the host CPU.
 */

#pragma once

#include "settings/collisions/constants.h"
#include "shared/sim/dynamics.h"


namespace sim {

/**
 * Generate an analytic sphere contact with the configured speculative sweep.
 *
 * If the spheres are not currently overlapping, a continuous collision
 * detection sweep solves for the earliest nonnegative time of impact.
 * Moving-apart pairs and negative discriminants cannot collide during the sweep.
 *
 * @param s    Active-body state view.
 * @param a    First body index.
 * @param b    Second body index.
 * @param dt_s Simulation or impact-aging interval in seconds.
 * @param c    Fixed-step world, material and solver settings.
 * @param out  Contact constraint populated when the pair passes the contact test.
 * @return True when the pair overlaps or requires speculative contact.
 */
SIM_HD inline bool sphereSphere(StateView s, int a, int b, Real dt_s, const WorldConfig& c, Contact& out) {
    constexpr Real MIDPOINT_SCALE = Real(.5);

    Vec3 d = s.position_m[b] - s.position_m[a];
    Vec3 normal = normalize(d);

    Real distance = length(d);
    Real r = s.halfExtents_m[a].x + s.halfExtents_m[b].x;
    Real depth = r - distance;

    if (depth < -c.contactSlop_m) {
        if (!c.enableCcd) return false;

        Vec3 relative = s.velocity_mps[b] - s.velocity_mps[a];
        Real aa = dot(relative, relative);
        Real bb = dot(d, relative);
        Real cc = dot(d, d) - r * r;
        Real discriminant = bb * bb - aa * cc;
        if (aa < settings::Collisions::Geometry::EPSILON || bb >= 0 || discriminant < 0) {
            return false;
        }

        Real impact_s = (-bb - Real(sqrt(discriminant))) / aa;
        if (impact_s < 0 || impact_s > dt_s) {
            return false;
        }

        normal = normalize(d + relative * impact_s);
        depth = 0;
    }

    out = {};
    out.a_i = a;
    out.b_i = b;
    out.normal = normal;
    out.depth_m = depth;
    out.point_m = s.position_m[a] + normal * (s.halfExtents_m[a].x - depth * MIDPOINT_SCALE);

    return true;
}


/**
 * Find a sphere/OBB contact in the box's local coordinates.
 *
 * When the sphere center is inside the box, the interior normal points inward
 * so the resulting impulse ejects the contained sphere.
 *
 * @param s      Active-body state view.
 * @param sphere Index of the sphere body.
 * @param box    Index of the oriented-box body.
 * @param c      Fixed-step world, material and solver settings.
 * @param out    Contact constraint populated when the pair passes the contact test.
 * @return True when the pair overlaps or requires speculative contact.
 */
SIM_HD inline bool sphereBox(StateView s, int sphere, int box, const WorldConfig& c, Contact& out) {
    Vec3 local = rotate(conjugate(s.orientation[box]), s.position_m[sphere] - s.position_m[box]);
    Vec3 h = s.halfExtents_m[box];
    Vec3 closest = {clamp(local.x, -h.x, h.x), clamp(local.y, -h.y, h.y), clamp(local.z, -h.z, h.z)};

    Vec3 delta = closest - local;
    Real distance = length(delta);
    Real radius = s.halfExtents_m[sphere].x;
    Vec3 n{};
    Real depth = radius - distance;

    if (distance < settings::Collisions::Geometry::EPSILON) {
        int axis = 0;
        Real gap = h.x - absolute(local.x);

        for (int k = 1; k < 3; ++k) {
            if (h[k] - absolute(local[k]) < gap) {
                axis = k;
                gap = h[k] - absolute(local[k]);
            }
        }

        n[axis] = local[axis] >= 0 ? Real(-1) : Real(1);
        depth = radius + gap;
        closest = local;
        closest[axis] = local[axis] >= 0 ? h[axis] : -h[axis];
    } else {
        n = delta / distance;
    }

    if (depth < -c.contactSlop_m) {
        return false;
    }

    out = {};
    out.a_i = sphere;
    out.b_i = box;
    out.normal = rotate(s.orientation[box], n);
    out.depth_m = depth;
    out.point_m = s.position_m[box] + rotate(s.orientation[box], closest);

    return true;
}


/**
 * Test OBB separating axes and generate the chosen contact normal/depth.
 *
 * @param s   Active-body state view.
 * @param a   First body index.
 * @param b   Second body index.
 * @param c   Fixed-step world, material and solver settings.
 * @param out Contact constraint populated when the pair passes the contact test.
 * @return True when the pair overlaps or requires speculative contact.
 */
SIM_HD inline bool boxBox(StateView s, int a, int b, const WorldConfig& c, Contact& out) {
    constexpr int SEPARATING_AXES_COUNT = 15;
    constexpr Real MAX_INITIAL_DEPTH = Real(1e20);
    constexpr Real MIDPOINT_SCALE = Real(.5);

    Vec3 axesA[3] = {
        rotate(s.orientation[a], {1, 0, 0}),
        rotate(s.orientation[a], {0, 1, 0}),
        rotate(s.orientation[a], {0, 0, 1})
    };
    Vec3 axesB[3] = {
        rotate(s.orientation[b], {1, 0, 0}),
        rotate(s.orientation[b], {0, 1, 0}),
        rotate(s.orientation[b], {0, 0, 1})
    };

    Vec3 delta = s.position_m[b] - s.position_m[a];
    Vec3 best{};
    Real depth = MAX_INITIAL_DEPTH;

    for (int k = 0; k < SEPARATING_AXES_COUNT; ++k) {
        Vec3 axis{};

        if (k < 3) {
            axis = axesA[k];
        } else if (k < 6) {
            axis = axesB[k - 3];
        } else {
            axis = cross(axesA[(k - 6) / 3], axesB[(k - 6) % 3]);
        }

        Real n = length(axis);

        if (n < settings::Collisions::Geometry::EPSILON) {
            continue;
        }

        axis = axis / n;
        Real overlap = projectRadius(s, a, axis) + projectRadius(s, b, axis) - absolute(dot(delta, axis));

        if (overlap < -c.contactSlop_m) {
            return false;
        }

        if (overlap < depth) {
            depth = overlap;
            best = dot(delta, axis) >= 0 ? axis : -axis;
        }
    }

    out = {};
    out.a_i = a;
    out.b_i = b;
    out.normal = best;
    out.depth_m = depth;
    out.point_m = (supportCentroid(s, a, best) + supportCentroid(s, b, -best)) * MIDPOINT_SCALE;

    return true;
}


/**
 * Generate one representative contact for any supported shape pair.
 *
 * Sphere pairs use analytic tests; mixed pairs route through sphere/box logic
 * and box pairs use the OBB separating-axis test. For mixed pairs, the query
 * runs with the sphere first before reversing the resulting normal. Contact
 * orientation is always from body a toward body b.
 *
 * @param s    Shared body-state view.
 * @param a    First body index.
 * @param b    Second body index.
 * @param dt_s Fixed step duration used by speculative/CCD sphere contacts.
 * @param c    Physics configuration.
 * @param out  Receives the generated contact.
 * @return True when the pair overlaps or requires speculative contact.
 */
SIM_HD inline bool pairContact(StateView s, int a, int b, Real dt_s, const WorldConfig& c, Contact& out) {
    bool sa = s.shape_i[a] == static_cast<int>(Shape::Sphere);
    bool sb = s.shape_i[b] == static_cast<int>(Shape::Sphere);

    if (sa && sb) { return sphereSphere(s, a, b, dt_s, c, out); }
    if (sa) { return sphereBox(s, a, b, c, out); }
    if (!sb) { return boxBox(s, a, b, c, out); }
    if (!sphereBox(s, b, a, c, out)) { return false; }

    out.a_i = a;
    out.b_i = b;
    out.normal = -out.normal;

    return true;
}

} // namespace sim
