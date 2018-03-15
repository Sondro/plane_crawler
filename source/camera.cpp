struct Camera {
    v3 pos,
       orientation,             // @Note (Ryan) X, Y, Z -> Yaw, Pitch, Roll (respectively)
       target_orientation;
    r32 interpolation_rate;
};

void update_camera(Camera *c) {
    c->orientation += (c->target_orientation - c->orientation) * c->interpolation_rate;
}
