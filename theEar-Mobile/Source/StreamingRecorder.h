#include <essentia/algorithmfactory.h>
#include <essentia/streaming/algorithms/poolstorage.h>
#include <essentia/scheduler/network.h>
#include <essentia/streaming/algorithms/ringbufferinput.h>

//==============================================================================
/** A simple class that acts as an AudioIODeviceCallback and writes the
 incoming audio data to a WAV file.
 */
class StreamingRecorder  : public AudioIODeviceCallback, public Thread, public ChangeBroadcaster
{
    
public:
    essentia::streaming::RingBufferInput* ringBufferInput;
    essentia::streaming::Algorithm *fc, *w, *spectrum, *spectralPeaks, *hpcpKey, *key, *mfcc, *rms, *spectralCentroid, *spectralFlatness;
    
    essentia::scheduler::Network* n;
    
    essentia::Pool pool, aggrPool;
    essentia::standard::Algorithm* aggr;
    
    String keyString, scaleString;
    essentia::Real rmsValue, spectralFlatnessValue, spectralCentroidValue;
    
    //As suggested by KeyExtractor
    int frameSize = 4096;
    int hopSize = 2048;
    
    int computeFrameCount = 8;
    
    StreamingRecorder () : Thread("ESSENTIA_THREAD"), sampleRate (0), recording(false)
    {
        setupEssentia();
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
        fc = factory.create("FrameCutter",
                       "frameSize", frameSize,
                       "hopSize", hopSize,
                       "silentFrames", "noise",
                       "startFromZero", false);
//
        // instantiate all required algorithms
        fc = factory.create("FrameCutter");
        
        w     = factory.create("Windowing", "type", "blackmanharris62");
        spectrum      = factory.create("Spectrum");
        spectralPeaks = factory.create("SpectralPeaks",
                                        "orderBy", "magnitude", "magnitudeThreshold", 1e-05,
                                        "minFrequency", 40, "maxFrequency", 5000, "maxPeaks", 10000);
        hpcpKey       = factory.create("HPCP");
        key           = factory.create("Key");
        key->configure();
        mfcc           = factory.create("MFCC");
        rms = factory.create("RMS");
        spectralFlatness = factory.create("FlatnessDB");
        spectralCentroid = factory.create("Centroid", "range", 44100.0/2.0);
        
        
        // Audio -> FrameCutter
        ringBufferInput->output("signal")    >>  fc->input("signal");
        
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
        
        const char* stats[] = { "mean"};
        
        aggr = essentia::standard::AlgorithmFactory::create("PoolAggregator",
                                                            "defaultStats", essentia::arrayToVector<std::string>(stats));
        aggr->input("input").set(pool);
        aggr->output("output").set(aggrPool);

        n = new essentia::scheduler::Network(ringBufferInput);
        n->runPrepare();
    }
    
    ~StreamingRecorder()
    {
        
        n->clear(); //This takes care of deleting the algorithms in the network...
        
        //But the network and aggr were allocated manually...
        delete n;
        delete aggr;
        
        essentia::shutdown();
        
        stop();
    }
    
    //==============================================================================
    void startRecording ()
    {
        stop();
        
        if (sampleRate > 0)
        {
            n->reset();
            pool.clear();
            aggrPool.clear();
            startThread();

            recording = true;
        }
    }
    
    void stop()
    {
        stopThread(1000);
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
    
    void audioDeviceIOCallback (const float** inputChannelData, int /*numInputChannels*/,
                                float** outputChannelData, int numOutputChannels,
                                int numSamples) override
    {
        if(recording) {
//            const ScopedLock sl (writerLock);
            
            //Put the samples into the RingBuffer
            //According to ringbufferimpl.h Essentia should handle thread safety...
            
            const AudioSampleBuffer buffer (const_cast<float**> (inputChannelData), 1, numSamples);
            
//            std::cout << buffer.getRMSLevel(0, 0, buffer.getNumSamples()) << "\n";
            
            ringBufferInput->add(const_cast<essentia::Real *> (inputChannelData[0]), numSamples);
        }
        
        // We need to clear the output buffers, in case they're full of junk..
        for (int i = 0; i < numOutputChannels; ++i)
            if (outputChannelData[i] != nullptr)
                FloatVectorOperations::clear (outputChannelData[i], numSamples);
    }
    
    void run()
    {
        int frameOutCount = 0;
        bool shouldClear = false;
        
        //This is the Essentia thread, you need to consume the RingBuffer and do your work
        while(!threadShouldExit())
        {
//                const ScopedLock sl (writerLock);
            
            n->runStep();
            
            //Dirty filthy hack
            //If we've got X frames send a forced message to the key thing to stop and output the key
            if(frameOutCount % computeFrameCount == 0 && computeFrameCount > 0) {
                key->shouldStop(true);
                key->process();
                
                if(pool.contains<std::string>("key")) {
                    keyString = pool.value<std::string>("key");
                    scaleString = pool.value<std::string>("scale");
                }
                
                aggr->compute();
                std::map<std::string, std::vector<essentia::Real>  > reals = pool.getRealPool();
                
                rmsValue = reals["rms"].back();
                spectralFlatnessValue = reals["spectralFlatness"].back();
                spectralCentroidValue = reals["spectralCentroid"].back();
                
                sendChangeMessage();
//                if(frameOutCount % 32 == 0)
//                    key->reset();
            }
            
            //Clear out the key algorithm
            if(frameOutCount % (computeFrameCount+1) == 0 && computeFrameCount > 0) {
//                key->reset();
                n->reset();
                aggrPool.clear();
                frameOutCount = 0;
            }
            
//            std::map<std::string, std::vector<essentia::Real>  > reals = pool.getRealPool();
//            
//            rmsValue = reals["rms"].back();
//            spectralFlatnessValue = reals["spectralFlatness"].back();
//            spectralCentroidValue = reals["spectralCentroid"].back();
        

            
            //Notify of change
            
            frameOutCount++;

//            sleep (1);
        }
    }
    
private:
//    AudioThumbnail& thumbnail;
    double sampleRate;
    bool recording = false;

};