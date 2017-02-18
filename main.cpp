#include <iostream>
#include <fstream>
#include <map>
#include <queue>

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

    bool operator<(const node &rhs) const {
        return data->p < rhs.data->p;
    }

    bool operator>(const node &rhs) const {
        std::cerr << "Hit >" << std::endl;
        return data->p > rhs.data->p;
    }
};

/* Custom comparator */
struct compare_nodes {
    bool operator()(const node *lhs, const node *rhs) const {
        return lhs->data->p > rhs->data->p;
    }
};


void print_usage() {
    std::cout << "Usage: huffman [-z compress | -x decompress] input_file output_file" << std::endl;
}

void build_codes(std::string s, node *n, int depth, h_tuple *dict[]) {
    if ((n->left) || (n->right)) {   // We hit on at least one leaf, so our code is not complete
        if (n->left)
            build_codes(s + "0", n->left, depth + 1, dict);
        if (n->right)  // We've exhausted all children
            build_codes(s + "1", n->right, depth + 1, dict);
    } else {
        // h_tuple *t = new h_tuple(depth, n->data->p, s);
        h_tuple *t = new h_tuple;
        t->bits = depth;
        t->p = n->data->p;
        t->code = s;
        dict[(int) n->name] = t;
    }
}

int main(int argc, char *argv[]) {
    std::ifstream input_file;
    std::ofstream output_file;
    std::string mode;

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
        // output_file.open(argv[3]);
    } else {
        print_usage();
        exit(1);
    }

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

    // char x = 97;
    // Condense the individual trees into a single tree.
    while (pq.size() > 1) {
        auto l = pq.top();
        pq.pop();
        auto r = pq.top();
        pq.pop();
        h_tuple *h = new h_tuple(l->data->p + r->data->p);
        // node n{x++, h};
        node *n = new node(0, h);
        n->left = l;
        n->right = r;
        pq.push(n);
    }


    // Build dictionary
    h_tuple *dictionary[256];
    for (int i = 0; i < 256; i++)
        dictionary[i] = new h_tuple;
    auto n = pq.top();
    build_codes("", n, 0, dictionary);

    for (int i = 0; i < 256; i++) {
        if (dictionary[i]->bits > 0)
            std::cout << (char) i << ":" << dictionary[i]->bits << ":" << dictionary[i]->code << std::endl;
    }

    // Store dictionary to file


    // Compress

    return 0;
}