/*
  ==============================================================================

    This file was auto-generated by the Jucer!

    It contains the basic startup code for a Juce application.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

AudioProcessor* JUCE_CALLTYPE createPluginFilter();


//==============================================================================
/** A demo synth sound that's just a basic sine wave.. */
class SineWaveSound : public SynthesiserSound
{
public:
    SineWaveSound() {}

    bool appliesToNote (int /*midiNoteNumber*/) override  { return true; }
    bool appliesToChannel (int /*midiChannel*/) override  { return true; }
};

//==============================================================================
/** A simple demo synth voice that just plays a sine wave.. */
class SineWaveVoice  : public SynthesiserVoice
{
public:
    SineWaveVoice()
        : angleDelta (0.0),
          tailOff (0.0)
    {
    }

    bool canPlaySound (SynthesiserSound* sound) override
    {
        return dynamic_cast<SineWaveSound*> (sound) != nullptr;
    }

    void startNote (int midiNoteNumber, float velocity,
                    SynthesiserSound* /*sound*/,
                    int /*currentPitchWheelPosition*/) override
    {
        currentAngle = 0.0;
        level = velocity * 0.15;
        tailOff = 0.0;

        double cyclesPerSecond = MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        double cyclesPerSample = cyclesPerSecond / getSampleRate();

        angleDelta = cyclesPerSample * 2.0 * double_Pi;
    }

    void stopNote (float /*velocity*/, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            // start a tail-off by setting this flag. The render callback will pick up on
            // this and do a fade out, calling clearCurrentNote() when it's finished.

            if (tailOff == 0.0) // we only need to begin a tail-off if it's not already doing so - the
                                // stopNote method could be called more than once.
                tailOff = 1.0;
        }
        else
        {
            // we're being told to stop playing immediately, so reset everything..

            clearCurrentNote();
            angleDelta = 0.0;
        }
    }

    void pitchWheelMoved (int /*newValue*/) override
    {
        // can't be bothered implementing this for the demo!
    }

    void controllerMoved (int /*controllerNumber*/, int /*newValue*/) override
    {
        // not interested in controllers in this case.
    }

    void renderNextBlock (AudioSampleBuffer& outputBuffer, int startSample, int numSamples) override
    {
        if (angleDelta != 0.0)
        {
            if (tailOff > 0)
            {
                while (--numSamples >= 0)
                {
                    const float currentSample = (float) (sin (currentAngle) * level * tailOff);

                    for (int i = outputBuffer.getNumChannels(); --i >= 0;)
                        outputBuffer.addSample (i, startSample, currentSample);

                    currentAngle += angleDelta;
                    ++startSample;

                    tailOff *= 0.99;

                    if (tailOff <= 0.005)
                    {
                        clearCurrentNote();

                        angleDelta = 0.0;
                        break;
                    }
                }
            }
            else
            {
                while (--numSamples >= 0)
                {
                    const float currentSample = (float) (sin (currentAngle) * level);

                    for (int i = outputBuffer.getNumChannels(); --i >= 0;)
                        outputBuffer.addSample (i, startSample, currentSample);

                    currentAngle += angleDelta;
                    ++startSample;
                }
            }
        }
    }

private:
    double currentAngle, angleDelta, level, tailOff;
};

class FloatParameter : public AudioProcessorParameter
{
public:

    FloatParameter (float defaultParameterValue, const String& paramName)
       : defaultValue (defaultParameterValue),
         value (defaultParameterValue),
         name (paramName)
    {
    }

    float getValue() const override
    {
        return value;
    }

    void setValue (float newValue) override
    {
        value = newValue;
    }

    float getDefaultValue() const override
    {
        return defaultValue;
    }

    String getName (int /* maximumStringLength */) const override
    {
        return name;
    }

    String getLabel() const override
    {
        return String();
    }

    float getValueForText (const String& text) const override
    {
        return text.getFloatValue();
    }

private:
    float defaultValue, value;
    String name;
};

const float defaultGain = 1.0f;
const float defaultDelay = 0.0f;

//==============================================================================
TheEarPluginAudioProcessor::TheEarPluginAudioProcessor()
: delayBuffer (2, 12000)
{
    // Set up our parameters. The base class will delete them for us.
    addParameter (gain  = new FloatParameter (defaultGain,  "Gain"));
    addParameter (delay = new FloatParameter (defaultDelay, "Delay"));

    lastUIWidth = 400;
    lastUIHeight = 200;

    lastPosInfo.resetToDefault();
    delayPosition = 0;

    // Add some voices to our synth, to play the sounds..
    for (int i = 4; --i >= 0;)
    {
        synth.addVoice (new SineWaveVoice());   // These voices will play our custom sine-wave sounds..
        synth.addVoice (new SamplerVoice());    // and these ones play the sampled sounds
    }
    
    // ..and add a sound for them to play...
//    setSynthSamples();
    
    formatManager.registerBasicFormats();
}

void TheEarPluginAudioProcessor::setUsingSineWaveSound()
{
    synth.clearSounds();
    synth.addSound (new SineWaveSound());
}

void TheEarPluginAudioProcessor::setSynthSamples(const Array<File>& listOfFiles)
{
    
//    ScopedPointer<AudioFormatReader> audioReader = (wavFormat.createReaderFor (new MemoryInputStream (BinaryData::cello_wav,
//                                                                                                    BinaryData::cello_wavSize,
//                                                                                                    false),
//                                                                             true));
    
    synth.clearSounds();
    
    int midiOffset = 36;
    
    for(int i=0; i< 15; i++) {
        int randomFileIndex = rand.nextInt(listOfFiles.size());
        
        std::cout << "here\n";
        
        ScopedPointer<AudioFormatReader> audioReader = formatManager.createReaderFor(listOfFiles[randomFileIndex]);
//        ScopedPointer<AudioFormatReader> audioReader = formatManager.createReaderFor(listOfFiles[0]);
        
        BigInteger midiRange;
        midiRange.setRange(midiOffset+i, 1, true);
        
        
        synth.addSound (new SamplerSound ("demo sound",
                                      *audioReader,
                                      midiRange,
                                      midiOffset+i,   // root midi note
                                      0.0,  // attack time
                                      0.1,  // release time
                                      10.0  // maximum sample length
                                      ));
    }
}

TheEarPluginAudioProcessor::~TheEarPluginAudioProcessor()
{

}

//==============================================================================
void TheEarPluginAudioProcessor::prepareToPlay (double newSampleRate, int /*samplesPerBlock*/)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    synth.setCurrentPlaybackSampleRate (newSampleRate);
    keyboardState.reset();
    delayBuffer.clear();
}

void TheEarPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    keyboardState.reset();
}

void TheEarPluginAudioProcessor::reset()
{
    // Use this method as the place to clear any delay lines, buffers, etc, as it
    // means there's been a break in the audio's continuity.
    delayBuffer.clear();
}

// quick-and-dirty function to format a bars/beats string
static String ppqToBarsBeatsString (double ppq, double /*lastBarPPQ*/, int numerator, int denominator)
{
    if (numerator == 0 || denominator == 0)
        return "1|1|0";
    
    const int ppqPerBar = (numerator * 4 / denominator);
    const double beats  = (fmod (ppq, ppqPerBar) / ppqPerBar) * numerator;
    
    const int bar    = ((int) ppq) / ppqPerBar + 1;
    const int beat   = ((int) beats) + 1;
    const int ticks  = ((int) (fmod (beats, 1.0) * 960.0 + 0.5));
    
    String s;
    s << bar << '|' << beat << '|' << ticks;
    return s;
}

int lastBeat;
MidiBuffer noteOnBuffer;

void TheEarPluginAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    // ask the host for the current time so we can display it...
    AudioPlayHead::CurrentPositionInfo newTime;
    
    if (getPlayHead() != nullptr && getPlayHead()->getCurrentPosition (newTime))
    {
        // Successfully got the current time from the host..
        lastPosInfo = newTime;
    }
    else
    {
        // If the host fails to fill-in the current time, we'll just clear it to a default..
        lastPosInfo.resetToDefault();
    }
    
    const int ppqPerBar = (newTime.timeSigNumerator * 4 / newTime.timeSigDenominator);
    const double beats  = (fmod (newTime.ppqPosition, ppqPerBar) / ppqPerBar) * newTime.timeSigNumerator;
    
    const int numSamples = buffer.getNumSamples();
    int channel, dp = 0;

    // Go through the incoming data, and apply our gain to it...
    for (channel = 0; channel < getNumInputChannels(); ++channel)
        buffer.applyGain (channel, 0, buffer.getNumSamples(), gain->getValue());
    
    
    MidiBuffer normalBuffer;
    
    //Get an iterator
    MidiBuffer::Iterator midiBufferIterator(midiMessages);
    
    for(int i=0; i<midiMessages.getNumEvents(); i++) {
        MidiMessage midiMessage;
        int samplePosition;
        midiBufferIterator.getNextEvent(midiMessage, samplePosition);
        
        if (midiMessage.isNoteOn()) {
            noteOnBuffer.addEvent(midiMessage, 0);
        }
        if(midiMessage.isNoteOff()) {
            noteOnBuffer.addEvent(midiMessage, samplePosition);
            normalBuffer.addEvent(midiMessage, samplePosition);
        }

    }
    
    double param, fractpart, intpart;
    
    fractpart = modf(newTime.ppqPosition, &intpart);
    
    bool newBeat = false;
    
    if(intpart != lastBeat) {
        newBeat = true;
    }
    
    lastBeat = intpart;
    
    
    
    // Now pass any incoming midi messages to our keyboard state object, and let it
    // add messages to the buffer if the user is clicking on the on-screen keys
    keyboardState.processNextMidiBuffer (midiMessages, 0, numSamples, true);

    // and now get the synth to process these midi events and generate its output.
    
    if(shouldQuantise) {
        if(newBeat) {
            synth.renderNextBlock (buffer, noteOnBuffer, 0, numSamples);
            noteOnBuffer.clear();
        }
        else
            synth.renderNextBlock (buffer, normalBuffer, 0, numSamples);
    } else {
        synth.renderNextBlock(buffer, midiMessages, 0, numSamples);
    }

    // Apply our delay effect to the new output..
    for (channel = 0; channel < getNumInputChannels(); ++channel)
    {
        float* channelData = buffer.getWritePointer (channel);
        float* delayData = delayBuffer.getWritePointer (jmin (channel, delayBuffer.getNumChannels() - 1));
        dp = delayPosition;

        for (int i = 0; i < numSamples; ++i)
        {
            const float in = channelData[i];
            channelData[i] += delayData[dp];
            delayData[dp] = (delayData[dp] + in) * delay->getValue();
            if (++dp >= delayBuffer.getNumSamples())
                dp = 0;
        }
    }

    delayPosition = dp;

    // In case we have more outputs than inputs, we'll clear any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    for (int i = getNumInputChannels(); i < getNumOutputChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
}

//==============================================================================
AudioProcessorEditor* TheEarPluginAudioProcessor::createEditor()
{
    return new TheEarPluginAudioProcessorEditor (*this);
}

//==============================================================================
void TheEarPluginAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // Here's an example of how you can use XML to make it easy and more robust:

    // Create an outer XML element..
    XmlElement xml ("MYPLUGINSETTINGS");

    // add some attributes to it..
    xml.setAttribute ("uiWidth", lastUIWidth);
    xml.setAttribute ("uiHeight", lastUIHeight);
    xml.setAttribute ("gain", gain->getValue());
    xml.setAttribute ("delay", delay->getValue());

    // then use this helper function to stuff it into the binary blob and return it..
    copyXmlToBinary (xml, destData);
}

void TheEarPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

    // This getXmlFromBinary() helper function retrieves our XML from the binary blob..
    ScopedPointer<XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr)
    {
        // make sure that it's actually our type of XML object..
        if (xmlState->hasTagName ("MYPLUGINSETTINGS"))
        {
            // ok, now pull out our parameters..
            lastUIWidth  = xmlState->getIntAttribute ("uiWidth", lastUIWidth);
            lastUIHeight = xmlState->getIntAttribute ("uiHeight", lastUIHeight);

            gain->setValue ((float) xmlState->getDoubleAttribute ("gain", gain->getValue()));
            delay->setValue ((float) xmlState->getDoubleAttribute ("delay", delay->getValue()));
        }
    }
}

const String TheEarPluginAudioProcessor::getInputChannelName (const int channelIndex) const
{
    return String (channelIndex + 1);
}

const String TheEarPluginAudioProcessor::getOutputChannelName (const int channelIndex) const
{
    return String (channelIndex + 1);
}

bool TheEarPluginAudioProcessor::isInputChannelStereoPair (int /*index*/) const
{
    return true;
}

bool TheEarPluginAudioProcessor::isOutputChannelStereoPair (int /*index*/) const
{
    return true;
}

bool TheEarPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool TheEarPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool TheEarPluginAudioProcessor::silenceInProducesSilenceOut() const
{
    return false;
}

double TheEarPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}



//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TheEarPluginAudioProcessor();
}
