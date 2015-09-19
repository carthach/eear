/*
  ==============================================================================

    EssentiaExtractor.cpp
    Created: 6 May 2015 12:56:43pm
    Author:  Cárthach Ó Nuanáin

  ==============================================================================
*/

#include "EssentiaExtractor.h"


int sampleRate = 44100;
int frameSize = 2048;
int hopSize = 1024;

std::vector<float> EssentiaExtractor::hannWindow(int size){
    std::vector<float> window;
    for (int i = 0; i < size; i++) {
        double multiplier = 0.5 * (1.0 - cos(2.0*M_PI*(float)i/(float)size));
        window.push_back(multiplier);
    }
    return window;
}


EssentiaExtractor::EssentiaExtractor(AudioFormatManager* formatManager)
{
    //Call this globally
//    essentia::init();
    

        
    this->formatManager = formatManager;
    

}

EssentiaExtractor::~EssentiaExtractor()
{
//    delete rhythmExtractor;
}

Array<File> EssentiaExtractor::getAudioFiles(const File& audioFolder)
{
    Array<File> audioFiles;
    
    DirectoryIterator iter (audioFolder, false, "*.mp3;*.wav");
    
    while (iter.next())
    {
        File theFileItFound (iter.getFile());
        audioFiles.add(theFileItFound);
    }
    
    return audioFiles;
}

AudioSampleBuffer EssentiaExtractor::audioFileToSampleBuffer(const File audioFile)
{
    //Read audio into buffer
    ScopedPointer<AudioFormatReader> reader;
    
    reader = formatManager->createReaderFor(audioFile);
  
    AudioSampleBuffer buffer(reader->numChannels, reader->lengthInSamples);
    reader->read(&buffer, 0, reader->lengthInSamples, 0, true, true);
    
    return buffer;
}

std::vector<Real> EssentiaExtractor::audioFileToVector(const File audioFile)
{
    AudioSampleBuffer buffer = audioFileToSampleBuffer(audioFile);
    
    //Convert buffer to Essentia vector and mono-ise as necessary
    std::vector<Real> essentiaSignal(buffer.getNumSamples());
    
    const float* leftChannelPtr = buffer.getReadPointer(0);
    const float* rightChannelPtr = NULL;

    if (buffer.getNumChannels() == 2)
        rightChannelPtr = buffer.getReadPointer(1);
    
    for(int i=0; i<buffer.getNumSamples(); i++) {
        essentiaSignal[i] = leftChannelPtr[i];
        if (buffer.getNumChannels() == 2) {
            essentiaSignal[i] = (leftChannelPtr[i] + rightChannelPtr[i]) / 2.0;
        }
    }
    
    return essentiaSignal;
}


bool EssentiaExtractor::vectorToAudioFile(const vector<Real> signal, const String fileName)
{
    ScopedPointer<WavAudioFormat> wavFormat =  new WavAudioFormat();
    
    File output(fileName);
    FileOutputStream *fost = output.createOutputStream();
    
    AudioFormatWriter* writer = wavFormat->createWriterFor(fost, 44100, 1, 16, StringPairArray(), 0);
    
    const float* signalPtr[1];
    signalPtr[0] = &signal[0];
    
    writer->writeFromFloatArrays(signalPtr, 1, signal.size());
    delete writer;
    
    return true;
}

//vector<Real> EssentiaExtractor::extractOnsetTimes(const vector<Real>& audio)
//{
//    Algorithm* extractoronsetrate = AlgorithmFactory::create("OnsetRate");
//    
//    Real onsetRate;
//    vector<Real> onsets;
//    
//    extractoronsetrate->input("signal").set(audio);
//    extractoronsetrate->output("onsets").set(onsets);
//    extractoronsetrate->output("onsetRate").set(onsetRate);
//    
//    extractoronsetrate->compute();
//    
//    delete extractoronsetrate;
//    
//    return onsets;
//}

vector<Real> EssentiaExtractor::extractPeakValues(const vector<vector<Real> >& slices)
{
    vector<Real> peakValues(slices.size(), 0.0f);
    for(int i=0; i<slices.size(); i++) {
        for(int j=0; j<slices[i].size(); j++) {
            float absSample = fabs(slices[i][j]);
            if (absSample > peakValues[i]) {
                peakValues[i] = absSample;
            }
        }
    }
    return peakValues;
}

vector<Real> EssentiaExtractor::extractOnsetTimes(const vector<Real>& audio)
{
    Algorithm* extractoronsetrate = AlgorithmFactory::create("OnsetRate");
    
    Real onsetRate;
    vector<Real> onsets;
    
    extractoronsetrate->input("signal").set(audio);
    extractoronsetrate->output("onsets").set(onsets);
//    extractoronsetrate->configure("ratioThreshold", 12.0);
//    extractoronsetrate->configure("combine", 100.0);
    extractoronsetrate->output("onsetRate").set(onsetRate);
    
    extractoronsetrate->compute();
    
    delete extractoronsetrate;
    
    return onsets;
}

vector<Real> EssentiaExtractor::getGlobalFeatures(const vector<Real>& audio)
{
    Algorithm* rhythmExtractor = AlgorithmFactory::create("RhythmExtractor2013", "method", "degara");
    
    Real bpm ,confidence;
    vector<Real> ticks, estimates, bpmIntervals;
    
    rhythmExtractor->input("signal").set(audio);
    rhythmExtractor->output("bpm").set(bpm);
    rhythmExtractor->output("ticks").set(ticks);
    rhythmExtractor->output("estimates").set(estimates);
    rhythmExtractor->output("bpmIntervals").set(bpmIntervals);
    rhythmExtractor->output("confidence").set(confidence);
    
    rhythmExtractor->compute();
    
    vector<Real> blah;
    blah.push_back(bpm);
    
    delete rhythmExtractor;
    
    return blah;
}

//vector<Real> EssentiaExtractor::getGlobalFeatures(const vector<Real>& audio)
//{
//    Algorithm* bpmHistogram = AlgorithmFactory::create("RhythmDescriptors");
//    
//    Real bpm;
//    
//    vector<Real> vec;
//    
//    bpmHistogram->input("signal").set(audio);
//    bpmHistogram->output("bpm").set(bpm);
//    bpmHistogram->output("beats_position").set("");
//    
//    bpmHistogram->compute();
//
//    vec.push_back(bpm);
//    
//    
//    delete bpmHistogram;
//    
//    return vec;
//}

vector<vector<Real> > EssentiaExtractor::extractOnsets(const vector<Real>& onsetTimes, const vector<Real>& audio)
{
    vector<vector<Real> > slices;
    AlgorithmFactory& factory = standard::AlgorithmFactory::instance();
    
    float audioLength = (float)audio.size() / (float)sampleRate;
    
    vector<Real>::const_iterator first = onsetTimes.begin()+1;
    vector<Real>::const_iterator last = onsetTimes.end();
    vector<Real> endTimes(first, last);
    endTimes.push_back(audioLength);
    
    Algorithm* slicer = factory.create("Slicer",
                                       "endTimes", endTimes,
                                       "startTimes",
                                       onsetTimes);
    
    slicer->input("audio").set(audio);
    slicer->output("frame").set(slices);
    slicer->compute();
    
    delete slicer;
    
//    for(int i=0; i<slices.size();i++) {
//        std::vector<float> hann = hannWindow(slices[i].size());
//        for(int j=0; j<slices[i].size(); j++) {
//            if(j <= 256)
//                slices[i][j] = slices[i][j] * hann[j];
//            if(j >= (float)slices[i].size() / 4.0)
//                slices[i][j] = slices[i][j] * hann[j];
//        }
//    }
    
    return slices;
}

inline void EssentiaExtractor::writeOnsets(const vector<vector<Real> >& slices, const String outputRoot)
{
    
    for(int i=0; i<slices.size(); i++)
    {
        String fileName = outputRoot + "slice_" + std::to_string(sliceID++) + ".wav";
        vectorToAudioFile(slices[i], fileName);
    }
}

Pool EssentiaExtractor::loadDataset(const String& jsonFilename)
{
    Pool pool;
    
    //We get the overall pool, merge and output
    Algorithm* yamlInput  = AlgorithmFactory::create("YamlInput", "format", "json");
    yamlInput->configure("filename", jsonFilename.toStdString());
    yamlInput->output("pool").set(pool);
    yamlInput->compute();
    
    delete yamlInput;
    this->globalOnsetPool = pool;
    
    return pool;
}

void EssentiaExtractor::writeLoop(float onsetTime, const vector<Real>& audio, float BPM, String outFileName)
{
    float startTimeInSamples = onsetTime * 44100.0;
    
    float lengthOfBeatInSamples = (1.0 / BPM) * 60.0 * 44100.0;

    float endTimeInSamples = startTimeInSamples + (lengthOfBeatInSamples * 8.0);
    
    while(endTimeInSamples >= (audio.size() - endTimeInSamples))
        endTimeInSamples = startTimeInSamples + (lengthOfBeatInSamples * 8.0);        
    
    vector<Real>::const_iterator first = audio.begin() + (int)startTimeInSamples;
    vector<Real>::const_iterator last = audio.begin() + (int)endTimeInSamples;
    
    vector<Real> newVec(first, last);
    
    vectorToAudioFile(newVec, outFileName);
}

vector<Real> EssentiaExtractor::randomLoop(const vector<Real>& onsetTimes, const vector<Real>& audio, Real BPM, String outFilename)
{
    float lengthOfBeatInSamples = (1.0 / BPM) * 60.0 * 44100.0;
    
    vector<Real> newVec;

    int randomOnsetIndex;
    float startTimeInSamples;
    float endTimeInSamples;
    
    do  {
        int randomOnsetIndex = random.nextInt(onsetTimes.size());
        startTimeInSamples = onsetTimes[randomOnsetIndex] * 44100.0;
        endTimeInSamples = startTimeInSamples + (lengthOfBeatInSamples * 8.0);
        
    } while(endTimeInSamples > (audio.size()-endTimeInSamples));

    vector<Real>::const_iterator first = audio.begin() + (int)startTimeInSamples;
    vector<Real>::const_iterator last = audio.begin() + (int)endTimeInSamples;

    newVec = vector<Real>(first, last);

    vectorToAudioFile(newVec, outFilename);
        
    return newVec;
}

String EssentiaExtractor::buildDataset(const File& audioFolder, bool writeOnsets)
{
    sliceID = 0;
    
    Array<File> filesToProcess = getAudioFiles(audioFolder);
    
    String outputRoot = audioFolder.getFullPathName() + "/dataset/";
    File datasetDirectory(outputRoot);

    datasetDirectory.createDirectory();

    globalOnsetPool.clear();
    
    File fileNames(outputRoot + "filesProcessed.txt");
    
    for(int i=0; i<filesToProcess.size(); i++) {
        String currentAudioFileName = filesToProcess[i].getFileName();
        
        std::cout << "Processing file: " << currentAudioFileName << "\n";
        fileNames.appendText(filesToProcess[i].getFileName() + "\n");
        
        vector<Real> signal = audioFileToVector(filesToProcess[i]);
        
        Real BPM =  getGlobalFeatures(signal)[0];
        
        std::cout << BPM << "\n";
        
        //------Onset Processing
        
        //Slice
        vector<Real> onsetTimes = extractOnsetTimes(signal);
        vector<vector<Real> > onsetSlices = extractOnsets(onsetTimes, signal);
        
        
        vector<vector<Real> > loops;
        
        int noOfLoops = 2;
    
        for(int j=0; j<noOfLoops; j++) {
            loops.push_back(randomLoop(onsetTimes, signal, BPM, outputRoot + String((i*noOfLoops)+j) + "_" + currentAudioFileName +  "_loop_" + String(j) + ".wav"));
        }

        Pool onsetPool = extractFeatures(loops, BPM);
        globalOnsetPool.merge(onsetPool, "append");
        
//        writeLoop(onsetTimes[5], signal, BPM, outputRoot + "/testy.wav");
        
        
        //Write
//        if(writeOnsets)
//            this->writeOnsets(onsetSlices, outputRoot);
        
        //Add to pool
//        Pool onsetPool = extractFeatures(onsetSlices, BPM);
        
//        globalOnsetPool.merge(onsetPool, "append");
    }
    
    String jsonFilename = outputRoot + "dataset.json";
    
    //We get the overall pool, merge and output
    Algorithm* yamlOutput  = standard::AlgorithmFactory::create("YamlOutput", "format", "json", "writeVersion", false);
    yamlOutput->input("pool").set(globalOnsetPool);
    yamlOutput->configure("filename", jsonFilename.toStdString());
    yamlOutput->compute();
    
//    File jsonFile(jsonFilename);
//    String jsonFileText = jsonFile.loadFileAsString();
//    jsonFileText = "%YAML:1.0\n" + jsonFileText;
//    jsonFile.replaceWithText(jsonFileText);
    
//    cv::Mat erbHi = poolToMat(pool);      
    
//    cv::Mat poolMat = globalPoolToMat();
    
//    cv::Mat pcaOut = pcaReduce(poolMat, 3);
    return jsonFilename;
}

StringArray EssentiaExtractor::featuresInPool()
{
    StringArray featureList;
    
    std::map<std::string, std::vector<Real> > realFeatures = globalOnsetPool.getRealPool();
    std::map<std::string, std::vector< std::vector<Real> > > vectorFeatures = globalOnsetPool.getVectorRealPool();
    
    for(RealIterator iterator = realFeatures.begin(); iterator != realFeatures.end(); iterator++)
        featureList.add(iterator->first);
    
    for(VectorIterator iterator = vectorFeatures.begin(); iterator != vectorFeatures.end(); iterator++) {
        for(int i=0; i<iterator->second[0].size(); i++)
            featureList.add(iterator->first + "_" + String(i));
    }
    
    return featureList;
}

cv::Mat EssentiaExtractor::poolToMat(const Pool& pool)
{
    using namespace cv;
    
    std::map<std::string, std::vector<Real> > realFeatures = pool.getRealPool();
    std::map<std::string, std::vector< std::vector<Real> > > vectorFeatures = pool.getVectorRealPool();
    
    cv::Mat mat;
    
    //No features return empty mat
    if(realFeatures.empty() && vectorFeatures.empty())
        return mat;
    
    //Do real Features first
    int noOfFeatures = realFeatures.size();
    RealIterator realIterator = realFeatures.begin();
    int noOfInstances = realIterator->second.size();
    
    Mat realFeaturesMatrix(noOfInstances, noOfFeatures, DataType<float>::type);
    
    int i=0;
    for(; realIterator != realFeatures.end(); realIterator++) {
        for(int j=0; j<realIterator->second.size(); j++)
            realFeaturesMatrix.at<float>(j,i) = realIterator->second[j];
        i++;
    }
    
    //If there are no vectorFeatures we are done
    if(vectorFeatures.empty())
        return realFeaturesMatrix;
    
    //Vector Features
    
    VectorIterator vectorIterator;
    int noOfVectorFeatures = 0;
    
    //noOfVectorFeatures = the sum of the dimensions of each feature
    for(vectorIterator = vectorFeatures.begin(); vectorIterator != vectorFeatures.end(); vectorIterator++) {
        if(!vectorIterator->second.empty()) {
            noOfVectorFeatures += vectorIterator->second[0].size(); //Get the dimensionality from the first instance
            jassert(noOfInstances == vectorIterator->second.size()); //The number of instances should agree with the Reals
        }
    }
    
    Mat vectorFeaturesMatrix(noOfInstances, noOfVectorFeatures, DataType<float>::type);
    
    //Instances
    for(int i=0; i<noOfInstances;i++) {
        //Vector
        int rowCounter = 0;
        for(vectorIterator = vectorFeatures.begin(); vectorIterator != vectorFeatures.end(); vectorIterator++) {
            for(int j=0; j<vectorIterator->second[i].size(); j++) {
                vectorFeaturesMatrix.at<float>(i,rowCounter) = vectorIterator->second[i][j];
                rowCounter++;
            }

        }
    }
    
    //Concatenate
    hconcat(realFeaturesMatrix, vectorFeaturesMatrix, mat);
    
    return mat;
}

/* Use openCV and PCA to collapse the MFCCs to 2D points for visualisation */

cv::Mat EssentiaExtractor::pcaReduce(cv::Mat mat, int noOfDimensions)
{
    cv::Mat projection_result;
    
    cv::PCA pca(mat,cv::Mat(),CV_PCA_DATA_AS_ROW, noOfDimensions);
    
    pca.project(mat,projection_result);
    
    return projection_result;
}

/* Use openCV and PCA to collapse the MFCCs to 2D points for visualisation */

void EssentiaExtractor::normaliseFeatures(cv::Mat mat)
{
    for(int i=0; i <mat.cols; i++) {
        cv::normalize(mat.col(i), mat.col(i), 0, 1, cv::NORM_MINMAX, CV_32F);
    }
}

cv::Mat EssentiaExtractor::globalPoolToMat()
{
    return poolToMat(globalOnsetPool);
}

void EssentiaExtractor::readYamlToMatrix(const String& yamlFilename, const StringArray& featureList)
{
    using namespace cv;
    
    FileStorage::FileStorage fs2(yamlFilename.toStdString(), FileStorage::READ);
    
    FileNode features = fs2["erbHi"];
    
    std::cout << (float)features[0];
    
    
//    // first method: use (type) operator on FileNode.
//    int frameCount = (int)fs2["frameCount"];
//    
//    std::string date;
//    // second method: use FileNode::operator >>
//    fs2["calibrationDate"] >> date;
//    
//    Mat cameraMatrix2, distCoeffs2;
//    fs2["cameraMatrix"] >> cameraMatrix2;
//    fs2["distCoeffs"] >> distCoeffs2;
//    
//    cout << "frameCount: " << frameCount << endl
//    << "calibration date: " << date << endl
//    << "camera matrix: " << cameraMatrix2 << endl
//    << "distortion coeffs: " << distCoeffs2 << endl;
//    
//    FileNode features = fs2["features"];
//    FileNodeIterator it = features.begin(), it_end = features.end();
//    int idx = 0;
//    std::vector<uchar> lbpval;
//    
//    // iterate through a sequence using FileNodeIterator
//    for( ; it != it_end; ++it, idx++ )
//    {
//        cout << "feature #" << idx << ": ";
//        cout << "x=" << (int)(*it)["x"] << ", y=" << (int)(*it)["y"] << ", lbp: (";
//        // you can also easily read numerical arrays using FileNode >> std::vector operator.
//        (*it)["lbp"] >> lbpval;
//        for( int i = 0; i < (int)lbpval.size(); i++ )
//            cout << " " << (int)lbpval[i];
//        cout << ")" << endl;
//    }

    fs2.release();
}

//Put your extractor code here
Pool EssentiaExtractor::extractFeatures(vector<vector<Real> >& slices, Real BPM)
{
    //The 3 levels of pools
    Pool framePool, aggrPool, onsetPool;
    
    
    AlgorithmFactory& factory = standard::AlgorithmFactory::instance();
    
    Algorithm* fc    = factory.create("FrameCutter",
                                      "frameSize", frameSize,
                                      "hopSize", hopSize);
    
    Algorithm* w     = factory.create("Windowing",
                                      "type", "hann");
    
    Algorithm* spec  = factory.create("Spectrum");
    
    Algorithm* mfcc  = factory.create("MFCC");
    

    
    // FrameCutter -> Windowing -> Spectrum
    std::vector<Real> frame, windowedFrame;
    
    fc->output("frame").set(frame);
    w->input("frame").set(frame);
    
    w->output("frame").set(windowedFrame);
    spec->input("frame").set(windowedFrame);

    // Spectrum -> MFCC
    std::vector<Real> spectrum, mfccCoeffs, mfccBands;
    
    spec->output("spectrum").set(spectrum);
    mfcc->input("spectrum").set(spectrum);
    
    mfcc->output("bands").set(mfccBands);
    mfcc->output("mfcc").set(mfccCoeffs);
    
    // Spectrum -> MFCC
    vector<Real> bands;
    
    Algorithm* erbBands = factory.create("ERBBands");

    erbBands->input("spectrum").set(spectrum);
    erbBands->output("bands").set(bands);
    
    //Stastical things
    float halfSampleRate = (float)sampleRate / 2.0;
    
    //Spectral Centroid
    Algorithm* centroid = factory.create("Centroid",
                                         "range", halfSampleRate);
    Real spectralCentroid;
    centroid->input("array").set(spectrum);
    centroid->output("centroid").set(spectralCentroid);
    
    //MHD descriptors
    Real pitchReal, pitchConfidence;
    Algorithm* pitch = factory.create("PitchYinFFT");
    
    pitch->input("spectrum").set(spectrum);
    pitch->output("pitch").set(pitchReal);
    pitch->output("pitchConfidence").set(pitchConfidence);
    
    pitch->output("pitch").set(pitchReal);
    
    Algorithm* RMS = factory.create("RMS");
    
    Real spectralFlatnessReal;
    Algorithm* spectralFlatness = factory.create("FlatnessDB");
    spectralFlatness->input("array").set(spectrum);
    spectralFlatness->output("flatnessDB").set(spectralFlatnessReal);
    
    //Central Moments
    Algorithm* centralMoments = factory.create("CentralMoments",
                                               "range", halfSampleRate);
    vector<Real> moments;
    centralMoments->input("array").set(spectrum);
    centralMoments->output("centralMoments").set(moments);
    
    Algorithm* distShape = factory.create("DistributionShape");
    Real spread, skewness, kurtosis;
    distShape->input("centralMoments").set(moments);
    distShape->output("spread").set(spread);
    distShape->output("skewness").set(skewness);
    distShape->output("kurtosis").set(kurtosis);
    
    //Global stats (don't set input until in the loop)
    Algorithm* zcr = factory.create("ZeroCrossingRate");
    Real zcrReal;
    zcr->output("zeroCrossingRate").set(zcrReal);
    
    Algorithm* lat = factory.create("LogAttackTime");
    Real latReal;
    lat->output("logAttackTime").set(latReal);
    
    Algorithm* envelope = factory.create("Envelope");
    vector<Real> envelopeSignal;
    envelope->output("signal").set(envelopeSignal);
    
    Algorithm* tct = factory.create("TCToTotal");
    Real tctReal;
    tct->input("envelope").set(envelopeSignal);
    tct->output("TCToTotal").set(tctReal);
    
    //Aggregate Pool stats
    string defaultStats[] = {"mean", "var"};
    Algorithm* poolAggregator = factory.create("PoolAggregator",
                                               "defaultStats", arrayToVector<string>(defaultStats));
    poolAggregator->input("input").set(framePool);
    poolAggregator->output("output").set(aggrPool);
    
    //Yaml Output in JSON format
    Algorithm* yamlOutput  = factory.create("YamlOutput", "format", "yaml");
    
    //Loop through all the slices
    int localSliceID = 0;
    vector<vector<Real> >::iterator sliceIterator;
    
    for(sliceIterator = slices.begin(); sliceIterator != slices.end(); sliceIterator++)
    {
        //Reset the framecutter and set to the new input
        fc->reset();
        fc->input("signal").set(*sliceIterator);
        
        //Reset the framewise algorithms
        w->reset();
        spec->reset();
        mfcc->reset();
        centroid->reset();
        centralMoments->reset();
        distShape->reset();
        
        framePool.clear();
        
        //Reset MHD descriptors
        spectralFlatness->reset();
        pitch->reset();
        
        //Start the frame cutter
        while (true) {
            
            // compute a frame
            fc->compute();
            
            // if it was the last one (ie: it was empty), then we're done.
            if (!frame.size()) {
                break;
            }
            
            // if the frame is silent, just drop it and go on processing
            if (isSilent(frame)) continue;
            
            //Spectrum and MFCC
            w->compute();
            spec->compute();
            
//            //MFCC
//            mfcc->compute();
//            framePool.add("mfcc",mfccCoeffs);
            
            centroid->compute();
            
            framePool.add("spectral_centroid", spectralCentroid);
            
            erbBands->compute();
//            framePool.add("erbbands", bands);
            
            centralMoments->compute();
            distShape->compute();
            
            //MHD
            pitch->compute();
            framePool.add("pitch", pitchReal);
            
            spectralFlatness->compute();
            framePool.add("flatness", spectralFlatnessReal);
            
//            framePool.add("spectral_spread", spread);
//            framePool.add("spectral_skewness", skewness);
//            framePool.add("spectral_kurtosis", kurtosis);
        }
        
        //Time to aggregate
        aggrPool.clear();
        poolAggregator->reset();
        poolAggregator->compute();
        
        //For merging pools together the JSON entries need to be vectorised (using .add function)
        //aggrPool gets rid of this, so a fix is to remove and add again
        
        //Reals
        std::map< std::string, Real>  reals = aggrPool.getSingleRealPool();
        typedef std::map<std::string, Real>::iterator realsIterator;
        
        for(realsIterator iterator = reals.begin(); iterator != reals.end(); iterator++) {
            aggrPool.remove(iterator->first);
            aggrPool.add(iterator->first, iterator->second);
        }

        
//        //Compute and add the global features
//        zcr->reset();
//        zcr->input("signal").set(*sliceIterator);
//        zcr->compute();
//        
//        lat->reset();
//        lat->input("signal").set(*sliceIterator);
//        lat->compute();
//        
//        envelope->reset();
//        envelope->input("signal").set(*sliceIterator);
//        envelope->compute();
//        
//        tct->reset();
//        tct->compute();
//        
//        aggrPool.add("zcr", zcrReal);
//        aggrPool.add("lat", latReal);
//        aggrPool.add("tct", tctReal);

        Real rmsValue;
        RMS->reset();
        RMS->input("array").set(*sliceIterator);
        RMS->output("rms").set(rmsValue);
        RMS->compute();

        aggrPool.add("RMS", rmsValue);
        
        
        //Get the mean of the erbBands to get lo/mid/hi
        
        std::map< std::string, std::vector<Real > >  vectors = aggrPool.getRealPool();
        
        vector<Real> aggrBands = vectors["erbbands.mean"];
        
        Algorithm* mean = factory.create("Mean");
        
        
//        //=========== ERB STUFF =========
//        //Get erbLo
//        vector<Real>::const_iterator first = aggrBands.begin();
//        vector<Real>::const_iterator last = aggrBands.begin() + 7;
//        vector<Real> erbLoBands(first, last);
//        
//        Real erbLo;
//        mean->input("array").set(erbLoBands);
//        mean->output("mean").set(erbLo);
//        mean->compute();
//        aggrPool.add("erbLo", erbLo);
//        
//        //Get erbMid
//        first = aggrBands.begin()+7;
//        last = aggrBands.begin()+28;
//        vector<Real> erbMidBands(first, last);
//        
//        Real erbMid;
//        
//        mean->reset();
//        mean->input("array").set(erbMidBands);
//        mean->output("mean").set(erbMid);
//        mean->compute();
//        aggrPool.add("erbMid", erbMid);
//        
//        //Get erbHi
//        first = aggrBands.begin()+28;
//        last = aggrBands.end();
//        vector<Real> erbHiBands(first, last);
//        
//        Real erbHi;
//        
//        mean->reset();
//        mean->input("array").set(erbHiBands);
//        mean->output("mean").set(erbHi);
//        mean->compute();
//        aggrPool.add("erbHi", erbHi);
//        
//        //Remove the original full erbBand vectors
//        aggrPool.remove("erbbands.mean");
//        aggrPool.remove("erbbands.var");
        
//        //Remove/Add mfcc vector
//        vector<Real> aggrBandsMean = vectors["mfcc.mean"];
//        vector<Real> aggrBandsVar = vectors["mfcc.var"];
//        aggrPool.remove("mfcc.mean");
//        aggrPool.remove("mfcc.var");
//        aggrPool.add("mfcc.mean", aggrBandsMean);
//        aggrPool.add("mfcc.var", aggrBandsVar);
        
        aggrPool.add("BPM", BPM);
        
        //If you want to output individual aggregate pools
        if(outputAggrPool) {
            yamlOutput->reset();
            yamlOutput->input("pool").set(aggrPool);
            string fileName = "slice_" + std::to_string(localSliceID++) + ".yaml";
            yamlOutput->configure("filename", fileName);
            yamlOutput->compute();
        }

        //Finally do the merge to the overall onsetPool
        onsetPool.merge(aggrPool, "append");
    }
    
    return onsetPool;
}