/*
  ==============================================================================

    PadGrid.h
    Created: 7 Sep 2015 5:35:52pm
    Author:  Cárthach Ó Nuanáin

  ==============================================================================
*/

#ifndef PADGRID_H_INCLUDED
#define PADGRID_H_INCLUDED
//#include "InterfaceComponent.h"



class PadCell : public Component
{
public:
    Point<int> initialPosition;
    
    MidiKeyboardState& midiKeyboardState;
    int midiNote;
    bool noteDown;
    

    
    DropShadower dropShadower;
    PadCell(MidiKeyboardState& s, int midiNote) : midiKeyboardState(s), dropShadower(DropShadow(Colours::white,5, Point<int>(0,0)))
    {
        this->midiNote = midiNote;
        dropShadower.setOwner(this);
    }
    
    ~PadCell()
    {

    }
    
    void paint (Graphics& g) override
    {
        g.fillAll (Colours::darkgrey);   // clear the background
        
//        g.setColour(Colours::white);
        Colour textColour(135, 205, 222);
        g.setColour(textColour);
        g.drawText(String(midiNote), 5, 5, 20, 20, Justification::left);
    }
    
    void mouseDown	(	const MouseEvent & 	event	) override
    {
        midiKeyboardState.noteOn(1, midiNote, 100);
        noteDown = true;
    }
    
    void mouseUp	(	const MouseEvent & 	event	) override
    {
        midiKeyboardState.noteOff(1, midiNote, 100);
        noteDown = false;
    }
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PadCell)
};

//==============================================================================
/*
*/
class PadGrid    : public Component, public Timer, private MidiKeyboardStateListener
{
public:
    bool shouldCheckState = false;
    OwnedArray<PadCell> pads;
    DropShadower dropShadower;
//    DropShadow dropShadow;
    
        bool resetEvent = false;
    
    float horizontalOffset, verticalOffset;
    
    MidiKeyboardState& midiKeyboardState;
    
    PadGrid(MidiKeyboardState& s) : dropShadower(DropShadow(Colours::black,5, Point<int>(0,0))), midiKeyboardState(s)
    {
        dropShadower.setOwner(this);
        
        midiKeyboardState.addListener(this);
        
        int midiOffset = 36;
        
        int midiNote = midiOffset;
        for(int i=0; i<16; i++) {
            pads.add(new PadCell(midiKeyboardState, midiOffset+i));
            addAndMakeVisible(pads[i]);
        }
        
        startTimerHz(20);
    }

    ~PadGrid()
    {
        midiKeyboardState.removeListener(this);
    }
    
    void paint (Graphics& g) override
    {
        /* This demo code just fills the component's background and
           draws some placeholder text to get you started.

           You should replace everything in this method with your own
           drawing code..
        */
        

        g.fillAll (Colours::black.withAlpha(0.7f));   // clear the background

//        g.setColour (Colours::grey);
//        g.drawRect (getLocalBounds(), 1);   // draw an outline around the component
//
//        g.setColour (Colours::lightblue);
//        g.setFont (14.0f);
//        g.drawText ("PadGrid", getLocalBounds(),
//                    Justification::centred, true);   // draw some placeholder text
    }
    
    void handleNoteOn (MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override
    {
        shouldCheckState = true;
    }
    
    void handleNoteOff (MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override
    {
        shouldCheckState = true;
    }
    
    void timerCallback() override
    {
        int midiInChannelMask = 0xffff;
        if (shouldCheckState)
        {
            shouldCheckState = false;
            
            for (int i = 0; i < pads.size(); ++i) {
                if (midiKeyboardState.isNoteOnForChannels(0xffff, pads[i]->midiNote)) {
                    pads[i]->setTopLeftPosition(pads[i]->initialPosition.getX()+2, pads[i]->initialPosition.getY()+2);
                    
                    if(midiKeyboardState.isNoteOnForChannels(0xffff, 51))
                        resetEvent = true;
                        
                }
                else {
                    pads[i]->setTopLeftPosition(pads[i]->initialPosition.getX(), pads[i]->initialPosition.getY());
                    
                    resetEvent = false;
                }
            }
        }
    }

    void resized()
    {
        // This method is where you should set the bounds of any child
        // components that your component contains..
        
        for(int i=0; i<pads.size(); i++) {
            int cols = 4;
            // arrange balls in a grid
            
            float colWidth = getWidth() / 4.0; // get grid cell size
            float colHeight = getHeight() / cols; // get grid cell size
            
            float col = (i % cols);
            float row = 3 - (floor(i / cols));
            
            float posX = col * colWidth + colWidth/4.0;
            float posY = row * colHeight + colHeight/4.0;
            
            pads[i]->initialPosition = Point<int>(posX, posY);
            pads[i]->setCentrePosition(posX, posY);
            pads[i]->setSize(getWidth()/8.0, getWidth()/8.0);
        
        }
        
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PadGrid)
};


#endif  // PADGRID_H_INCLUDED
