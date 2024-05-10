#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#endif

#include "app.hpp"

static std::unique_ptr<App> g_app;

[[maybe_unused]] static void update()
{
    g_app->update_and_draw();
}

int main()
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    SetTargetFPS(60);
    g_app = std::make_unique<App>();

#ifdef PLATFORM_WEB
    emscripten_set_main_loop(update, 0, 1);
#else
    while (!g_app->should_close()) {
        update();
    }
#endif

    return EXIT_SUCCESS;
}