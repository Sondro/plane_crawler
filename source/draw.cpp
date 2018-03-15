Texture small_font, tiles;

void init_draw() {
    small_font = load_texture("font");
    tiles = load_texture("tiles");
}

void clean_up_draw() {
    clean_up_texture(&small_font);
    clean_up_texture(&tiles);
}
