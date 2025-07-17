#include <fstream>
#include <queue>
#include <vector>
#include <map>
#include <unordered_map>
#include <cstdint>


class BitWriter {
private:
    std::ofstream& output;
    uint8_t buffer;
    uint8_t bitCount;

public:
    explicit BitWriter(std::ofstream& out) : output(out), buffer(0), bitCount(0) {}

    void writeBit(uint8_t bit) {
        buffer = (buffer << 1) | (bit & 1);
        bitCount++;
        if (bitCount == 8) {
            output.put(buffer);
            buffer = 0;
            bitCount = 0;
        }
    }

    void writeByte(uint8_t byte) {
        for (int i = 7; i >= 0; i--) {
            writeBit((byte >> i) & 1);
        }
    }

    void flush() {
        while (bitCount != 0) {
            writeBit(0);
        }
    }

    ~BitWriter() {
        flush();
    }
};

class BitReader {
private:
    std::ifstream& input;
    uint8_t buffer;
    uint8_t bitCount;

public:
    explicit BitReader(std::ifstream& in) : input(in), buffer(0), bitCount(0) {}

    uint8_t readBit() {
        if (bitCount == 0) {
            buffer = input.get();
            bitCount = 8;
        }
        uint8_t bit = (buffer >> 7) & 1;
        buffer <<= 1;
        bitCount--;
        return bit;
    }

    uint8_t readByte() {
        uint8_t byte = 0;
        for (int i = 0; i < 8; i++) {
            byte = (byte << 1) | readBit();
        }
        return byte;
    }
};


struct HuffmanNode
{
    uint8_t symbol;
    uint32_t freq;

    HuffmanNode* left;
    HuffmanNode* right;

    HuffmanNode(uint8_t sym, uint32_t fr) : symbol(sym), freq(fr), left(nullptr), right(nullptr) {}
    HuffmanNode(HuffmanNode* l, HuffmanNode* r) : symbol(0), freq(l -> freq + r -> freq), left(l), right(r) {}
};

struct Compare
{
    bool operator()(HuffmanNode* a, HuffmanNode* b)
    {
        return a -> freq, b -> freq;
    }

};

void freeTree(HuffmanNode* node)
{
    if(node)
    {
        freeTree(node -> left);
        freeTree(node -> right);
        delete node;
    }
}

std::unordered_map<uint8_t, uint32_t> buildFrequencyTable(std::ifstream& input)
{
    std::unordered_map<uint8_t, uint32_t> freqTable;
    uint8_t byte;

    while(input.read(reinterpret_cast<char*>(&byte), sizeof(byte)))
    {
        freqTable[byte]++;
    }
    
    input.clear();
    input.seekg(0);

    return freqTable;
}

void generateCodes(HuffmanNode* node, std::string code, std::unordered_map<uint8_t, std::string>& codes)
{
    if(!node)
    {
        return;
    }

    if(!node->left && !node -> right)
    {
        codes[node->symbol] = code;
        return;
    }

    generateCodes(node->left, code + "0", codes);
    generateCodes(node->right, code + "1", codes);
}


HuffmanNode* buildHuffmanTree(std::unordered_map<uint8_t, uint32_t>& freqTable)
{
    std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, Compare> pq;

    for(const auto& pair : freqTable)
    {
        pq.push(new HuffmanNode(pair.first, pair.second));
    }

    while(pq.size() > 1)
    {
        HuffmanNode* left = pq.top(); pq.pop();
        HuffmanNode* right = pq.top(); pq.pop();
        pq.push(new HuffmanNode(left, right));
    }

    return pq.empty() ? nullptr : pq.top();
}

void compress(const std::string& inputFile, const std::string& outputFile)
{
    std::ifstream input(inputFile, std::ios::binary);

    if(!input)
    {
        std::cerr << "Open File ERR" << std::endl;
        return;
    }

    std::ofstream output(outputFile, std::ios::binary);
    if(!output)
    {
        std::cerr << "Open File ERR" << std::endl;
        return;
    }

    auto freqTable = buildFrequencyTable(input);
    HuffmanNode* root = buildHuffmanTree(freqTable);
    
    std::unordered_map<uint8_t, std::string> codes;
    generateCodes(root, "", codes);

    uint32_t fileSize = 0;

    for(const auto& pair : freqTable)
    {
        fileSize += pair.second;
    }

    output.write(reinterpret_cast<const char*>(&fileSize), sizeof(fileSize));
    uint32_t uniqueCount = freqTable.size();
    output.write(reinterpret_cast<const char*>(&uniqueCount), sizeof(uniqueCount));

    for(const auto& pair : freqTable)
    {
        uint8_t symbol = pair.first;
        uint32_t freq = pair.second;

        output.put(symbol);
        output.write(reinterpret_cast<const char*>(&freq), sizeof(freq));
    }

    BitWriter bitWriter(output);
    uint8_t byte;

    while(input.read(reinterpret_cast<char*>(&byte), input.good()))
    {
        for(char bit : codes[byte])
        {
            bitWriter.writeBit(bit == '1' ? 1 : 0);
        }
    }

    freeTree(root);
}

void decompress(const std::string& inputFie, const std::string& outputFile)
{
    std::ifstream input(inputFie, std::ios::binary);
    
    if (!input) 
    {
        std::cerr << "Error opening input file" << std::endl;
        return;
    }
    
    std::ofstream output(outputFile, std::ios::binary);
    
    if (!output)
    {
        std::cerr << "Error opening output file" << std::endl;
        return;
    }

    uint32_t fileSize;
    input.read(reinterpret_cast<char*>(&fileSize), sizeof(fileSize));

    uint32_t uniqueCount;
    input.read(reinterpret_cast<char*>(&uniqueCount), sizeof(uniqueCount));

    std::unordered_map<uint8_t, uint32_t> freqTable;
    
    
    for(uint32_t i = 0; i < uniqueCount; i++)
    {
        uint8_t symbol = input.get();
        uint32_t freq;
        input.read(reinterpret_cast<char*>(&freq), sizeof(freq));
        freqTable[symbol] = freq;
    }

    HuffmanNode* root = buildHuffmanTree(freqTable);

    BitReader bitReadr(input);
    HuffmanNode* currentNode = root;

    for(uint32_t i = 0; i < fileSize; i++)
    {
        while(currentNode->left || currentNode->right)
        {
            uint8_t bit = bitReadr.readBit();
            currentNode = (bit == 0) ? currentNode->left : currentNode->right;
        }
        output.put(currentNode->symbol);
        currentNode = root;
    }
    freeTree(root);
}


/*int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " c|d input output" << std::endl;
        return 1;
    }
    
    std::string mode = argv[1];
    std::string inputFile = argv[2];
    std::string outputFile = argv[3];
    
    if (mode == "c") {
        compress(inputFile, outputFile);
    } else if (mode == "d") {
        decompress(inputFile, outputFile);
    } else {
        std::cerr << "Invalid mode. Use 'c' for compress or 'd' for decompress" << std::endl;
        return 1;
    }
    
    return 0;
}*/