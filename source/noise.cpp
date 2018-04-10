#define NOISE_SEED 2271998

global const u8 noise_lookup[] = {
	51, 152, 157, 231, 235, 201, 241,
	242, 116, 200, 78, 83, 248, 221,
	71, 0, 181, 186, 150, 31, 143,
	61, 104, 219, 107, 247, 215, 179,
	188, 173, 183, 160, 243, 35, 155,
	154, 67, 54, 169, 177, 170, 58,
	251, 232, 38, 37, 120, 224, 191,
	230, 145, 103, 73, 218, 2, 167,
	227, 36, 147, 174, 87, 79, 29, 135,
	50, 193, 86, 182, 12, 184, 137,
	100, 6, 110, 156, 127, 96, 30, 244,
	90, 140, 252, 69, 151, 208, 212,
	21, 93, 164, 74, 168, 125, 144,
	238, 112, 176, 60, 222, 45, 41,
	214, 199, 209, 34, 53, 178, 72,
	126, 131, 85, 98, 239, 46, 24, 17,
	197, 115, 20, 249, 205, 129, 185,
	187, 210, 119, 237, 172, 32, 44,
	105, 109, 27, 180, 236, 42, 7, 5,
	149, 25, 82, 33, 26, 111, 62, 130,
	228, 77, 97, 66, 217, 153, 146, 245,
	4, 114, 138, 158, 11, 196, 52, 171,
	39, 211, 113, 133, 159, 204, 128,
	23, 233, 229, 134, 99, 195, 190,
	68, 16, 220, 56, 63, 223, 162, 43,
	57, 234, 65, 225, 49, 250, 166, 1,
	206, 226, 165, 121, 124, 84, 253,
	81, 40, 47, 9, 88, 139, 80, 8, 28,
	64, 95, 192, 70, 207, 202, 136, 55,
	216, 189, 13, 75, 15, 102, 161, 148,
	142, 117, 141, 18, 76, 254, 19, 14,
	89, 91, 3, 101, 163, 213, 92, 255,
	203, 132, 194, 22, 198, 94, 59, 175,
	240, 246, 118, 123, 106, 10, 48, 122
};

i32 noise2(i32 x, i32 y) {
    i32 temp = noise_lookup[(y + NOISE_SEED) % 256];
    return noise_lookup[(temp + x) % 256];
}

r32 linear_interpolate(r32 x, r32 y, r32 s) {
    return x + s * (y-x);
}

r32 smooth_interpolate(r32 x, r32 y, r32 s) {
    return linear_interpolate(x, y, s * s * (3-2*s));
}

r32 noise2d(r32 x, r32 y) {
    i32 x_i32 = x,
        y_i32 = y;

    r32 x_frac = x - x_i32,
        y_frac = y - y_i32;

    i32 s = noise2(x_i32, y_i32),
        t = noise2(x_i32+1, y_i32),
        u = noise2(x_i32, y_i32+1),
        v = noise2(x_i32+1, y_i32+1),
        low = smooth_interpolate(s, t, x_frac),
        high = smooth_interpolate(u, v, x_frac);

    return smooth_interpolate(low, high, y_frac);
}

r32 perlin_2d(r32 x, r32 y, r32 freq, i32 depth) {
    r32 xa = x * freq,
        ya = y * freq,
        amp = 1.0,
        fin = 0,
        div = 0.0;

    for(i32 i = 0; i < depth; ++i) {
        div += 256 * amp;
        fin += noise2d(xa, ya) * amp;
        amp /= 2;
        xa *= 2;
        ya *= 2;
    }

    return fin/div;
}
