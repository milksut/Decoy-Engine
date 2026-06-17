#include "Layers/App.h"
#include "Layers/Minecraft_layer.h"

int main()
{
    App::Config cfg;
    cfg.enable_vsync = false;
    App app(cfg);
    app.push_layer(std::make_unique<MinecraftLayer>(app));
    app.run();
    return 0;
}