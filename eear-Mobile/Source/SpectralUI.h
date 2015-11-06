//
//  SpectralUI.h
//  theEar-Mobile
//
//  Created by martin hermant on 17/09/15.
//
//

#ifndef __theEar_Mobile__SpectralUI__
#define __theEar_Mobile__SpectralUI__

//#include "juce_gui_basics.h"



using namespace juce;
using namespace std;

class  SpectralUI  {
    
    
public:
    SpectralUI(){
        
        rms = -1;
        flatness = 1.;
        centroid = .5;
        bounds.setWidth( 100);
        bounds.setHeight(100);
        
        
    };
    
    
    bool needsUpdate(){
        return Time::currentTimeMillis()-lastRefreshTime> 100;
    }
    Path getPath(){
        lastRefreshTime = Time::currentTimeMillis();
        
        start.x=0;
        middle.x = .5;
        end.x = 1;
        
        
        
        start.y = ((1-rms) + (1-flatness))/2;
        middle.y =1-(rms/2 + 0.5 );
        middle.x = centroid;
        end.y =  ((1-rms) + (1-flatness))/2;
        
        start.y = jmax(jmin(1.0f,start.y),0.0f);
        middle.x = jmax(jmin(1.0f,middle.x),0.0f);
        middle.y = jmax(jmin(1.0f,middle.y),0.0f);
        end.y = jmax(jmin(1.0f,end.y),0.0f);
        
        
        start*=Point<float>(bounds.getWidth(),bounds.getHeight());
        start = start+ Point<float>(bounds.getX(),bounds.getY());
        middle*=Point<float>(bounds.getWidth(),bounds.getHeight());
        middle = middle+ Point<float>(bounds.getX(),bounds.getY());
        end*=Point<float>(bounds.getWidth(),bounds.getHeight());
        end = end+ Point<float>(bounds.getX(),bounds.getY());
        

        path.clear();
        path.startNewSubPath(bounds.getX(),bounds.getBottom());
        path.lineTo(start);
        float middleStrength = (middle.x-start.x)*0.3;
        path.cubicTo(Point<float>(middleStrength,start.y), Point<float>(middle.x-middleStrength,middle.y), middle);//
        middleStrength = (end.x - middle.x)*.3;
        path.cubicTo(Point<float>(middle.x+middleStrength,middle.y), Point<float>(end.x-middleStrength,end.y), end);//

        path.lineTo(bounds.getRight(),bounds.getBottom());
        path.closeSubPath();
        

        

        return path;
    };

    int64 lastRefreshTime ;
    
    Path path;
    Rectangle<int> bounds;
    Point<float> start,middle,end;
    
    float rms,flatness,centroid;
    
    
};

#endif /* defined(__theEar_Mobile__SpectralUI__) */
