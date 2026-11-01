#include "VolumeScale.h"
#include "GUIToolkit/Consts.h"
#include "StationApp/GUI/Graphs/ChannelBadge.h"
#include "StationApp/GUI/NormalizedUnitTransformer.h"
#include <limits>
#include <optional>
#include <vector>

VolumeScale::VolumeScale()
{
    setOpaque(true);
}

VolumeScale::~VolumeScale() = default;

void VolumeScale::paint(juce::Graphics &g)
{
    // fill background
    g.fillAll(KHOLORS_COLOR_CARD_BACKGROUND);

    drawChannelBadges(g, getLocalBounds(), sharedFonts->monospaceFont);
}
