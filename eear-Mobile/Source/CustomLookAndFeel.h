/*
  ==============================================================================

    LookAndFeel.h
    Created: 2 Feb 2016 3:54:38pm
    Author:  Cárthach Ó Nuanáin

  ==============================================================================
*/

#ifndef LOOKANDFEEL_H_INCLUDED
#define LOOKANDFEEL_H_INCLUDED

struct CustomLookAndFeel    : public LookAndFeel_V3
{
        static juce::Colour back(){return  juce::Colour::fromRGB(21, 24, 27);};
        static juce::Colour text(){return  juce::Colour::fromRGB(119,195,214);};    
};




#endif  // LOOKANDFEEL_H_INCLUDED
