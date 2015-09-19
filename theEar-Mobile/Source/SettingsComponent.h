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

class SettingsComponent : public Component, public Button::Listener {
public:
    Label ipAddressLabel, portNumberLabel, oscInfoLabel;
    TextEditor ipAddressTextBox, portNumberTextBox;
    TextButton saveButton;
    AudioRecordingDemo* recorder;
    
    SettingsComponent(AudioRecordingDemo* recorder)
    {
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
        recorder->oscIP = ipAddressTextBox.getText();
        recorder->oscPort = portNumberTextBox.getText().getIntValue();
        
    }
};

#endif
