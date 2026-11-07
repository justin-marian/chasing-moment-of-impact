/**
 * @file
 * Shared deterministic host/device manifolds operations.
 *
 * Inline definitions are required to allow CUDA device compilation while
 * sharing the exact same implementation logic with the host CPU.
 */

#pragma once

#include "settings/collisions/constants.h"
#include "shared/sim/contacts.h"


namespace sim {

/**
 * Clip a convex contact polygon against one reference-face half-space.
 *
 * Input and output buffers must not alias; callers alternate scratch buffers
 * between successive clipping planes.
 *
 * @param input   Source polygon vertices.
 * @param count   Number of valid input vertices.
 * @param output  Destination scratch buffer.
 * @param axis    Outward clipping-plane direction.
 * @param center  Reference-face center.
 * @param limit_m Allowed projected distance from center in metres.
 * @return Number of vertices written to output.
 */
SIM_HD inline int clipPolygon(
    const Vec3* input, 
    int count, 
    Vec3* output, 
    Vec3 axis, 
    Vec3 center, 
    Real limit_m
) {
    const Real EPSILON = settings::Collisions::Geometry::EPSILON;

    if (!count) return 0;

    int written = 0;
    
    // Start by looking at the edge connecting the last vertex to the first.
    Vec3 previous = input[count - 1];
    Real before = dot(previous - center, axis) - limit_m;

    for (int i = 0; i < count; ++i) {
        Vec3 current = input[i];
        Real after = dot(current - center, axis) - limit_m;
        
        // Check which side of the clipping plane each vertex lies on.
        bool previousInside = before <= EPSILON;
        bool currentInside = after <= EPSILON;
        
        // If the edge crosses the plane, compute the intersection point.
        if (previousInside != currentInside) {
            Real fraction = before / (before - after);
            output[written++] = previous + (current - previous) * fraction;
        }

        // If the current vertex is inside, keep it.
        if (currentInside) {
            output[written++] = current;
        }
        
        previous = current;
        before = after;
    }

    return written;
}


/**
 * Generate the contact manifold for one body pair.
 *
 * Incident/reference face clipping retains at most four representative
 * manifold points. Sphere involvement yields a single contact. Aligned OBB
 * faces are clipped to produce up to four representative points while
 * preserving the base normal.
 *
 * @param s     Shared body-state view.
 * @param a     First body index.
 * @param b     Second body index.
 * @param dt_s  Fixed step duration.
 * @param c     Physics configuration.
 * @param out   Storage for the generated manifold contacts.
 * @return Number of contacts written, or zero when the pair does not contact.
 */
SIM_HD inline int pairContacts(
    StateView s, 
    int a, 
    int b, 
    Real dt_s, 
    const WorldConfig& c, 
    Contact* out
) {
    // Tolerances for alignment and duplicate point rejection.
    constexpr Real OBB_ALIGNMENT_THRESHOLD = Real(.999);
    constexpr Real DUPLICATE_DISTANCE = Real(1e-5);
    constexpr Real MIDPOINT_SCALE = Real(.5);

    Contact base{};
    
    // Get the base single-point contact (handles spheres and initial overlap).
    if (!pairContact(s, a, b, dt_s, c, base)) return 0;

    out[0] = base;
    
    // Spheres only ever produce a single contact point, so we can return early.
    if (s.shape_i[a] == static_cast<int>(Shape::Sphere) || 
        s.shape_i[b] == static_cast<int>(Shape::Sphere)) {
        return 1;
    }

    // Extract the principal axes of both OBBs in world space.
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

    Real alignA = 0, alignB = 0;
    int indexA = 0, indexB = 0;

    // Find which axis of each body is most aligned with the collision normal.
    for (int k = 0; k < 3; ++k) {
        Real da = absolute(dot(axesA[k], base.normal));
        Real db = absolute(dot(axesB[k], base.normal));
        
        if (da > alignA) {
            alignA = da;
            indexA = k;
        }
        
        if (db > alignB) {
            alignB = db;
            indexB = k;
        }
    }

    // If the faces aren't aligned enough, fall back to the single point contact.
    if (maximum(alignA, alignB) < OBB_ALIGNMENT_THRESHOLD) return 1;

    // Choose the body with the most aligned face as the reference.
    bool referenceA = alignA >= alignB;
    int reference = referenceA ? a : b;
    int incident = referenceA ? b : a;
    int faceAxis = referenceA ? indexA : indexB;

    Vec3 normal = referenceA ? base.normal : -base.normal;
    Vec3* axes = referenceA ? axesA : axesB;
    Vec3* other = referenceA ? axesB : axesA;

    Vec3 h = s.halfExtents_m[reference];
    Vec3 ih = s.halfExtents_m[incident];
    
    // Center of the reference face.
    Vec3 center = s.position_m[reference] + normal * h[faceAxis];

    int incidentAxis = 0;
    Real alignment = 0;

    // Find the incident face on the other body (most anti-aligned with the normal).
    for (int k = 0; k < 3; ++k) {
        Real v = absolute(dot(other[k], normal));
        if (v > alignment) {
            alignment = v;
            incidentAxis = k;
        }
    }

    Real sign = dot(other[incidentAxis], normal) > 0 ? Real(-1) : Real(1);
    
    // Center of the incident face.
    Vec3 incidentCenter = s.position_m[incident] + other[incidentAxis] * (ih[incidentAxis] * sign);

    int iu = (incidentAxis + 1) % 3, iv = (incidentAxis + 2) % 3;
    
    // Basis vectors for the incident face.
    Vec3 u = other[iu] * ih[iu];
    Vec3 v = other[iv] * ih[iv];
    
    int ru = (faceAxis + 1) % 3, rv = (faceAxis + 2) % 3, count = 4;

    Vec3 second[12];
    
    // Initial incident polygon (4 corners).
    Vec3 first[12] = {
        incidentCenter - u - v, 
        incidentCenter + u - v, 
        incidentCenter + u + v, 
        incidentCenter - u + v
    };

    // Clip the incident polygon against the four side planes of the reference face.
    count = clipPolygon(first, count, second, axes[ru], center, h[ru]);
    count = clipPolygon(second, count, first, -axes[ru], center, h[ru]);
    count = clipPolygon(first, count, second, axes[rv], center, h[rv]);
    count = clipPolygon(second, count, first, -axes[rv], center, h[rv]);

    int written = 0;
    
    // Limit to max 4 manifold points for stability and performance.
    int samples = count > 4 ? 4 : count;

    for (int k = 0; k < samples; ++k) {
        Vec3 point = first[k * count / samples];
        Real depth = dot(center - point, normal);
        
        // Ignore points that are separated by more than the allowed slop.
        if (depth < -c.contactSlop_m) continue;

        bool duplicate = false;
        
        // Ensure the new point isn't too close to an existing one.
        for (int j = 0; j < written; ++j) {
            if (length(out[j].point_m - (point + normal * (depth * MIDPOINT_SCALE))) < DUPLICATE_DISTANCE) {
                duplicate = true;
            }
        }

        if (duplicate) continue;

        Contact ct = base;
        ct.point_m = point + normal * (depth * MIDPOINT_SCALE);
        ct.depth_m = depth;
        out[written++] = ct;
    }

    return written ? written : 1;
}


/**
 * Generate a contact between one body support point and a static world-box face.
 *
 * Speculative contact may be emitted before penetration when CCD predicts the
 * body would cross the wall during the current step.
 *
 * @param s     Shared body-state view.
 * @param a     Body index.
 * @param face  Encoded wall face in axis*2 + side form.
 * @param dt_s  Fixed step duration.
 * @param c     Physics configuration.
 * @param out   Receives the wall contact.
 * @return True when the wall requires a contact constraint.
 */
SIM_HD inline bool wallContact(StateView s, int a, int face, Real dt_s, const WorldConfig& c, Contact& out) {
    // Decode face index into axis (0-2) and sign (+1 or -1).
    int axis = face / 2;
    Real sign = (face % 2) == 0 ? Real(-1) : Real(1);

    Vec3 normal{};
    normal[axis] = sign;

    // Calculate penetration depth against the wall.
    Real depth = sign * s.position_m[a][axis] + s.aabbExtent_m[a][axis] - c.boxHalfExtents_m[axis];
    Vec3 point = supportCentroid(s, a, normal);
    
    // Relative velocity along the wall normal.
    Real outward = dot(s.velocity_mps[a] + cross(s.angularVelocity_rad_s[a], point - s.position_m[a]), normal);

    // Skip if not penetrating and CCD doesn't predict a crossing this frame.
    if (depth < -c.contactSlop_m && (!c.enableCcd || depth + outward * dt_s < 0)) return false;

    out = {};
    out.a_i = a;
    out.b_i = -1;
    out.normal = normal;
    out.depth_m = depth;
    out.point_m = point;
    
    return true;
}


/**
 * Generate a wall contact from one OBB corner, or delegate sphere walls to wallContact.
 *
 * The CCD test includes a conservative rotational sweep margin so fast angular
 * motion cannot rotate a corner through the world boundary between steps.
 *
 * @param s      Shared body-state view.
 * @param a      Body index.
 * @param face   Encoded wall face.
 * @param corner OBB corner bit mask in the range [0, 7].
 * @param dt_s   Fixed step duration.
 * @param c      Physics configuration.
 * @param out    Receives the wall contact.
 * @return True when this vertex requires a wall constraint.
 */
SIM_HD inline bool wallVertexContact(
    StateView s, 
    int a, 
    int face, 
    int corner, 
    Real dt_s, 
    const WorldConfig& c, 
    Contact& out
) {
    // Spheres use the analytic wallContact path.
    if (s.shape_i[a] == static_cast<int>(Shape::Sphere)) {
        return corner == 0 && wallContact(s, a, face, dt_s, c, out);
    }

    int axis = face / 2;
    Real sign = face % 2 ? Real(1) : Real(-1);

    Vec3 normal{};
    normal[axis] = sign;
    Vec3 half = s.halfExtents_m[a];

    // Decode corner bitmask into local coordinates.
    Vec3 local{
        corner & 1 ? half.x : -half.x, 
        corner & 2 ? half.y : -half.y, 
        corner & 4 ? half.z : -half.z
    };

    Vec3 offset = rotate(s.orientation[a], local);
    Vec3 point = s.position_m[a] + offset;
    
    Real depth = sign * point[axis] - c.boxHalfExtents_m[axis];
    Real outward = dot(s.velocity_mps[a] + cross(s.angularVelocity_rad_s[a], offset), normal);

    // Second-order rotational sweep bound to prevent tunneling.
    Real rotationMargin = s.boundingRadius_m[a] * dot(s.angularVelocity_rad_s[a], s.angularVelocity_rad_s[a]) * dt_s * dt_s;
    
    // Absolute bound to ensure we don't miss a corner crossing.
    Real conservativeSweep = sign * s.position_m[a][axis] + s.boundingRadius_m[a] + length(s.velocity_mps[a]) * dt_s - c.boxHalfExtents_m[axis];

    // Skip if not penetrating and CCD doesn't predict a crossing.
    if (depth < -c.contactSlop_m && (!c.enableCcd || (depth + outward * dt_s + rotationMargin < 0 && conservativeSweep < 0))) {
        return false;
    }

    out = {};
    out.a_i = a;
    out.b_i = -1;
    out.normal = normal;
    out.point_m = point;
    out.depth_m = depth;
    
    return true;
}

} // namespace sim
