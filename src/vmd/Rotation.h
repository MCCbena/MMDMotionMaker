
//
// Created by server on 24/12/23.
//

#ifndef TEST_ROTATION_H
#define TEST_ROTATION_H
#include <math.h>
#define deg_to_rad(deg) ((deg)*M_PI/180)

struct Quaternion{
    double w;
    double x;
    double y;
    double z;
};

struct Euler{
    double x;
    double y;
    double z;
};

struct Matrix{
    double value[3][3];
};

double qdot(struct Quaternion q1, struct Quaternion q2){
    return q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
}

//クォータニオンを逆クォータニオンにする
struct Quaternion inverse(struct Quaternion q){
    struct Quaternion quaternion;
    quaternion.x = -q.x;
    quaternion.y = -q.y;
    quaternion.z = -q.z;
    quaternion.w = q.w;

    return q;
}

struct Quaternion mul(struct Quaternion q, double f) {
    struct Quaternion quaternion;
    quaternion.x = f * q.x;
    quaternion.y = f * q.y;
    quaternion.z = f * q.z;
    quaternion.w = f * q.w;

    return quaternion;
}

struct Quaternion qmul(struct Quaternion q1, struct Quaternion q2){
    struct Quaternion q;
    q.x = q2.w * q1.x - q2.z * q1.y + q2.y * q1.z + q2.x * q1.w;
    q.y = q2.z * q1.x + q2.w * q1.y - q2.x * q1.z + q2.y * q1.w;
    q.z = -q2.y * q1.x + q2.x * q1.y + q2.w * q1.z + q2.z * q1.w;
    q.w = -q2.x * q1.x - q2.y * q1.y - q2.z * q1.z + q2.w * q1.w;

    return q;
}


struct Quaternion add(struct Quaternion q1, struct Quaternion q2) {
    struct Quaternion quaternion;
    quaternion.x = q1.x + q2.x;
    quaternion.y = q1.y + q2.y;
    quaternion.z = q1.z + q2.z;
    quaternion.w = q1.w + q2.w;

    return quaternion;
}

struct Quaternion SphericalLinearInterpolation(struct Quaternion q1, struct Quaternion q2, const double t){
    double dot = qdot(q1, q2);
    if(dot < 0){
        q2 = mul(q2, -1);
        dot = qdot(q1, q2);
    }
    if(dot > 1) dot = 1;
    if(dot < -1) dot = -1;
    double r = acos(dot);
    if(r==0){
        return q1;
    }
    double is = 1.0/sin(r);

    if(t==0.0f){
        return q1;
    }else if(t==1.0f){
        return q2;
    }
    return add(
            mul(q1, sin((1.0-t) * r) * is),
            mul(q2, sin(t * r) * is)
    );
}
struct Quaternion LinearInterpolation(struct Quaternion q1, struct Quaternion q2, const double t) {
    return add(
            mul(q1, (1-t)),
            mul(q2, t)
            );
}

//TODO ジンバルロックの対応ができていない
//回転順序はYXZ
struct Euler QuaternionToEuler(double qw, double qx, double qy, double qz){
    struct Euler euler;
    euler.x = asin(-(2*qy*qz-2*qx*qw))*180/M_PI;
    if(cos(euler.x)==0.0f){
        euler.y = atan(-(2*qx*qz+2*qy*qw)/(2*qw*qw+2*qx*qx-1))*180/M_PI;
        euler.z = 0;
    } else{
        euler.z = atan((2*qx*qy+2*qz*qw)/(2*qw*qw+2*qy*qy-1))*180/M_PI;
        euler.y = atan((2*qx*qz+2*qy*qw)/(2*qw*qw+2*qz*qz-1))*180/M_PI;
    }

    return euler;
}

struct Matrix QuaternionToMatrix(struct Quaternion q){
    struct Matrix matrix;
    double qx, qy, qz, qw;
    qx = q.x;
    qy = q.y;
    qz = q.z;
    qw = q.w;

    matrix.value[0][0] = 2*pow(qw,2) + 2* pow(qx,2)-1;
    matrix.value[0][1] = 2*qx*qy - 2*qz*qw;
    matrix.value[0][2] = 2*qx*qz + 2*qy*qw;

    matrix.value[1][0] = 2*qx*qy + 2*qz*qw;
    matrix.value[1][1] = 2*pow(qw,2) + 2*pow(qy, 2)-1;
    matrix.value[1][2] = 2*qy*qz - 2*qx*qw;

    matrix.value[2][0] = 2*qx*qz - 2*qy*qw;
    matrix.value[2][1] = 2*qy*qz + 2*qx*qw;
    matrix.value[2][2] = 2*pow(qw,2) + 2*pow(qz, 2)-1;

    return matrix;
}

#endif //TEST_ROTATION_H
