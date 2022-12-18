#include "./BST.hpp"
#include "./vis.hpp"

// #include <chrono>
// #include <thread>
#include <random>

using std::cout;
using std::endl;

int main() {
    BST<int, int> bst;

    /*
        O código abaixo pode ser usado para inserir valores aleatórios na árvore.
        Note que apenas as chaves são exibidas, pois os valores são irrelevantes
        para a organização geral da estrutura.
    */
    // const int n = 24;
    // std::vector<int> vec;
    // vec.reserve(n);
    // std::random_device rd;
    // std::mt19937 gen(rd());
    // std::uniform_int_distribution<int> distrib(-999, 999);
    // for (int i = 0; i < n; ++i) {
    //     try {
    //         int value = distrib(gen);
    //         vec.push_back(value);
    //         bst.insert(value, 0);
    //     } catch (std::invalid_argument& e) {
    //         vec.pop_back();
    //         --i;
    //     }
    // }
    // cout << vec << endl;

    std::vector<int> vec = {520, -309, 950, 685, -50, 296, 348, -393, 807, 474, -974, 817, -267, 198, -540, 954, 769, -946, 243, 604, -167, 672, 461, 508};
    for (int value : vec) 
        bst.insert(value, 0);

    // bst.print();
    // bst.print_inorder();
    // bst.print_postorder();
    // bst.print_preorder();
    // bst.print_breadth();

    Visualization<BST<int, int>::Node*> system(bst.get_root(), false, 1280, 720);

    system.draw(30, false);
    cout << "Done" << endl;
    // std::cin.get();
    return 0;
}
