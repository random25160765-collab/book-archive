#include <iostream>
#include <unordered_map>
#include <string>
using namespace std;

int main() {

    unordered_map<string, int> umap;

    cout << "It's a simple tutorial of unordered_map" << endl;
    cout << "=======================================" << endl;
    cout << "Let's add some elements..." << endl;

    // insert / emplace / insert_or_assign
    umap["what_1"] = 1;
    umap.insert({"what_2", 2});
    umap.insert(make_pair("what_3", 3));
    umap.emplace("what_4", 4);
    umap.insert_or_assign("what_4", 4);
    umap.insert_or_assign("what_5", 5);

    // [] operator
    int a1 = umap["what_1"];
    int a6 = umap["what_6"]; // something strange happen

    // at / find
    try {
        int a2 = umap.at("what_2");
        int a7 = umap.at("what_7");
    } catch (const out_of_range& e) {
        cout << "you suck" << endl;
    }

    auto it = umap.find("what_6"); // let's find what_6
    unordered_map<string, int>::iterator it2 = umap.find("what_6"); // it's also ok
    cout << "Key: " << it->first << endl;
    cout << "Another method:" << (*it).first << endl;
    auto& [key, value] = *it;
    cout << "well... " << key << endl;

    // 3 methods to iterate
    cout << "First!" << endl;
    for (const auto& pair : umap) {
        cout << "  " << pair.first << "; " << pair.second << endl;
    }
    cout << "Second!" << endl;
    for (const auto& [name, score] : umap) {
        cout << "  " << name << "; " << score << endl;
    }
    cout << "Third!" << endl;
    for (auto it = umap.begin(); it != umap.end(); it++) {
        cout << "  " << it->first << "; " << it->second << endl;
    }

    // contains (C++ 20) / erase / count
    if (umap.contains("what_6")) {
        cout << "well." << endl;
    }
    umap.erase("what_6");
    if (umap.count("what_6") == 0) {
        cout << "wtf?? it was deleted?" << endl;
    }

    // erase_if (C++ 20)
    erase_if(umap, [](const auto& pair) {
        return pair.second < 3;
    });

    // size / empty / bucket_count
    cout << "Size: " << umap.size() << endl;
    cout << "Empty? " << (umap.empty() ? "Yes" : "No") << endl;
    cout << "Bucket count: " << umap.bucket_count() << endl;

    // clear
    umap.clear();

    return 0;
}