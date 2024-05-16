#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <curl\curl.h>
#include "json.hpp"

/*
 Progress: (5/13/24)
// got the thingspeak fetching and JSON parsing to work
*/


using json = nlohmann::json;
using namespace::std;

const string curlCertPath = "./curl-ca-bundle.crt";
const string CalibrationFile = "SensorCalibrationData.csv"; // file where calibration coeffecients are mapped to each sensor
const string fetchFile = "SensorFetchData.csv"; // data that stores map of sensor names and thingspeak ID and #'s


// Callback function to write received data into a string
size_t WriteCallback(void *contents, size_t size, size_t nmemb, string *data) {
    data->append((char*)contents, size * nmemb);
    return size * nmemb;
}
// Function to fetch data from ThingSpeak API
vector<double> fetchDataFromThingSpeak(const int& ChannelID, const int& Field, int length) {
    vector<double> data;

    string url = "https://api.thingspeak.com/channels/" + to_string(ChannelID) + "/field/" + to_string(Field) + ".json";

    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();
    if(curl) {
        // Set the URL
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        // curl_easy_setopt(curl, CURLOPT_CAINFO, curlCertPath.c_str());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);


        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        // Response data
        string response_data;
        // Set the response data pointer
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
        // Perform the request
        res = curl_easy_perform(curl);
        // Check for errors
        if(res != CURLE_OK) {
            cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
        }
        else {
           json jsonResponse = json::parse(response_data);
            int count = 0;
            for (const auto& item : jsonResponse["feeds"]) {
                if (count >= length) {
                    break; // Stop if we've reached the desired number of entries
                }
                // Check if the field is null
                if (!item["field" + std::to_string(Field)].is_null()) {
                    double value = std::stod(item["field" + std::to_string(Field)].get<std::string>());
                    data.push_back(value);
                } else {
                    // If field is null, push 0
                    data.push_back(0.0);
                }
                ++count;
            }
        }
        curl_easy_cleanup(curl);
    }
    return data;
}
// function to perform linear regression
vector<double> linearRegression(const vector<double>& x, const vector<double>& y) {
    if (x.size() != y.size()) {
        cout<<"Input vectors must have the same non-zero size"<<endl;
        return {1,0};// default value for error( no calibration)
    }else if(x.empty()){
        throw invalid_argument("Sensor Data is empty");
    }

    double sumX = 0.0, sumY = 0.0, sumXY = 0.0, sumXSquare = 0.0;
    int n = x.size();

    // Calculate sums
    for (int i = 0; i < n; ++i) {
        sumX += x[i];
        sumY += y[i];
        sumXY += x[i] * y[i];
        sumXSquare += x[i] * x[i];
    }

    // Calculate coefficients
    double xMean = sumX / n;
    double yMean = sumY / n;
    double slope = (sumXY - n * xMean * yMean) / (sumXSquare - n * xMean * xMean);
    double intercept = yMean - slope * xMean;

    return {slope, intercept};
}
   // function to read CSV data 
vector<vector<string>> readSensorDataCSV(const string& filename) {
    ifstream file(filename);
    vector<vector<string>> data;
    string line;

    while (getline(file, line)) {
        stringstream lineStream(line);
        string cell;
        vector<string> sensorData;

        while (getline(lineStream, cell, ',')) {
            sensorData.push_back(cell);
        }

        data.push_back(sensorData);
    }

    return data;
}
//clear csv
void ClearCSV(const string filename){
    fstream file(filename,ios::out|ios::trunc);
    file.close();
}
// function to write calibration data to CSV
void writeCSV(const string& filename, const vector<vector<string>>& data) {
   // Open file in truncate mode to clear existing content and write mode to ensure it's cleared
    fstream file(filename,ios::app);

    for (const auto& row : data) {
        for (auto it = row.begin(); it != row.end(); ++it) {
            file << *it;
            if (it != row.end() - 1) {
                file << ",";
            }
        }
        file << endl;
    }
}
// main logic of looping through sensors and writing to CSV
void SensorLoop(vector<vector<string>>& csvDATA, vector<double> calibrationData){
    for (const auto& sensor: csvDATA){
        int fetchedChannelID = stoi(sensor[1]);
        int fetchedField = stoi(sensor[2]);
        double length = 8;
        //fetching sensor data for current sensor
        vector<double> sensorData = fetchDataFromThingSpeak(fetchedChannelID,fetchedField,length);
        // doing regression against input data
        vector<double> calibData = linearRegression(sensorData,calibrationData);
        // assigingin variables for clarity
        double slope = calibData[0];
        double intercept = calibData[1]; 
        //  writing data to output CSV
        writeCSV(CalibrationFile, {{sensor[0],to_string(fetchedChannelID),to_string(fetchedField),to_string(slope),to_string(intercept)}});
    }
}
int main() {

    //===================================================================================
    // this should be the temperature data read off of the thermometer
    vector<double> calibData = {73.947,74.192,74.178,74.235,74.054,74.255,74.065,73.802};
    //===================================================================================
    ClearCSV(CalibrationFile);
    vector<vector<string>> sensorDATA = readSensorDataCSV(fetchFile);
    SensorLoop(sensorDATA,calibData);
    cout<<"Done! Check 'SensorCalibration.csv' for output"<<endl;
    return 0;
}
