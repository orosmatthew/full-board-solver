#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#endif

#include "app.hpp"

static std::unique_ptr<App> g_app;

[[maybe_unused]] static void update()
{
    g_app->update();
}

int main()
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    g_app = std::make_unique<App>();

#ifdef PLATFORM_WEB
    emscripten_set_main_loop(update, 0, 1);
#else
    while (!g_app->should_close()) {
        g_app->update();
    }
#endif

    return EXIT_SUCCESS;
}