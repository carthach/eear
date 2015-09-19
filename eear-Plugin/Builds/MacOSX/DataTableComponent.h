//==============================================================================
/**
    This class shows how to implement a TableListBoxModel to show in a TableListBox.
*/
#ifndef TABLEDEMOCOMPONENT_H
#define TABLEDEMOCOMPONENT_H

class DataTableComponent : public Component,
                              public TableListBoxModel
{
public:
    Array<float> weights;
    DataTableComponent()   : font (14.0f)
    {
        // Load some data from an embedded XML file..
//        loadData();
        
        // Create our table component and add it to this component..
        
        addAndMakeVisible (table);
        table.setModel (this);
        
        // give it a border
        table.setColour (ListBox::outlineColourId, Colours::grey);
        table.setOutlineThickness (1);
        
        // we could now change some initial settings..
        table.getHeader().setSortColumnId (1, true); // sort forwards by the ID column
        table.getHeader().setColumnVisible (7, false); // hide the "length" column until the user shows it
        
        // un-comment this line to have a go of stretch-to-fit mode
        // table.getHeader().setStretchToFitActive (true);
        
        table.setMultipleSelectionEnabled (true);
    }
    
    //==============================================================================
    // this loads the embedded database XML file into memory
    void updateData(StringArray headerData, cv::Mat data)
    {
        table.getHeader().removeAllColumns();

        weights.clear();
        
        for(int i=0; i<headerData.size(); i++) {
            table.getHeader().addColumn (headerData[i],
                                         i+1,
                                         100,
                                         50, 400,
                                         TableHeaderComponent::defaultFlags);
            weights.add(1.0);
        }
        
        numRows = data.rows+1;
        
        this->headerData = headerData;
        this->data = data;
        
        table.updateContent();
    }
    
    void updateSelectedData()
    {
        
    }

    // This is overloaded from TableListBoxModel, and must return the total number of rows in our table
    int getNumRows() override
    {
        return numRows;
    }

    // This is overloaded from TableListBoxModel, and should fill in the background of the whole row
    void paintRowBackground (Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) override
    {
        if (rowIsSelected)
            g.fillAll (Colours::lightblue);
        else if (rowNumber % 2)
            g.fillAll (Colour (0xffeeeeee));
    }

    // This is overloaded from TableListBoxModel, and must paint any cells that aren't using custom
    // components.
    void paintCell (Graphics& g, int rowNumber, int columnId,
                    int width, int height, bool /*rowIsSelected*/) override
    {
        g.setColour (Colours::black);
        g.setFont (font);
        
        //columnId is indexed from 1
        if(columnId <= headerData.size() && rowNumber > 0) {
                float value = data.at<float>(rowNumber-1, columnId-1);
                g.drawText (String(value), 2, 0, width - 4, height, Justification::centredLeft, true);
        }
        
        g.setColour (Colours::black.withAlpha (0.2f));

        g.fillRect (width - 1, 0, 1, height);
    }
    
    // This is overloaded from TableListBoxModel, and tells us that the user has clicked a table header
    // to change the sort order.
    void sortOrderChanged (int newSortColumnId, bool isForwards) override
    {
        if (newSortColumnId != 0)
        {
//            excluded[newSortColumnId] = !excluded[newSortColumnId];
            
//            DemoDataSorter sorter (getAttributeNameForColumnId (newSortColumnId), isForwards);
//            dataList->sortChildElements (sorter);
//
//            table.updateContent();
        }
    }

//    // This is overloaded from TableListBoxModel, and must update any custom components that we're using
    Component* refreshComponentForCell (int rowNumber, int columnId, bool /*isRowSelected*/,
                                        Component* existingComponentToUpdate) override
    {
//        if (columnId == 1 || columnId == 7) // The ID and Length columns do not have a custom component
        if (rowNumber > 0) // The ID and Length columns do not have a custom component
        {
//            jassert (existingComponentToUpdate == 0);
            return 0;
        }
        else if (rowNumber == 0) // For the ratings column, we return the custom combobox component
        {
            RatingColumnCustomComponent* ratingsBox = (RatingColumnCustomComponent*) existingComponentToUpdate;

            // If an existing component is being passed-in for updating, we'll re-use it, but
            // if not, we'll have to create one.
            if (ratingsBox == 0)
                ratingsBox = new RatingColumnCustomComponent (*this);

            ratingsBox->setRowAndColumn (rowNumber, columnId);

            return ratingsBox;
        }
//        else // The other columns are editable text columns, for which we use the custom Label component
//        {
//            EditableTextCustomComponent* textLabel = (EditableTextCustomComponent*) existingComponentToUpdate;
//
//            // same as above...
//            if (textLabel == 0)
//                textLabel = new EditableTextCustomComponent (*this);
//
//            textLabel->setRowAndColumn (rowNumber, columnId);
//            
//            return textLabel;
//        }
    }

    // This is overloaded from TableListBoxModel, and should choose the best width for the specified
    // column.
    int getColumnAutoSizeWidth (int columnId) override
    {
//        if (columnId == 5)
//            return 100; // (this is the ratings column, containing a custom combobox component)
//
//        int widest = 32;
//
//        // find the widest bit of text in this column..
//        for (int i = getNumRows(); --i >= 0;)
//        {
//            const XmlElement* rowElement = dataList->getChildElement (i);
//
//            if (rowElement != 0)
//            {
//                const String text (rowElement->getStringAttribute (getAttributeNameForColumnId (columnId)));
//
//                widest = jmax (widest, font.getStringWidth (text));
//            }
//        }
//
//        return widest + 8;
    }

    // A couple of quick methods to set and get cell values when the user changes them
    int getRating (const int rowNumber) const
    {
//        return dataList->getChildElement (rowNumber)->getIntAttribute ("Rating");
    }

    void setRating (const int rowNumber, const int newRating)
    {
//        dataList->getChildElement (rowNumber)->setAttribute ("Rating", newRating);
    }

    String getText (const int columnNumber, const int rowNumber) const
    {
//        return dataList->getChildElement (rowNumber)->getStringAttribute ( getAttributeNameForColumnId(columnNumber));
        return "DUMMY TEXT";
    }

    void setText (const int columnNumber, const int rowNumber, const String& newText)
    {
//        const String& columnName = table.getHeader().getColumnName (columnNumber);
//        dataList->getChildElement (rowNumber)->setAttribute (columnName, newText);
    }

    //==============================================================================
    void resized() override
    {
        // position our table with a gap around its edge
        
        
        table.setBoundsInset(BorderSize<int>(8));
    }



private:
    TableListBox table;     // the table component itself
    Font font;
    var json;

//    ScopedPointer<XmlElement> demoData;   // This is the XML document loaded from the embedded file "demo table data.xml"
//    XmlElement* columnList; // A pointer to the sub-node of demoData that contains the list of columns
//    XmlElement* dataList;   // A pointer to the sub-node of demoData that contains the list of data rows
    int numRows;            // The number of rows of data we've got
    int numColumns;
    StringArray headerData;
    cv::Mat data;


        //==============================================================================
        // This is a custom component containing a combo box, which we're going to put inside
        // our table's "rating" column.
        class RatingColumnCustomComponent    : public Component, public Slider::Listener
        {
        public:
            RatingColumnCustomComponent (DataTableComponent& owner_)
                : owner (owner_)
            {
                // just put a combo box inside this component
                addAndMakeVisible (slider);
                slider.setSliderStyle (Slider::LinearBar);
                slider.setRange(0.001, 10.0);
                slider.setSkewFactor(0.5);
                slider.setValue(1.0);
                slider.addListener(this);
//                s->setTextValueSuffix (" gallons");
                // when the combo is changed, we'll get a callback.
            }
    
            void resized() override
            {
                slider.setBoundsInset (BorderSize<int> (2));
            }
    
            // Our demo code will call this when we may need to update our contents
            void setRowAndColumn (const int newRow, const int newColumn)
            {
                row = newRow;
                columnId = newColumn;
//                comboBox.setSelectedId (owner.getRating (row), dontSendNotification);
            }
    
            
            void sliderValueChanged	(	Slider * 	slider	)
            {
                owner.weights.set(columnId-1, slider->getValue());
            }

    
        private:
            DataTableComponent& owner;
            Slider slider;
            int row, columnId;
        };

    
//    //==============================================================================
//    // This is a custom Label component, which we use for the table's editable text columns.
//    class EditableTextCustomComponent : public Label
//    {
//    public:
//        EditableTextCustomComponent (TableDemoComponent& owner_)
//            : owner (owner_)
//        {
//            // double click to edit the label text; single click handled below
//            setEditable (false, true, false);
//            setColour (textColourId, Colours::black);
//        }
//
//        void mouseDown (const MouseEvent& event) override
//        {
//            // single click on the label should simply select the row
//            owner.table.selectRowsBasedOnModifierKeys (row, event.mods, false);
//
//            Label::mouseDown (event);
//        }
//
//        void textWasEdited() override
//        {
//            owner.setText (columnId, row, getText());
//        }
//
//        // Our demo code will call this when we may need to update our contents
//        void setRowAndColumn (const int newRow, const int newColumn)
//        {
//            row = newRow;
//            columnId = newColumn;
//            setText (owner.getText(columnId, row), dontSendNotification);
//        }
//
//    private:
//        TableDemoComponent& owner;
//        int row, columnId;
//    };


//    //==============================================================================
//    // This is a custom component containing a combo box, which we're going to put inside
//    // our table's "rating" column.
//    class RatingColumnCustomComponent    : public Component,
//                                           public ComboBoxListener
//    {
//    public:
//        RatingColumnCustomComponent (TableDemoComponent& owner_)
//            : owner (owner_)
//        {
//            // just put a combo box inside this component
//            addAndMakeVisible (comboBox);
//            comboBox.addItem ("fab", 1);
//            comboBox.addItem ("groovy", 2);
//            comboBox.addItem ("hep", 3);
//            comboBox.addItem ("mad for it", 4);
//            comboBox.addItem ("neat", 5);
//            comboBox.addItem ("swingin", 6);
//            comboBox.addItem ("wild", 7);
//
//            // when the combo is changed, we'll get a callback.
//            comboBox.addListener (this);
//            comboBox.setWantsKeyboardFocus (false);
//        }
//
//        void resized() override
//        {
//            comboBox.setBoundsInset (BorderSize<int> (2));
//        }
//
//        // Our demo code will call this when we may need to update our contents
//        void setRowAndColumn (const int newRow, const int newColumn)
//        {
//            row = newRow;
//            columnId = newColumn;
//            comboBox.setSelectedId (owner.getRating (row), dontSendNotification);
//        }
//
//        void comboBoxChanged (ComboBox* /*comboBoxThatHasChanged*/) override
//        {
//            owner.setRating (row, comboBox.getSelectedId());
//        }
//
//    private:
//        TableDemoComponent& owner;
//        ComboBox comboBox;
//        int row, columnId;
//    };

    //==============================================================================
    // A comparator used to sort our data when the user clicks a column header
    class DemoDataSorter
    {
    public:
        DemoDataSorter (const String attributeToSort_, bool forwards)
            : attributeToSort (attributeToSort_),
              direction (forwards ? 1 : -1)
        {
        }

        int compareElements (XmlElement* first, XmlElement* second) const
        {
//            int result = first->getStringAttribute (attributeToSort)
//                           .compareNatural (second->getStringAttribute (attributeToSort));
//
//            if (result == 0)
//                result = first->getStringAttribute ("ID")
//                           .compareNatural (second->getStringAttribute ("ID"));
//
//            return direction * result;
        }

    private:
        String attributeToSort;
        int direction;
    };
    
    // (a utility method to search our XML for the attribute that matches a column ID)
    String getAttributeNameForColumnId (const int columnId) const
    {
//        forEachXmlChildElement (*columnList, columnXml)
//        {
//            if (columnXml->getIntAttribute ("columnId") == columnId)
//                return columnXml->getStringAttribute ("name");
//        }

        return String::empty;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DataTableComponent)
};

#endif