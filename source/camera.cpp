struct Camera {
    v3 pos,
    orientation,             // NOTE(Ryan):  X, Y, Z -> Yaw, Pitch, Roll (respectively)
    target_orientation;
    r32 interpolation_rate;
};

void update_camera(Camera *c) {
    c->orientation += (c->target_orientation - c->orientation) * c->interpolation_rate * delta_t;
}

void set_view_with_camera(Camera *c) {
    v3 target = c->pos + v3(
        cos(c->orientation.x),
        sin(c->orientation.y),
        sin(c->orientation.x)
        );
    view = m4_lookat(c->pos, target);
}