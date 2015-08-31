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

#include <essentia/algorithmfactory.h>
#include <essentia/essentiamath.h>
#include <essentia/pool.h>

using namespace std;
using namespace essentia;
using namespace essentia::standard;

//==============================================================================
/* This is a rough-and-ready circular buffer, used to allow the audio thread to
 push data quickly into a queue, allowing a background thread to come along and
 write it to disk later.
 */
class CircularAudioBuffer
{
public:
    CircularAudioBuffer (const int numChannels, const int numSamples)
    : buffer (numChannels, numSamples)
    {
        clear();
    }
    
    ~CircularAudioBuffer()
    {
    }
    
    void clear()
    {
        buffer.clear();
        
        const ScopedLock sl (bufferLock);
        bufferValidStart = bufferValidEnd = 0;
    }
    
    void addSamplesToBuffer (const AudioSampleBuffer& sourceBuffer, int numSamples)
    {
        const int bufferSize = buffer.getNumSamples();
        
        bufferLock.enter();
        int newDataStart = bufferValidEnd;
        int newDataEnd = newDataStart + numSamples;
        const int actualNewDataEnd = newDataEnd;
        bufferValidStart = jmax (bufferValidStart, newDataEnd - bufferSize);
        bufferLock.exit();
        
        newDataStart %= bufferSize;
        newDataEnd %= bufferSize;
        
        if (newDataEnd < newDataStart)
        {
            for (int i = jmin (buffer.getNumChannels(), sourceBuffer.getNumChannels()); --i >= 0;)
            {
                buffer.copyFrom (i, newDataStart, sourceBuffer, i, 0, bufferSize - newDataStart);
                buffer.copyFrom (i, 0, sourceBuffer, i, bufferSize - newDataStart, newDataEnd);
            }
        }
        else
        {
            for (int i = jmin (buffer.getNumChannels(), sourceBuffer.getNumChannels()); --i >= 0;)
                buffer.copyFrom (i, newDataStart, sourceBuffer, i, 0, newDataEnd - newDataStart);
        }
        
        const ScopedLock sl (bufferLock);
        bufferValidEnd = actualNewDataEnd;
    }
    
    int readSamplesFromBuffer (AudioSampleBuffer& destBuffer, int numSamples)
    {
        const int bufferSize = buffer.getNumSamples();
        
        bufferLock.enter();
        int availableDataStart = bufferValidStart;
        const int numSamplesDone = jmin (numSamples, bufferValidEnd - availableDataStart);
        int availableDataEnd = availableDataStart + numSamplesDone;
        bufferValidStart = availableDataEnd;
        bufferLock.exit();
        
        availableDataStart %= bufferSize;
        availableDataEnd %= bufferSize;
        
        if (availableDataEnd < availableDataStart)
        {
            for (int i = jmin (buffer.getNumChannels(), destBuffer.getNumChannels()); --i >= 0;)
            {
                destBuffer.copyFrom (i, 0, buffer, i, availableDataStart, bufferSize - availableDataStart);
                destBuffer.copyFrom (i, bufferSize - availableDataStart, buffer, i, 0, availableDataEnd);
            }
        }
        else
        {
            for (int i = jmin (buffer.getNumChannels(), destBuffer.getNumChannels()); --i >= 0;)
                destBuffer.copyFrom (i, 0, buffer, i, availableDataStart, numSamplesDone);
        }
        
        return numSamplesDone;
    }
    
private:
    CriticalSection bufferLock;
    AudioSampleBuffer buffer;
    int bufferValidStart, bufferValidEnd;
};


//==============================================================================
/** A simple class that acts as an AudioIODeviceCallback and writes the
 incoming audio data to a WAV file.
 */
class ThreadAudioRecorder  : public AudioIODeviceCallback, public Thread, public ChangeBroadcaster
{
public:
//    vector<Real> signalBuffer, wavFile;
//    int signalBufferCounter = 0;
//    int signalBufferSize = 0;
    
    int frameSize = 2048;
    int hopSize = 1024;
    
    Algorithm *fc, *w, *spec, *spectralPeaks, *hpcp, *key, *rms, *spectralCentroid, *spectralFlatness, *centroid;
    
    std::vector<Real> essentiaSignal;
    
    // FrameCutter -> Windowing -> Spectrum
    std::vector<Real> frame, windowedFrame, spectrum, frequencies, magnitudes, pcp;
    Real keyStrength, rmsValue, centroidValue, spectralFlatnessReal;
    
    std::string keyString, scaleString;

    
    String updateString;
    
    ThreadAudioRecorder (AudioThumbnail& thumbnailToUpdate)
    : Thread("mir_thread"), circularBuffer (2, 44100) , thumbnail (thumbnailToUpdate),
    sampleRate (0), nextSampleNum (0)
    {
        essentiaSetup();
    }
    
    void essentiaSetup()
    {
        essentia::init();
        
        AlgorithmFactory& factory = standard::AlgorithmFactory::instance();
        
        // instantiate all required algorithms
        fc   = factory.create("FrameCutter");
        fc->input("signal").set(essentiaSignal);
        fc->output("frame").set(frame);
        
        w  = factory.create("Windowing", "type", "blackmanharris62");
        w->input("frame").set(frame);
        w->output("frame").set(windowedFrame);
        
        spec = factory.create("Spectrum");
        spec->input("frame").set(windowedFrame);
        spec->output("spectrum").set(spectrum);
        
        spectralPeaks = factory.create("SpectralPeaks",
                                        "orderBy", "magnitude", "magnitudeThreshold", 1e-05,
                                        "minFrequency", 40, "maxFrequency", 5000, "maxPeaks", 10000);
        
        spectralPeaks->input("spectrum").set(spectrum);
        spectralPeaks->output("frequencies").set(frequencies);
        spectralPeaks->output("magnitudes").set(magnitudes);
        
        hpcp  = factory.create("HPCP");
        hpcp->input("frequencies").set(frequencies);
        hpcp->input("magnitudes").set(magnitudes);
        hpcp->output("hpcp").set(pcp);

        key = factory.create("Key");
        key->input("pcp").set(pcp);
        key->output("key").set(keyString);
        key->output("scale").set(scaleString);
        key->output("strength").set(keyStrength);
        
        rms = factory.create("RMS");
        rms->input("array").set(essentiaSignal);
        rms->output("rms").set(rmsValue);
        
        spectralFlatness = factory.create("FlatnessDB");
        spectralFlatness->input("array").set(spectrum);
        spectralFlatness->output("flatnessDB").set(spectralFlatnessReal);
        
        centroid = factory.create("Centroid", "range", 44100/2.0);
        centroid->input("array").set(spectrum);
        centroid->output("centroid").set(centroidValue);
    }
    
    ~ThreadAudioRecorder()
    {
        stop();
        
        delete fc;
        delete w;
        delete spec;
        delete spectralPeaks;
        delete hpcp;
        delete key;
        delete rms;
        delete spectralCentroid;
        delete spectralFlatness;
        delete centroid;
        
        essentia::shutdown();
    }
    
    //==============================================================================
    void startRecording (const File& file)
    {
        stop();
        
        if (sampleRate > 0)
        {
            if (sampleRate > 0)
            {
                startThread();
                
                circularBuffer.clear();
                recording = true;
            }
        }
    }
    
    void stop()
    {
//        // First, clear this pointer to stop the audio callback from using our writer object..
//        {
//            const ScopedLock sl (writerLock);
//            activeWriter = nullptr;
//        }
//        
//        // Now we can delete the writer object. It's done in this order because the deletion could
//        // take a little time while remaining data gets flushed to disk, so it's best to avoid blocking
//        // the audio callback while this happens.
//        threadedWriter = nullptr;
        
        recording = false;
        
//        stopThread(5000);
        
        stopThread(20000);
    }
    
    bool isRecording() const
    {
        return isThreadRunning() && recording;
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
    
    void audioDeviceIOCallback (const float** inputChannelData, int numInputChannels,
                                float** outputChannelData, int numOutputChannels,
                                int numSamples) override
    {
        
        if (recording)
        {
//            std::cout << numSamples << "\n";
            // Create an AudioSampleBuffer to wrap our incomming data, note that this does no allocations or copies, it simply references our input data
            const AudioSampleBuffer buffer (const_cast<float**> (inputChannelData), numInputChannels, numSamples);
            circularBuffer.addSamplesToBuffer (buffer, numSamples);
//            nextSampleNum += numSamples;
        }
        
        // We need to clear the output buffers, in case they're full of junk..
        for (int i = 0; i < numOutputChannels; ++i)
            if (outputChannelData[i] != nullptr)
                FloatVectorOperations::clear (outputChannelData[i], numSamples);
    }
    
    //Thread for Essentia processing
    void run()
    {
        AudioSampleBuffer tempBuffer (2, frameSize*2);
        while (! threadShouldExit())
        {
            int numSamplesReady = circularBuffer.readSamplesFromBuffer (tempBuffer, tempBuffer.getNumSamples());
            
            if (numSamplesReady > 0) {
                essentiaSignal = bufferToVector(tempBuffer);
                analyse();
            }
            
            Thread::sleep (1);
        }

    }
    
    void analyse()
    {
        fc->reset();
        w->reset();
        spec->reset();
        
        spectralPeaks->reset();
        hpcp->reset();
        key->reset();
        rms->reset();
        
        spectralFlatness->reset();
        centroid->reset();
        
        //Start the frame cutter
        while (true) {
            // compute a frame
            fc->compute();

            // if it was the last one (ie: it was empty), then we're done.
            if (!frame.size()) {
                break;
            }

            // if the frame is silent, just drop it and go on processing
            if (isSilent(frame)) continue;

            w->compute();
            spec->compute();
            spectralPeaks->compute();
            hpcp->compute();
            key->compute();
            spectralFlatness->compute();

        }
        
        std::cout << "SIGNAL\n";


        rms->compute();
        centroid->compute();
        
        sendChangeMessage();
    }
    
    std::vector<Real> bufferToVector(const AudioSampleBuffer& buffer)
    {
        //Convert buffer to Essentia vector and mono-ise as necessary
        std::vector<Real> essentiaSignal(buffer.getNumSamples());
        
        const float* leftChannelPtr = buffer.getReadPointer(0);
        const float* rightChannelPtr = NULL;
        
        if (buffer.getNumChannels() == 2)
            rightChannelPtr = buffer.getReadPointer(1);
        
        for(int i=0; i<buffer.getNumSamples(); i++) {
            essentiaSignal[i] = leftChannelPtr[i];
            if (buffer.getNumChannels() == 2) {
                essentiaSignal[i] = (leftChannelPtr[i] + rightChannelPtr[i]) / 2.0;
            }
        }
        
        return essentiaSignal;
    }
    
private:
    AudioThumbnail& thumbnail;
    double sampleRate;
    int64 nextSampleNum;
    
    bool recording;
    
    CriticalSection writerLock;
    
    CircularAudioBuffer circularBuffer;
};

//==============================================================================
/** A simple class that acts as an AudioIODeviceCallback and writes the
 incoming audio data to a WAV file.
 */
class InputAudioProcessor  : public AudioIODeviceCallback, public ChangeBroadcaster
{
public:
    vector<Real> signalBuffer, wavFile;
    int signalBufferCounter = 0;
    int signalBufferSize = 0;
    
    int frameSize = 2048;
    int hopSize = 1024;
    
    
    Algorithm* fc, *w, *spec, *mfcc;
    
    Algorithm* keyExtractor;
    
    std::string keyString, scaleString;
    
    String updateString;
    
    AudioSampleBuffer audioFileToSampleBuffer(const File audioFile)
    {
        //Read audio into buffer
        ScopedPointer<AudioFormatReader> reader;
        
        AudioFormatManager formatManager;
        formatManager.registerBasicFormats();
        
        reader = formatManager.createReaderFor(audioFile);
        
        AudioSampleBuffer buffer(reader->numChannels, reader->lengthInSamples);
        reader->read(&buffer, 0, reader->lengthInSamples, 0, true, true);
        
        return buffer;
    }
    
    std::vector<Real> audioFileToVector(const File audioFile)
    {
        AudioSampleBuffer buffer = audioFileToSampleBuffer(audioFile);
        
        //Convert buffer to Essentia vector and mono-ise as necessary
        std::vector<Real> essentiaSignal(buffer.getNumSamples());
        
        const float* leftChannelPtr = buffer.getReadPointer(0);
        const float* rightChannelPtr = NULL;
        
        if (buffer.getNumChannels() == 2)
            rightChannelPtr = buffer.getReadPointer(1);
        
        for(int i=0; i<buffer.getNumSamples(); i++) {
            essentiaSignal[i] = leftChannelPtr[i];
            if (buffer.getNumChannels() == 2) {
                essentiaSignal[i] = (leftChannelPtr[i] + rightChannelPtr[i]) / 2.0;
            }
        }
        
        return essentiaSignal;
    }
    
    std::vector<Real> bufferToVector(const AudioSampleBuffer& buffer)
    {
        //Convert buffer to Essentia vector and mono-ise as necessary
        std::vector<Real> essentiaSignal(buffer.getNumSamples());
        
        const float* leftChannelPtr = buffer.getReadPointer(0);
        const float* rightChannelPtr = NULL;
        
        if (buffer.getNumChannels() == 2)
            rightChannelPtr = buffer.getReadPointer(1);
        
        for(int i=0; i<buffer.getNumSamples(); i++) {
            essentiaSignal[i] = leftChannelPtr[i];
            if (buffer.getNumChannels() == 2) {
                essentiaSignal[i] = (leftChannelPtr[i] + rightChannelPtr[i]) / 2.0;
            }
        }
        
        return essentiaSignal;
    }
    
    
    InputAudioProcessor (AudioThumbnail& thumbnailToUpdate)
    : backgroundThread ("Audio Recorder Thread"), thumbnail (thumbnailToUpdate),  sampleRate (0), nextSampleNum (0)
    {
        essentia::init();
        
        AlgorithmFactory& factory = standard::AlgorithmFactory::instance();
        
        fc    = factory.create("FrameCutter",
                                          "frameSize", frameSize,
                                          "hopSize", hopSize);
        
        w     = factory.create("Windowing",
                                          "type", "hann");
        
        spec  = factory.create("Spectrum");
        
        mfcc  = factory.create("MFCC");
        
        keyExtractor = factory.create("KeyExtractor");
        
        keyString = "";
        scaleString = "";
        
//        wavFile = audioFileToVector(File("/Users/carthach/Desktop/C_MAJOR.wav"));
        
        backgroundThread.startThread();
    }
    
    ~InputAudioProcessor()
    {
        delete fc;
        delete w;
        delete spec;
        delete mfcc;
        
        delete keyExtractor;
        
        essentia::shutdown();
    }
    
    //==============================================================================
    void audioDeviceAboutToStart (AudioIODevice* device) override
    {
        sampleRate = device->getCurrentSampleRate();
        
        signalBufferSize = 2056;
        
        signalBuffer.resize(signalBufferSize, 0.0f);
    }
    
    void audioDeviceStopped() override
    {
        sampleRate = 0;
    }
    
    void audioDeviceIOCallback (const float** inputChannelData, int /*numInputChannels*/,
                                float** outputChannelData, int numOutputChannels,
                                int numSamples) override
    {                
        // Create an AudioSampleBuffer to wrap our incomming data, note that this does no allocations or copies, it simply references our input data
        const AudioSampleBuffer buffer (const_cast<float**> (inputChannelData), thumbnail.getNumChannels(), numSamples);
        thumbnail.addBlock (nextSampleNum, buffer, 0, numSamples);
        nextSampleNum += numSamples;
        
        for(int i=0; i<buffer.getNumSamples(); i++) {
                signalBuffer[signalBufferCounter] = inputChannelData[0][i];
                if (buffer.getNumChannels() == 2) {
                    signalBuffer[signalBufferCounter] = (inputChannelData[0][i] + inputChannelData[1][i]) / 2.0;
                }
            
                if(signalBufferCounter < signalBufferSize)
                    signalBufferCounter++;
                else {
                    signalBufferCounter = 0;
//                    analyse();
                }
            }
        
        // We need to clear the output buffers, in case they're full of junk..
        for (int i = 0; i < numOutputChannels; ++i)
            if (outputChannelData[i] != nullptr)
                FloatVectorOperations::clear (outputChannelData[i], numSamples);
    }
    
    void analyse()
    {
//        // FrameCutter -> Windowing -> Spectrum
//        std::vector<Real> frame, windowedFrame;
//        
//        fc->reset();
//        fc->input("signal").set(signalBuffer);
//        
//        fc->output("frame").set(frame);
//        w->input("frame").set(frame);
//        
//        w->output("frame").set(windowedFrame);
//        spec->input("frame").set(windowedFrame);
//        
//        // Spectrum -> MFCC
//        std::vector<Real> spectrum, mfccCoeffs, mfccBands;
//        
//        spec->output("spectrum").set(spectrum);
//        mfcc->input("spectrum").set(spectrum);
//        
//        mfcc->output("bands").set(mfccBands);
//        mfcc->output("mfcc").set(mfccCoeffs);
//        
//        //Reset the framewise algorithms
//        w->reset();
//        spec->reset();
//        mfcc->reset();
//        
//        //Start the frame cutter
//        while (true) {
//            // compute a frame
//            fc->compute();
//            
//            // if it was the last one (ie: it was empty), then we're done.
//            if (!frame.size()) {
//                break;
//            }
//            
//            // if the frame is silent, just drop it and go on processing
//            if (isSilent(frame)) continue;
//            
//            //Spectrum and MFCC
//            w->compute();
//            spec->compute();
//            
//            mfcc->compute();
//        }
        
        Real strength;
        
        keyExtractor->reset();
        
        keyExtractor->input("audio").set(signalBuffer);
        keyExtractor->output("key").set(keyString);
        keyExtractor->output("scale").set(scaleString);
        keyExtractor->output("strength").set(strength);
        
        keyExtractor->compute();
        
        updateString = keyString + " " + scaleString;
        
        sendChangeMessage();
        
        
    }
    
private:
    AudioThumbnail& thumbnail;
    TimeSliceThread backgroundThread; // the thread that will write our audio data to disk
    double sampleRate;
    int64 nextSampleNum;
    
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
        g.fillAll (Colours::darkgrey);
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
            g.setFont (14.0f);
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
                            private ChangeListener
{
public:
    AudioRecordingDemo(AudioDeviceManager* deviceManager)
        : recorder (recordingThumbnail.getAudioThumbnail())
    {
        setOpaque (true);
        addAndMakeVisible (liveAudioScroller);

        addAndMakeVisible (explanationLabel);
        explanationLabel.setText ("This page demonstrates how to record a wave file from the live audio input..\n\nPressing record will start recording a file in your \"Documents\" folder.", dontSendNotification);
        explanationLabel.setFont (Font (15.00f, Font::plain));
        explanationLabel.setJustificationType (Justification::topLeft);
        explanationLabel.setEditable (false, false, false);
        explanationLabel.setColour (TextEditor::textColourId, Colours::black);
        explanationLabel.setColour (TextEditor::backgroundColourId, Colour (0x00000000));

        addAndMakeVisible (recordButton);
        recordButton.setButtonText ("Record");
        recordButton.addListener (this);
        recordButton.setColour (TextButton::buttonColourId, Colour (0xffff5c5c));
        recordButton.setColour (TextButton::textColourOnId, Colours::black);

        addAndMakeVisible (recordingThumbnail);
        
        this->deviceManager = deviceManager;

        deviceManager->addAudioCallback (&liveAudioScroller);
        deviceManager->addAudioCallback (&recorder);
        
        juce::Logger *log = juce::Logger::getCurrentLogger();
        String message("Hello World!");
        log->writeToLog(message);
        
        recorder.addChangeListener(this);
    }

    ~AudioRecordingDemo()
    {
        deviceManager->removeAudioCallback (&recorder);
        deviceManager->removeAudioCallback (&liveAudioScroller);
    }

    void paint (Graphics& g) override
    {
        g.fillAll (Colours::green);
        
        g.setColour(Colours::black);
        g.drawFittedText (recorder.updateString, 50, 300, 150,50, Justification::left, 2);
        
    }

    void resized() override
    {
        Rectangle<int> area (getLocalBounds());
        liveAudioScroller.setBounds (area.removeFromTop (80).reduced (8));
        recordingThumbnail.setBounds (area.removeFromTop (80).reduced (8));
        recordButton.setBounds (area.removeFromTop (36).removeFromLeft (140).reduced (8));
        explanationLabel.setBounds (area.reduced (8));
    }

private:
    AudioDeviceManager* deviceManager;
    LiveScrollingAudioDisplay liveAudioScroller;
    RecordingThumbnail recordingThumbnail;
//    AudioRecorder recorder;
    
    ThreadAudioRecorder recorder;
    Label explanationLabel;
    TextButton recordButton;
    
    bool recording = false;

    void startRecording()
    {
//        const File file (File::getSpecialLocation (File::userDocumentsDirectory)
//                            .getNonexistentChildFile ("Juce Demo Audio Recording", ".wav"));
        
        const File file ("/sdcard/out.wav");
        
        recorder.startRecording (file);

        recordButton.setButtonText ("Stop");
        recordingThumbnail.setDisplayFullThumbnail (false);
        
        recording = true;
    }

    void stopRecording()
    {
        recorder.stop();
        recordButton.setButtonText ("Record");
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
    
    void changeListenerCallback (ChangeBroadcaster *source)
    {
        repaint();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioRecordingDemo)
};
