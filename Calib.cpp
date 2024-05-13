#include <iostream>
#include <string>
#include <vector>
#include <curl\curl.h>
#include "json.hpp"

/*
 Progress: (5/13/24)
// got the thingspeak fetching and JSON parsing to work
*/

const std::string curlCertPath = "./curl/bin/curl-ca-bundle.crt";

using json = nlohmann::json;
using namespace::std;


// Callback function to write received data into a string
size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *data) {
    data->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Function to fetch data from ThingSpeak API
std::vector<double> fetchDataFromThingSpeak(const int& ChannelID, const int& Field, int length) {
    std::vector<double> data;

    // URL for the ThingSpeak API
    std::string url = "https://api.thingspeak.com/channels/" + to_string(ChannelID) + "/field/" + to_string(Field) + ".json";

    // Initialize cURL
    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();
    if(curl) {
        // Set the URL
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER, 0L);
        // Set the write callback function
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        // Response data
        std::string response_data;
        // Set the response data pointer
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
        // Perform the request
        res = curl_easy_perform(curl);
        // Check for errors
        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }
        else {
            // Parse the JSON response and extract data
            json jsonResponse = json::parse(response_data);
            // Extract the values from JSON
            int count = 0;
            for (const auto& item : jsonResponse["feeds"]) {
                if (count >= length) {
                    break; // Stop if we've reached the desired number of entries
                }
                double value = std::stod(item["field"+to_string(Field)].get<std::string>());
                data.push_back(value);
                ++count;
            }
        }
        // Cleanup
        curl_easy_cleanup(curl);
    }
    return data;
}

int main() {
    // Example usage
    int ChannelID = 1694548;
    int Field = 5;
    int length = 10; // Number of most recent entries to fetch
    std::vector<double> fetchedData = fetchDataFromThingSpeak(ChannelID, Field, length);
    // Output the fetched data
    std::cout << "Fetched Data:" << std::endl;
    for (double value : fetchedData) {
        std::cout << value << " ";
    }
    std::cout << std::endl;
    return 0;
}
