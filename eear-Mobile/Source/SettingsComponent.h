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
        addAndMakeVisible (frameSlider);
        frameSlider.setRange (3.0, 40, 1.0);
        frameSlider.setValue (recorder->recorder.computeFrameCount, dontSendNotification);
        frameSlider.setSliderStyle (Slider::LinearHorizontal);
        frameSlider.setTextBoxStyle (Slider::TextBoxRight, false, 30, 20);
        frameSlider.addListener (this);
        
        addAndMakeVisible (frameSliderLabel);
        frameSliderLabel.setText ("Size of Buffer", dontSendNotification);
        //        frameSliderLabel.attachToComponent (&frameSlider, true);
        
        addAndMakeVisible (ipAddressLabel);
        ipAddressLabel.setText ("IP Address:", dontSendNotification);
        ipAddressLabel.setMouseClickGrabsKeyboardFocus(true);
        ipAddressLabel.setWantsKeyboardFocus(true);
        addAndMakeVisible (ipAddressTextBox);
        ipAddressTextBox.setText("127.0.0.1");
        ipAddressTextBox.addListener(this);

        addAndMakeVisible (oscInfoLabel);
        oscInfoLabel.setText ("Set OSC Info:", dontSendNotification);
        addAndMakeVisible (portNumberLabel);
        portNumberLabel.setText ("Port Number:", dontSendNotification);
        portNumberLabel.setMouseClickGrabsKeyboardFocus(true);
        portNumberLabel.setWantsKeyboardFocus(true);
        
        
        addAndMakeVisible (portNumberTextBox);
        portNumberTextBox.setText("8000");
        portNumberTextBox.addListener(this);
    
        continuousRecording.setButtonText("Set continuous recording");
        continuousRecording.setClickingTogglesState(true);
        addAndMakeVisible(continuousRecording);
        continuousRecording.addListener(this);
        
        
        addAndMakeVisible (sensitivitySlider);
        sensitivitySlider.setRange (1, 20, 1);
        sensitivitySlider.setValue (recorder->recorder.sensitivity, dontSendNotification);
        sensitivitySlider.setSliderStyle (Slider::LinearHorizontal);
        sensitivitySlider.setTextBoxStyle (Slider::TextBoxRight, false, 30, 20);
        sensitivitySlider.addListener (this);
        //
        addAndMakeVisible (sensitivitySliderLabel);
        sensitivitySliderLabel.setText ("Keep chords", dontSendNotification);

        
        
        
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
    
    Rectangle<int> labelBounds= getLocalBounds().withWidth(160);
    Rectangle<int> valueBounds=labelBounds.withWidth(getLocalBounds().getWidth()-labelBounds.getWidth());
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

//
}

void sliderValueChanged (Slider *slider) override
{
if (slider == &frameSlider)
{
    int frameValue = int(slider->getValue());
//    if(frameValue != recorder->recorder.computeFrameCount)
//    {
//        if (recorder->recorder.isRecording())
//            recorder->stopRecording();
//     }
    recorder->recorder.computeFrameCount = slider->getValue();
}
else if(slider == &sensitivitySlider)
{
    recorder->recorder.sensitivity = jmax(1.0, slider->getValue());
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
