/*
 ==============================================================================
 
 This file is part of the JUCE library.
 Copyright (c) 2015 - ROLI Ltd.
 
 Permission is granted to use this software under the terms of either:
 a) the GPL v2 (or any later version)
 b) the Affero GPL v3
 
 Details of these licenses can be found at: www.gnu.org/licenses
 
 JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 
 ------------------------------------------------------------------------------
 
 To release a closed-source product which uses JUCE, commercial licenses are
 available: visit www.juce.com for more information.
 
 ==============================================================================
 */



#include "../JuceLibraryCode/JuceHeader.h"
#include "AudioLiveScrollingDisplay.h"
#include "StreamingRecorder.h"
#include "StandardRecorder.h"
#include <sstream>


#include "SpectralUI.h"

class SimpleDeviceManagerInputLevelMeter  : public Component,
public Timer
{
public:
    SimpleDeviceManagerInputLevelMeter (AudioDeviceManager& m)
    : manager (m), level (0)
    {
        startTimer (50);
        manager.enableInputLevelMeasurement (true);
    }
    
    ~SimpleDeviceManagerInputLevelMeter()
    {
        manager.enableInputLevelMeasurement (false);
    }
    
    void timerCallback() override
    {
        if (isShowing())
        {
            const float newLevel = (float) manager.getCurrentInputLevel();
            
            if (std::abs (level - newLevel) > 0.005f)
            {
                level = newLevel;
                repaint();
            }
        }
        else
        {
            level = 0;
        }
    }
    
    void paint (Graphics& g) override
    {
        getLookAndFeel().drawLevelMeter (g, getWidth(), getHeight(),
                                         (float) exp (log (level) / 3.0)); // (add a bit of a skew to make the level more obvious)
    }
    
private:
    AudioDeviceManager& manager;
    float level;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleDeviceManagerInputLevelMeter)
};


//==============================================================================
/** A simple class that acts as an AudioIODeviceCallback and writes the
 incoming audio data to a WAV file.
 */
class AudioRecorder  : public AudioIODeviceCallback
{
public:
    AudioRecorder (AudioThumbnail& thumbnailToUpdate)
    : thumbnail (thumbnailToUpdate),
    backgroundThread ("Audio Recorder Thread"),
    sampleRate (0), nextSampleNum (0), activeWriter (nullptr)
    {
        backgroundThread.startThread();
    }
    
    ~AudioRecorder()
    {
        stop();
    }
    
    //==============================================================================
    void startRecording (const File& file)
    {
        stop();
        
        if (sampleRate > 0)
        {
            // Create an OutputStream to write to our destination file...
            file.deleteFile();
            ScopedPointer<FileOutputStream> fileStream (file.createOutputStream());
            
            if (fileStream != nullptr)
            {
                // Now create a WAV writer object that writes to our output stream...
                WavAudioFormat wavFormat;
                AudioFormatWriter* writer = wavFormat.createWriterFor (fileStream, sampleRate, 1, 16, StringPairArray(), 0);
                
                if (writer != nullptr)
                {
                    fileStream.release(); // (passes responsibility for deleting the stream to the writer object that is now using it)
                    
                    // Now we'll create one of these helper objects which will act as a FIFO buffer, and will
                    // write the data to disk on our background thread.
                    threadedWriter = new AudioFormatWriter::ThreadedWriter (writer, backgroundThread, 32768);
                    
                    // Reset our recording thumbnail
                    thumbnail.reset (writer->getNumChannels(), writer->getSampleRate());
                    nextSampleNum = 0;
                    
                    // And now, swap over our active writer pointer so that the audio callback will start using it..
                    const ScopedLock sl (writerLock);
                    activeWriter = threadedWriter;
                }
            }
        }
    }
    
    void stop()
    {
        // First, clear this pointer to stop the audio callback from using our writer object..
        {
            const ScopedLock sl (writerLock);
            activeWriter = nullptr;
        }
        
        // Now we can delete the writer object. It's done in this order because the deletion could
        // take a little time while remaining data gets flushed to disk, so it's best to avoid blocking
        // the audio callback while this happens.
        threadedWriter = nullptr;
    }
    
    bool isRecording() const
    {
        return activeWriter != nullptr;
    }
    
    //==============================================================================
    void audioDeviceAboutToStart (AudioIODevice* device) override
    {
        sampleRate = device->getCurrentSampleRate();
    }
    
    void audioDeviceStopped() override
    {
        sampleRate = 0;
    }
    
    void audioDeviceIOCallback (const float** inputChannelData, int /*numInputChannels*/,
                                float** outputChannelData, int numOutputChannels,
                                int numSamples) override
    {
        const ScopedLock sl (writerLock);
        
        if (activeWriter != nullptr)
        {
            activeWriter->write (inputChannelData, numSamples);
            
            // Create an AudioSampleBuffer to wrap our incomming data, note that this does no allocations or copies, it simply references our input data
            const AudioSampleBuffer buffer (const_cast<float**> (inputChannelData), thumbnail.getNumChannels(), numSamples);
            thumbnail.addBlock (nextSampleNum, buffer, 0, numSamples);
            nextSampleNum += numSamples;
        }
        
        // We need to clear the output buffers, in case they're full of junk..
        for (int i = 0; i < numOutputChannels; ++i)
            if (outputChannelData[i] != nullptr)
                FloatVectorOperations::clear (outputChannelData[i], numSamples);
    }
    
private:
    AudioThumbnail& thumbnail;
    TimeSliceThread backgroundThread; // the thread that will write our audio data to disk
    ScopedPointer<AudioFormatWriter::ThreadedWriter> threadedWriter; // the FIFO used to buffer the incoming data
    double sampleRate;
    int64 nextSampleNum;
    
    CriticalSection writerLock;
    AudioFormatWriter::ThreadedWriter* volatile activeWriter;
};



//==============================================================================
class RecordingThumbnail  : public Component,
private ChangeListener
{
public:
    RecordingThumbnail()
    : thumbnailCache (10),
    thumbnail (512, formatManager, thumbnailCache),
    displayFullThumb (false)
    {
        formatManager.registerBasicFormats();
        thumbnail.addChangeListener (this);
    }
    
    ~RecordingThumbnail()
    {
        thumbnail.removeChangeListener (this);
    }
    
    AudioThumbnail& getAudioThumbnail()     { return thumbnail; }
    
    void setDisplayFullThumbnail (bool displayFull)
    {
        displayFullThumb = displayFull;
        repaint();
    }
    
    void paint (Graphics& g) override
    {
        g.fillAll();
        g.setColour (Colours::lightgrey);
        
        if (thumbnail.getTotalLength() > 0.0)
        {
            const double endTime = displayFullThumb ? thumbnail.getTotalLength()
            : jmax (30.0, thumbnail.getTotalLength());
            
            Rectangle<int> thumbArea (getLocalBounds());
            thumbnail.drawChannels (g, thumbArea.reduced (2), 0.0, endTime, 1.0f);
        }
        else
        {
//            g.setFont (14.0f);
            g.drawFittedText ("(No file recorded)", getLocalBounds(), Justification::centred, 2);
        }
    }
    
private:
    AudioFormatManager formatManager;
    AudioThumbnailCache thumbnailCache;
    AudioThumbnail thumbnail;
    bool displayFullThumb;
    
    void changeListenerCallback (ChangeBroadcaster* source) override
    {
        if (source == &thumbnail)
            repaint();
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RecordingThumbnail)
};

//==============================================================================
class AudioRecordingDemo  : public Component,
private Button::Listener,
private ChangeListener,
private SliderListener

{
public:
    String oscIP = "127.0.0.1";
    int oscPort = 8000;
    
    AudioRecordingDemo(AudioDeviceManager* deviceManager)
    : recorder (), inputMeter(*deviceManager)
    {
        setOpaque (true);
#if defined JUCE_ANDROID || defined JUCE_IOS
#error( Need to define media Path for android / ios)
        assert(false);
#else
        File Resources = File::getSpecialLocation(juce::File::SpecialLocationType::currentApplicationFile).getChildFile("Contents").getChildFile("Resources");
#endif
        
            File file(Resources.getChildFile("AbyssinicaSIL.Juce"));
            
//            int memSize = file.getSize();
            cout << file.exists() << endl;
//            void * fontBlock = malloc(memSize);
//            FileInputStream fstream(file);
//
//            fstream.read(fontBlock, memSize);
//            Typeface::createSystemTypefaceFor (fontBlock,memSize);
//            cout << memSize << endl;
//            free( fontBlock);
            
//            Font f ("Abyssinica SIL",70,Font::FontStyleFlags::plain);
//            CustomTypeface t;
//            
//            t.addGlyphsFromOtherTypeface(*f.getTypeface(), 'A', 42);
//            FileOutputStream * outf = file.createOutputStream();
//           t.writeToStream(*outf);
            
            
            FileInputStream * inf = file.createInputStream();
        

        
        


        addAndMakeVisible (liveAudioScroller);
        
        //        addAndMakeVisible (explanationLabel);
        //        explanationLabel.setText ("This page demonstrates how to record a wave file from the live audio input..\n\nPressing record will start recording a file in your \"Documents\" folder.", dontSendNotification);
        //        explanationLabel.setFont (Font (15.00f, Font::plain));
        //        explanationLabel.setJustificationType (Justification::topLeft);
        //        explanationLabel.setEditable (false, false, false);
        //        explanationLabel.setColour (TextEditor::textColourId, Colours::black);
        //        explanationLabel.setColour (TextEditor::backgroundColourId, Colour (0x00000000));
        
        addAndMakeVisible(inputMeter);
        
        
        addAndMakeVisible (recordButton);
        recordButton.addListener (this);
        

        
        {
            File file(Resources.getChildFile("recordButtonPushed.png"));
            FileInputStream fstream(file);
            recordButtonImagePushed =ImageFileFormat::loadFrom (fstream);
        }
        {
            File file(Resources.getChildFile("recordButtonNotPushed.png"));
            FileInputStream fstream(file);
            recordButtonImageNotPushed =ImageFileFormat::loadFrom (fstream);
        }
        
        {
            
            recordButton.setImages(false, // resize button to image
                                   true, // resize image if button change
                                   true, // keep image ratio
                                   recordButtonImageNotPushed, // normal
                                   1.0f,
                                   juce::Colour(),
                                   Image(), // over
                                   1.0f,
                                   juce::Colour(),
                                   recordButtonImagePushed, // pushed
                                   1.0f,
                                   juce::Colour());
        }
        
        
        addAndMakeVisible(spectrum);
        spectrum.setStrokeFill(Colours::white);
        spectrum.setStrokeThickness(6);
        spectrum.setFill(Colours::transparentWhite);
        spectrum.setPath(spectralHandler.getPath());
        
//        addAndMakeVisible (keyScaleLabel);
//        keyScaleLabel.setText ("Key/Scale:", dontSendNotification);
        

        
        keyScaleTextBox.setFont(Font("Arial",67,Font::FontStyleFlags::plain), true);
        keyScaleTextBox.setFontHeight(67);

        
        keyScaleTextBox.setColour(Colour::fromRGB(119,195,214));
        addAndMakeVisible (keyScaleTextBox);
//        addAndMakeVisible (rmsLabel);
//        rmsLabel.setText ("RMS:", dontSendNotification);
//        
//        addAndMakeVisible (rmsTextBox);
//        rmsTextBox.setReadOnly(true);
//        rmsTextBox.setColour(TextEditor::backgroundColourId, Colours::lightgrey);
//        
//        addAndMakeVisible (spectralFlatnessLabel);
//        spectralFlatnessLabel.setText ("Spectral Flatness:", dontSendNotification);
//        
//        addAndMakeVisible (spectralFlatnessTextBox);
//        spectralFlatnessTextBox.setReadOnly(true);
//        spectralFlatnessTextBox.setColour(TextEditor::backgroundColourId, Colours::lightgrey);
//        
//        addAndMakeVisible (spectralCentroidLabel);
//        spectralCentroidLabel.setText ("Spectral Centroid:", dontSendNotification);
        
//        addAndMakeVisible (spectralCentroidTextBox);
//        spectralCentroidTextBox.setReadOnly(true);
//        spectralCentroidTextBox.setColour(TextEditor::backgroundColourId, Colours::lightgrey);
//        

        
        
        addAndMakeVisible (sensitivitySlider);
        sensitivitySlider.setRange (1.0, 44100.0/512.0, 1.0);
        sensitivitySlider.setValue (50, dontSendNotification);
        sensitivitySlider.setSliderStyle (Slider::LinearHorizontal);
        sensitivitySlider.setTextBoxStyle (Slider::TextBoxRight, false, 50, 20);
        sensitivitySlider.addListener (this);
        
        addAndMakeVisible (sensitivitySliderLabel);
        sensitivitySliderLabel.setText ("Sensitivity", dontSendNotification);
        sensitivitySliderLabel.attachToComponent (&sensitivitySlider, true);
        
        addAndMakeVisible (frameSlider);
        frameSlider.setRange (1.0, 44100.0/512.0, 1.0);
        frameSlider.setValue (50, dontSendNotification);
        frameSlider.setSliderStyle (Slider::LinearHorizontal);
        frameSlider.setTextBoxStyle (Slider::TextBoxRight, false, 50, 20);
        frameSlider.addListener (this);
        
        addAndMakeVisible (frameSliderLabel);
        frameSliderLabel.setText ("Size of Buffer", dontSendNotification);
        frameSliderLabel.attachToComponent (&frameSlider, true);
        
        //        addAndMakeVisible (recordingThumbnail);
        
        this->deviceManager = deviceManager;
        
        deviceManager->addAudioCallback (&liveAudioScroller);
        deviceManager->addAudioCallback (&recorder);
        
        recorder.addChangeListener(this);
    }
    
    ~AudioRecordingDemo()
    {
        deviceManager->removeAudioCallback (&recorder);
        deviceManager->removeAudioCallback (&liveAudioScroller);
    }
    
    void paint (Graphics& g) override
    {
        g.fillAll (juce::Colour::fromRGB(21, 24, 27));
        g.setColour(Colours::black);
        g.setFont(60);
        
    }
    
    void resized() override
    {
        Rectangle<int> area (getLocalBounds());
        liveAudioScroller.setBounds (area.removeFromTop (80).reduced (8));
        //        recordingThumbnail.setBounds (area.removeFromTop (80).reduced (8));
        
        keyScaleTextBox.setBounds(area.removeFromTop (136));
        keyScaleTextBox.setFontHeight(1000);
        
        spectrum.setBounds(area.removeFromTop (136));
        spectralHandler.bounds  = spectrum.getBounds();
        recordButton.setBounds (area.removeFromTop (136).removeFromLeft (140).reduced (8));
        explanationLabel.setBounds (area.reduced (8));
        
        int xOffset = recordButton.getX()+130;
        
        Rectangle<int> labelBounds = recordButton.getBounds().withHeight(30);
        Rectangle<int> valueBounds = labelBounds.withX(xOffset);
        
        int labelY = explanationLabel.getY()+5;
        
//        sensitivitySliderLabel.setBounds(labelBounds.withY(labelY));
//        sensitivitySlider.setBounds (valueBounds.withY(labelY));
//        
//        frameSliderLabel.setBounds(labelBounds.withY(labelY+=30));
//        frameSlider.setBounds (valueBounds.withY(labelY));
//        
//        keyScaleLabel.setBounds(labelBounds.withY(labelY+=30));
//        
//        rmsLabel.setBounds(labelBounds.withY(labelY+=30));
//        rmsTextBox.setBounds(valueBounds.withY(labelY));
//        
//        spectralFlatnessLabel.setBounds(labelBounds.withY(labelY+=30));
//        spectralFlatnessTextBox.setBounds(valueBounds.withY(labelY));
//        spectralCentroidLabel.setBounds(labelBounds.withY(labelY+=30));
//        spectralCentroidTextBox.setBounds(valueBounds.withY(labelY));
        

        
        inputMeter.setBounds(recordButton.getBounds().withX(xOffset));
        inputMeter.setSize(recordButton.getWidth()/2,recordButton.getHeight() );
        
        
    }
    
private:
    AudioDeviceManager* deviceManager;
    LiveScrollingAudioDisplay liveAudioScroller;
    RecordingThumbnail recordingThumbnail;
    //    AudioRecorder recorder;
    //    StandardRecorder recorder;
    StreamingRecorder recorder;
    Label explanationLabel;
    TextEditor rmsTextBox, spectralFlatnessTextBox, spectralCentroidTextBox;
    
    ImageButton recordButton;
    
    Image recordButtonImagePushed;
    Image recordButtonImageNotPushed;
    
    DrawablePath spectrum;
    SpectralUI spectralHandler;
    
    
    DatagramSocket datagramSocket;
    SimpleDeviceManagerInputLevelMeter inputMeter;
    
    Slider frameSlider, sensitivitySlider;
    Label frameSliderLabel, sensitivitySliderLabel;
    
    Label keyScaleLabel, rmsLabel, spectralFlatnessLabel, spectralCentroidLabel;

    
    
    Font font;
    DrawableText keyScaleTextBox;
    

    
    bool recording = false;
    
    void startRecording()
    {
        //        const File file (File::getSpecialLocation (File::userDocumentsDirectory)
        //                            .getNonexistentChildFile ("Juce Demo Audio Recording", ".wav"));
        
        recorder.startRecording ();
        
        recordButton.setButtonText ("Stop");
        recordingThumbnail.setDisplayFullThumbnail (false);
        
        recording = true;
    }
    
    void stopRecording()
    {
        recorder.stop();
        
        
        //        recordButton.setButtonText ("Listen!");
        recordingThumbnail.setDisplayFullThumbnail (true);
        
        recording = false;
    }
    
    void buttonClicked (Button* button) override
    {
        if (button == &recordButton)
        {
            if (recording)
                stopRecording();
            else
                startRecording();
        }
    }
    
    void 	sliderValueChanged (Slider *slider) override
    {
        if (slider == &frameSlider) {
            int frameValue = int(slider->getValue());
            if(frameValue != recorder.computeFrameCount) {
                if (recorder.isRecording())
                    stopRecording();
                recorder.computeFrameCount = frameValue;
                
            }
        }
    }
    
    void changeListenerCallback (ChangeBroadcaster *source)
    {
        char buffer[1024];
        osc::OutboundPacketStream p( buffer, 1024 );
        
        p
        << osc::BeginMessage( "/earData" )
        << recorder.keyString.toRawUTF8()
        << recorder.scaleString.toRawUTF8()
        << recorder.rmsValue
        << recorder.spectralFlatnessValue
        << recorder.spectralCentroidValue
        << osc::EndMessage;
        //        << osc::BeginMessage( "/test2" )
        //        << true << 24 << (float)10.8 << "world" << osc::EndMessage
        //
        //        juce::Logger *log = juce::Logger::getCurrentLogger();
        //        String message(ipAddressTextBox.getText());
        //        log->writeToLog(message);
        //        message = portNumberTextBox.getText();
        //        log->writeToLog(message);
        
//        datagramSocket.write(ipAddressTextBox.getText(), portNumberTextBox.getText().getIntValue(), p.Data(), p.Size());
        datagramSocket.write(oscIP, oscPort, p.Data(), p.Size());
        
        std::cout << oscIP;
        std::cout << oscPort << "\n\n\n";
        
        
        if(spectralHandler.needsUpdate()){
            
            String keyScaleString = recorder.keyString + " " + recorder.scaleString;
            keyScaleTextBox.setText(keyScaleString);
            rmsTextBox.setText(String(float(recorder.rmsValue), 10));
            spectralFlatnessTextBox.setText(String(recorder.spectralFlatnessValue));
            spectralCentroidTextBox.setText(String(recorder.spectralCentroidValue));
        spectralHandler.rms = 2000*log(1+recorder.rmsValue);
        spectralHandler.flatness = recorder.spectralFlatnessValue;
            spectralHandler.centroid = juce::jmap(log2(recorder.spectralCentroidValue),log2(400.0f),log2(5000.0f),0.0f,1.0f);

            
            spectrum.setPath(spectralHandler.getPath());
            
            repaint();
        }
        
        //
        
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioRecordingDemo)
};
