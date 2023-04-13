#ifndef BST_HPP_
#define BST_HPP_

#include <iostream>
#include <string>
#include <vector>

// Implementação simplória de uma Binary Search Tree feita para
// a disciplina de Estruturas de Dados, agora tenho como reaproveitar

template<class K, class V>
class BST {
 public:
    struct Node {
        Node* m_left;
        Node* m_right;
        Node* m_parent;
        K m_key;
        V m_value;
        uint size;

        Node(K key, V value) {
            this->m_left = nullptr;
            this->m_right = nullptr;
            this->m_parent = nullptr;
            this->m_value = value;
            this->m_key = key;
            this->size = 1;
        }

        // Visualização:
        Node* left() {
            return this->m_left;
        }

        Node* right() {
            return this->m_right;
        }

        Node* parent() {
            return this->m_parent;
        }

        K key() {
            return this->m_key;
        }

        V value() {
            return this->m_value;
        }
    };

    BST() {
        this->root = nullptr;
        this->is_list = false;
    }

    ~BST() {
        if (this->is_list) {
            Node* current = this->root;
            if (current) {
                Node* next = current->m_right;
                while (next) {
                    delete current;
                    current = next;
                    next = current->m_right;
                }
                delete current;
            }
        } else {
            this->postorder_delete(this->root);
        }
    }

    Node* get_root() {
        return this->root;
    }

    void insert(const K key, const V value) {
        this->root = this->insert(key, value, this->root);
    }

    V& search(const K key) {
        Node* node = this->search(key, this->root);
        if (!node) {
            throw std::invalid_argument("Key " + std::to_string(key) +
                " not found.");
        }
        return node->m_value;
    }

    void to_list() {
        if (!this->root) {
            return;
        }
        this->is_list = true;
        std::vector<Node*> current_queue;
        std::vector<Node*> next_queue;
        std::vector<Node*>* cur = &current_queue;
        std::vector<Node*>* next = &next_queue;
        std::vector<Node*>* temp;
        Node* node = this->root;
        bool is_even = true;
        if (node->m_left) {
            cur->push_back(node->m_left);
        }
        if (node->m_right) {
            cur->push_back(node->m_right);
        }
        node->m_left = nullptr;
        node->m_right = nullptr;
        do {
            if (is_even) {
                for (int i = cur->size() - 1; i >= 0; --i) {
                    node = cur->at(i);
                    if (node->m_right) {
                        next->push_back(node->m_right);
                    }
                    if (node->m_left) {
                        next->push_back(node->m_left);
                    }
                }
                node->m_parent->m_right = node;
                node->m_left = node->m_parent;
            } else {
                for (int i = cur->size() - 1; i >= 0; --i) {
                    node = cur->at(i);
                    if (node->m_left) {
                        next->push_back(node->m_left);
                    }
                    if (node->m_right) {
                        next->push_back(node->m_right);
                    }
                }
                node->m_parent->m_right = node;
                node->m_left = node->m_parent;
            }
            for (int j = 0; j < cur->size() - 1; ++j) {
                cur->at(j)->m_right = cur->at(j + 1);
                cur->at(j + 1)->m_left = cur->at(j);
            }
            cur->back()->m_right = nullptr;
            cur->clear();
            temp = cur;
            cur = next;
            next = temp;
            is_even = !is_even;
        } while (cur->size());
    }

    void print_preorder() {
        this->print_preorder(this->root);
        std::cout << std::endl;
    }

    void print_inorder() {
        this->print_inorder(this->root);
        std::cout << std::endl;
    }

    void print_postorder() {
        this->print_postorder(this->root);
        std::cout << std::endl;
    }

    void print_breadth() {
        std::vector<Node*> queue(this->root->size);
        Node* node;
        uint i = 0;
        queue.push_back(this->root);
        while (i < queue.size()) {
            node = queue[i++];
            std::cout << node->m_key << ", ";
            if (node->m_left) {
                queue.push_back(node->m_left);
            }
            if (node->m_right) {
                queue.push_back(node->m_right);
            }
        }
        std::cout << std::endl;
    }

    void printOdd() {
        this->printOdd(this->root, true);
        std::cout << std::endl;
    }

    void printMaxK(int k) {
        this->keys.reserve(k);
        this->printMaxK(this->root, k);
        std::cout << "[ ";
        for (int i = k - 1; i >= 0; --i) {
            std::cout << this->keys[i] << ", ";
        }
        std::cout << "]" << std::endl;
        this->keys.clear(false);
    }

    void grow_doubles(int max_k = 15) {
        if (this->root) {
            throw std::runtime_error("Tree is not empty.");
        }
        this->root = this->grow_doubles(1, max_k);
    }

    Node* grow_doubles(int k, int max_k) {
        if (k > max_k) {
            return nullptr;
        }
        Node* cur = new Node(k, k);
        cur->m_left = this->grow_doubles(k * 2, max_k);
        if (cur->m_left) {
            cur->m_left->m_parent = cur;
            cur->size += cur->m_left->size;
        }
        cur->m_right = this->grow_doubles(k * 2 + 1, max_k);
        if (cur->m_right) {
            cur->m_right->m_parent = cur;
            cur->size += cur->m_right->size;
        }
        return cur;
    }

    void print() const {
        if (!this->root) {
            return;
        }
        if (this->is_list) {
            Node* node = this->root;
            std::cout << "[ ";
            while (node->m_right) {
                std::cout << node->m_key << ", ";
                node = node->m_right;
            }
            std::cout << node->m_key << " ]";
        } else {
            print("", this->root, false);
        }
        std::cout << std::endl;
    }

 private:
    Node* root;
    std::vector<K> keys;
    bool is_list;

    Node* search(const K key, Node* node) {
        if (!node) {
            return nullptr;
        }
        if (key == node->m_key) {
            return node;
        }
        if (key > node->m_key) {
            return this->search(key, node->m_right);
        }
        return this->search(key, node->m_left);
    }

    Node* rotate_right(Node* node) {
        Node* other = node->m_left;
        node->m_left = other->m_right;
        other->m_right = node;
        return other;
    }

    Node* rotate_left(Node* node) {
        Node* other = node->m_right;
        node->m_right = other->m_left;
        other->m_left = node;
        return other;
    }

    Node* insert(const K key, const V value, Node* node) {
        if (!node) {
            return new Node(key, value);
        }
        if (key < node->m_key) {
            node->m_left = this->insert(key, value, node->m_left);
            node->m_left->m_parent = node;
        } else if (key > node->m_key) {
            node->m_right = this->insert(key, value, node->m_right);
            node->m_right->m_parent = node;
        } else {
            throw std::invalid_argument("Can't insert duplicated key " +
                std::to_string(key) + ".");
        }
        ++node->size;
        return node;
    }

    void print(const std::string& prefix, Node* node, bool is_left) const {
        if (node) {
            std::cout << prefix;
            std::cout << (is_left ? "├── " : "└── ");

            std::cout << node->m_key << " " << node->size << std::endl;

            print(prefix + (is_left ? "│   " : "    "), node->m_left, true);
            print(prefix + (is_left ? "│   " : "    "), node->m_right, false);
        }
    }

    void print_preorder(Node* node) {
        if (node) {
            std::cout << node->m_key << ", ";
            this->print_preorder(node->m_left);
            this->print_preorder(node->m_right);
        }
    }

    void print_inorder(Node* node) {
        if (node) {
            this->print_inorder(node->m_left);
            std::cout << node->m_key << ", ";
            this->print_inorder(node->m_right);
        }
    }

    void print_postorder(Node* node) {
        if (node) {
            this->print_postorder(node->m_left);
            this->print_postorder(node->m_right);
            std::cout << node->m_key << ", ";
        }
    }

    bool printOdd(Node* node, bool is_odd) {
        if (node) {
            is_odd = this->printOdd(node->m_left, is_odd);
            if (is_odd) {
                std::cout << node->m_key << ", ";
            }
            is_odd = this->printOdd(node->m_right, !is_odd);
        }
        return is_odd;
    }

    void printMaxK(Node* node, uint k) {
        if (node->m_right) {
            this->printMaxK(node->m_right, k);
        }
        if (this->keys.size() < k) {
            this->keys.push_back(node->m_key);
        }
        if (node->m_left) {
            if (this->keys.size() < k) {
                this->printMaxK(node->m_left, k);
            }
        }
    }
    
    void postorder_delete(Node* node) {
        if (node) {
            postorder_delete(node->m_left);
            postorder_delete(node->m_right);
            delete node;
        }
    }
};

#endif  // BST_HPP_
