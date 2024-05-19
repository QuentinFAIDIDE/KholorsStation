#include "FontsLoader.h"

#include "FragmentMonoFont.h"
#include "RobotoFont.h"

FontsLoader::FontsLoader()
{
    monospaceFont = juce::Typeface::createSystemTypefaceFor(FragmentMonoFont::FragmentMonoRegular_ttf,
                                                            FragmentMonoFont::FragmentMonoRegular_ttfSize);
    roboto = juce::Typeface::createSystemTypefaceFor(RobotoFont::RobotoRegular_ttf, RobotoFont::RobotoRegular_ttfSize);
    robotoBold = juce::Typeface::createSystemTypefaceFor(RobotoFont::RobotoBold_ttf, RobotoFont::RobotoBold_ttfSize);
}