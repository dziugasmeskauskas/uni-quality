///////////////////////////////////////////////////////////
//  Preprocess.cpp
//  Implementation of the Class Preprocess
//  Created on:      05-Vas-2014 17:36:45
//  Original author: Povilas
///////////////////////////////////////////////////////////

#define _ELPP_THREAD_SAFE
#define _ELPP_STL_LOGGING
#define _ELPP_NO_DEFAULT_LOG_FILE

#include "Preprocess.h"
#include "HelperMethods.h"
#include <math.h>
#include <numeric>
#include "logging/easylogging++.h"


Preprocess::Preprocess(InitDamisService* initFile):ServeRequest(initFile)
{
    LOG (INFO) << "Data preprocessing has been called";
altOutFile = NULL;

// Preprocess::writeClass.reserve(0);
   // ServeRequest::writeData.reserve(0);
   // ServeRequest::tmpDataVector.reserve(0);

   /* DamisService::noOfDataRows = this->getDataDoubleFormat().size();
    DamisService::noOfDataAttr = this->getDataDoubleFormat().at(0).size();*/
}

void Preprocess::filterData(bool retFilteredData, double zValue, int attrIndex)
{
    LOG (INFO) << "Initiating filter data function. Got parameters retFilteredData - " << retFilteredData <<" zValue - " << zValue<< " attrIndex - " << attrIndex;

    if (attrIndex < 1){
        LOG (INFO) << "Attribute could not be 0";
    }else {
        attrIndex = attrIndex - 1;

        std::vector<int> exObjIndex;
        int noOfObjects = serveFile->getNumberOfObjects();
        exObjIndex.reserve(0);

        ServeRequest::tmpDataVector.reserve(noOfObjects);

        //double mean = 0;
        double stdev = 0;

        for (int j = 0; j < noOfObjects; j++)
            ServeRequest::tmpDataVector.push_back(serveFile->getDoubleDataAt(j, attrIndex));

        //mean = HelperMethods::getMean(DamisService::tmpDataVector);
        stdev = HelperMethods::getStd(ServeRequest::tmpDataVector); //daliname is N o ne is N-1

        for (int i = 0; i < noOfObjects; i++)
            if (fabs(ServeRequest::tmpDataVector.at(i)) > zValue * stdev)
                exObjIndex.push_back(i);

        ServeRequest::tmpDataVector.clear();

        bool skip = false; //init for safety
        for (int i = 0; i < noOfObjects; i++)
        {
            skip = false
            for (int z = 0; z < exObjIndex.size(); z++)
                if (i == exObjIndex.at(z))
                {
                    skip = true;
                    break;
                }

            for (int j = 0; j < serveFile->getNumberOfAttributes(); j++)
            {
                if (retFilteredData && !skip || !retFilteredData && skip)
                    ServeRequest::tmpDataVector.push_back(serveFile->getDoubleDataAt(i,j));
                else
                     break;
            }
            if (!ServeRequest::tmpDataVector.empty())
                {
                    if (serveFile->isClassFound())
                        Preprocess::writeClass.push_back(serveFile->getStringClass().at(i));
                        ServeRequest::writeData.push_back(ServeRequest::tmpDataVector);

                    ServeRequest::tmpDataVector.clear();
                }
        }

        this->writeDataToFile(outFile->getFilePath(),prepareDataSection(ServeRequest::writeData, Preprocess::writeClass), prepareAttributeSection(serveFile->getAttributeName(),serveFile->getAttributeType(),serveFile->getStringClassAttribute()));
    }
}

void Preprocess::normData(bool normMeanStd, double a, double b)
{
    LOG (INFO) << "Initializing norm data function. Got parameters normMeanStd - "<<normMeanStd <<" a - " <<a << " b - " << b;

    ServeRequest::tmpDataVector.reserve(serveFile->getNumberOfObjects()); //norms each columns not row, thus initialize elements for total row number
    ServeRequest::writeData.reserve(serveFile->getNumberOfObjects());

    for (int i = 0; i < serveFile->getNumberOfObjects(); i++)
    {
        for (int j = 0; j < serveFile->getNumberOfAttributes(); j++)
            ServeRequest::tmpDataVector.push_back(serveFile->getDoubleData().at(i).at(j));

        ServeRequest::writeData.push_back(ServeRequest::tmpDataVector);
        ServeRequest::tmpDataVector.clear();
    }


    for (int i = 0; i < serveFile->getNumberOfAttributes(); i++) //need to calculate mean and std for each attribute in every row
    {
        for (int j = 0; j < serveFile->getNumberOfObjects(); j++)
            {
                ServeRequest::tmpDataVector.push_back(serveFile->getDoubleDataAt(j,i));
               // std::cout << i << " " << j << " " << tmpDataVector.at(j) << std::endl;
            }

        if (normMeanStd) //make average equal to a and dispersion equal to b
        {
            double mean = HelperMethods::getMean(ServeRequest::tmpDataVector);
            double stdev = HelperMethods::getStd(ServeRequest::tmpDataVector); //daliname is N o ne is N-1

            for (int z = 0; z < serveFile->getNumberOfObjects(); z++) // update attribute values in each row
                ServeRequest::writeData.at(z).at(i) = (ServeRequest::tmpDataVector.at(z) - mean + a) * b / stdev; //update data
        }
        else
        {
            double min = *std::min_element(ServeRequest::tmpDataVector.begin(), ServeRequest::tmpDataVector.end());
            double max = *std::max_element(ServeRequest::tmpDataVector.begin(), ServeRequest::tmpDataVector.end());

            for (int z = 0; z < serveFile->getNumberOfObjects(); z++) // update attribute values in each row
                {
                    ServeRequest::writeData.at(z).at(i) = a + (double)((ServeRequest::tmpDataVector.at(z) - min) * (b - a)) / (max - min); //update data
 //                   std::cout << z << " " << i << " " << ServeRequest::writeData.at(z).at(i) << std::endl;
                }
        }
        ServeRequest::tmpDataVector.clear();
    }
    this->writeDataToFile(outFile->getFilePath(),prepareDataSection(ServeRequest::writeData, serveFile->getStringClass()), prepareAttributeSection(serveFile->getAttributeName(),serveFile->getAttributeType(),serveFile->getStringClassAttribute()));
}

void Preprocess::splitData(bool reshufleObjects,double firstSubsetPerc, double secondSubsetPerc)
{
    LOG (INFO) << "Initiaing data split. Got parameters reshufleObjects - "<< reshufleObjects <<" firstSubsetPerc - " <<firstSubsetPerc << " secondSubsetPerc - "<< secondSubsetPerc;

   // ServeRequest::tmpDataVector.resize(serveFile->getNumberOfAttributes());
    int objectNumber = serveFile->getNumberOfObjects();

    std::vector<int> objectIndex;
    objectIndex.reserve(objectNumber);

    int firstCount = ceil(firstSubsetPerc / 100.0 * objectNumber);  //qtt of vectors in first set
    int secondCount = ceil(secondSubsetPerc / 100.0 * objectNumber);                //qtt of vectors in second set

    if ((firstCount + secondCount) > objectNumber)
            secondCount -= objectNumber - (firstCount + secondCount);

  //  ServeRequest::writeData.resize(firstCount, ServeRequest::tmpDataVector);
   // Preprocess::writeClass.resize(firstCount);

    std::vector<std::vector<double>> secondData;
    secondData.reserve(secondCount);
    std::vector<std::string> secondClass;
    secondClass.reserve(secondCount);

    //initialize vector with index values
    for( int i = 0; i < serveFile->getNumberOfObjects(); i++)
        objectIndex.push_back(i);


    std::vector<int>* v = objectNumber;

    if (reshufleObjects)
    {
        //reshuffling
        Preprocess::reshuffleObjects(objectNumber,v);
    }

    for (int i = 0; i < firstCount + secondCount; i++)
    {

        for (int j = 0; j < serveFile->getNumberOfAttributes(); j++)
            ServeRequest::tmpDataVector.push_back(serveFile->getDoubleDataAt((objectIndex.at(i)),j));

        if (i < firstCount)
        {
            if (serveFile->isClassFound())
                Preprocess::writeClass.push_back(serveFile->getStringClass().at(objectIndex.at(i)));
                ServeRequest::writeData.push_back(ServeRequest::tmpDataVector);
        }
        else
        {
            if (serveFile->isClassFound())
                secondClass.push_back(serveFile->getStringClass().at(objectIndex.at(i)));
                secondData.push_back(ServeRequest::tmpDataVector);
        }
        ServeRequest::tmpDataVector.clear();
    }

    this->writeDataToFile(outFile->getFilePath(),prepareDataSection(ServeRequest::writeData , Preprocess::writeClass),prepareAttributeSection(serveFile->getAttributeName(),serveFile->getAttributeType(),serveFile->getStringClassAttribute()));

    this->altOutFile = new DamisFile("_output_");

    this->writeDataToFile(altOutFile->getFilePath(),prepareDataSection(secondData , secondClass),prepareAttributeSection(serveFile->getAttributeName(),serveFile->getAttributeType(),serveFile->getStringClassAttribute()));
}

void Preprocess::reshuffleObjects(int objectNumber, vector<int> *v){
    std::vector<int> objectIndex = v;
    int fIndex, sIndex; // index that values must be swaped
    int tmp;
    for (int i = 0; i < objectNumber * 5; i++)
            {
                //generate indexes
                fIndex = HelperMethods::getRrandomInRange(0, objectNumber);
                sIndex = HelperMethods::getRrandomInRange(0, objectNumber);
                //change index order
                tmp = objectIndex.at(fIndex);
                objectIndex.at(fIndex) = objectIndex.at(sIndex);
                objectIndex.at(sIndex) = tmp;
            }
    v->objectIndex;
}

void Preprocess::transposeData()
{
    LOG (INFO) << "Initiating data transpose";

   /* ServeRequest::tmpDataVector.resize(serveFile->getNumberOfAttributes());
    ServeRequest::writeData.resize(serveFile->getNumberOfObjects(), ServeRequest::tmpDataVector);*/

    std::vector<std::string> attrNames;
    std::string tmp = "attr";


    for (int i = 0; i < serveFile->getNumberOfObjects(); i++) //rows becomes attributes
    {
        //memory optimization
        tmp.append(std::to_string(static_cast<long>(i+1)));
        tmp.append(" NUMERIC");
        attrNames.push_back(tmp);
        tmp = "attr";
    }

    std::vector<std::string> dummy;
    dummy.reserve(0); //pass dummy vector to write function since no attribute after transform are left

    for (int i = 0; i < serveFile ->getNumberOfAttributes(); i++)
    {
        for (int j = 0; j < serveFile->getNumberOfObjects(); j++)
        {
            ServeRequest::tmpDataVector.push_back(serveFile->getDoubleDataAt(j,i));
            //std::cout << i << " " << j <<std::endl;
        }
        ServeRequest::writeData.push_back(ServeRequest::tmpDataVector);
        ServeRequest::tmpDataVector.clear();
    }
    this->writeDataToFile(outFile->getFilePath(), prepareDataSection(ServeRequest::writeData, dummy), prepareAttributeSection(attrNames, dummy, dummy));
}

void Preprocess::cleanData()
{
    LOG (INFO) << "Initiating data cleaning";
    this->writeDataToFile(outFile->getFilePath(),
                          prepareDataSection(serveFile->getDoubleData(),
                                             serveFile->getStringClass()),
                          prepareAttributeSection(serveFile->getAttributeName(),
                                                  serveFile->getAttributeType(),
                                                  serveFile->getStringClassAttribute()));
}

Preprocess::~Preprocess()
{

}
