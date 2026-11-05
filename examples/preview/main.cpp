#include "src/preview.h"

huxerui::View App() { return qrcode_demo::PreviewApp(); }

const huxerui::Application application{
    App,
    {.window = {.title = "HuxerUI QRCode Preview", .initial_size = {1100, 840}}},
};

int main() { return huxerui::RunApplication(); }
