#include <iostream>
#include <vector>
#include <list>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <cctype>
#include <random>
#include <iomanip>
#include <cmath>
#include <map>


#ifdef _WIN32
#include <windows.h>
void setupConsole() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}
#else
void setupConsole() {}
#endif


std::string readFileToString(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Не удалось открыть файл: " + filepath);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::vector<std::string> processTextToWords(const std::string& text_utf8) {
    std::vector<std::string> words;
    std::string current_word;

    for (char c_byte : text_utf8) {
        unsigned char u_c_byte = static_cast<unsigned char>(c_byte);
        if (std::isspace(u_c_byte) || std::ispunct(u_c_byte)) {
            if (!current_word.empty()) {
                words.push_back(current_word);
                current_word.clear();
            }
        } else {
            current_word += c_byte;
        }
    }
    if (!current_word.empty()) {
        words.push_back(current_word);
    }
    return words;
}

std::string normalizeWordToLower(const std::string& input_str) {
    std::string result_str;
    result_str.reserve(input_str.length());

    for (size_t i = 0; i < input_str.length(); ++i) {
        unsigned char b1 = static_cast<unsigned char>(input_str[i]);

        if (b1 >= 0x41 && b1 <= 0x5A) {
            result_str += static_cast<char>(std::tolower(b1));
        } else if (b1 == 0xD0) {
            if (i + 1 < input_str.length()) {
                unsigned char b2 = static_cast<unsigned char>(input_str[i+1]);
                if (b2 >= 0x90 && b2 <= 0x9F) {
                    result_str += static_cast<char>(0xD0);
                    result_str += static_cast<char>(b2 + 0x20);
                } else if (b2 >= 0xA0 && b2 <= 0xAF) {
                    result_str += static_cast<char>(0xD1);
                    result_str += static_cast<char>(b2 - 0x20);
                } else if (b2 == 0x81) {
                    result_str += static_cast<char>(0xD1);
                    result_str += static_cast<char>(0x91);
                } else {
                    result_str += static_cast<char>(b1);
                    result_str += static_cast<char>(b2);
                }
                i++;
            } else {
                result_str += static_cast<char>(b1);
            }
        } else {
            result_str += static_cast<char>(b1);
        }
    }
    return result_str;
}


namespace DictionaryWithHashTable {

struct HashNode {
    std::string key;
    int value;

    HashNode(std::string k, int v) : key(std::move(k)), value(v) {}
};

class HashTable {
private:
    std::vector<std::list<HashNode>> table;
    size_t num_elements;
    size_t table_size;
    static constexpr double MAX_LOAD_FACTOR = 0.75;

    size_t hashFunction(const std::string& key) const {
        size_t hash_val = 0;
        size_t p = 31;
        size_t p_pow = 1;
        for (char c_byte : key) {
            hash_val = (hash_val + static_cast<unsigned char>(c_byte) * p_pow) % table_size;
            p_pow = (p_pow * p) % table_size;
        }
        return hash_val;
    }

    void rehash() {
        size_t old_table_size = table_size;
        table_size = table_size * 2 + 1;
        std::vector<std::list<HashNode>> old_table = std::move(table);

        table.assign(table_size, std::list<HashNode>());
        num_elements = 0;

        for (size_t i = 0; i < old_table_size; ++i) {
            for (const auto& node : old_table[i]) {
                add(node.key, node.value);
            }
        }
    }

public:
    HashTable(size_t initial_size = 101) : table_size(initial_size), num_elements(0) {
        if (table_size == 0) table_size = 1;
        table.resize(table_size);
    }

    void add(const std::string& key, int value) {
        if (static_cast<double>(num_elements + 1) / table_size >= MAX_LOAD_FACTOR) {
            rehash();
        }

        size_t index = hashFunction(key);
        for (auto& node : table[index]) {
            if (node.key == key) {
                node.value = value;
                return;
            }
        }
        table[index].emplace_back(key, value);
        num_elements++;
    }

    int* get(const std::string& key) {
        size_t index = hashFunction(key);
        for (auto& node : table[index]) {
            if (node.key == key) {
                return &node.value;
            }
        }
        return nullptr;
    }

    const int* get(const std::string& key) const {
        size_t index = hashFunction(key);
        for (const auto& node : table[index]) {
            if (node.key == key) {
                return &node.value;
            }
        }
        return nullptr;
    }


    bool remove(const std::string& key) {
        size_t index = hashFunction(key);
        auto& bucket = table[index];
        for (auto it = bucket.begin(); it != bucket.end(); ++it) {
            if (it->key == key) {
                bucket.erase(it);
                num_elements--;
                return true;
            }
        }
        return false;
    }

    void clear() {
        for (auto& bucket : table) {
            bucket.clear();
        }
        num_elements = 0;
    }

    void print(std::ostream& os = std::cout) const {
        os << "{";
        bool first_item = true;
        for (const auto& bucket : table) {
            for (const auto& node : bucket) {
                if (!first_item) {
                    os << ", ";
                }
                os << "'" << node.key << "': " << node.value;
                first_item = false;
            }
        }
        os << "}";
    }

    void visualize(std::ostream& os = std::cout) const {
        os << "Визуализация Хеш-таблицы (размер: " << table_size << ", элементы: " << num_elements << "):" << std::endl;
        for (size_t i = 0; i < table.size(); ++i) {
            os << "Корзина [" << i << "]: ";
            if (table[i].empty()) {
                os << "<пусто>";
            } else {
                bool first_in_bucket = true;
                for (const auto& node : table[i]) {
                    if (!first_in_bucket) {
                        os << " -> ";
                    }
                    os << "(\"" << node.key << "\": " << node.value << ")";
                    first_in_bucket = false;
                }
            }
            os << std::endl;
        }
    }
};

class Dictionary {
private:
    HashTable ht;

    /*std::string toLowerASCII(std::string s) const {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        return s;
    }*/
    std::string normalizeWord(const std::string& s) const {
        return ::normalizeWordToLower(s);
    }

public:
    Dictionary(size_t initial_capacity = 101) : ht(initial_capacity) {}

    void addWord(const std::string& word_raw) {
        if (word_raw.empty()) return;
        //std::string word = toLowerASCII(word_raw);
        std::string word = normalizeWord(word_raw);


        int* current_val_ptr = ht.get(word);
        if (current_val_ptr) {
            (*current_val_ptr)++;
        } else {
            ht.add(word, 1);
        }
    }

    void removeWord(const std::string& word_raw) {
        if (word_raw.empty()) return;
        //std::string word = toLowerASCII(word_raw);
        std::string word = normalizeWord(word_raw);
        ht.remove(word);
    }

    bool findWord(const std::string& word_raw) const {
        if (word_raw.empty()) return false;
        //std::string word = toLowerASCII(word_raw);
        std::string word = normalizeWord(word_raw);

        const int* count_ptr = ht.get(word);
        if (count_ptr) {
            std::cout << "Слово '" << word_raw << "' (ключ: '" << word << "') найдено, частота: " << *count_ptr << std::endl;
            return true;
        } else {
            std::cout << "Слово '" << word_raw << "' (ключ: '" << word << "') не найдено." << std::endl;
            return false;
        }
    }

    void clear() {
        ht.clear();
        std::cout << "Словарь (хеш-таблица) очищен." << std::endl;
    }

    void loadFromFile(const std::string& filepath, bool append = false) {
        if (!append) {
            clear();
        }
        try {
            std::string content = readFileToString(filepath);
            std::vector<std::string> words = processTextToWords(content);
            for (const std::string& word : words) {
                if (!word.empty()) addWord(word);
            }
            std::cout << "Словарь загружен/дополнен из файла '" << filepath << "' (хеш-таблица)." << std::endl;
        } catch (const std::runtime_error& e) {
            std::cerr << "Ошибка при загрузке из файла (хеш-таблица): " << e.what() << std::endl;
        }
    }

    void print(std::ostream& os = std::cout) const {
        ht.print(os);
    }

    void visualizeStructure(std::ostream& os = std::cout) const {
        ht.visualize(os);
    }
};

}

namespace DictionaryWithRBTree {

enum Color { RED, BLACK };

struct Node {
    std::string key;
    int value;
    Color color;
    Node *parent, *left, *right;

    Node(std::string k, int v, Color c = RED, Node* p = nullptr, Node* l = nullptr, Node* r = nullptr)
        : key(std::move(k)), value(v), color(c), parent(p), left(l), right(r) {}
};

class RBTree {
private:
    Node* root;
    Node* NIL;

    void leftRotate(Node* x) {
        Node* y = x->right;
        x->right = y->left;
        if (y->left != NIL) {
            y->left->parent = x;
        }
        y->parent = x->parent;
        if (x->parent == NIL) {
            root = y;
        } else if (x == x->parent->left) {
            x->parent->left = y;
        } else {
            x->parent->right = y;
        }
        y->left = x;
        x->parent = y;
    }

    void rightRotate(Node* y) {
        Node* x = y->left;
        y->left = x->right;
        if (x->right != NIL) {
            x->right->parent = y;
        }
        x->parent = y->parent;
        if (y->parent == NIL) {
            root = x;
        } else if (y == y->parent->right) {
            y->parent->right = x;
        } else {
            y->parent->left = x;
        }
        x->right = y;
        y->parent = x;
    }

    void insertFixup(Node* z) {
        while (z->parent->color == RED) {
            if (z->parent == z->parent->parent->left) {
                Node* y = z->parent->parent->right;
                if (y->color == RED) {
                    z->parent->color = BLACK;
                    y->color = BLACK;
                    z->parent->parent->color = RED;
                    z = z->parent->parent;
                } else {
                    if (z == z->parent->right) {
                        z = z->parent;
                        leftRotate(z);
                    }
                    z->parent->color = BLACK;
                    z->parent->parent->color = RED;
                    rightRotate(z->parent->parent);
                }
            } else {
                Node* y = z->parent->parent->left;
                if (y->color == RED) {
                    z->parent->color = BLACK;
                    y->color = BLACK;
                    z->parent->parent->color = RED;
                    z = z->parent->parent;
                } else {
                    if (z == z->parent->left) {
                        z = z->parent;
                        rightRotate(z);
                    }
                    z->parent->color = BLACK;
                    z->parent->parent->color = RED;
                    leftRotate(z->parent->parent);
                }
            }
             if (z == root) break; // Оптимизация: если z дошел до корня
        }
        root->color = BLACK;
    }

    Node* findNode(const std::string& key) const {
        Node* current = root;
        while (current != NIL && current->key != key) {
            if (key < current->key) {
                current = current->left;
            } else {
                current = current->right;
            }
        }
        return current;
    }

    void transplant(Node* u, Node* v) {
        if (u->parent == NIL) {
            root = v;
        } else if (u == u->parent->left) {
            u->parent->left = v;
        } else {
            u->parent->right = v;
        }
        v->parent = u->parent;
    }

    Node* minimum(Node* node) {
        while (node->left != NIL) {
            node = node->left;
        }
        return node;
    }

    void deleteFixup(Node* x) {
        while (x != root && x->color == BLACK) { // x "несет" дополнительный черный цвет
            if (x == x->parent->left) { // x - левый ребенок
                Node* w = x->parent->right; // w - брат x
                // случай 1: Брат w красный
                if (w->color == RED) {
                    w->color = BLACK;
                    x->parent->color = RED;
                    leftRotate(x->parent);
                    w = x->parent->right; // новый брат w (теперь он гарантированно черный)
                }
                // теперь w точно черный
                // случай 2: брат w черный, и оба его ребенка черные
                if (w->left->color == BLACK && w->right->color == BLACK) {
                    w->color = RED; // "отдаем" черный цвет w родителю
                    x = x->parent;  // перемещаем проблему (доп. черный) вверх к родителю
                } else {
                    // случай 3: брат w черный, левый ребенок w красный, правый черный
                    if (w->right->color == BLACK) { // (значит, w->left->color == RED)
                        w->left->color = BLACK;
                        w->color = RED;
                        rightRotate(w);
                        w = x->parent->right; // новый брат w (для случая 4)
                    }
                    // случай 4: брат w черный, и его правый ребенок красный
                    // (w->left->color может быть любым)
                    w->color = x->parent->color; // w наследует цвет родителя
                    x->parent->color = BLACK;    // родитель становится черным
                    w->right->color = BLACK;     // правый ребенок w становится черным
                    leftRotate(x->parent);
                    x = root; // проблема решена, завершаем цикл
                }
            } else { // x - правый ребенок (симметричные случаи)
                Node* w = x->parent->left; // w - брат x
                // случай 1 (симметричный)
                if (w->color == RED) {
                    w->color = BLACK;
                    x->parent->color = RED;
                    rightRotate(x->parent);
                    w = x->parent->left;
                }
                // случай 2 (симметричный)
                if (w->right->color == BLACK && w->left->color == BLACK) {
                    w->color = RED;
                    x = x->parent;
                } else {
                    // случай 3 (симметричный): брат w черный, правый ребенок w красный, левый черный
                    if (w->left->color == BLACK) { // (значит, w->right->color == RED)
                        w->right->color = BLACK;
                        w->color = RED;
                        leftRotate(w);
                        w = x->parent->left;
                    }
                    // случай 4 (симметричный)
                    w->color = x->parent->color;
                    x->parent->color = BLACK;
                    w->left->color = BLACK;
                    rightRotate(x->parent);
                    x = root;
                }
            }
        }
        x->color = BLACK; // убираем "дополнительный черный" с x (если x не NIL) или окрашиваем корень.
    }

    void destroyRecursive(Node* node) {
        if (node != NIL) {
            destroyRecursive(node->left);
            destroyRecursive(node->right);
            delete node;
        }
    }

    void inorderPrintRecursive(Node* node, std::ostream& os, bool& first_item) const {
        if (node != NIL) {
            inorderPrintRecursive(node->left, os, first_item);
            if (!first_item) {
                os << ", ";
            }
            os << "'" << node->key << "': " << node->value << " (" << (node->color == RED ? "R" : "B") << ")";
            first_item = false;
            inorderPrintRecursive(node->right, os, first_item);
        }
    }

    bool printGivenLevel(Node* node, int level, std::ostream& os, bool first_on_level) const {
        if (node == NIL) {
            return false;
        }
        if (level == 1) {
            if (!first_on_level) {
                os << "  ";
            }
            os << "(\"" << node->key << "\":" << node->value << (node->color == RED ? " R" : " B") << ")";
            return true;
        } else if (level > 1) {
            bool left_printed = printGivenLevel(node->left, level - 1, os, first_on_level);
            bool right_printed = printGivenLevel(node->right, level - 1, os, first_on_level && !left_printed);
            return left_printed || right_printed;
        }
        return false;
    }

    int getMaxDepth(Node* node) const {
        if (node == NIL) return 0;
        int left_depth = getMaxDepth(node->left);
        int right_depth = getMaxDepth(node->right);
        return std::max(left_depth, right_depth) + 1;
    }

    void printTreeRecursive(Node* node, int space_increment, int current_space, std::ostream& os) const {
        if (node == NIL) {
            return;
        }

        current_space += space_increment;

        printTreeRecursive(node->right, space_increment, current_space, os);

        os << std::endl;
        for (int i = space_increment; i < current_space; i++) {
            os << " ";
        }
        os << "(\"" << node->key << "\":" << node->value << (node->color == RED ? " R" : " B") << ")";

        printTreeRecursive(node->left, space_increment, current_space, os);
    }

public:
    RBTree() {
        NIL = new Node("", 0, BLACK);
        NIL->parent = NIL;
        NIL->left = NIL;
        NIL->right = NIL;
        root = NIL;
    }

    ~RBTree() {
        destroyRecursive(root);
        delete NIL;
    }

    RBTree(const RBTree&) = delete;
    RBTree& operator=(const RBTree&) = delete;

    void insert(const std::string& key, int value) {
        Node* z = new Node(key, value, RED, NIL, NIL, NIL);
        Node* y = NIL;
        Node* x = root;

        while (x != NIL) {
            y = x;
            if (z->key < x->key) {
                x = x->left;
            } else if (z->key > x->key) {
                x = x->right;
            } else {
                x->value = value;
                delete z;
                return;
            }
        }

        z->parent = y;
        if (y == NIL) {
            root = z;
        } else if (z->key < y->key) {
            y->left = z;
        } else {
            y->right = z;
        }
        insertFixup(z);
    }

    int* search(const std::string& key) {
        Node* node = findNode(key);
        return (node == NIL) ? nullptr : &node->value;
    }
    const int* search(const std::string& key) const {
        Node* node = findNode(key);
        return (node == NIL) ? nullptr : &node->value;
    }

    bool remove(const std::string& key) {
        Node* z = findNode(key);
        if (z == NIL) return false;

        Node* y = z;
        Node* x;
        Color y_original_color = y->color;

        if (z->left == NIL) {
            x = z->right;
            transplant(z, z->right);
        } else if (z->right == NIL) {
            x = z->left;
            transplant(z, z->left);
        } else {
            y = minimum(z->right);
            y_original_color = y->color;
            x = y->right;
            if (y->parent == z) { // y - непосредственный потомок z
                if (x != NIL) x->parent = y; // если x не NIL, его родителем должен стать y
            } else {
                transplant(y, y->right);
                y->right = z->right;
                y->right->parent = y;
            }
            transplant(z, y);
            y->left = z->left;
            y->left->parent = y;
            y->color = z->color;
        }
        delete z;

        if (y_original_color == BLACK) {
            deleteFixup(x);
        }
        return true;
    }

    void clear() {
        destroyRecursive(root);
        root = NIL;
    }

    void print(std::ostream& os = std::cout) const {
        os << "{";
        bool first = true;
        inorderPrintRecursive(root, os, first);
        os << "}";
    }

    void visualize(std::ostream& os = std::cout) const {
        os << "Визуализация Красно-Черного Дерева:" << std::endl;
        if (root == NIL) {
            os << "<дерево пусто>" << std::endl;
            return;
        }

        const int SPACE_INCREMENT = 15;
        printTreeRecursive(root, SPACE_INCREMENT, 0, os);
        os << std::endl << std::endl << "Конец визуализации." << std::endl;
    }
};


class Dictionary {
private:
    RBTree rbt;
    /*std::string toLowerASCII(std::string s) const {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        return s;
    }*/
    std::string normalizeWord(const std::string& s) const {
        return ::normalizeWordToLower(s);
    }

public:
    Dictionary() = default;

    void addWord(const std::string& word_raw) {
        if (word_raw.empty()) return;
        //std::string word = toLowerASCII(word_raw);
        std::string word = normalizeWord(word_raw);

        int* current_val_ptr = rbt.search(word);
        if (current_val_ptr) {
            rbt.insert(word, (*current_val_ptr) + 1);
        } else {
            rbt.insert(word, 1);
        }
    }

    void removeWord(const std::string& word_raw) {
        if (word_raw.empty()) return;
        //std::string word = toLowerASCII(word_raw);
        std::string word = normalizeWord(word_raw);
        rbt.remove(word);
    }

    bool findWord(const std::string& word_raw) const {
        if (word_raw.empty()) return false;
        //std::string word = toLowerASCII(word_raw);
        std::string word = normalizeWord(word_raw);

        const int* count_ptr = rbt.search(word);
        if (count_ptr) {
            std::cout << "Слово '" << word_raw << "' (ключ: '" << word << "') найдено, частота: " << *count_ptr << std::endl;
            return true;
        } else {
            std::cout << "Слово '" << word_raw << "' (ключ: '" << word << "') не найдено." << std::endl;
            return false;
        }
    }

    void clear() {
        rbt.clear();
        std::cout << "Словарь (КЧ-дерево) очищен." << std::endl;
    }

    void loadFromFile(const std::string& filepath, bool append = false) {
        if (!append) {
            clear();
        }
        try {
            std::string content = readFileToString(filepath);
            std::vector<std::string> words = processTextToWords(content);
            for (const std::string& word : words) {
                 if (!word.empty()) addWord(word);
            }
            std::cout << "Словарь загружен/дополнен из файла '" << filepath << "' (КЧ-дерево)." << std::endl;
        } catch (const std::runtime_error& e) {
            std::cerr << "Ошибка при загрузке из файла (КЧ-дерево): " << e.what() << std::endl;
        }
    }

    void print(std::ostream& os = std::cout) const {
        rbt.print(os);
    }

    void visualizeStructure(std::ostream& os = std::cout) const {
        rbt.visualize(os);
    }
};

}


namespace RLE {

const double CHAMPER_A = 1.57;
const double CHAMPER_M = 4.0;

const int MIN_RUN_LENGTH = 3;

double champernownePDF(double x){
    return CHAMPER_A / (M_PI * std::cosh(CHAMPER_A * (x - CHAMPER_M)));
}

std::string generateRandomText(size_t approx_target_chars, const std::string& filename = "random_text_custom_dist.txt") {
    std::vector<std::string> base_char_pool;
    std::vector<std::string> RUS_LOWER = {
        "а", "б", "в", "г", "д", "е", "ж", "з", "и", "й", "к", "л", "м",
        "н", "о", "п", "р", "с", "т", "у", "ф", "х", "ц", "ч", "ш", "щ",
        "ы", "э", "ю", "я"
    };
    std::vector<std::string> DIGITS = {"0","1","2","3","4","5","6","7","8","9"};
    std::vector<std::string> SPECIALS = {" ", ".", ",", "!", "?", "-", ":"};

    for (const auto& s : RUS_LOWER) base_char_pool.push_back(s);
    for (const auto& s : DIGITS) base_char_pool.push_back(s);
    for (const auto& s : SPECIALS) base_char_pool.push_back(s);

    std::vector<double> weights;
    weights.reserve(base_char_pool.size());
    double total_calculated_weight = 0.0;

    for (size_t i = 0; i < base_char_pool.size(); ++i) {
        double x_for_char = static_cast<double>(i);
        double weight = champernownePDF(x_for_char);
        weight = std::max(weight, 1e-9);
        weights.push_back(weight);
        total_calculated_weight += weight;
    }

    std::random_device rd;
    std::mt19937 rng(rd());
    std::discrete_distribution<size_t> char_dist(weights.begin(), weights.end());

    std::uniform_real_distribution<double> run_probability_dist(0.0, 1.0);
    double probability_of_run = 0.20;
    std::uniform_int_distribution<size_t> run_length_dist(MIN_RUN_LENGTH, 10);

    std::string result_text;
    result_text.reserve(approx_target_chars * 2);
    size_t current_char_count = 0;

    while (current_char_count < approx_target_chars) {
        size_t char_idx = char_dist(rng);
        const std::string& selected_char_utf8 = base_char_pool[char_idx];

        if (run_probability_dist(rng) < probability_of_run && (current_char_count + MIN_RUN_LENGTH < approx_target_chars) ) {
            size_t length_of_run = run_length_dist(rng);
            length_of_run = std::min(length_of_run, approx_target_chars - current_char_count);

            if (length_of_run >= MIN_RUN_LENGTH ){
                for (size_t k_run = 0; k_run < length_of_run; ++k_run) {
                    result_text += selected_char_utf8;
                }
                current_char_count += length_of_run;
            } else {
                 result_text += selected_char_utf8;
                 current_char_count++;
            }
        } else {
            result_text += selected_char_utf8;
            current_char_count++;
        }
    }

    std::ofstream outfile(filename, std::ios::binary);
    if (outfile.is_open()) {
        outfile.write(result_text.data(), result_text.length());
        outfile.close();
        std::cout << "Сгенерирован файл '" << filename << "' (сгенерировано " << current_char_count
                  << " UTF-8 симв., размер файла " << result_text.length() << " байт) с использованием champernownePDF." << std::endl;
    } else {
        std::cerr << "Не удалось создать файл для генерации: " << filename << std::endl;
    }
    return result_text;
}

/*std::string generateRandomText(size_t approx_target_chars, const std::string& filename = "random_text_with_runs.txt") {
    std::vector<std::pair<std::string, double>> weighted_chars;

    std::vector<std::string> RUS_LOWER = {
        "а", "б", "в", "г", "д", "е", "ж", "з", "и", "й", "к", "л", "м",
        "н", "о", "п", "р", "с", "т", "у", "ф", "х", "ц", "ч", "ш", "щ",
        "ы", "э", "ю", "я"
    };
    std::vector<std::string> COMMON_RUS_LOWER = {"о", "е", "а", "и", "н", "т", "с", "р", "в", "л"};
    double common_char_weight = 5.0;
    double rare_char_weight = 1.0;

    for (const auto& s : RUS_LOWER) {
        bool is_common = false;
        for (const auto& cs : COMMON_RUS_LOWER) {
            if (s == cs) {
                is_common = true;
                break;
            }
        }
        weighted_chars.push_back({s, (is_common ? common_char_weight : rare_char_weight)});
    }

    std::vector<std::string> DIGITS = {"0","1","2","3","4","5","6","7","8","9"};
    double digit_weight = 0.8;
    for (const auto& s : DIGITS) {
        weighted_chars.push_back({s, digit_weight});
    }

    double space_weight = 15.0;
    double punct_weight = 0.5;
    weighted_chars.push_back({" ", space_weight});
    weighted_chars.push_back({".", punct_weight * 2});
    weighted_chars.push_back({",", punct_weight * 1.5});
    weighted_chars.push_back({"!", punct_weight});
    weighted_chars.push_back({"?", punct_weight});
    weighted_chars.push_back({"-", punct_weight});
    weighted_chars.push_back({":", punct_weight});

    std::vector<std::string> char_pool;
    std::vector<double> weights;
    for (const auto& p : weighted_chars) {
        char_pool.push_back(p.first);
        weights.push_back(p.second);
    }

    std::random_device rd;
    std::mt19937 rng(rd());

    std::discrete_distribution<size_t> char_dist(weights.begin(), weights.end());

    std::uniform_real_distribution<double> run_probability_dist(0.0, 1.0);
    double probability_of_run = 0.20;
    std::uniform_int_distribution<size_t> run_length_dist(3, 10);

    std::string result_text;
    result_text.reserve(approx_target_chars * 2);

    size_t current_char_count = 0;

    while (current_char_count < approx_target_chars) {
        size_t char_idx = char_dist(rng);
        const std::string& selected_char_utf8 = char_pool[char_idx];

        if (run_probability_dist(rng) < probability_of_run && (current_char_count + 3 < approx_target_chars) ) {
            size_t length_of_run = run_length_dist(rng);
            length_of_run = std::min(length_of_run, approx_target_chars - current_char_count);
            if (length_of_run < 3 && current_char_count + length_of_run < approx_target_chars) {
                 result_text += selected_char_utf8;
                 current_char_count++;
            } else if (length_of_run >=3 ){
                for (size_t k = 0; k < length_of_run; ++k) {
                    result_text += selected_char_utf8;
                }
                current_char_count += length_of_run;
            } else {
                 result_text += selected_char_utf8;
                 current_char_count++;
            }
        } else {
            result_text += selected_char_utf8;
            current_char_count++;
        }
    }

    std::ofstream outfile(filename, std::ios::binary);
    if (outfile.is_open()) {
        outfile.write(result_text.data(), result_text.length());
        outfile.close();
        std::cout << "Сгенерирован файл '" << filename << "' (сгенерировано " << current_char_count
                  << " UTF-8 симв., размер файла " << result_text.length() << " байт)." << std::endl;
    } else {
        std::cerr << "Не удалось создать файл для генерации: " << filename << std::endl;
    }
    return result_text;
}*/

std::string advancedRleEncode(const std::string& input) {
    if (input.empty()) return "";

    std::stringstream encoded_ss;
    size_t i = 0;
    const size_t n = input.length();
    const char SEPARATOR = '#';

    while (i < n) {
        char current_char = input[i];
        size_t count = 1;
        size_t j = i + 1;
        while (j < n && input[j] == current_char) {
            count++;
            j++;
        }

        if (count >= MIN_RUN_LENGTH) {
            encoded_ss << std::to_string(count) << SEPARATOR << current_char;
            i = j;
        } else {
            size_t literal_start = i;
            size_t k = i;
            while(k < n) {
                char check_char_literal = input[k];
                size_t run_len_ahead = 0;
                size_t temp_k = k;
                while(temp_k < n && input[temp_k] == check_char_literal) {
                    run_len_ahead++;
                    temp_k++;
                }
                if (run_len_ahead >= MIN_RUN_LENGTH) {
                    break;
                }
                k += run_len_ahead;
            }
            size_t literal_length = k - literal_start;
            if (literal_length > 0) {
                encoded_ss << "-" << std::to_string(literal_length) << SEPARATOR;
                encoded_ss << input.substr(literal_start, literal_length);
            }
            i = k;
        }
    }
    return encoded_ss.str();
}

std::string advancedRleDecode(const std::string& encoded_input) {
    if (encoded_input.empty()) return "";

    std::stringstream decoded_ss;
    size_t i = 0;
    const size_t n = encoded_input.length();
    const char SEPARATOR = '#';

    while (i < n) {
        char first_byte_of_seq = encoded_input[i];
        bool is_negative_run = false;

        if (first_byte_of_seq == '-') {
            is_negative_run = true;
            i++;
            if (i >= n || !std::isdigit(static_cast<unsigned char>(encoded_input[i]))) {
                 throw std::runtime_error("RLE Decode: '-' not followed by a digit or EOF.");
            }
        } else if (!std::isdigit(static_cast<unsigned char>(first_byte_of_seq))) {
            throw std::runtime_error("RLE Decode: Sequence does not start with '-' or digit. Found: '" + std::string(1,first_byte_of_seq) + "' at position " + std::to_string(i));
        }

        std::string num_str;
        while (i < n && std::isdigit(static_cast<unsigned char>(encoded_input[i]))) {
            num_str += encoded_input[i];
            i++;
        }

        if (i >= n || encoded_input[i] != SEPARATOR) {
            throw std::runtime_error("RLE Decode: Missing separator '" + std::string(1, SEPARATOR) + "' after number at pos ~" + std::to_string(i) + " (num_str was " + num_str + ")");
        }
        i++;

        if (num_str.empty()) {
            throw std::runtime_error("RLE Decode: Failed to read number for length/count.");
        }

        int count_or_length = 0;
        try {
            count_or_length = std::stoi(num_str);
        } catch (const std::out_of_range& oor) {
            throw std::runtime_error("RLE Decode: Number '" + num_str + "' out of range for int.");
        } catch (const std::invalid_argument& ia) {
             throw std::runtime_error("RLE Decode: Number '" + num_str + "' is not a valid integer.");
        }

        if (count_or_length <= 0) {
             throw std::runtime_error("RLE Decode: Invalid count/length (<=0): " + std::to_string(count_or_length));
        }

        if (is_negative_run) {
            if (i + count_or_length > n) {
                throw std::runtime_error("RLE Decode: Not enough data for literal sequence. Expected " + std::to_string(count_or_length) + ", available " + std::to_string(n-i));
            }
            decoded_ss << encoded_input.substr(i, count_or_length);
            i += count_or_length;
        } else {
            if (i >= n) {
                 throw std::runtime_error("RLE Decode: Missing char_to_repeat after count and separator.");
            }
            char char_to_repeat = encoded_input[i];
            i++;
            for (int k_rep = 0; k_rep < count_or_length; ++k_rep) {
                decoded_ss << char_to_repeat;
            }
        }
    }
    return decoded_ss.str();
}

}

namespace Fano {

struct FanoNode {
    char symbol;
    double probability;

    FanoNode(char sym, double prob) : symbol(sym), probability(prob) {}
};

void generateFanoCodes(std::vector<FanoNode*>& nodes, int start, int end, std::string prefix, std::map<char, std::string>& codes) {
    if (start > end) {
        return;
    }
    if (start == end) {
        codes[nodes[start]->symbol] = (prefix.empty() && nodes.size() == 1) ? "0" : prefix;
        return;
    }

    double totalWeight = 0;
    for (int i = start; i <= end; ++i) {
        totalWeight += nodes[i]->probability;
    }

    if (totalWeight == 0) {
        for(int i = start; i <= end; ++i) {
            codes[nodes[i]->symbol] = prefix + ( (i-start) % 2 == 0 ? "0" : "1");
        }
        return;
    }

    double partialWeight = 0;
    int splitIndex = start;
    for (; splitIndex < end; ++splitIndex) {
        if (partialWeight + nodes[splitIndex]->probability > totalWeight / 2.0 && partialWeight > 0) {
            splitIndex--;
            break;
        }
        partialWeight += nodes[splitIndex]->probability;
        if (partialWeight >= totalWeight / 2.0) {
            break;
        }
    }
    if (splitIndex < start) splitIndex = start;
    if (splitIndex >= end && start < end) splitIndex = end - 1;


    generateFanoCodes(nodes, start, splitIndex, prefix + "0", codes);
    if (splitIndex + 1 <= end) {
         generateFanoCodes(nodes, splitIndex + 1, end, prefix + "1", codes);
    }
}

std::map<char, std::string> buildFanoCodes(const std::string& text) {
    std::map<char, double> frequencies;
    if (text.empty()) {
        return {};
    }
    for (char c : text) {
        frequencies[c]++;
    }

    std::vector<FanoNode*> nodes;
    nodes.reserve(frequencies.size());
    for (auto& pair : frequencies) {
        nodes.push_back(new FanoNode(pair.first, pair.second / text.size()));
    }

    std::sort(nodes.begin(), nodes.end(), [](FanoNode* a, FanoNode* b) {
        return a->probability > b->probability;
    });

    std::map<char, std::string> codes;
    if (!nodes.empty()) {
        generateFanoCodes(nodes, 0, static_cast<int>(nodes.size()) - 1, "", codes);
    }

    for (FanoNode* node_ptr : nodes) {
        delete node_ptr;
    }

    return codes;
}

std::string encodeFano(const std::string& text, const std::map<char, std::string>& codes) {
    std::string encoded_bit_string;
    if (text.empty() || codes.empty()) return encoded_bit_string;

    encoded_bit_string.reserve(text.length() * 4);

    for (char c : text) {
        auto it = codes.find(c);
        if (it != codes.end()) {
            encoded_bit_string += it->second;
        } else {
             std::cerr << "ПРЕДУПРЕЖДЕНИЕ (encodeFano): Код для символа '" << c << "' (ASCII: " << static_cast<int>(static_cast<unsigned char>(c)) << ") не найден!" << std::endl;
        }
    }
    return encoded_bit_string;
}

std::string decodeFano(const std::string& encoded_bit_string, const std::map<char, std::string>& codes) {
    if (encoded_bit_string.empty() || codes.empty()) return "";

    std::map<std::string, char> reversedCodes;
    for (auto& pair : codes) {
        if (!pair.second.empty()) {
            reversedCodes[pair.second] = pair.first;
        }
    }

    std::string decoded_text;
    std::string current_code_buffer;
    decoded_text.reserve(encoded_bit_string.length());

    for (char bit_char : encoded_bit_string) {
        current_code_buffer += bit_char;
        auto it = reversedCodes.find(current_code_buffer);
        if (it != reversedCodes.end()) {
            decoded_text += it->second;
            current_code_buffer.clear();
        }
    }
    if (!current_code_buffer.empty()) {
         std::cerr << "ПРЕДУПРЕЖДЕНИЕ (decodeFano): Остались необработанные биты в буфере: " << current_code_buffer << std::endl;
    }
    return decoded_text;
}

}

void handleHashTableDictionary();
void handleRBTreeDictionary();
void handleRleOperations();

template<typename DictType>
void dictionarySubMenuLoop(DictType& dictionary, const std::string& dict_name);

void printMainMenu() {
    std::cout << "\n--- Главное Меню ---" << std::endl;
    std::cout << "1. Работать со словарем на Хеш-таблице" << std::endl;
    std::cout << "2. Работать со словарем на Красно-Черном дереве" << std::endl;
    std::cout << "3. RLE кодирование/декодирование текста" << std::endl;
    std::cout << "0. Выход" << std::endl;
    std::cout << "Ваш выбор: ";
}

void printDictionaryMenu(const std::string& dictionary_type) {
    std::cout << "\n--- Меню Словаря (" << dictionary_type << ") ---" << std::endl;
    std::cout << "1. Добавить слово" << std::endl;
    std::cout << "2. Удалить слово" << std::endl;
    std::cout << "3. Найти слово" << std::endl;
    std::cout << "4. Загрузить словарь из файла (перезаписать)" << std::endl;
    std::cout << "5. Дополнить словарь из файла" << std::endl;
    std::cout << "6. Очистить словарь" << std::endl;
    std::cout << "7. Показать текущее содержимое словаря (стандартный print)" << std::endl;
    std::cout << "8. Визуализировать структуру" << std::endl;
    std::cout << "0. Вернуться в главное меню" << std::endl;
    std::cout << "Ваш выбор: ";
}

void printRleMenu() { // Новое название, если старое используется
    std::cout << "\n--- Меню RLE и Статистического Сжатия ---" << std::endl;
    std::cout << "1. Одноступенчатый RLE (генерация текста)" << std::endl;
    std::cout << "2. Двухступенчатый RLE -> RLE (генерация текста)" << std::endl;
    std::cout << "3. Двухступенчатый RLE -> Фано (генерация текста)" << std::endl;
    std::cout << "4. RLE для файла 'sample_text_rus.txt'" << std::endl;
    std::cout << "5. Фано для файла 'sample_text_rus.txt'" << std::endl;
    std::cout << "0. Вернуться в главное меню" << std::endl;
    std::cout << "Ваш выбор: ";
}

int getUserChoice(int min_val, int max_val) {
    int choice;
    while (true) {
        std::string line;
        std::getline(std::cin, line);
        std::stringstream ss(line);
        if (ss >> choice && ss.eof()) {
            if (choice >= min_val && choice <= max_val) {
                return choice;
            }
        }
        std::cout << "Некорректный ввод. Пожалуйста, введите число от " << min_val << " до " << max_val << ": ";
    }
}


int main() {
    setupConsole();

    int main_choice;
    do {
        printMainMenu();
        main_choice = getUserChoice(0, 3);

        switch (main_choice) {
            case 1:
                handleHashTableDictionary();
                break;
            case 2:
                handleRBTreeDictionary();
                break;
            case 3:
                handleRleOperations();
                break;
            case 0:
                std::cout << "Выход из программы." << std::endl;
                break;
        }
    } while (main_choice != 0);

    return 0;
}


void handleHashTableDictionary() {
    using namespace DictionaryWithHashTable;
    static Dictionary dict_ht;
    dictionarySubMenuLoop(dict_ht, "Хеш-таблица");
}

void handleRBTreeDictionary() {
    using namespace DictionaryWithRBTree;
    static Dictionary dict_rbt;
    dictionarySubMenuLoop(dict_rbt, "КЧ-дерево");
}

template<typename DictType>
void dictionarySubMenuLoop(DictType& dictionary, const std::string& dict_name) {
    int dict_choice;
    std::string word, filepath;

    do {
        printDictionaryMenu(dict_name);
        dict_choice = getUserChoice(0, 8);

        try {
            switch (dict_choice) {
                case 1:
                    std::cout << "Введите слово для добавления: ";
                    std::getline(std::cin, word);
                    dictionary.addWord(word);
                    std::cout << "Слово '" << word << "' обработано." << std::endl;
                    break;
                case 2:
                    std::cout << "Введите слово для удаления: ";
                    std::getline(std::cin, word);
                    dictionary.removeWord(word);
                    std::cout << "Слово '" << word << "' удалено (если существовало)." << std::endl;
                    break;
                case 3:
                    std::cout << "Введите слово для поиска: ";
                    std::getline(std::cin, word);
                    dictionary.findWord(word);
                    break;
                case 4:
                    std::cout << "Введите имя файла для загрузки (например, sample_text_rus.txt): ";
                    std::getline(std::cin, filepath);
                    dictionary.loadFromFile(filepath, false);
                    break;
                case 5:
                    std::cout << "Введите имя файла для дополнения (например, sample_text_rus.txt): ";
                    std::getline(std::cin, filepath);
                    dictionary.loadFromFile(filepath, true);
                    break;
                case 6:
                    dictionary.clear();
                    break;
                case 7:
                    std::cout << "Содержимое словаря: ";
                    dictionary.print();
                    std::cout << std::endl;
                    break;
                case 8:
                    dictionary.visualizeStructure();
                    break;
                case 0:
                    std::cout << "Возврат в главное меню..." << std::endl;
                    break;
            }
        } catch (const std::runtime_error& e) {
            std::cerr << "Произошла ошибка: " << e.what() << std::endl;
        }
    } while (dict_choice != 0);
}


void handleRleOperations() {
    int rle_choice;
    std::string filename;
    std::string text_to_process;

    auto print_compression_ratio = [](const std::string& stage_name, size_t original_size, size_t compressed_size) {
        if (compressed_size > 0) {
            double ratio = static_cast<double>(original_size) / compressed_size;
            std::cout << "Коэффициент сжатия (" << stage_name << "): "
                      << std::fixed << std::setprecision(2) << ratio << std::endl;
        } else if (original_size == 0 && compressed_size == 0) {
            std::cout << "Коэффициент сжатия (" << stage_name << "): N/A (исходные и сжатые данные пусты)." << std::endl;
        }
        else {
            std::cout << "Коэффициент сжатия (" << stage_name << "): N/A (сжатый размер 0, исходный > 0 - это ошибка или идеальное сжатие)." << std::endl;
        }
    };

    do {
        printRleMenu();
        rle_choice = getUserChoice(0, 5);

        try {
            switch (rle_choice) {
            case 1:
                {
                    const size_t TEXT_SIZE_FOR_DEBUG = 10000;
                    std::string original_text = RLE::generateRandomText(TEXT_SIZE_FOR_DEBUG, "debug_rle_generated.txt");

                    if (original_text.empty()) {
                        std::cout << "Не удалось сгенерировать текст для отладки." << std::endl;
                        break;
                    }

                    std::cout << "\n--- ОТЛАДКА RLE: ОДНОСТУПЕНЧАТЫЙ ---" << std::endl;
                    std::cout << "Размер исходного текста: " << original_text.length() << " байт (" << TEXT_SIZE_FOR_DEBUG << " UTF-8 символов было запрошено)." << std::endl;
                    std::cout << "Исходный сгенерированный текст:" << std::endl;


                    std::string encoded_text = RLE::advancedRleEncode(original_text);
                    std::cout << "\nРазмер закодированного текста: " << encoded_text.length() << " байт." << std::endl;
                    std::cout << "Закодированный текст:" << std::endl;

                    try {
                        std::string decoded_text = RLE::advancedRleDecode(encoded_text);
                        std::cout << "\nРазмер декодированного текста: " << decoded_text.length() << " байт." << std::endl;

                        if (decoded_text == original_text) {
                            std::cout << "Проверка RLE: Декодирование ВЕРНО." << std::endl;
                        } else {
                            std::cout << "Проверка RLE: ОШИБКА декодирования!" << std::endl;
                            std::cout << "Сравнение байт (оригинал | декод | позиция расхождения):" << std::endl;
                            for(size_t k=0; k < std::max(original_text.length(), decoded_text.length()); ++k) {
                                unsigned char orig_char = (k < original_text.length()) ? static_cast<unsigned char>(original_text[k]) : 0;
                                unsigned char dec_char = (k < decoded_text.length()) ? static_cast<unsigned char>(decoded_text[k]) : 0;
                                if (orig_char != dec_char || k >= original_text.length() || k >= decoded_text.length()) {
                                     std::cout << "Поз " << k << ": "
                                               << static_cast<int>(orig_char) << " ('" << ((k < original_text.length()) ? original_text[k] : ' ') << "')"
                                               << " | "
                                               << static_cast<int>(dec_char) << " ('" << ((k < decoded_text.length()) ? decoded_text[k] : ' ') << "')"
                                               << (orig_char != dec_char ? " <--- РАСХОЖДЕНИЕ" : "")
                                               << std::endl;
                                    if (orig_char != dec_char) break;
                                }
                            }
                        }
                    } catch (const std::runtime_error& e) {
                        std::cerr << "Произошла ошибка RLE при декодировании: " << e.what() << std::endl;
                    }
                    std::cout << "--- КОНЕЦ ОТЛАДКИ RLE ---" << std::endl;
                }
                break;
                case 2:
                     {
                        std::string random_text = RLE::generateRandomText(10000, "random_rle_test_2stage.txt");
                        if (random_text.empty()) break;
                        std::cout << "Исходный текст: " << random_text.substr(0, std::min((size_t)50, random_text.length())) << "..." << std::endl;
                        std::string encoded1 = RLE::advancedRleEncode(random_text);
                        std::cout << "1-й этап RLE: " << encoded1.substr(0, std::min((size_t)50, encoded1.length())) << "..." << std::endl;
                        std::string encoded2 = RLE::advancedRleEncode(encoded1);
                        std::cout << "2-й этап RLE: " << encoded2.substr(0, std::min((size_t)50, encoded2.length())) << "..." << std::endl;
                        std::cout << "Размер исходного: " << random_text.length() << ", после 1-го этапа: " << encoded1.length() << ", после 2-го этапа: " << encoded2.length() << std::endl;

                        std::string decoded_stage2 = RLE::advancedRleDecode(encoded2);
                        std::string decoded_final = RLE::advancedRleDecode(decoded_stage2);
                        if (decoded_final == random_text) {
                            std::cout << "Проверка двухступенчатого RLE: Декодирование ВЕРНО." << std::endl;
                        } else {
                            std::cout << "Проверка двухступенчатого RLE: ОШИБКА декодирования!" << std::endl;
                        }
                    }
                    break;
                case 3:
                    {
                        const size_t TEXT_SIZE_FOR_DEBUG = 10000;
                        std::string original_text = RLE::generateRandomText(TEXT_SIZE_FOR_DEBUG, "debug_rle_fano.txt");
                        if (original_text.empty()) {
                             std::cout << "Не удалось сгенерировать текст." << std::endl;
                             break;
                        }
                        std::cout << "\n--- Тест: Двухступенчатый RLE -> Фано ---" << std::endl;
                        std::cout << "Размер исходного текста: " << original_text.length() << " байт." << std::endl;

                        std::string rle_encoded_text = RLE::advancedRleEncode(original_text);
                        std::cout << "Размер после RLE: " << rle_encoded_text.length() << " байт." << std::endl;
                        if (rle_encoded_text.empty() && !original_text.empty()) {
                             std::cout << "RLE кодирование вернуло пустую строку для непустого входа!" << std::endl;
                             break;
                        }
                         if (rle_encoded_text.empty() && original_text.empty()) {
                             std::cout << "Исходный текст пуст, RLE также пуст." << std::endl;
                        }

                        std::cout << "Применение Фано к результату RLE..." << std::endl;
                        std::map<char, std::string> fano_codes = Fano::buildFanoCodes(rle_encoded_text);

                        std::string fano_encoded_bit_string = Fano::encodeFano(rle_encoded_text, fano_codes);
                        size_t fano_bit_string_len_bytes = fano_encoded_bit_string.length();

                        std::cout << "Размер после Фано (длина битовой строки): " << fano_bit_string_len_bytes << " символов ('0'/'1')." << std::endl;
                        double fano_compression_over_rle = 0.0;
                        if (fano_bit_string_len_bytes > 0) {
                             fano_compression_over_rle = (static_cast<double>(rle_encoded_text.length()) * 8.0) / fano_bit_string_len_bytes;
                        }
                        std::cout << "Условный коэфф. сжатия Ф поверх RLE (биты RLE / биты Ф): "
                                  << std::fixed << std::setprecision(2) << fano_compression_over_rle << std::endl;

                        // Общий "условный" коэффициент сжатия
                        double overall_pseudo_ratio = 0.0;
                        if (fano_bit_string_len_bytes > 0) {
                            overall_pseudo_ratio = (static_cast<double>(original_text.length()) * 8.0) / fano_bit_string_len_bytes;
                        }
                         std::cout << "Общий УСЛОВНЫЙ коэфф. сжатия (биты оригинала / биты Ф): "
                                  << std::fixed << std::setprecision(2) << overall_pseudo_ratio << std::endl;


                        // 3. Декодирование
                        std::cout << "Декодирование..." << std::endl;
                        std::string decoded_from_fano = Fano::decodeFano(fano_encoded_bit_string, fano_codes);
                        if (decoded_from_fano == rle_encoded_text) {
                            std::cout << "Декодирование Фано -> RLE: ВЕРНО." << std::endl;
                            std::string final_decoded_text = RLE::advancedRleDecode(decoded_from_fano);
                            if (final_decoded_text == original_text) {
                                std::cout << "Полное декодирование RLE -> Фано -> RLE -> Оригинал: ВЕРНО." << std::endl;
                            } else {
                                std::cout << "ОШИБКА полного декодирования!" << std::endl;
                            }
                        } else {
                            std::cout << "ОШИБКА декодирования Фано -> RLE!" << std::endl;
                        }
                    }
                    break;
                case 4:
                    std::cout << "Обработка файла 'sample_text_rus.txt'..." << std::endl;
                    try {
                        text_to_process = readFileToString("sample_text_rus.txt");
                        std::cout << "Исходный текст из файла: " << text_to_process.substr(0, std::min((size_t)50, text_to_process.length())) << "..." << std::endl;
                        std::string encoded = RLE::advancedRleEncode(text_to_process);
                        std::cout << "Закодировано RLE: " << encoded.substr(0, std::min((size_t)50, encoded.length())) << "..." << std::endl;
                        std::cout << "Размер исходного: " << text_to_process.length() << ", закодированного: " << encoded.length() << std::endl;
                        std::string decoded = RLE::advancedRleDecode(encoded);
                        if (decoded == text_to_process) {
                            std::cout << "Проверка RLE для файла: Декодирование ВЕРНО." << std::endl;
                        } else {
                            std::cout << "Проверка RLE для файла: ОШИБКА декодирования!" << std::endl;
                        }
                    } catch (const std::runtime_error& e_file) {
                         std::cerr << "Ошибка при работе с файлом 'sample_text_rus.txt': " << e_file.what() << std::endl;
                    }
                    break;
                case 5:
                    std::cout << "Обработка файла 'sample_text_rus.txt'..." << std::endl;
                    try {
                        text_to_process = readFileToString("sample_text_rus.txt");
                        std::cout << "Исходный текст из файла: " << text_to_process.substr(0, std::min((size_t)50, text_to_process.length())) << "..." << std::endl;
                        std::map<char, std::string> fano_codes = Fano::buildFanoCodes(text_to_process);
                        std::string fano_encoded_bit_string = Fano::encodeFano(text_to_process, fano_codes);
                        size_t fano_bit_string_len_bytes = fano_encoded_bit_string.length();

                        std::cout << "Размер после Фано (длина битовой строки): " << fano_bit_string_len_bytes << " символов ('0'/'1')." << std::endl;
                        double fano_compression_over_rle = 0.0;
                        if (fano_bit_string_len_bytes > 0) {
                             fano_compression_over_rle = (static_cast<double>(text_to_process.length()) * 8.0) / fano_bit_string_len_bytes;
                        }
                        std::cout << "Условный коэфф. сжатия Ф: "
                                  << std::fixed << std::setprecision(2) << fano_compression_over_rle << std::endl;
                        std::string decoded_from_fano = Fano::decodeFano(fano_encoded_bit_string, fano_codes);
                        if (decoded_from_fano == text_to_process) {
                            std::cout << "Проверка Фано для файла: Декодирование ВЕРНО." << std::endl;
                        } else {
                            std::cout << "Проверка Фано для файла: ОШИБКА декодирования!" << std::endl;
                        }
                    } catch (const std::runtime_error& e_file) {
                         std::cerr << "Ошибка при работе с файлом 'sample_text_rus.txt': " << e_file.what() << std::endl;
                    }
                    break;
                case 0:
                    std::cout << "Возврат в главное меню..." << std::endl;
                    break;
            }
        } catch (const std::runtime_error& e) {
             std::cerr << "Произошла ошибка RLE: " << e.what() << std::endl;
        }
    } while (rle_choice != 0);
}


