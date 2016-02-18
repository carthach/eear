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
#include <string>
//#include "AudioRecordingDemo.cpp"

class SettingsComponent : public Component, public Button::Listener, Slider::Listener,TextEditor::Listener {
public:
    Label ipAddressLabel, portNumberLabel, oscInfoLabel;
    TextEditor ipAddressTextBox, portNumberTextBox;
    TextButton continuousRecording;
    AudioRecordingDemo* recorder;
    Slider frameSlider;
    Label frameSliderLabel;
    
    Slider sensitivitySlider;
    Label sensitivitySliderLabel;
    
    //    Label keyScaleLabel, rmsLabel, spectralFlatnessLabel, spectralCentroidLabel;
    
    
    SettingsComponent(AudioRecordingDemo* recorder)
    {
        PropertiesFile::Options options;
        options.applicationName = File::createLegalFileName ("eearMobile");
        options.commonToAllUsers = true;
        options.osxLibrarySubFolder = "Application Support";
        properties.setStorageParameters(options );
        
        PropertiesFile * pFile = properties.getUserSettings();
        
        
        
        addAndMakeVisible (frameSlider);
        frameSlider.setRange (3.0, 40, 1.0);
        frameSlider.addListener (this);
        frameSlider.setValue (pFile->getIntValue("BufSize" , 23), sendNotification);
        frameSlider.setSliderStyle (Slider::LinearHorizontal);
        frameSlider.setTextBoxStyle (Slider::TextBoxRight, false, 30, 20);
        
        
        addAndMakeVisible (frameSliderLabel);
        frameSliderLabel.setText ("Size of Buffer", dontSendNotification);
        //        frameSliderLabel.attachToComponent (&frameSlider, true);
        
        addAndMakeVisible (ipAddressLabel);
        ipAddressLabel.setText ("IP Address:", dontSendNotification);
        ipAddressLabel.setMouseClickGrabsKeyboardFocus(true);
        ipAddressLabel.setWantsKeyboardFocus(true);
        addAndMakeVisible (ipAddressTextBox);
        
        ipAddressTextBox.addListener(this);
        ipAddressTextBox.setText(pFile->getValue("ipAddress" , "127.0.0.1"), sendNotification);
        ipAddressTextBox.
        addAndMakeVisible (oscInfoLabel);
        oscInfoLabel.setText ("Set OSC Info:", dontSendNotification);
        addAndMakeVisible (portNumberLabel);
        portNumberLabel.setText ("Port Number:", dontSendNotification);
        portNumberLabel.setMouseClickGrabsKeyboardFocus(true);
        portNumberLabel.setWantsKeyboardFocus(true);
        
        
        addAndMakeVisible (portNumberTextBox);
        portNumberTextBox.addListener(this);
        portNumberTextBox.setText(pFile->getValue("port" , "8000"), sendNotification);
        
        
        continuousRecording.setButtonText("Set continuous recording");
        continuousRecording.setClickingTogglesState(true);
        addAndMakeVisible(continuousRecording);
        continuousRecording.addListener(this);
        if(pFile->getBoolValue("continuous" , false))
           continuousRecording.triggerClick();
        
        addAndMakeVisible (sensitivitySlider);
        sensitivitySlider.setRange (0, 10, .1);
        sensitivitySlider.addListener (this);
        
        
        sensitivitySlider.setValue (pFile->getDoubleValue("filterOut" , 0), sendNotification);
        sensitivitySlider.setSliderStyle (Slider::LinearHorizontal);
        sensitivitySlider.setTextBoxStyle (Slider::TextBoxRight, false, 30, 20);
        
        //
        addAndMakeVisible (sensitivitySliderLabel);
        sensitivitySliderLabel.setText ("Filter out chords", dontSendNotification);


        
        
        this->recorder = recorder;
    }
    
    ~SettingsComponent()
    {
        properties.saveIfNeeded();
        
    }
    ApplicationProperties properties;
    //=======================================================================
    
    
    void textEditorFocusLost (TextEditor& t) {
        
            //            recorder->oscPort =std::stoi(t.getText().toStdString());
            int port= portNumberTextBox.getText().getIntValue();
            properties.getUserSettings()->setValue("port", port);

            String ip =ipAddressTextBox.getText();
            properties.getUserSettings()->setValue("ipAddress", ip);
        
        jassert(recorder->setOSCInfo(ip,port));
        
    }
    
    void resized() override
    {
    //Just one sub component
    
    int padBorder = 10;
    Rectangle<int> labelBounds= getLocalBounds().withWidth(160).withX(padBorder);
    Rectangle<int> valueBounds=labelBounds.withWidth(getLocalBounds().getWidth()-labelBounds.getRight() - padBorder);
    valueBounds.setX(labelBounds.getRight() );


    int height = getLocalBounds().getHeight() / 7;
    int pad = 20;
    // max height
    int toReduce = jmax(0, height/2-25);
    //        oscInfoLabel.setBounds(labelBounds.withY(labelY+=height));
    valueBounds.removeFromTop(pad);labelBounds.removeFromTop(pad);
    
    ipAddressLabel.setBounds(labelBounds.removeFromTop(height));
    
    ipAddressTextBox.setBounds(valueBounds.removeFromTop(height).reduced(0,toReduce));

    
    valueBounds.removeFromTop(pad);labelBounds.removeFromTop(pad);
    portNumberLabel.setBounds(labelBounds.removeFromTop(height));
    portNumberTextBox.setBounds(valueBounds.removeFromTop(height).reduced(0,toReduce));
    
    valueBounds.removeFromTop(pad);labelBounds.removeFromTop(pad);
    frameSliderLabel.setBounds(labelBounds.removeFromTop(height));
    frameSlider.setBounds (valueBounds.removeFromTop(height));
    
    valueBounds.removeFromTop(pad);labelBounds.removeFromTop(pad);
    sensitivitySliderLabel.setBounds(labelBounds.removeFromTop(height));
    sensitivitySlider.setBounds (valueBounds.removeFromTop(height));
    
    valueBounds.removeFromTop(pad);labelBounds.removeFromTop(pad);
    continuousRecording.setBounds(valueBounds.removeFromTop(height).getUnion(labelBounds.removeFromTop(height)));
    
    
}

void buttonClicked (Button * b) override
{
//        recorder->ipAddress = ipAddressTextBox.getText();
//        recorder->portNumber = portNumberTextBox.getText().getIntValue();
    recorder->setContinuousRecording(b->getToggleState());
    properties.getUserSettings()->setValue("continuous", b->getToggleState());
//
}

void sliderValueChanged (Slider *slider) override
{
if (slider == &frameSlider)
{
    int frameValue = int(slider->getValue());

    recorder->recorder.computeFrameCount = slider->getValue();
    properties.getUserSettings()->setValue("BufSize", frameValue);
}
else if(slider == &sensitivitySlider)
{
    recorder->recorder.sensitivity =  slider->getValue();
    properties.getUserSettings()->setValue("filterOut", recorder->recorder.sensitivity);
    recorder->recorder.clearKeyPool(recorder->recorder.keyPoolMajor);
    recorder->recorder.clearKeyPool(recorder->recorder.keyPoolMinor);
}
}


void paint (Graphics& g) override
{
// (Our component is opaque, so we must completely fill the background with a solid colour)
//        g.fillAll(Colours::black);


// You can add your drawing code here!
}

};

#endif
