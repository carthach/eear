#ifndef STANDARDRECORDER_H
#define STANDARDRECORDER_H

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
class StandardRecorder  : public AudioIODeviceCallback, public Thread, public ChangeBroadcaster
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
    
    StandardRecorder (AudioThumbnail& thumbnailToUpdate)
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
                                       "minFrequency", 500, "maxFrequency", 5000, "maxPeaks", 10000);
        
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
    
    ~StandardRecorder()
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

#endif