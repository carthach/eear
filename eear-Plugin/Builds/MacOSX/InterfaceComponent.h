//
//  InterfaceComponent.h
//  eear-Plugin
//
//  Created by Cárthach Ó Nuanáin on 19/09/2015.
//
//

#ifndef eear_Plugin_InterfaceComponent_h
#define eear_Plugin_InterfaceComponent_h
#include "PluginProcessor.h"



class InterfaceComponent : public Component,
                            public SliderListener,
                            public Timer,
                            public Button::Listener,
                            public ChangeListener
{
public:
    MidiKeyboardComponent midiKeyboard;
    Label infoLabel, gainLabel, delayLabel;
    Slider gainSlider, delaySlider;
    ScopedPointer<ResizableCornerComponent> resizer;
    ComponentBoundsConstrainer resizeLimits;
    EarOSCServer earOSCServer;
    
    TextButton resetSynthButton;
    Label currentKeyScaleLabel, liveKeyScaleLabel;
    TextEditor currentKeyScaleTextBox, liveKeyScaleTextBox;
    
    PadGrid padGrid;
    
    AudioProcessor& processor;
    
    AudioPlayHead::CurrentPositionInfo lastDisplayedPosition;
    
    InterfaceComponent(MidiKeyboardState& s, AudioProcessor& p) :
    midiKeyboard (s, MidiKeyboardComponent::horizontalKeyboard),
    infoLabel (String::empty),
    gainLabel ("", "Throughput level:"),
    delayLabel ("", "Delay:"),
    gainSlider ("gain"),
    delaySlider ("delay"),
    earOSCServer(8000),
    padGrid(s),
    processor(p)
    {
        // add some sliders..
        addAndMakeVisible (gainSlider);
        gainSlider.setSliderStyle (Slider::Rotary);
        gainSlider.addListener (this);
        gainSlider.setRange (0.0, 1.0, 0.01);
        
        addAndMakeVisible (delaySlider);
        delaySlider.setSliderStyle (Slider::Rotary);
        delaySlider.addListener (this);
        delaySlider.setRange (0.0, 1.0, 0.01);
        
        // add some labels for the sliders..
        gainLabel.attachToComponent (&gainSlider, false);
        gainLabel.setFont (Font (11.0f));
        
        delayLabel.attachToComponent (&delaySlider, false);
        delayLabel.setFont (Font (11.0f));
        
        // add the midi keyboard component..
        addAndMakeVisible (midiKeyboard);
        
        // add a label that will display the current timecode and status..
        addAndMakeVisible (infoLabel);
        infoLabel.setColour (Label::textColourId, Colours::blue);
        
        // add the triangular resizer component for the bottom-right of the UI
        addAndMakeVisible (resizer = new ResizableCornerComponent (this, &resizeLimits));
        resizeLimits.setSizeLimits (150, 150, 1024, 768);
        
        //Loop button
        addAndMakeVisible(resetSynthButton);
        resetSynthButton.setButtonText("Reset Loops");
        resetSynthButton.addListener(this);
        
        //Key Scale Label
        
        addAndMakeVisible (currentKeyScaleLabel);
        currentKeyScaleLabel.setText ("Current Key:", dontSendNotification);
        
        addAndMakeVisible (currentKeyScaleTextBox);
        currentKeyScaleTextBox.setReadOnly(true);
        currentKeyScaleTextBox.setColour(TextEditor::backgroundColourId, Colours::lightgrey);
        
        addAndMakeVisible (liveKeyScaleLabel);
        liveKeyScaleLabel.setText ("Live Key:", dontSendNotification);
        
        addAndMakeVisible (liveKeyScaleTextBox);
        liveKeyScaleTextBox.setReadOnly(true);
        liveKeyScaleTextBox.setColour(TextEditor::backgroundColourId, Colours::lightgrey);
        
        
        
        earOSCServer.addChangeListener(this);
        
        
        // set our component's initial size to be the last one that was stored in the filter's settings
        //    setSize (owner.lastUIWidth,
        //             owner.lastUIHeight);
        
        
        startTimer (50);
        
        addAndMakeVisible(padGrid);
        
        setSize(1024, 768);
    }
    
    //==============================================================================
    TheEarPluginAudioProcessor& getProcessor() const
    {
        return static_cast<TheEarPluginAudioProcessor&> (processor);
    }
    
    void buttonClicked(Button* button)
    {
        if(button == &resetSynthButton) {
            String path = "/Users/carthach/Dev/git/GiantSteps/eear/eear-demo-mhd-pd/MHD-short-loops";
            
            //        if(String(earOSCServer.key) != "") {
            //            Array<File> matchingFiles = getAudioFiles(File(path), earOSCServer.key, earOSCServer.scale);
            //            if(matchingFiles.size() > 0)
            //                getProcessor().setSynthSamples(matchingFiles);
            //        }
            getProcessor().setSynthSamples(getAudioFiles(File(path), "E", "minor"));
        }
    }
    
    // This timer periodically checks whether any of the filter's parameters have changed...
    void timerCallback()
    {
        TheEarPluginAudioProcessor& ourProcessor = getProcessor();
        
        AudioPlayHead::CurrentPositionInfo newPos (ourProcessor.lastPosInfo);
        
        if (lastDisplayedPosition != newPos)
            displayPositionInfo (newPos);
        
        gainSlider.setValue (ourProcessor.gain->getValue(), dontSendNotification);
        delaySlider.setValue (ourProcessor.delay->getValue(), dontSendNotification);
    }
    
    Array<File> getAudioFiles(const File& audioFolder, String key, String scale)
    {
        Array<File> audioFiles;
        
        String dirtyPattern = "*_*-*_" + key + "-" + scale + "_128_*.wav";
        
        DirectoryIterator iter (audioFolder, false, dirtyPattern);
        
        while (iter.next())
        {
            File theFileItFound (iter.getFile());
            audioFiles.add(theFileItFound);
            
            std::cout << theFileItFound.getFileName() << "\n";
        }
        
        return audioFiles;
    }
    
    void changeListenerCallback (ChangeBroadcaster *source)
    {
        String keyScaleString = String(earOSCServer.key) + " " + String(earOSCServer.scale);
        currentKeyScaleTextBox.setText(keyScaleString);
    }
    
    // This is our Slider::Listener callback, when the user drags a slider.
    void sliderValueChanged (Slider* slider)
    {
        if (AudioProcessorParameter* param = getParameterFromSlider (slider))
        {
            // It's vital to use setValueNotifyingHost to change any parameters that are automatable
            // by the host, rather than just modifying them directly, otherwise the host won't know
            // that they've changed.
            param->setValueNotifyingHost ((float) slider->getValue());
        }
    }
    
    void sliderDragStarted (Slider* slider)
    {
        if (AudioProcessorParameter* param = getParameterFromSlider (slider))
        {
            param->beginChangeGesture();
        }
    }
    
    void sliderDragEnded (Slider* slider)
    {
        if (AudioProcessorParameter* param = getParameterFromSlider (slider))
        {
            param->endChangeGesture();
        }    
    }
    
    AudioProcessorParameter* getParameterFromSlider (const Slider* slider) const
    {
        if (slider == &gainSlider)
            return getProcessor().gain;
        
        if (slider == &delaySlider)
            return getProcessor().delay;
        
        return nullptr;
    }
    
    //==============================================================================
    void paint (Graphics& g)
    {
        g.setGradientFill (ColourGradient (Colours::white, 0, 0,
                                           Colours::grey, 0, (float) getHeight(), false));
        
        g.fillAll();
    }
    
    void resized()
    {
        infoLabel.setBounds (10, 4, 400, 25);
        gainSlider.setBounds (20, 60, 150, 40);
        delaySlider.setBounds (200, 60, 150, 40);
        
        resetSynthButton.setBounds(20, 100, 100, 25);
        
        currentKeyScaleLabel.setBounds(20, 140, 100, 20);
        currentKeyScaleTextBox.setBounds(100, 140, 100, 20);
        
        liveKeyScaleLabel.setBounds(200, 140, 100, 20);
        liveKeyScaleTextBox.setBounds(300, 140, 100, 20);
        
        padGrid.setBounds(20, 180, 640, 480);
        
        const int keyboardHeight = 70;
        midiKeyboard.setBounds (4, getHeight() - keyboardHeight - 4, getWidth() - 8, keyboardHeight);
        
        resizer->setBounds (getWidth() - 16, getHeight() - 16, 16, 16);
        
        //    getProcessor().lastUIWidth = getWidth();
        //    getProcessor().lastUIHeight = getHeight();
        
                
    }
    
    //==============================================================================
    // quick-and-dirty function to format a timecode string
    static String timeToTimecodeString (const double seconds)
    {
        const double absSecs = std::abs (seconds);
        
        const int hours =  (int) (absSecs / (60.0 * 60.0));
        const int mins  = ((int) (absSecs / 60.0)) % 60;
        const int secs  = ((int) absSecs) % 60;
        
        String s (seconds < 0 ? "-" : "");
        
        s << String (hours).paddedLeft ('0', 2) << ":"
        << String (mins) .paddedLeft ('0', 2) << ":"
        << String (secs) .paddedLeft ('0', 2) << ":"
        << String (roundToInt (absSecs * 1000) % 1000).paddedLeft ('0', 3);
        
        return s;
    }
    
    // quick-and-dirty function to format a bars/beats string
    static String ppqToBarsBeatsString (double ppq, double /*lastBarPPQ*/, int numerator, int denominator)
    {
        if (numerator == 0 || denominator == 0)
            return "1|1|0";
        
        const int ppqPerBar = (numerator * 4 / denominator);
        const double beats  = (fmod (ppq, ppqPerBar) / ppqPerBar) * numerator;
        
        const int bar    = ((int) ppq) / ppqPerBar + 1;
        const int beat   = ((int) beats) + 1;
        const int ticks  = ((int) (fmod (beats, 1.0) * 960.0 + 0.5));
        
        String s;
        s << bar << '|' << beat << '|' << ticks;
        return s;
    }
    

    // Updates the text in our position label.
    void displayPositionInfo (const AudioPlayHead::CurrentPositionInfo& pos)
    {
        lastDisplayedPosition = pos;
        String displayText;
        displayText.preallocateBytes (128);
        
        displayText << String (pos.bpm, 2) << " bpm, "
        << pos.timeSigNumerator << '/' << pos.timeSigDenominator
        << "  -  " << timeToTimecodeString (pos.timeInSeconds)
        << "  -  " << ppqToBarsBeatsString (pos.ppqPosition, pos.ppqPositionOfLastBarStart,
                                            pos.timeSigNumerator, pos.timeSigDenominator);
        
        if (pos.isRecording)
            displayText << "  (recording)";
        else if (pos.isPlaying)
            displayText << "  (playing)";
        
        infoLabel.setText ("[" + SystemStats::getJUCEVersion() + "]   " + displayText, dontSendNotification);
    }
};

#endif
