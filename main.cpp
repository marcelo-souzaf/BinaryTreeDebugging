#include "./BST.hpp"
#include "./vis.hpp"

#include <random>

/*
    O código abaixo pode ser usado para inserir valores aleatórios na árvore.
    Note que apenas as chaves são exibidas, pois os valores são irrelevantes
    para a organização geral da estrutura.
*/

using std::cout;
using std::endl;
using std::vector;

void add_int_elements(int count, vector<int>& vec, BST<int, int>& bst) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<int> distrib(-99, 99);
    for (int i = 0; i < count; ++i) {
        try {
            int value = distrib(gen);
            vec.push_back(value);
            bst.insert(value, value);
        } catch (std::invalid_argument& e) {
            vec.pop_back();
            --i;
        }
    }
}

void add_float_elements(int count, vector<float>& vec, BST<float, float>& bst) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> distrib(-999, 999);
    for (int i = 0; i < count; ++i) {
        try {
            float value = distrib(gen);
            vec.push_back(value);
            bst.insert(value, value);
        } catch (std::invalid_argument& e) {
            vec.pop_back();
            --i;
        }
    }
}

int main() {
    BST<int, int> bst;
    constexpr int n = 24;
    vector<int> vec;
    vec.reserve(n);
    add_int_elements(n / 2, vec, bst);

    // BST<float, float> bst;
    // constexpr int n = 24;
    // vector<float> vec;
    // vec.reserve(n);
    // add_float_elements(n / 2, vec, bst);

    // bst.print();
    // bst.print_inorder();
    // bst.print_postorder();
    // bst.print_preorder();
    // bst.print_breadth();

    Visualization<decltype(bst)::Node*> system(bst.get_root(), false, 1280, 720);
    system.draw(10, true);

    // add_float_elements(n / 2, vec, bst);
    add_int_elements(n / 2, vec, bst);
    
    system.draw(120, false);

    return 0;
}
