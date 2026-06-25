#include "Layers/App.h"
#include "Layers/gurard_lyer.h"

int main()
{
    //App::Config cfg;
    //cfg.enable_vsync = false;
    //App app(cfg);
    //app.push_layer(std::make_unique<MinecraftLayer>(app));
    //app.run();

    Guard_demo_layer game_layer;
    while (true)
    {
        game_layer.Update();
        game_layer.Render();
    }
    return 0;
}