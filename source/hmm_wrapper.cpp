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
