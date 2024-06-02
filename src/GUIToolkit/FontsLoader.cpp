#include "FontsLoader.h"

#include "GUIData.h"

FontsLoader::FontsLoader()
{
    monospaceFont =
        juce::Typeface::createSystemTypefaceFor(GUIData::FragmentMonoRegular_ttf, GUIData::FragmentMonoRegular_ttfSize);
    roboto = juce::Typeface::createSystemTypefaceFor(GUIData::RobotoRegular_ttf, GUIData::RobotoRegular_ttfSize);
    robotoBold = juce::Typeface::createSystemTypefaceFor(GUIData::RobotoBold_ttf, GUIData::RobotoBold_ttfSize);
}