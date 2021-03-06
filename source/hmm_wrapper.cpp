typedef hmm_v2 v2;
typedef hmm_v3 v3;
typedef hmm_v4 v4;
typedef hmm_m4 m4;

#define v2(x, y)        HMM_Vec2(x, y)
#define v3(x, y, z)     HMM_Vec3(x, y, z)
#define v4(x, y, z, w)  HMM_Vec4(x, y, z, w)

#define m4d(d)               HMM_Mat4d(d)
#define m4_translate(m, v)   HMM_Multiply(m, HMM_Translate(v))
#define m4_scale(m, v)       HMM_Multiply(m, HMM_Scale(v))
#define m4_rotate(m, a)      HMM_Multiply(m, a, v3(0, 0, 1))
#define m4_lookat(e, t)      HMM_LookAt(e, t, v3(0, 1, 0))

m4 m4_inverse(m4 m) {
    r32 coef00 = m.Elements[2][2] * m.Elements[3][3] - m.Elements[3][2] * m.Elements[2][3];
    r32 coef02 = m.Elements[1][2] * m.Elements[3][3] - m.Elements[3][2] * m.Elements[1][3];
    r32 coef03 = m.Elements[1][2] * m.Elements[2][3] - m.Elements[2][2] * m.Elements[1][3];
    
    r32 coef04 = m.Elements[2][1] * m.Elements[3][3] - m.Elements[3][1] * m.Elements[2][3];
    r32 coef06 = m.Elements[1][1] * m.Elements[3][3] - m.Elements[3][1] * m.Elements[1][3];
    r32 coef07 = m.Elements[1][1] * m.Elements[2][3] - m.Elements[2][1] * m.Elements[1][3];
    
    r32 coef08 = m.Elements[2][1] * m.Elements[3][2] - m.Elements[3][1] * m.Elements[2][2];
    r32 coef10 = m.Elements[1][1] * m.Elements[3][2] - m.Elements[3][1] * m.Elements[1][2];
    r32 coef11 = m.Elements[1][1] * m.Elements[2][2] - m.Elements[2][1] * m.Elements[1][2];
    
    r32 coef12 = m.Elements[2][0] * m.Elements[3][3] - m.Elements[3][0] * m.Elements[2][3];
    r32 coef14 = m.Elements[1][0] * m.Elements[3][3] - m.Elements[3][0] * m.Elements[1][3];
    r32 coef15 = m.Elements[1][0] * m.Elements[2][3] - m.Elements[2][0] * m.Elements[1][3];
    
    r32 coef16 = m.Elements[2][0] * m.Elements[3][2] - m.Elements[3][0] * m.Elements[2][2];
    r32 coef18 = m.Elements[1][0] * m.Elements[3][2] - m.Elements[3][0] * m.Elements[1][2];
    r32 coef19 = m.Elements[1][0] * m.Elements[2][2] - m.Elements[2][0] * m.Elements[1][2];
    
    r32 coef20 = m.Elements[2][0] * m.Elements[3][1] - m.Elements[3][0] * m.Elements[2][1];
    r32 coef22 = m.Elements[1][0] * m.Elements[3][1] - m.Elements[3][0] * m.Elements[1][1];
    r32 coef23 = m.Elements[1][0] * m.Elements[2][1] - m.Elements[2][0] * m.Elements[1][1];
    
    v4 fac0 = v4(coef00, coef00, coef02, coef03);
    v4 fac1 = v4(coef04, coef04, coef06, coef07);
    v4 fac2 = v4(coef08, coef08, coef10, coef11);
    v4 fac3 = v4(coef12, coef12, coef14, coef15);
    v4 fac4 = v4(coef16, coef16, coef18, coef19);
    v4 fac5 = v4(coef20, coef20, coef22, coef23);
    
    v4 vec0 = v4(m.Elements[1][0], m.Elements[0][0], m.Elements[0][0], m.Elements[0][0]);
    v4 vec1 = v4(m.Elements[1][1], m.Elements[0][1], m.Elements[0][1], m.Elements[0][1]);
    v4 vec2 = v4(m.Elements[1][2], m.Elements[0][2], m.Elements[0][2], m.Elements[0][2]);
    v4 vec3 = v4(m.Elements[1][3], m.Elements[0][3], m.Elements[0][3], m.Elements[0][3]);
    
    v4 inv0 = vec1 * fac0 - vec2 * fac1 + vec3 * fac2;
    v4 inv1 = vec0 * fac0 - vec2 * fac3 + vec3 * fac4;
    v4 inv2 = vec0 * fac1 - vec1 * fac3 + vec3 * fac5;
    v4 inv3 = vec0 * fac2 - vec1 * fac4 + vec2 * fac5;
    
    v4 sign_a = v4(+1, -1, +1, -1);
    v4 sign_b = v4(-1, +1, -1, +1);
    
    m4 inverse;
    foreach8(i, 4) {
        inverse.Elements[0][i] = inv0.Elements[i] * sign_a.Elements[i];
    }
    foreach8(i, 4) {
        inverse.Elements[1][i] = inv1.Elements[i] * sign_b.Elements[i];
    }
    foreach8(i, 4) {
        inverse.Elements[2][i] = inv2.Elements[i] * sign_a.Elements[i];
    }
    foreach8(i, 4) {
        inverse.Elements[3][i] = inv3.Elements[i] * sign_b.Elements[i];
    }
    
    v4 row0 = v4(inverse.Elements[0][0], inverse.Elements[1][0], inverse.Elements[2][0], inverse.Elements[3][0]);
    v4 m0 = v4(m.Elements[0][0], m.Elements[0][1], m.Elements[0][2], m.Elements[0][3]);
    v4 dot0 = (m0 * row0);
    r32 dot1 = (dot0.x + dot0.y) + (dot0.z + dot0.w);
    
    r32 one_over_det = 1 / dot1;
    
    return inverse * one_over_det;
}

r32 distance2_32(v2 a, v2 b) {
    return HMM_LengthSquaredVec2(a - b);
}

#define length(v) HMM_LengthVec2(v)