/*
 ==============================================================================
 
 This file was auto-generated!
 
 ==============================================================================
 */

#ifndef MAINCOMPONENT_H_INCLUDED
#define MAINCOMPONENT_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include "AudioRecordingDemo.cpp"
#include "SettingsComponent.h"
#include "Common.h"
#include "CustomLookAndFeel.h"

//==============================================================================
class MainTab  : public TabbedComponent
{
public:
    MainTab (AudioDeviceManager& deviceManager)
    : TabbedComponent (TabbedButtonBar::TabsAtTop)
    {
        setTabBarDepth(60);
        
        addTab ("Interface",  eear::Colour::back(), new AudioRecordingDemo(&deviceManager), true);
        addTab ("Settings",  eear::Colour::back(), new SettingsComponent((AudioRecordingDemo *)getTabContentComponent(0)), true);        
    }
};

//==============================================================================
/*
 This component lives inside our window, and this is where you should put all
 your controls and content.
 */
class MainContentComponent   : public AudioAppComponent
{
public:
    //==============================================================================
    MainContentComponent() : mainTab(deviceManager)
    {
        // specify the number of input and output channels that we want to open
        
        //This needs to be MONO for Essentia input
        setAudioChannels (1, 2);
        
        juce::AudioDeviceManager::AudioDeviceSetup setup;
        deviceManager.getAudioDeviceSetup(setup);
        
        setup.bufferSize = 1024;
        deviceManager.setAudioDeviceSetup(setup, true);
        //        deviceManager.in
        
        addAndMakeVisible(mainTab);
        
        LookAndFeel::setDefaultLookAndFeel (&lookAndFeel);

        lookAndFeel.setColour(Label::ColourIds::textColourId, Colours::white);
        lookAndFeel.setColour(TextEditor::ColourIds::backgroundColourId, Colours::grey);
        lookAndFeel.setColour(Slider::ColourIds::textBoxBackgroundColourId, Colours::grey);
//        lookAndFeel.setColour(Label::ColourIds::textColourId, juce::Colour::fromRGB(119,195,214));
    }
    
    ~MainContentComponent()
    {
        shutdownAudio();
    }
    
    //=======================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override
    {
        // This function will be called when the audio device is started, or when
        // its settings (i.e. sample rate, block size, etc) are changed.
        
        // You can use this function to initialise any resources you might need,
        // but be careful - it will be called on the audio thread, not the GUI thread.
        
        // For more details, see the help for AudioProcessor::prepareToPlay()
    }
    
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
        // Your audio-processing code goes here!
        
        // For more details, see the help for AudioProcessor::getNextAudioBlock()
        
        // Right now we are not producing any data, in which case we need to clear the buffer
        // (to prevent the output of random noise)
        bufferToFill.clearActiveBufferRegion();
    }
    
    void releaseResources() override
    {
        // This will be called when the audio device stops, or when it is being
        // restarted due to a setting change.
        
        // For more details, see the help for AudioProcessor::releaseResources()
    }
    
    //=======================================================================
    void paint (Graphics& g) override
    {
        // (Our component is opaque, so we must completely fill the background with a solid colour)
        g.fillAll(Colours::black);
        
        
        
        
        // You can add your drawing code here!
    }
    
    void resized() override
    {
        //Just one sub component
        mainTab.setBounds(getLocalBounds());
    }
    
    
private:
    //==============================================================================
    
    // Your private member variables go here...
    //    AudioRecordingDemo recorder;
    
    MainTab mainTab;
    CustomLookAndFeel lookAndFeel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};



// (This function is called by the app startup code to create our main component)
Component* createMainContentComponent()     { return new MainContentComponent(); }


#endif  // MAINCOMPONENT_H_INCLUDED
