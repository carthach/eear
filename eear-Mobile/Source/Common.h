//
//  Common.h
//  
//
//  Created by Martin Hermant on 19/09/2015.
//
//

#ifndef _Common_h
#define _Common_h
//#include "juce_core.h"
using namespace juce;


namespace eear{
    class Colour{
    public:
        static juce::Colour back(){return  juce::Colour::fromRGB(21, 24, 27);};
        static juce::Colour text(){return  juce::Colour::fromRGB(119,195,214);};
    };
}
#endif
