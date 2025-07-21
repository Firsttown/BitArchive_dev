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


struct HuffmanNode {
    uint8_t symbol;
    uint32_t freq;
    HuffmanNode* left;
    HuffmanNode* right;

    HuffmanNode(uint8_t sym, uint32_t fr) : symbol(sym), freq(fr), left(nullptr), right(nullptr) {}
    HuffmanNode(HuffmanNode* l, HuffmanNode* r) : symbol(0), freq(l->freq + r->freq), left(l), right(r) {}
    
    bool isLeaf() const { return !left && !right; }
};



struct Compare
{
    bool operator()(HuffmanNode* a, HuffmanNode* b)
    {
        if (a->freq != b->freq)
            return a->freq > b->freq;
        return a->symbol > b->symbol;
    }

};

void serializeTree(HuffmanNode* root, BitWriter& writer) 
{
    if (!root) return;
    
    if (root->isLeaf()) {
        writer.writeBit(1);
        writer.writeByte(root->symbol);
    } else {
        writer.writeBit(0);
        serializeTree(root->left, writer);
        serializeTree(root->right, writer);
    }
}

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

void serializeTree(HuffmanNode* root, BitWriter& writer) 
{
    if (!root) return;
    
    if (root->isLeaf()) {
        writer.writeBit(1);
        writer.writeByte(root->symbol);
    } else {
        writer.writeBit(0);
        serializeTree(root->left, writer);
        serializeTree(root->right, writer);
    }
}

HuffmanNode* deserializeTree(BitReader& reader) 
{
    uint8_t bit = reader.readBit();
    
    if (bit == 1) {
        uint8_t symbol = reader.readByte();
        return new HuffmanNode(symbol, 0); // Частота не важна при распаковке
    } else {
        HuffmanNode* left = deserializeTree(reader);
        HuffmanNode* right = deserializeTree(reader);
        return new HuffmanNode(left, right);
    }
}

void compress(const std::string& inputFile, const std::string& outputFile) {
    std::ifstream input(inputFile, std::ios::binary);
    if (!input) throw std::runtime_error("Cannot open input file: " + inputFile);

    std::ofstream output(outputFile, std::ios::binary);
    if (!output) throw std::runtime_error("Cannot open output file: " + outputFile);

    auto freqTable = buildFrequencyTable(input);
    HuffmanNode* root = buildHuffmanTree(freqTable);
    
    
    std::unordered_map<uint8_t, std::string> codes;
    generateCodes(root, "", codes);

    
    uint32_t fileSize = 0;
    for (const auto& pair : freqTable) fileSize += pair.second;
    output.write(reinterpret_cast<const char*>(&fileSize), sizeof(fileSize));

    BitWriter bitWriter(output);
    
   
    serializeTree(root, bitWriter);
    
   
    uint8_t byte;
    while (input.read(reinterpret_cast<char*>(&byte), sizeof(byte))) {
        for (char bit : codes[byte]) {
            bitWriter.writeBit(bit == '1' ? 1 : 0);
        }
    }

    freeTree(root);
}

void decompress(const std::string& inputFile, const std::string& outputFile) 
{
    std::ifstream input(inputFile, std::ios::binary);
    if (!input) throw std::runtime_error("Cannot open input file: " + inputFile);
    
    std::ofstream output(outputFile, std::ios::binary);
    if (!output) throw std::runtime_error("Cannot open output file: " + outputFile);

    
    uint32_t fileSize;
    input.read(reinterpret_cast<const char*>(&fileSize), sizeof(fileSize));
    if (fileSize == 0) return;

    BitReader bitReader(input);
    
   
    HuffmanNode* root = deserializeTree(bitReader);
    
    
    for (uint32_t i = 0; i < fileSize; i++) {
        HuffmanNode* node = root;
        while (!node->isLeaf()) {
            uint8_t bit = bitReader.readBit();
            node = (bit == 0) ? node->left : node->right;
        }
        output.put(node->symbol);
    }

    freeTree(root);
}
