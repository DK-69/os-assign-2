#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <queue>
#include <stack>
#include <unordered_set>
#include <deque>

using namespace std;
uint64_t page_faults = 0;

// Command-line argument structure
struct Arguments {
    uint64_t pageSize;
    uint64_t numMemoryFrames;
    string replacementPolicy;
    string allocationPolicy;
    string traceFilePath;
};

// Base PageTable class
class PageTable {
protected:
    uint64_t maxFrames;  // Maximum frames allowed for each page table

public:
    PageTable(uint64_t frames) : maxFrames(frames) {}
    virtual void addPage(uint64_t pageId) = 0;
    virtual bool checkPage(uint64_t pageId) = 0;
    virtual void evictPage() = 0;  // New function to manage eviction
};

// FIFO PageTable
class FifoTable : public PageTable {
private:
    queue<uint64_t> fifoQueue;           // Queue to implement FIFO order
    unordered_set<uint64_t> pages;       // Set to check if a page is in the table

public:
    FifoTable(uint64_t frames) : PageTable(frames) {}

    bool checkPage(uint64_t pageId) override {
        return pages.find(pageId) != pages.end();
    }

    void evictPage() override {
        uint64_t oldPage = fifoQueue.front();
        uint64_t mn = oldPage * 4;
        cout << "Erased page: " <<oldPage<<endl;
        fifoQueue.pop();
        pages.erase(oldPage);
        
 
    }

    void addPage(uint64_t pageId) override {
        // Evict if necessary before adding a new page
        if (pages.size() >= maxFrames) {
            evictPage();
        } 
        fifoQueue.push(pageId);  // Add new page at the end
        pages.insert(pageId);
        cout << "Inseterd frame of VA: " <<pageId<<endl;
    }
};

// LRU PageTable (Placeholder with example eviction logic)
class LruTable : public PageTable {
private:
    deque<uint64_t> Lru_array;           // Queue to implement FIFO order
    unordered_set<uint64_t> pages;  
public:
    LruTable(uint64_t frames) : PageTable(frames) {}

    bool checkPage(uint64_t pageId) override {
        // Implement LRU check logic here
        if (pages.find(pageId) != pages.end()){
            stack<uint64_t> Remove;
            while(Lru_array.back() != pageId){
                uint64_t ab = Lru_array.back();
                Remove.push(ab);
                Lru_array.pop_back();
            }
            Lru_array.pop_back();
            while(Remove.size()!=0){
                uint64_t rev = Remove.top();
                Lru_array.push_back(rev);
                Remove.pop();
            }
            Lru_array.push_back(pageId);
            return true;

        }
        else{
            return false;
        }
       
    }

    void evictPage() override {
        // Implement LRU eviction logic here
        uint64_t oldPage = Lru_array.front();
        uint64_t mn = oldPage * 4;
        cout << "Erased page: " <<oldPage<<endl;
        Lru_array.pop_front();
        pages.erase(oldPage);
    }

    void addPage(uint64_t pageId) override {
        // Implement LRU add logic here
        if (pages.size() >= maxFrames) {
            evictPage();
        } 
        Lru_array.push_back(pageId);  // Add new page at the end
        pages.insert(pageId);
        cout << "Inseterd frame of VA: " <<pageId<<endl;
    
    }
};

// Function to create the correct PageTable based on replacement policy
PageTable* createPageTable(const string& policy, uint64_t number_of_frames) {
    if (policy == "fifo") {
        return new FifoTable(number_of_frames);
    } else if (policy == "lru") {
        return new LruTable(number_of_frames);
    }
    cerr << "Unknown replacement policy: " << policy << endl;
    exit(1);
}

// Function to parse command-line arguments
Arguments parseArguments(int argc, char *argv[]) {
    if (argc < 6) {
        cerr << "Usage: " << argv[0] << " <page-size> <number-of-memory-frames> <replacement-policy> <allocation-policy> <path-to-memory-trace-file>" << endl;
        exit(1);
    }

    Arguments args;
    args.pageSize = stoi(argv[1]);
    args.numMemoryFrames = stoi(argv[2]);
    args.replacementPolicy = argv[3];
    args.allocationPolicy = argv[4];
    args.traceFilePath = argv[5];

    return args;
}

int main(int argc, char *argv[]) {
    // Parse command-line arguments
    Arguments args = parseArguments(argc, argv);

    PageTable* globalPageTable = nullptr;
    map<uint64_t, PageTable*> localPageTables;

    // Determine allocation type and create appropriate page tables
    bool isGlobalAllocation = (args.allocationPolicy == "global");

    if (isGlobalAllocation) {
        // Create a single global page table
        globalPageTable = createPageTable(args.replacementPolicy, args.numMemoryFrames);
    }

    uint64_t frames_left = args.numMemoryFrames;

    // Open the trace file
    ifstream traceFile(args.traceFilePath);
    if (!traceFile.is_open()) {
        cerr << "Error opening file: " << args.traceFilePath << endl;
        return 1;
    }

    // Process each line from the trace file
    string line;
    queue<uint64_t> local_list_array;
    while (getline(traceFile, line)) {
        uint64_t processId;
        uint64_t virtualAddress;

        // Parse process ID and virtual address (format: <process id>,<virtual address>)
        sscanf(line.c_str(), "%ld,%ld", &processId, &virtualAddress);
        uint64_t pageId = virtualAddress / args.pageSize; // Calculate page ID
        // cout << "Line of: " << virtualAddress << endl;
        // Global allocation logic
        if (isGlobalAllocation) {
            if (!globalPageTable->checkPage(pageId)) {
                page_faults++;
                cout<<"----------------->Page fault at : "<< virtualAddress<<endl;
                globalPageTable->addPage(pageId);
            }
        }
        // Local allocation logic
        else {
            local_list_array.push(processId);
            // Check if a page table exists for this process ID; if not, create one
            if (localPageTables.find(processId) == localPageTables.end()) {
                if(frames_left==0){
                    if (local_list_array.size()>0){
                    uint64_t k = local_list_array.front();
                    local_list_array.pop();
                    localPageTables[k]->evictPage();
                    frames_left++;
                    }
                }
                localPageTables[processId] = createPageTable(args.replacementPolicy, args.numMemoryFrames);
            }
            
            PageTable* processTable = localPageTables[processId];

            // Check if a page fault occurs
            if (!processTable->checkPage(pageId)) {
                page_faults++;
                cout<< "------------->Page fault at: "<< virtualAddress<<endl;
                
                if (frames_left==0) {
                    processTable->evictPage();
                    frames_left++;
                }
                processTable->addPage(pageId);
                frames_left--;
            }
        }
    }

    traceFile.close();

    // Output total page faults
    cout << "Total page faults: " << page_faults << endl;

    // Cleanup dynamically allocated page tables
    delete globalPageTable;
    for (auto& entry : localPageTables) {
        delete entry.second;
    }

    return 0;
}
