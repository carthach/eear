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
    TextButton saveButton;
    AudioRecordingDemo* recorder;
    Slider frameSlider;
    Label frameSliderLabel;
    
    Slider sensitivitySlider;
    Label sensitivitySliderLabel;
    
    //    Label keyScaleLabel, rmsLabel, spectralFlatnessLabel, spectralCentroidLabel;
    
    
    SettingsComponent(AudioRecordingDemo* recorder)
    {
        addAndMakeVisible (frameSlider);
        frameSlider.setRange (1.0, 44100.0/512.0, 1.0);
        frameSlider.setValue (recorder->recorder.computeFrameCount, dontSendNotification);
        frameSlider.setSliderStyle (Slider::LinearHorizontal);
        frameSlider.setTextBoxStyle (Slider::TextBoxRight, false, 30, 20);
        frameSlider.addListener (this);
        
        addAndMakeVisible (frameSliderLabel);
        frameSliderLabel.setText ("Size of Buffer", dontSendNotification);
        //        frameSliderLabel.attachToComponent (&frameSlider, true);
        
        addAndMakeVisible (ipAddressLabel);
        ipAddressLabel.setText ("IP Address:", dontSendNotification);
        
        addAndMakeVisible (ipAddressTextBox);
        ipAddressTextBox.setText("127.0.0.1");
        ipAddressTextBox.addListener(this);
        
        addAndMakeVisible (oscInfoLabel);
        oscInfoLabel.setText ("Set OSC Info:", dontSendNotification);
        addAndMakeVisible (portNumberLabel);
        portNumberLabel.setText ("Port Number:", dontSendNotification);
        
        addAndMakeVisible (portNumberTextBox);
        portNumberTextBox.setText("8000");
        portNumberTextBox.addListener(this);
        
        //        saveButton.setButtonText("Save");
        //        addAndMakeVisible(saveButton);
        
        
        
        addAndMakeVisible (sensitivitySlider);
        sensitivitySlider.setRange (0, 20, 1);
        sensitivitySlider.setValue (20 - recorder->recorder.sensitivity, dontSendNotification);
        sensitivitySlider.setSliderStyle (Slider::LinearHorizontal);
        sensitivitySlider.setTextBoxStyle (Slider::TextBoxRight, false, 30, 20);
        sensitivitySlider.addListener (this);
        //
        addAndMakeVisible (sensitivitySliderLabel);
        sensitivitySliderLabel.setText ("New chords sensitivity", dontSendNotification);
        sensitivitySliderLabel.attachToComponent (&sensitivitySlider, true);
        
        
        
        this->recorder = recorder;
    }
    
    ~SettingsComponent()
    {
        
    }
    
    //=======================================================================
    
    
    void textEditorFocusLost (TextEditor& t) {
        if(&t == &portNumberTextBox ){
            //            recorder->oscPort =std::stoi(t.getText().toStdString());
            recorder->oscPort= t.getText().getIntValue();
        }
        else if (&t == &ipAddressTextBox){
            recorder->oscIP =t.getText();
        }
        
        
    }
    
    void resized() override
    {
    //Just one sub component
    
    Rectangle<int> labelBounds(20, 0, 100, 30);
    Rectangle<int> valueBounds(125, 0, 150, 25);
    
    int labelY = 0;
    int height = 50;
    
    //        oscInfoLabel.setBounds(labelBounds.withY(labelY+=height));
    
    ipAddressLabel.setBounds(labelBounds.withY(labelY+=height));
    ipAddressTextBox.setBounds(valueBounds.withY(labelY));
    portNumberLabel.setBounds(labelBounds.withY(labelY+=height));
    portNumberTextBox.setBounds(valueBounds.withY(labelY));
    
    frameSliderLabel.setBounds(labelBounds.withY(labelY+=height));
    frameSlider.setBounds (valueBounds.withY(labelY));
    
    
    sensitivitySliderLabel.setBounds(labelBounds.withY(labelY+=height));
    sensitivitySlider.setBounds (valueBounds.withY(labelY));
    
    
}

void buttonClicked (Button *) override
{
//        recorder->ipAddress = ipAddressTextBox.getText();
//        recorder->portNumber = portNumberTextBox.getText().getIntValue();
//
}

void sliderValueChanged (Slider *slider) override
{
if (slider == &frameSlider)
{
    int frameValue = int(slider->getValue());
    if(frameValue != recorder->recorder.computeFrameCount)
    {
        if (recorder->recorder.isRecording())
            recorder->stopRecording();
            }
    recorder->recorder.computeFrameCount = slider->getValue();
}
else if(slider == &sensitivitySlider)
{
    recorder->recorder.sensitivity = jmax(2.0,slider->getMaximum() - slider->getValue());
    
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
