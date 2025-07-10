int main() {
    SceneRenderer app;
    app.init();

    gbg::TextureData tex = gbg::loadTexture(TEXTURE_PATH);

    gbg::Model mod =
        gbg::loadModel("../../data/models/pony-cartoon/Pony_cartoon.obj");

    app.addTexture(tex);
    app.addModel(mod);

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
