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

#ifndef JUCE_SEEKSAMPLER_H_INCLUDED
#define JUCE_SEEKSAMPLER_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"


//==============================================================================
/**
    A subclass of SynthesiserSound that represents a sampled audio clip.

    This is a pretty basic sampler, and just attempts to load the whole audio stream
    into memory.

    To use it, create a Synthesiser, add some SeekSamplerVoice objects to it, then
    give it some SampledSound objects to play.

    @see SeekSamplerVoice, Synthesiser, SynthesiserSound
*/
class JUCE_API  SeekSamplerSound    : public SynthesiserSound
{
public:
    //==============================================================================
    /** Creates a sampled sound from an audio reader.

        This will attempt to load the audio from the source into memory and store
        it in this object.

        @param name         a name for the sample
        @param source       the audio to load. This object can be safely deleted by the
                            caller after this constructor returns
        @param midiNotes    the set of midi keys that this sound should be played on. This
                            is used by the SynthesiserSound::appliesToNote() method
        @param midiNoteForNormalPitch   the midi note at which the sample should be played
                                        with its natural rate. All other notes will be pitched
                                        up or down relative to this one
        @param attackTimeSecs   the attack (fade-in) time, in seconds
        @param releaseTimeSecs  the decay (fade-out) time, in seconds
        @param maxSampleLengthSeconds   a maximum length of audio to read from the audio
                                        source, in seconds
    */
    SeekSamplerSound (const String& name,
                  AudioFormatReader& source,
                  const BigInteger& midiNotes,
                  int midiNoteForNormalPitch,
                  double attackTimeSecs,
                  double releaseTimeSecs,
                  double maxSampleLengthSeconds,
    float sampleBPM,
    float trackBPM
                            /*std::vector<double> onsetTimesInSamples */
                      );

    /** Destructor. */
    ~SeekSamplerSound();

    //==============================================================================
    /** Returns the sample's name */
    const String& getName() const noexcept                  { return name; }

    /** Returns the audio sample data.
        This could return nullptr if there was a problem loading the data.
    */
    AudioSampleBuffer* getAudioData() const noexcept        { return data; }


    //==============================================================================
    bool appliesToNote (int midiNoteNumber) override;
    bool appliesToChannel (int midiChannel) override;
    
    double startSample;
    
    std::vector<double> onsetTimesInSamples;
    bool canPlay = true;
    
    void setAttack(const double attackTimeSecs);
    void setRelease(const double releaseTimeSecs);

    double trackBPM;

private:
    //==============================================================================
    friend class SeekSamplerVoice;

    String name;
    ScopedPointer<AudioSampleBuffer> data;
    double sourceSampleRate;
    BigInteger midiNotes;
    int length, attackSamples, releaseSamples;
    int midiRootNote;
    
    double sampleBPM;

    JUCE_LEAK_DETECTOR (SeekSamplerSound)
};


//==============================================================================
/**
    A subclass of SynthesiserVoice that can play a SeekSamplerSound.

    To use it, create a Synthesiser, add some SeekSamplerVoice objects to it, then
    give it some SampledSound objects to play.

    @see SeekSamplerSound, Synthesiser, SynthesiserVoice
*/
class JUCE_API  SeekSamplerVoice    : public SynthesiserVoice
{
public:
    //==============================================================================
    /** Creates a SeekSamplerVoice. */
    SeekSamplerVoice();

    /** Destructor. */
    ~SeekSamplerVoice();

    //==============================================================================
    bool canPlaySound (SynthesiserSound*) override;

    void startNote (int midiNoteNumber, float velocity, SynthesiserSound*, int pitchWheel) override;
    void stopNote (float velocity, bool allowTailOff) override;

    void pitchWheelMoved (int newValue) override;
    void controllerMoved (int controllerNumber, int newValue) override;

    void renderNextBlock (AudioSampleBuffer&, int startSample, int numSamples) override;


private:
    //==============================================================================
    double pitchRatio;
    double sourceSamplePosition;
    float lgain, rgain, attackReleaseLevel, attackDelta, releaseDelta;
    bool isInAttack, isInRelease;

    JUCE_LEAK_DETECTOR (SeekSamplerVoice)
};


#endif   // JUCE_SAMPLER_H_INCLUDED
