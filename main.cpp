#include <iostream>
#include <fstream>
#include <map>
#include <queue>
#include <stack>
#include <unordered_map>
#include <chrono>

/* Huffman Tuple
 * This is the data. */
struct h_tuple {
    int bits;
    double p;
    std::string code;

    h_tuple() {
        bits = 0;
        p = 0.0;
        code = "";
    }

    h_tuple(int b, double pr, std::string s) {
        bits = b;
        p = pr;
        code = s;
    }

    h_tuple(double pr) {
        bits = 0;
        p = pr;
        code = "";
    }
};

/* Tree node */
struct node {
    char name;
    h_tuple *data;
    node *left = nullptr;
    node *right = nullptr;

    node(char n, h_tuple *d) {
        name = n;
        data = d;
    }

    bool operator<(const node &rhs) const { return data->p < rhs.data->p; }

    bool operator>(const node &rhs) const { return data->p > rhs.data->p; }
};

/* Custom comparator */
struct compare_nodes {
    bool operator()(const node *lhs, const node *rhs) const { return lhs->data->p > rhs->data->p; }
};


void print_usage() {
    std::cout << "Usage: huffman [-z compress | -x decompress] input_file output_file" << std::endl;
}

void build_codes(std::string s, node *n, int depth, h_tuple *dict[]) {
    if ((n->left) || (n->right)) {   // We hit on at least one leaf, so our code is not complete
        if (n->left)
            build_codes(s + "1", n->left, depth + 1, dict);
        if (n->right)  // We've exhausted all children
            build_codes(s + "0", n->right, depth + 1, dict);
    } else {
        h_tuple *t = new h_tuple;
        t->bits = depth;
        t->p = n->data->p;
        t->code = s;
        dict[(int) n->name] = t;
    }
}

void stringToLongBuffer(std::string s, unsigned long &buffer) {
    for (uint b = 0; b < s.length(); b++) {
        buffer = buffer << 1;
        int t = s.at(b);
        if (t == 49)
            buffer++;
    }
}

std::string recoverBitStringFromFile(int bits, std::ifstream &input, unsigned char &buffer, unsigned char &buf_mask) {
    std::string retval = "";
    for (int i = 0; i < bits; i++) {
        if (buf_mask == 0) {
            buffer = (unsigned char) input.get();
            buf_mask = 128;
        }

        uint bit = (buffer & buf_mask);
        if (bit > 0)
            retval += "1";
        else
            retval += "0";

        buf_mask = buf_mask >> 1;
    }

    return retval;
}

void bytesToBitsOut(unsigned long &buffer, int &length, std::ofstream &output) {
    std::stack<unsigned char> output_stack;

    auto offset = (length - (length % 8)) / 8;
    auto data = buffer >> (length % 8);  // Align our buffer to bytes
    if (offset > 0) {
        while (offset > 0) {
            unsigned char t = (unsigned char) (data & 0xFF);
            output_stack.push(t);
            data = data >> 8;
            offset--;
        }

        while (!output_stack.empty()) {
            unsigned char out = output_stack.top();
            output_stack.pop();
            output << out;
        }

        // Compute remaining data mask
        uint mask = 0;
        for (int i = 0; i < (length % 8); i++) {
            mask = mask << 1;
            mask++;
        }

        buffer &= mask;
        length %= 8;
    }
}

int main(int argc, char *argv[]) {
    std::ifstream input_file;
    std::ofstream output_file;
    std::string mode;
    h_tuple *dictionary[256];

    /* Command line argument parsing */
    if (argc == 4) {
        mode.assign(argv[1]);
        if (!((mode == "-z") || (mode == "-x"))) {
            std::cout << "Illegal argument." << std::endl;
            print_usage();
            exit(1);
        }
        input_file.open(argv[2]);
        if (input_file.fail()) {
            std::cout << "File I/O failure.  Quitting." << std::endl;
            exit(1);
        }
        output_file.open(argv[3]);
    } else {
        print_usage();
        exit(1);
    }

    auto start = std::chrono::high_resolution_clock::now();
    if (mode == "-z") {
        // Gather statistics
        double counter = 0.0;
        double p_x[256] = {};

        unsigned char in;
        while (in = (unsigned char) input_file.get(), input_file.good()) {
            p_x[in]++;
            counter++;
        }
        input_file.close();
        for (int i = 0; i < 256; i++)
            p_x[i] /= counter;

        // Build tree and dictionary
        std::priority_queue<node *, std::vector<node *>, compare_nodes> pq;
        for (int i = 0; i < 256; i++) {
            if (p_x[i] > 0) {
                h_tuple *t = new h_tuple(p_x[i]);
                node *n = new node((char) i, t);
                pq.push(n);
            }
        }

        // Condense the individual trees into a single tree.
        while (pq.size() > 1) {
            auto l = pq.top();
            pq.pop();
            auto r = pq.top();
            pq.pop();
            h_tuple *h = new h_tuple(l->data->p + r->data->p);
            node *n = new node(0, h);
            n->left = l;
            n->right = r;
            pq.push(n);
        }

        // Build dictionary
        for (int i = 0; i < 256; i++)
            dictionary[i] = new h_tuple;
        auto n = pq.top();
        build_codes("", n, 0, dictionary);

        /* Store dictionary to file */
        // Store the dictionary preamble
        for (int i = 0; i < 256; i++)
            output_file << (char) (dictionary[i]->bits);

        // Store the bits
        unsigned long buffer = 0;
        int buf_len = 0;
        for (int i = 0; i < 256; i++) {
            std::string c = dictionary[i]->code;
            if (c != "") {
                buf_len += dictionary[i]->bits;
                stringToLongBuffer(c, buffer);  // <-- This function is the problem.
                bytesToBitsOut(buffer, buf_len, output_file);
            }
        }
        // Handle the residual bits
        if (buf_len > 0) {
            buffer = buffer << (8 - buf_len);
            buf_len = 8;
            bytesToBitsOut(buffer, buf_len, output_file);
        }

        // Encode the input file and write to output
        input_file.open(argv[2]);
        std::string encoded;
        buf_len = 0;
        buffer = 0;
        while (in = (unsigned char) input_file.get(), input_file.good()) {
            buf_len += dictionary[in]->bits;
            stringToLongBuffer(dictionary[in]->code, buffer);
            bytesToBitsOut(buffer, buf_len, output_file);
        }
        // Handle the residual bits
        if (buf_len > 0) {
            buffer = buffer << (8 - buf_len);
            buf_len = 8;
            bytesToBitsOut(buffer, buf_len, output_file);
        }
    } else if (mode == "-x") {
        // Recover the dictionary preamble
        unsigned char in;
        for (int i = 0; i < 256; i++) {
            in = (unsigned char) input_file.get();
            dictionary[i] = new h_tuple(in, 0, "");
        }

        // Recover the codewords and build lookup table
        unsigned char buffer = 0;
        unsigned char buffer_mask = 0;
        std::unordered_map<std::string, char> codebook;

        for (int i = 0; i < 256; i++)
            if (dictionary[i]->bits > 0) {
                dictionary[i]->code = recoverBitStringFromFile(dictionary[i]->bits, input_file, buffer, buffer_mask);
                codebook[dictionary[i]->code] = (char) i;
            }

        // Recover bits from file; output if they match a code
        std::string string_buffer = "";
        buffer = 0;
        buffer_mask = 0;
        while (input_file.good()) {
            string_buffer += recoverBitStringFromFile(1, input_file, buffer, buffer_mask);
            auto plain_char = codebook.find(string_buffer);
            if (plain_char != codebook.end()) {
                output_file << plain_char->second;
                string_buffer = "";
            }
        }
    }

    auto stop = std::chrono::high_resolution_clock::now();
    std::cout << "Running time: " << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << "ms." << std::endl;

    input_file.close();
    output_file.close();
    return 0;
}