#ifndef STREAMINGRECORDER_H
#define STREAMINGRECORDER_H

#include <essentia/algorithmfactory.h>
#include <essentia/streaming/algorithms/poolstorage.h>
#include <essentia/scheduler/network.h>
#include <essentia/streaming/algorithms/ringbufferinput.h>
#include <map>
#include <string>
#include <tuple>
#include <iostream>
#include <utility>

using namespace std;


//==============================================================================
/** A simple class that acts as an AudioIODeviceCallback and writes the
 incoming audio data to a WAV file.
 */
class StreamingRecorder  : public AudioIODeviceCallback, public Thread, public ChangeBroadcaster
{
    
public:
    essentia::streaming::RingBufferInput* ringBufferInput;
    essentia::streaming::Algorithm *fc, *w, *spectrum, *spectralPeaks, *hpcpKey, *key, *mfcc, *rms, *spectralCentroid, *spectralFlatness , *highPass;
    
    essentia::scheduler::Network* n;
    
    essentia::Pool pool;
    
    String keyString, scaleString;
    float keyStrength;
    essentia::Real rmsValue, spectralFlatnessValue, spectralCentroidValue;
    map< String,float > keyPoolMajor;
    map< String,float > keyPoolMinor;
    
    float rmsLevel = 0.0f;
    float rmsThreshold = 0.05f;
    //    alpha filter on rms values
    float rmsAlpha = 0.1;
    //As suggested by KeyExtractor
    int frameSize = 4096;
    int hopSize = 2048;
    
    int computeFrameCount = 15;
    int  sensitivity = 1;
    
    StreamingRecorder () : Thread("ESSENTIA_THREAD"), sampleRate (0), recording(false)
    {
        setupEssentia();
    }
    
    
    void clearKeyPool(map<String,float > & keyPool)
    {
        keyPool["A"] = 0;
        keyPool["A#"] = 0;
        keyPool["B"] = 0;
        keyPool["B#"] = 0;
        keyPool["C"] = 0;
        keyPool["C#"] = 0;
        keyPool["D"] = 0;
        keyPool["D#"] = 0;
        keyPool["E"] = 0;
        keyPool["E#"] = 0;
        keyPool["F"] = 0;
        keyPool["F#"] = 0;
        keyPool["G"] = 0;
        keyPool["G#"] = 0;
        
    }
    
    String getBestInPool(map<String,float > & keyPool)
    {
        String res;
        int max = 0;
        for(auto k:keyPool){
            if(k.second>max){
                max = k.second;
                res = k.first;
            }
        }
        
        return res;
    }
    
    
    
    
    void setupEssentia()
    {
        essentia::init();
        
        //        essentia::setDebugLevel(essentia::ENetwork);
        
        essentia::streaming::AlgorithmFactory& factory = essentia::streaming::AlgorithmFactory::instance();
        
        ringBufferInput = new essentia::streaming::RingBufferInput();
        ringBufferInput->declareParameters();
        ringBufferInput->essentia::Configurable::configure("bufferSize", 44100);
        
        //A pointer to the Ring Buffer so we can access its goodies
        //        ringBufferInputPtr = static_cast<essentia::streaming::RingBufferInput*>(ringBufferInput);
        
        // instantiate all required algorithms
        //        fc = factory.create("FrameCutter");
        
        // instantiate all required algorithms
        
        highPass =  factory.create("HighPass","cutoffFrequency",300,"sampleRate",44100);
        fc = factory.create("FrameCutter",
                            "frameSize", frameSize,
                            "hopSize", hopSize,
                            "silentFrames", "noise",
                            "startFromZero", false);
        //
        // instantiate all required algorithms
        //        fc = factory.create("FrameCutter");
        
        w     = factory.create("Windowing", "type", "blackmanharris62");
        spectrum      = factory.create("Spectrum");
        spectralPeaks = factory.create("SpectralPeaks",
                                       "orderBy", "magnitude", "magnitudeThreshold", 1e-08,
                                       "minFrequency", 40, "maxFrequency", 6000, "maxPeaks", 10000);
        hpcpKey       = factory.create("HPCP");
        key           = factory.create("Key");
        key->configure();
        mfcc           = factory.create("MFCC");
        rms = factory.create("RMS");
        spectralFlatness = factory.create("FlatnessDB");
        spectralCentroid = factory.create("Centroid", "range", 44100.0/2.0);
        
        
        // Audio -> FrameCutter
        ringBufferInput->output("signal")    >> highPass->input("signal");
        highPass->output("signal") >> fc->input("signal");
        
        // connect inner algorithms
        fc->output("frame")          >>  w->input("frame");
        w->output("frame")            >>  spectrum->input("frame");
        spectrum->output("spectrum")          >>  spectralPeaks->input("spectrum");
        spectralPeaks->output("magnitudes")   >>  hpcpKey->input("magnitudes");
        spectralPeaks->output("frequencies")  >>  hpcpKey->input("frequencies");
        hpcpKey->output("hpcp")               >>  key->input("pcp");
        
        hpcpKey->output("hpcp") >> PC(pool, "hpcp");
        
        spectrum->output("spectrum")  >>  mfcc->input("spectrum");
        
        mfcc->output("bands")     >>  essentia::streaming::NOWHERE;                   // we don't want the mel bands
        mfcc->output("mfcc")      >>  PC(pool, "mfcc");
        
        // attach output proxy(ies)
        key->output("key")       >>  PC(pool, "key");
        key->output("scale")     >>  PC(pool, "scale");
        key->output("strength")  >>  PC(pool, "strength");
        
        w->output("frame") >> rms->input("array");
        rms->output("rms") >> PC(pool, "rms");
        
        spectrum->output("spectrum") >> spectralFlatness->input("array");
        spectralFlatness->output("flatnessDB") >> PC(pool, "spectralFlatness");
        
        
        spectrum->output("spectrum") >> spectralCentroid->input("array");
        spectralCentroid->output("centroid") >> PC(pool, "spectralCentroid");
        
        
        n = new essentia::scheduler::Network(ringBufferInput);
        n->runPrepare();
        startThread();
    }
    
    ~StreamingRecorder()
    {
        
        // thread is waiting conditionBuffer from ringBufInput...
        ringBufferInput->shouldStop(true);
        vector<float> dumb(2*frameSize);
        ringBufferInput->add(&dumb[0], 2*frameSize);
        stopThread(1000);
        
        
        n->clear(); //This takes care of deleting the algorithms in the network...
        
        //But the network and aggr were allocated manually...
        delete n;
        //        delete aggr;
        
        essentia::shutdown();
        
        
    }
    
    //==============================================================================
    void startRecording ()
    {
        
        if (sampleRate > 0)
        {
            n->reset();
            pool.clear();
            clearKeyPool(keyPoolMajor);
            clearKeyPool(keyPoolMinor);
            
            //            aggrPool.clear();
            
            recording = true;
        }
    }
    
    void stop()
    {
        recording = false;
        
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

void audioDeviceIOCallback (const float** inputChannelData, int numIn/*numInputChannels*/,
                            float** outputChannelData, int numOutputChannels,
                            int numSamples) override
{
if(recording) {
    //            const ScopedLock sl (writerLock);
    
    //Put the samples into the RingBuffer
    //According to ringbufferimpl.h Essentia should handle thread safety...
    
    AudioSampleBuffer buffer (const_cast<float**> (inputChannelData), 1, numSamples);
    
    rmsLevel = rmsAlpha * rmsLevel + (1-rmsAlpha)*buffer.getRMSLevel(0, 0, buffer.getNumSamples());
    
    //            std::cout << buffer.getRMSLevel(0, 0, buffer.getNumSamples()) << "\n";
    
    ringBufferInput->add(const_cast<essentia::Real *> (inputChannelData[0]), numSamples);
}

// We need to clear the output buffers, in case they're full of junk..
for (int i = 0; i < numOutputChannels; ++i)
if (outputChannelData[i] != nullptr)
FloatVectorOperations::clear (outputChannelData[i], numSamples);
}

void run() override
{

int frameOutCount = 0;
bool inited = false;

//This is the Essentia thread, you need to consume the RingBuffer and do your work
while(isThreadRunning() && !threadShouldExit())
{
    //                const ScopedLock sl (writerLock);
    
    n->runStep();
    frameOutCount++;
    //Dirty filthy hack
    //If we've got X frames send a forced message to the key thing to stop and output the key
    
    
    if(frameOutCount % computeFrameCount == 0 && computeFrameCount > 0  ){//&& rmsLevel >= rmsThreshold) {
        
        key->shouldStop(true);
        bool enoughFramesForHPCP = true;
        try{
            key->process();
        }
        catch(essentia::EssentiaException e){
            DBG(e.what());
            enoughFramesForHPCP = false;
        }
        
        if(enoughFramesForHPCP){
            n->runStep();
            if(pool.contains<std::string>("key"))
            {
                keyString = pool.value<std::string>("key");
                scaleString = (pool.value<std::string>("scale")=="minor"?"m":"M");
                keyStrength = pool.value<essentia::Real>("strength");
                
            }
            else{
                keyStrength = 0;
            }
            if(rmsLevel<0.005){
                keyStrength = 0;
            }
            
            
            std::map<std::string, std::vector<essentia::Real>  > reals = pool.getRealPool();
            
            rmsValue = reals["rms"].back();
            spectralFlatnessValue = reals["spectralFlatness"].back();
            spectralCentroidValue = reals["spectralCentroid"].back();
            
            // filter median
            
            for(auto & k:keyPoolMajor){
                k.second = jmin(40.0f,jmax(0.0f,k.second*0.9f -1));
            }
            for(auto & k:keyPoolMinor){
                k.second = jmin(40.0f,jmax(0.0f,k.second*0.9f -1));
            }
            if(scaleString=="m"){keyPoolMinor[keyString ]+=sensitivity*keyStrength;}
            else{keyPoolMajor[keyString ]+=sensitivity*keyStrength;}
            String minorBest = getBestInPool(keyPoolMinor);
            String majorBest = getBestInPool(keyPoolMajor);
            if(keyPoolMinor[minorBest]>keyPoolMajor[majorBest]){
                scaleString = "m";
                keyString = minorBest;
            }
            else{
                keyString = majorBest;
                scaleString = "M";
            }
            
            
            //
            
            
            
        }
        
        sendChangeMessage();
        
        frameOutCount = 0;
        key->reset();
        key->shouldStop(false);
    }
    
    //                if(frameOutCount % 32 == 0)
    //                    key->reset();
}



}


private:

vector<int> oldKeyValues;
vector<float> oldStrengths;


//    AudioThumbnail& thumbnail;
double sampleRate;
bool recording = false;

};

#endif