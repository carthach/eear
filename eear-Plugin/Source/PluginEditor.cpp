/*
  ==============================================================================

    This file was auto-generated by the Jucer!

    It contains the basic startup code for a Juce application.

  ==============================================================================
*/

#include "PluginEditor.h"

//==============================================================================
TheEarPluginAudioProcessorEditor::TheEarPluginAudioProcessorEditor (TheEarPluginAudioProcessor& owner)
    : AudioProcessorEditor (owner),
    mainTab(owner.keyboardState, processor)


{
    Colour textColour(135, 205, 222);
    lookAndFeel.setColour(Label::ColourIds::textColourId, textColour);
    lookAndFeel.setColour(ToggleButton::ColourIds::textColourId, textColour);
    setLookAndFeel(&lookAndFeel);

    
    addAndMakeVisible(mainTab);
    setSize(800, 700);
}

TheEarPluginAudioProcessorEditor::~TheEarPluginAudioProcessorEditor()
{
}


//==============================================================================
void TheEarPluginAudioProcessorEditor::paint (Graphics& g)
{
//    g.setGradientFill (ColourGradient (Colours::white, 0, 0,
//                                       Colours::grey, 0, (float) getHeight(), false));
    
    Colour backgroundColour(28, 31, 36);
    g.fillAll(backgroundColour);
}

void TheEarPluginAudioProcessorEditor::resized()
{


    mainTab.setBounds(getLocalBounds());
    
}


