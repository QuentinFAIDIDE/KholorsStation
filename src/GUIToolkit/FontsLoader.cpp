#include "FontsLoader.h"

#include "GUIData.h"

FontsLoader::FontsLoader()
{
    monospaceFont =
        juce::Typeface::createSystemTypefaceFor(GUIData::FragmentMonoRegular_ttf, GUIData::FragmentMonoRegular_ttfSize);
    roboto = juce::Typeface::createSystemTypefaceFor(GUIData::RobotoRegular_ttf, GUIData::RobotoRegular_ttfSize);
    robotoMedium = juce::Typeface::createSystemTypefaceFor(GUIData::RobotoMedium_ttf, GUIData::RobotoMedium_ttfSize);
    robotoBold = juce::Typeface::createSystemTypefaceFor(GUIData::RobotoBold_ttf, GUIData::RobotoBold_ttfSize);
    robotoBlack = juce::Typeface::createSystemTypefaceFor(GUIData::RobotoBlack_ttf, GUIData::RobotoBlack_ttfSize);
    dimmedLabelFont = monospaceFont.withHeight(DIMMED_LABEL_FONT_HEIGHT).withExtraKerningFactor(DIMMED_LABEL_KERNING);
}