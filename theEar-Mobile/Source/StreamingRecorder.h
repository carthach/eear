#include <essentia/algorithmfactory.h>
#include <essentia/streaming/algorithms/poolstorage.h>
#include <essentia/scheduler/network.h>
#include "ringbufferinput.h"


//==============================================================================
/** A simple class that acts as an AudioIODeviceCallback and writes the
 incoming audio data to a WAV file.
 */
class StreamingRecorder  : public AudioIODeviceCallback, public Thread, public ChangeBroadcaster
{
    
public:
    essentia::streaming::Algorithm* ringBufferInput;
    essentia::streaming::RingBufferInput* ringBufferInputPtr;
    
    essentia::streaming::Algorithm *fc, *w, *spectrum, *spectralPeaks, *hpcpKey, *key, *mfcc;
    
    essentia::scheduler::Network* n;
    
    essentia::Pool pool, aggrPool;
    
    essentia::standard::Algorithm* aggr;
    
    String keyString;
    
    int frameSize = 4096;
    int hopSize = 2048;
    
    StreamingRecorder (AudioThumbnail& thumbnailToUpdate)
    : thumbnail (thumbnailToUpdate), Thread("ESSENTIA_THREAD"),
    sampleRate (0), nextSampleNum (0), recording(false)
    {
//        backgroundThread.startThread();
        
        setupEssentia();
    }
    
    void setupEssentia()
    {
        essentia::init();
        
        essentia::setDebugLevel(essentia::ENetwork);
        
        essentia::streaming::AlgorithmFactory& factory = essentia::streaming::AlgorithmFactory::instance();
        
        ringBufferInput = factory.create("RingBufferInput");
        ringBufferInput->configure("bufferSize", 44100);
        
        //A pointer to the Ring Buffer so we can access its goodies
        ringBufferInputPtr = static_cast<essentia::streaming::RingBufferInput*>(ringBufferInput);

        // instantiate all required algorithms
        fc = factory.create("FrameCutter");
        
//        // instantiate all required algorithms
//        fc = factory.create("FrameCutter",
//                       "frameSize", frameSize,
//                       "hopSize", hopSize,
//                       "silentFrames", "noise",
//                       "startFromZero", false);
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
//        stop();
        
        delete n;
    }
    
    //==============================================================================
    void startRecording (const File& file)
    {
        stop();
        
        if (sampleRate > 0)
        {
            n->reset();
            pool.clear();
            startThread();
//            if (fileStream != nullptr)
//            {
//                // Now create a WAV writer object that writes to our output stream...
//                WavAudioFormat wavFormat;
//                AudioFormatWriter* writer = wavFormat.createWriterFor (fileStream, sampleRate, 1, 16, StringPairArray(), 0);
//                
//                if (writer != nullptr)
//                {
//                    fileStream.release(); // (passes responsibility for deleting the stream to the writer object that is now using it)
//                    
//                    // Now we'll create one of these helper objects which will act as a FIFO buffer, and will
//                    // write the data to disk on our background thread.
//                    threadedWriter = new AudioFormatWriter::ThreadedWriter (writer, backgroundThread, 32768);
//                    
//                    // Reset our recording thumbnail
//                    thumbnail.reset (writer->getNumChannels(), writer->getSampleRate());
//                    nextSampleNum = 0;
//                    
//                    // And now, swap over our active writer pointer so that the audio callback will start using it..
//                    const ScopedLock sl (writerLock);
//                    activeWriter = threadedWriter;
//                }
//            }
            recording = true;
        }
    }
    
    void stop()
    {
        // First, clear this pointer to stop the audio callback from using our writer object..
        {
//            const ScopedLock sl (writerLock);
//            activeWriter = nullptr;
        }
        
        // Now we can delete the writer object. It's done in this order because the deletion could
        // take a little time while remaining data gets flushed to disk, so it's best to avoid blocking
        // the audio callback while this happens.
//        threadedWriter = nullptr;
        
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
    
    int bufferCount=0;
    
    void audioDeviceIOCallback (const float** inputChannelData, int /*numInputChannels*/,
                                float** outputChannelData, int numOutputChannels,
                                int numSamples) override
    {

        
        if(recording) {
//            const ScopedLock sl (writerLock);
            
            //Put the samples into the RingBuffer
            //According to ringbufferimpl.h Essentia should handle thread safety
            
            ringBufferInputPtr->add(const_cast<essentia::Real *> (inputChannelData[0]), numSamples);
        }
        
        lastNumSamples = numSamples;        
        
        //Old file writer stuff
//        if (activeWriter != nullptr)
//        {
//            activeWriter->write (inputChannelData, numSamples);
//            
//            // Create an AudioSampleBuffer to wrap our incomming data, note that this does no allocations or copies, it simply references our input data
//            const AudioSampleBuffer buffer (const_cast<float**> (inputChannelData), thumbnail.getNumChannels(), numSamples);
//            thumbnail.addBlock (nextSampleNum, buffer, 0, numSamples);
//            nextSampleNum += numSamples;
//        }
        
        // We need to clear the output buffers, in case they're full of junk..
        for (int i = 0; i < numOutputChannels; ++i)
            if (outputChannelData[i] != nullptr)
                FloatVectorOperations::clear (outputChannelData[i], numSamples);
    }
    
    void run()
    {
        int frameCount = 0;
        

        //This is the Essentia thread, you need to consume the RingBuffer and do your work
        while(!threadShouldExit())
        {
            {
                
//                const ScopedLock sl (writerLock);
                
                n->runStep();
                
                //Dirty filthy hack
                //If we've got X frames send a forced message to the key thing to stop and output the key
                if(frameCount % 8 == 0 && frameCount > 0) {
                    key->shouldStop(true);
                    key->process();
                    
                    if(pool.contains<std::string>("key"))
                        keyString = pool.value<std::string>("key") + " " + pool.value<std::string>("scale");
                    

                    pool.clear();
                    
                    if(frameCount % 32 == 0)
                        key->reset();                    
                }
                

                
                
                
                sendChangeMessage();
                
                //Notify of change
                
                frameCount++;
            }
            Thread::sleep (1);
        }
    }
    
    
    
private:
    AudioThumbnail& thumbnail;
    ScopedPointer<AudioFormatWriter::ThreadedWriter> threadedWriter; // the FIFO used to buffer the incoming data
    double sampleRate;
    int64 nextSampleNum;
               
    int lastNumSamples = 0;
    
    CriticalSection writerLock;

    bool recording = false;

};