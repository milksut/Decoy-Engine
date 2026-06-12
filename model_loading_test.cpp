#include "App.h"
#include "GameLayer.h"

int main()
{
    App::Config cfg;
    cfg.enable_vsync = false;

    // cfg.window_config.width  = 1920;
    // cfg.window_config.height = 1080;
    // cfg.window_config.title  = "Oyun";

    App app(cfg);

    GameLayer* game = new GameLayer(&app);
    game->grid_amount = 25;

    app.push_layer(game);

    app.run();

    return 0;
}