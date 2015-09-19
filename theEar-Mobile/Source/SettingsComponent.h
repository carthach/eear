//
//  SettingsComponent.h
//  theEar-Mobile
//
//  Created by Cárthach Ó Nuanáin on 19/09/2015.
//
//

#ifndef theEar_Mobile_SettingsComponent_h
#define theEar_Mobile_SettingsComponent_h

#include "../JuceLibraryCode/JuceHeader.h"
//#include "AudioRecordingDemo.cpp"

class SettingsComponent : public Component, public Button::Listener, Slider::Listener {
public:
    Label ipAddressLabel, portNumberLabel, oscInfoLabel;
    TextEditor ipAddressTextBox, portNumberTextBox;
    TextButton saveButton;
    AudioRecordingDemo* recorder;
    Slider frameSlider;
    Label frameSliderLabel;
    
    SettingsComponent(AudioRecordingDemo* recorder)
    {
        addAndMakeVisible (frameSlider);
        frameSlider.setRange (1.0, 44100.0/512.0, 1.0);
        frameSlider.setValue (50, dontSendNotification);
        frameSlider.setSliderStyle (Slider::LinearHorizontal);
        frameSlider.setTextBoxStyle (Slider::TextBoxRight, false, 50, 20);
        frameSlider.addListener (this);
        
        addAndMakeVisible (frameSliderLabel);
        frameSliderLabel.setText ("Size of Buffer", dontSendNotification);
        frameSliderLabel.attachToComponent (&frameSlider, true);
        
        addAndMakeVisible (ipAddressLabel);
        ipAddressLabel.setText ("IP Address:", dontSendNotification);
        
        addAndMakeVisible (ipAddressTextBox);
        ipAddressTextBox.setText("127.0.0.1");
        
        addAndMakeVisible (oscInfoLabel);
        oscInfoLabel.setText ("Set OSC Info:", dontSendNotification);
        addAndMakeVisible (portNumberLabel);
        portNumberLabel.setText ("Port Number:", dontSendNotification);
        
        addAndMakeVisible (portNumberTextBox);
        portNumberTextBox.setText("8000");
        
        saveButton.setButtonText("Save");
        addAndMakeVisible(saveButton);
        
        this->recorder = recorder;
    }
    
    ~SettingsComponent()
    {
        
    }
    
    //=======================================================================
    void paint (Graphics& g) override
    {
        // (Our component is opaque, so we must completely fill the background with a solid colour)
//        g.fillAll(Colours::black);
        
        
        // You can add your drawing code here!
    }
    
    void resized() override
    {
        //Just one sub component
        
        Rectangle<int> labelBounds(20, 0, 100, 20);
        Rectangle<int> valueBounds(150, 0, 100, 20);
        
        int labelY = 0;
        
        frameSliderLabel.setBounds(labelBounds.withY(labelY+=30));
        frameSlider.setBounds (valueBounds.withY(labelY));

    
        oscInfoLabel.setBounds(labelBounds.withY(labelY+=40));
        
        ipAddressLabel.setBounds(labelBounds.withY(labelY+=30));
        ipAddressTextBox.setBounds(valueBounds.withY(labelY));
        portNumberLabel.setBounds(labelBounds.withY(labelY+=30));
        portNumberTextBox.setBounds(valueBounds.withY(labelY));
        
        saveButton.setBounds(labelBounds.withY(labelY+=30));
        saveButton.addListener(this);

    }
    
    void buttonClicked (Button *) override
    {
        recorder->ipAddress = ipAddressTextBox.getText();
        recorder->portNumber = portNumberTextBox.getText().getIntValue();
        
    }
    
    void sliderValueChanged (Slider *slider) override
    {
        if (slider == &frameSlider) {
            int frameValue = int(slider->getValue());
            if(frameValue != recorder->recorder.computeFrameCount) {
                if (recorder->recorder.isRecording())
                    recorder->stopRecording();
                recorder->recorder.computeFrameCount = frameValue;
                
            }
        }
    }
};

#endif
