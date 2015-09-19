//
//  SettingsComponent.h
//  eear-Plugin
//
//  Created by Cárthach Ó Nuanáin on 19/09/2015.
//
//

#ifndef eear_Plugin_SettingsComponent_h
#define eear_Plugin_SettingsComponent_h

class SettingsComponent : public Component {
    public SettingsComponent()
    {
        
    }
    
    void paint (Graphics& g)
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
    
};

#endif
