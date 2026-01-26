//
// Created by Santiago Fuentes on 1/26/26.
//

#ifndef PROJECTION_3D_2D_QUATERNION_H
#define PROJECTION_3D_2D_QUATERNION_H

namespace math {
    struct Quat {
        float w, x, y, z;
    };
    Quat multiply(Quat q1, Quat q2);
}

#endif //PROJECTION_3D_2D_QUATERNION_H