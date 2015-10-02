//
//  DataTab.cpp
//  sliceAnalyser
//
//  Created by Cárthach Ó Nuanáin on 07/05/2015.
//
//

#ifndef DATATAB_INCLUDED
#define DATATAB_INCLUDED

#ifndef JUCE_USE_MP3AUDIOFORMAT
#define JUCE_USE_MP3AUDIOFORMAT 1
#endif

#include "../JuceLibraryCode/JuceHeader.h"
#include "EssentiaExtractor.h"
#include "DataTableComponent.h"

//==============================================================================
struct ListBoxContents  : public ListBoxModel
{
    // The following methods implement the necessary virtual functions from ListBoxModel,
    // telling the listbox how many rows there are, painting them, etc.
    int getNumRows() override
    {
        return visibleData.size();
    }
    
    void paintListBoxItem (int rowNumber, Graphics& g,
                           int width, int height, bool rowIsSelected) override
    {
        if (rowIsSelected)
            g.fillAll (Colours::lightblue);
        
        g.setColour (Colours::black);
        g.setFont (height * 0.7f);
        
//        int selectedIndex = selectedData[rowNumber];
        
        g.drawText (data[visibleData[rowNumber]],
                    5, 0, width, height,
                    Justification::centredLeft, true);
    }
    
    var getDragSourceDescription (const SparseSet<int>& selectedRows) override
    {
        // for our drag description, we'll just make a comma-separated list of the selected row
        // numbers - this will be picked up by the drag target and displayed in its box.
        StringArray rows;
        
        for (int i = 0; i < selectedRows.size(); ++i)
            rows.add (String (selectedRows[i] + 1));
        
        return rows.joinIntoString (", ");
    }
    
public:
    StringArray data;
    Array<int> visibleData;
};

class DataComponent : public Component,
                public Button::Listener,
                public FileDragAndDropTarget
{
public:
    cv::Mat featureMatrix;
    PropertiesFile* propertiesFile;
    ApplicationProperties applicationProperties;
    File datasetFolder;
    TextEditor datasetFolderTextBox;
    
    ListBox sourceListBox, destinationListBox;
    ListBoxContents sourceModel, destinationModel;
    TextButton selectionLeftButton, allLeftButton, selectionRightButton, allRightButton, updateFeaturesButton, outputLoopButton;
    
    DataTableComponent tableComponent;
    InterfaceComponent* interfaceComponent;

  
    DataComponent(InterfaceComponent* interfaceComponent) : extractor(&formatManager)
    {
        formatManager.registerBasicFormats();
        this->interfaceComponent = interfaceComponent;
        
        String formats = formatManager.getWildcardForAllFormats();
        
//        formatManager.registerFormat(&coreAudioFormat, false);
        
        addAndMakeVisible (fileLoadButton);
        fileLoadButton.setButtonText ("Load Folder");
        fileLoadButton.addListener(this);
        fileLoadButton.setColour (TextButton::buttonColourId, Colour (0xff79ed7f));
        
        addAndMakeVisible (buildDatasetButton);
        buildDatasetButton.setButtonText ("Build Dataset");
        buildDatasetButton.addListener(this);
        buildDatasetButton.setColour (TextButton::buttonColourId, Colour (0xff79ed7f));
        
        addAndMakeVisible (outputLoopButton);
        outputLoopButton.setButtonText ("Output Loop");
        outputLoopButton.addListener(this);
        outputLoopButton.setColour (TextButton::buttonColourId, Colour (0xff79ed7f));
        
        sourceListBox.setModel(&sourceModel);
        sourceListBox.setMultipleSelectionEnabled (true);        
        addAndMakeVisible (sourceListBox);
        
        destinationListBox.setModel(&destinationModel);
        destinationListBox.setMultipleSelectionEnabled (true);
        addAndMakeVisible (destinationListBox);
        
        //Buttons
        addAndMakeVisible(selectionLeftButton);
        selectionLeftButton.setButtonText("<");
        selectionLeftButton.addListener(this);
        
        addAndMakeVisible(allLeftButton);
        allLeftButton.setButtonText("<<");
        allLeftButton.addListener(this);
        
        addAndMakeVisible(selectionRightButton);
        selectionRightButton.setButtonText(">");
        selectionRightButton.addListener(this);
        
        addAndMakeVisible(allRightButton);
        allRightButton.setButtonText(">>");
        allRightButton.addListener(this);
        
        addAndMakeVisible(updateFeaturesButton);
        updateFeaturesButton.setButtonText("Update");
        updateFeaturesButton.addListener(this);
    
        PropertiesFile::Options options;
        options.applicationName = "sliceAnalyser";
        options.osxLibrarySubFolder = "Application Support/sliceAnalyser";
        applicationProperties.setStorageParameters(options);
        
        propertiesFile = applicationProperties.getUserSettings();

        if(propertiesFile->containsKey("lastDatasetFolder")) {
            datasetFolder = propertiesFile->getValue("lastDatasetFolder");
            interfaceComponent->datasetPath = datasetFolder.getFullPathName();
            String lastDatasetFile = datasetFolder.getFullPathName() + "/dataset/dataset.json";
        
            if(File(lastDatasetFile).existsAsFile()) {
                loadDataset(lastDatasetFile);
            }
        }
        
        // Create an address box..
        addAndMakeVisible (datasetFolderTextBox);
//        datasetFolderTextBox.setTextToShowWhenEmpty ("Enter a web address, e.g. http://www.juce.com", Colours::grey);
//        datasetFolderTextBox.addListener (this);
        
        datasetFolderTextBox.setText(datasetFolder.getFullPathName());
        
        addAndMakeVisible(tableComponent);
    }
    
    void resized () override
    {
        Rectangle<int> bottom = getLocalBounds();
        Rectangle<int> top = bottom.removeFromTop(400);
        
//        Rectangle<int> topLeft = top.removeFromLeft(top.getWidth()/8);
//        fileLoadButton.setBounds(topLeft);
//        
//        Rectangle<int> topLeftNext = topLeft.withX(topLeft.getX()+topLeft.getWidth());
//        buildDatasetButton.setBounds(topLeftNext);
        
        fileLoadButton.setBounds (10, 5, 100, 50);
        buildDatasetButton.setBounds (110, 5, 100, 50);
        datasetFolderTextBox.setBounds (226, 16, 300, 25);

        outputLoopButton.setBounds (550, 5, 100, 50);
        
        sourceListBox.setTopLeftPosition(15, 100);
        sourceListBox.setSize(200, 250);
        destinationListBox.setTopLeftPosition(325, 100);
        destinationListBox.setSize(200, 250);

        Rectangle<int> buttonPos(250, 100, 50, 25);
        selectionLeftButton.setBounds(buttonPos);
        allLeftButton.setBounds(buttonPos.withY(130));
        selectionRightButton.setBounds(buttonPos.withY(170));
        allRightButton.setBounds(buttonPos.withY(200));

        updateFeaturesButton.setSize(75, 25);
        updateFeaturesButton.setCentrePosition(buttonPos.getX(), 250);
        
        
        tableComponent.setBounds(bottom);
    }
    
    StringArray getHeaderData()
    {
        //Build headers
        StringArray headerData;
        
        for(int i=0; i<destinationModel.visibleData.size(); i++)
            headerData.add(destinationModel.data[destinationModel.visibleData[i]]);
        
        return headerData;
    }
    
    cv::Mat getSelectedFeatureMatrix(StringArray headerData, cv::Mat featureMatrix)
    {
        cv::Mat data(featureMatrix.rows, headerData.size(), cv::DataType<float>::type);
        
        for(int i=0; i<headerData.size(); i++)
            featureMatrix.col(featureMap[headerData[i]]).copyTo(data.col(i));
        
        return data;
    }
    
private:
    TextButton fileLoadButton, buildDatasetButton;
    AudioFormatManager formatManager;
    CoreAudioFormat coreAudioFormat;
    EssentiaExtractor extractor;
    HashMap<String, int> featureMap;

    void updateListBox(ListBox& fromListBox, ListBox& toListBox, ListBoxContents& fromModel,ListBoxContents& toModel)
    {
        const SparseSet<int>& selectedRows = fromListBox.getSelectedRows();
        
        for(int i=selectedRows.size()-1; i>=0; i--) {
            int sourceIndex = fromModel.visibleData[selectedRows[i]];
            fromModel.visibleData.remove(selectedRows[i]);
            toModel.visibleData.add(sourceIndex);
        }
        
        fromListBox.updateContent();
        toListBox.updateContent();
    }
    
    void buttonClicked (Button* buttonThatWasClicked) override
    {
        if(buttonThatWasClicked == &selectionRightButton)
        {
            updateListBox(sourceListBox, destinationListBox, sourceModel, destinationModel);
        } else if(buttonThatWasClicked == &selectionLeftButton) {
            updateListBox(destinationListBox, sourceListBox, destinationModel, sourceModel);
        } else if(buttonThatWasClicked == &allRightButton) {
            sourceListBox.selectRangeOfRows(0, sourceModel.visibleData.size());
            updateListBox(sourceListBox, destinationListBox, sourceModel, destinationModel);
        } else if(buttonThatWasClicked == &allLeftButton) {
            destinationListBox.selectRangeOfRows(0, destinationModel.visibleData.size());
            updateListBox(destinationListBox, sourceListBox, destinationModel, sourceModel);
        } else if(buttonThatWasClicked == &updateFeaturesButton) {
            updateTable();
        }
        else if (buttonThatWasClicked == &fileLoadButton)
        {
            FileChooser fc ("Choose a directory...",
                            File::getCurrentWorkingDirectory(),
                            "*",
                            true);
            
            if (fc.browseForDirectory())
            {
                datasetFolder = fc.getResult();
                
                String datasetFilename = datasetFolder.getFullPathName() + "/dataset/dataset.json";
                
                if(File(datasetFilename).existsAsFile())
                    loadDataset(datasetFilename);
            }
        }
        else if (buttonThatWasClicked == &buildDatasetButton)
        {
            buildDataset(datasetFolder);
        }
        else if (buttonThatWasClicked == &outputLoopButton)
        {

        }
        
        this->datasetFolder = datasetFolder;
        propertiesFile->setValue("lastDatasetFolder", datasetFolder.getFullPathName());
        propertiesFile->save();
    }
    
    void loadDataset(const String& jsonFile)
    {
        extractor.loadDataset(jsonFile);
        updateData();
    }
    
    void buildDataset(const File& datasetFolder)
    {
        extractor.buildDataset(File(datasetFolder.getFullPathName()), true);
        updateData();
    }
    
    void updateTable()
    {
        StringArray headerData = getHeaderData();
        cv::Mat data = getSelectedFeatureMatrix(headerData, featureMatrix);
        tableComponent.updateData(headerData, data);
    }
    
    void updateData()
    {
        featureMatrix = extractor.globalPoolToMat();
        StringArray featureList = extractor.featuresInPool();
        
        for(int i=0; i<featureList.size(); i++)
            featureMap.set(featureList[i], i);
        
        tableComponent.updateData(featureList, featureMatrix);
        datasetFolderTextBox.setText(datasetFolder.getFullPathName());
        interfaceComponent->datasetPath = datasetFolder.getFullPathName();
        
        sourceModel.data = featureList;
        sourceListBox.updateContent();
        
        destinationModel.data = featureList;
        for(int i=0; i<featureList.size(); i++)
            destinationModel.visibleData.add(i);
        destinationListBox.updateContent();
        
//        clusterData(featureMatrix);
        
//        datasetFolderTextBox.setColour(TextEditor::ColourIds::backgroundColourId, Colours::lightgreen);
    }
    
    void clusterData(cv::Mat data)
    {
        cv::Mat labels;
        cv::kmeans(data, 3, labels, cv::TermCriteria( cv::TermCriteria::EPS+cv::TermCriteria::COUNT, 10, 1.0), 3, cv::KMEANS_PP_CENTERS, cv::noArray());        
    }
    
    bool isInterestedInFileDrag (const StringArray& /*files*/) override
    {
        return true;
    }
    
    void filesDropped (const StringArray& files, int /*x*/, int /*y*/) override
    {

        datasetFolder = files[0];
        
        String datasetFilename = datasetFolder.getFullPathName() + "/dataset/dataset.json";
        
        if(File(datasetFilename).existsAsFile())
            loadDataset(datasetFilename);
        
        this->datasetFolder = datasetFolder;
        propertiesFile->setValue("lastDatasetFolder", datasetFolder.getFullPathName());
        propertiesFile->save();
        
        datasetFolderTextBox.setText(datasetFolder.getFullPathName());
    }
};

#endif