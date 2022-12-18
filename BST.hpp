#ifndef BST_HPP_
#define BST_HPP_

#include <iostream>
#include <string>
#include <vector>

// Implementação simplória de uma Binary Search Tree feita para
// a disciplina de Estruturas de Dados, agora tenho como reaproveitar

template<class T1, class T2>
struct BST {
    struct Node {
        Node* m_left;
        Node* m_right;
        Node* m_parent;
        T1 m_key;
        T2 m_value;
        uint size;

        Node(T1 key, T2 value) {
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

        T1 key() {
            return this->m_key;
        }

        T2 value() {
            return this->m_value;
        }
    };

    Node* root;
    std::vector<T1> keys;
    bool is_list;

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

    void postorder_delete(Node* node) {
        if (node) {
            postorder_delete(node->m_left);
            postorder_delete(node->m_right);
            delete node;
        }
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

    Node* insert(T1 key, T2 value, Node* node) {
        if (!node) {
            if (!this->root) {
                this->root = new Node(key, value);
            }
            return new Node(key, value);
        }
        if (node->m_key < key) {
            node->m_right = this->insert(key, value, node->m_right);
            node->m_right->m_parent = node;
        } else if (node->m_key > key) {
            node->m_left = this->insert(key, value, node->m_left);
            node->m_left->m_parent = node;
        } else {
            throw std::invalid_argument("Can't insert duplicated key " +
                std::to_string(key) + ".");
        }
        ++node->size;
        return node;
    }

    void insert(T1 key, T2 value) {
        this->insert(key, value, this->root);
    }

    Node* search(T1 key, Node* node) {
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

    Node* to_list() {
        if (!this->root) {
            return nullptr;
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
            cur->clear(false);
            temp = cur;
            cur = next;
            next = temp;
            is_even = !is_even;
        } while (cur->size());
        return this->root;
    }

    void printOdd() {
        this->printOdd(this->root, true);
        std::cout << std::endl;
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

    void print_inorder() {
        this->print_inorder(this->root);
        std::cout << std::endl;
    }

    void print_inorder(Node* node) {
        if (node) {
            this->print_inorder(node->m_left);
            std::cout << node->m_key << ", ";
            this->print_inorder(node->m_right);
        }
    }

    void print_preorder() {
        this->print_preorder(this->root);
        std::cout << std::endl;
    }

    void print_preorder(Node* node) {
        if (node) {
            std::cout << node->m_key << ", ";
            this->print_preorder(node->m_left);
            this->print_preorder(node->m_right);
        }
    }

    void print_postorder() {
        this->print_postorder(this->root);
        std::cout << std::endl;
    }

    void print_postorder(Node* node) {
        if (node) {
            this->print_postorder(node->m_left);
            this->print_postorder(node->m_right);
            std::cout << node->m_key << ", ";
        }
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

    void print(const std::string& prefix, Node* node, bool is_left) const {
        if (node) {
            std::cout << prefix;
            std::cout << (is_left ? "├── " : "└── ");

            std::cout << node->m_key << " " << node->size << std::endl;

            print(prefix + (is_left ? "│   " : "    "), node->m_left, true);
            print(prefix + (is_left ? "│   " : "    "), node->m_right, false);
        }
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
};

#endif  // BST_HPP_
