/*
  ==============================================================================

    EssentiaExtractor.h
    Created: 6 May 2015 12:56:43pm
    Author:  Cárthach Ó Nuanáin

  ==============================================================================
*/

#ifndef ESSENTIAEXTRACTOR_H_INCLUDED
#define ESSENTIAEXTRACTOR_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include "MyIncludes.h"

using namespace std;
using namespace essentia;
using namespace essentia::standard;

class EssentiaExtractor {
public:
    EssentiaExtractor(AudioFormatManager* formatManager);
    ~EssentiaExtractor();
    
    AudioFormatManager* formatManager; // for reading files
    
    //Folder loading
    Array<File> getAudioFiles(const File& audioFolder);
    
    //Audio Loading
    AudioSampleBuffer audioFileToSampleBuffer(const File audioFile);
    vector<Real> audioFileToVector(const File audioFile);
    bool vectorToAudioFile(const vector<Real> signal, const String fileName);
    
    //Handling onsets
    vector<Real> extractOnsetTimes(const vector<Real>& audio);
    vector<vector<Real> > extractOnsets(const vector<Real>& onsetTimes, const vector<Real>& audio);
    vector<vector<Real> > extractOnsets(const vector<Real>& onsetTimes, const vector<Real>& audio, int numOnsets);
    
    vector<Real> extractPeakValues(const vector<vector<Real> >& slices);
    void writeOnsets(const vector<vector<Real> >& slices, const String outputRoot);
    
    //Extract features for a vector of onsets
    Pool extractFeatures(vector<vector<Real> >& slices, Real BPM);
    
    //Build a whole dataset and output a json/yaml file
    String buildDataset(const File& audioFolder, bool writeOnsets);
    Pool loadDataset(const String& jsonFileName);
    
    vector<Real> getGlobalFeatures(const vector<Real>& audio);
    
    //Conversion to OpenCV matrices
    void readYamlToMatrix(const String& yamlFileName, const StringArray& featureList);
    cv::Mat poolToMat(const Pool& pool);
    cv::Mat globalPoolToMat();
    cv::Mat pcaReduce(cv::Mat mat, int noOfDimensions);
    void normaliseFeatures(cv::Mat mat); //in-place
    
    int sliceID = 0;
    
    bool outputAggrPool = false;
    Pool globalOnsetPool;
    
    typedef std::map<std::string, std::vector<Real> >::iterator RealIterator;
    typedef std::map<std::string, std::vector< std::vector<Real> > >::iterator VectorIterator;
    
    StringArray featuresInPool();
    
    std::vector<float> hannWindow(int size);
    
    //STUFF
    void writeLoop(float onsetTime, const vector<Real>& audio, float BPM, String outFileName);
    
    vector<Real> randomLoop(const vector<Real>& onsetTimes, const vector<Real>& audio, Real BPM, String outFilename);
    
    Random random;
};


#endif  // ESSENTIAEXTRACTOR_H_INCLUDED
