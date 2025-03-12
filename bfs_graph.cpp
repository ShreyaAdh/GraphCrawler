#include <iostream>
#include <deque>
#include <set>
#include <vector>
#include <string>
#include <unordered_map>
#include <future> // For async parallel execution
#include <curl/curl.h>
#include <rapidjson/document.h>
#include <chrono>
#include <fstream> // For file output

using namespace std;
using namespace rapidjson;

const string BASE_URL = "http://hollywood-graph-crawler.bridgesuncc.org/neighbors/";

// Cache API responses to avoid redundant requests
unordered_map<string, vector<string>> cache;

// Callback function for libcurl
size_t WriteCallback(void *contents, size_t size, size_t nmemb, string *output)
{
    size_t totalSize = size * nmemb;
    output->append((char *)contents, totalSize);
    return totalSize;
}

// Function to encode spaces in URLs
string encode_spaces(const string &str)
{
    string encoded;
    for (char c : str)
    {
        if (c == ' ')
            encoded += "%20";
        else
            encoded += c;
    }
    return encoded;
}

// Function to fetch neighbors using libcurl
vector<string> fetch_neighbors(const string &node)
{
    if (cache.find(node) != cache.end())
        return cache[node]; // Return cached result

    vector<string> neighbors;
    string url = BASE_URL + encode_spaces(node);
    string response;

    CURL *curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        CURLcode res = curl_easy_perform(curl);

        if (res == CURLE_OK)
        {
            Document doc;
            doc.Parse(response.c_str());
            if (doc.HasMember("neighbors") && doc["neighbors"].IsArray())
            {
                for (auto &n : doc["neighbors"].GetArray())
                {
                    neighbors.push_back(n.GetString());
                }
            }
        }
        else
        {
            cerr << " CURL Request Failed: " << curl_easy_strerror(res) << endl;
        }
        curl_easy_cleanup(curl);
    }

    cache[node] = neighbors; // Store result in cache
    return neighbors;
}

// BFS Traversal Function (Parallelized)
set<string> bfs_traversal(const string &start_node, int max_depth)
{
    deque<pair<string, int>> q;
    set<string> visited;

    q.push_back({start_node, 0});
    visited.insert(start_node);

    cout << " Starting BFS from: " << start_node << endl;

    while (!q.empty())
    {
        int batch_size = q.size();
        vector<future<vector<string>>> tasks; // Parallel API calls
        vector<pair<string, int>> new_nodes;
        for (int i = 0; i < batch_size; i++)
        {
            auto [node, depth] = q.front();
            q.pop_front();

            if (depth >= max_depth)
                continue;

            // Fetch neighbors asynchronously
            tasks.push_back(async(launch::async, fetch_neighbors, node));
            // Store current depth for later use
            new_nodes.push_back({node, depth});
        }

        // Collect results and expand BFS
        for (size_t i = 0; i < tasks.size(); i++)
        {
            vector<string> neighbors = tasks[i].get();
            int depth = new_nodes[i].second; // Correct depth of the current node

            for (const string &neighbor : neighbors)
            {
                if (visited.insert(neighbor).second) // If not visited
                {
                    q.push_back({neighbor, depth + 1}); // Now increments correctly
                }
            }
        }
    }

    return visited;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        cerr << "Usage: " << argv[0] << " <start_node> <max_depth>" << endl;
        return 1;
    }

    string start_node = argv[1];
    int max_depth = stoi(argv[2]);

    auto start_time = chrono::high_resolution_clock::now();

    set<string> result = bfs_traversal(start_node, max_depth);

    auto end_time = chrono::high_resolution_clock::now();
    double exec_time = chrono::duration<double, milli>(end_time - start_time).count();

    // Create filename dynamically
    string filename = "bfs_output_" + start_node + ".txt";
    for (char &c : filename)
    {
        if (c == ' ')
            c = '_'; // Replace spaces with underscores
    }

    ofstream output_file(filename);
    if (!output_file)
    {
        cerr << " Error opening file: " << filename << endl;
        return 1;
    }

    // Write to file
    output_file << "\n BFS completed. Nodes reachable within depth " << max_depth << " from " << start_node << ":\n";
    for (const string &node : result)
    {
        output_file << "   ➜ " << node << endl;
    }

    output_file << "\n Execution Time: " << exec_time << " ms\n";
    output_file.close();

    cout << "BFS results saved in " << filename << endl;
    return 0;
}
