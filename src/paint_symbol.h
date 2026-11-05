#pragma once

#include <huxerui/qrcode.h>
#include <huxerui/paint.h>

namespace huxerui::qrcode::detail {

void PaintSymbol(PaintContext& paint, Size size, const Symbol& symbol, Color foreground, Color background);

} // namespace huxerui::qrcode::detail
