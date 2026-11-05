#include "app.h"

huxerui::View App() { return qrcode_demo::PreviewApp(); }

const huxerui::Application application{
    App,
    {.window = {.title = "Lib-QRCode Preview", .initial_size = {1100.0F, 840.0F}}},
};
