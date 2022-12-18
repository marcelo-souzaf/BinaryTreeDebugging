#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <freetype2/ft2build.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include FT_FREETYPE_H

#include <array>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

typedef unsigned int uint;

const int number_of_circle_sides = 90;

/// Namespace que contém informações globais quanto ao funcionamento da janela,
/// permitindo que eles sejam modificados mesmo por funções estáticas.
namespace vis {
    GLFWwindow* window;
    uint width;
    uint height;
}

/// Exibe os elementos de um vetor de tipo genérico.
template<typename T>
std::ostream& operator<<(std::ostream& os, std::vector<T>& data) {
    os << '[';
    if (data.size() == 0) {
        os << " ]";
    } else {
        for (int i = 0; i < data.size() - 1; ++i) {
            os << data[i] << ", ";
        }
        os << data.back() << ']';
    }
    return os;
}

template<typename T>
class Visualization {
 public:
    /**
     * Contrói um objeto de visualização que carrega os dados
     * necessários para a exibição de uma árvore binária usando OpenGL.
     * 
     * @tparam T Ponteiro para o tipo do nó da árvore.
     * @param node Ponteiro para o nó raiz da árvore.
     * @param fullscreen Define se o aplicativo abre em tela cheia.
     * @param width Válido apenas se fullscreen for falso, define a largura da janela.
     * @param height Válido apenas se fullscreen for falso, define a altura da janela.
     * @param path_to_font Caminho até a fonte a ser usada.
     */
    Visualization(T node, bool fullscreen = false, uint width = 1024,
    uint height = 768, const std::string& path_to_font = \
    "dependencies/RobotoMono-Medium.ttf") : root_node(node) {
        vis::width = width;
        vis::height = height;
        this->buffer = new char[1024];
        this->glyph_map = new Glyph[128];
        this->stride = false;
        for (int i = 0; i < 3; ++i) {
            this->shaders[i] = 0;
            this->VAO[i] = 0;
            this->VBO[i] = 0;
        }
        this->start(path_to_font, fullscreen);
    }

    ~Visualization() {
        this->reset();
        delete[] this->buffer;
        delete[] this->glyph_map;
    }

    void set_root(T root) {
        this->root_node = root;
    }

    /// Redefine o tamanho da janela.
    static void set_window_size(uint width, uint height) {
        set_window_size(vis::window, width, height);
    }

    // Redefine o tamanho da janela. Chamada sempre que a janela é redimensionada.
    static void set_window_size(GLFWwindow* window, int width, int height) {
        vis::width = width;
        vis::height = height;
        glViewport(0, 0, width, height);
        if (glfwGetCurrentContext()) {
            glfwSetWindowSize(vis::window, width, height);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            glfwSwapBuffers(vis::window);
            glfwPollEvents();
        }
    }

    /// Ativa ou desatiava a tela cheia.
    static void toggle_fullscreen() {
        if (glfwGetCurrentContext()) {
            GLFWmonitor* monitor = glfwGetWindowMonitor(vis::window);
            if (monitor) {
                glfwSetWindowMonitor(vis::window, nullptr, 0, 0, vis::width, vis::height, 0);
                glViewport(0, 0, vis::width, vis::height);
            } else {
                monitor = glfwGetPrimaryMonitor();
                const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                glfwSetWindowMonitor(vis::window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
                set_window_size(vis::window, mode->width, mode->height);
            }
        }
    }

    /// Aguarda pelo número especificado de segundos.
    static void wait(double seconds) {
        double end_time = glfwGetTime() + seconds;
        while (glfwGetTime() < end_time) {
            continue;
        }
    }

    /**
     * Desenha a árvore binária na janela, podendo ser uma visualização estática
     * que se encaixa inteiramente no tamanho da janela ou uma visualização
     * dinâmica que exibe apenas parte da árvore e pode ser controlada com as
     * setas direcionais ou as teclas WASD.
     * 
     * @param delay Tempo até a saída automática da função, em segundos.
     * @param fit_to_screen Define se a visualização é estática ou dinâmica.
     * @return Booleano que indica operação bem sucedida.
     */
    bool draw(double delay = 0.0, bool fit_to_screen = true) {
        if (!vis::window) {
            std::cerr << "Impossível desenhar com janela fechada." << std::endl;
            return false;
        }
        if (glfwWindowShouldClose(vis::window)) {
            this->reset();
            return false;
        }
        if (delay == 0.0) {
            if (fit_to_screen) delay = 5.0;
            else delay = 120.0;
        }

        log_error();
        glClearColor(0.85f, 0.85f, 0.85f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (fit_to_screen) {
            this->draw_tree_static();
        } else {
            this->draw_tree_dynamic(delay);
        }

        glfwSwapBuffers(vis::window);
        glfwPollEvents();
        if (fit_to_screen) wait(delay);
        log_error();
        return true;
    }
 private:
    struct Glyph {
        uint texture_id;
        uint width;
        uint height;
        int bearing_x;
        int bearing_y;
        int advance;
    };

    T root_node;
    char* buffer;
    Glyph* glyph_map;
    uint shaders[3];
    uint VAO[3];
    uint VBO[3];
    bool stride;

    enum Shape : uint {
        Node = 0,
        Line,
        Text
    };

    // Lê um arquivo de shader e compila o código-fonte para uso no programa
    uint compile_shader(GLenum type, Shape shape) {
        std::ifstream file;
        std::string file_path;
        switch (shape) {
            case Shape::Node:
                file_path = "dependencies/node.xs";
                break;
            case Shape::Line:
                file_path = "dependencies/line.xs";
                break;
            case Shape::Text:
                file_path = "dependencies/text.xs";
                break;
        }
        if (type == GL_VERTEX_SHADER) {
            file_path[18] = 'v';
        } else {  // if (type == GL_FRAGMENT_SHADER) {
            file_path[18] = 'f';
        }
        file.open(file_path);

        // // Obtém o tamanho do arquivo e posiciona o cursor no início
        // file.seekg(0, file.end);
        // uint length = file.tellg();
        // file.seekg(0, file.beg);

        // Armazena o código fonte do shader
        file.read(this->buffer, 1024);
        this->buffer[file.gcount()] = '\0';

        uint id = glCreateShader(type);
        glShaderSource(id, 1, &this->buffer, nullptr);
        glCompileShader(id);
        log_error(id, GL_COMPILE_STATUS);
        file.close();
        return id;
    }

    void create_shader_program(Shape shape) {
        uint program = glCreateProgram();
        uint vs = compile_shader(GL_VERTEX_SHADER, shape);
        uint fs = compile_shader(GL_FRAGMENT_SHADER, shape);

        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);
        glValidateProgram(program);

        log_error(program, GL_LINK_STATUS);
        log_error(program, GL_VALIDATE_STATUS);

        glDeleteShader(vs);
        glDeleteShader(fs);
        this->shaders[shape] = program;
    }

    // Cria um círculo de raio 1.0 e centro (0, 0) com o número de lados
    // especificado na constante acima, tudo isso em tempo de compilação.
    constexpr float* create_circle() {
        int index = 0;
        int n = number_of_circle_sides;
        float* circle = new float[n * 2];
        for (float angle = 0; index < n * 2; angle += (glm::two_pi<float>() / n)) {
            circle[index++] = glm::cos(angle);
            circle[index++] = glm::sin(angle);
        }
        return circle;
    }

    void  create_line_data() {
        this->create_shader_program(Shape::Line);
        glGenVertexArrays(1, &this->VAO[Shape::Line]);
        glGenBuffers(1, &this->VBO[Shape::Line]);
        glBindVertexArray(this->VAO[Shape::Line]);
        glBindBuffer(GL_ARRAY_BUFFER, this->VBO[Shape::Line]);
        // Aloca espaço para 1024 floats, que equivale a 512 vértices e 256 linhas
        // que conectam os nós da árvore
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 1024, nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
        this->use_program(Shape::Line);
        int line_transform_location = glGetUniformLocation(this->shaders[Shape::Line], "transform");
        float default_transform[4][4] = {
            {1.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, 1.0f}
        };
        glUniformMatrix4fv(line_transform_location, 1, GL_FALSE, &default_transform[0][0]);
        glBindVertexArray(0);
    }

    void create_node_data() {
        this->create_shader_program(Shape::Node);
        float* circle = this->create_circle();
        glGenVertexArrays(1, &this->VAO[Shape::Node]);
        glGenBuffers(1, &this->VBO[Shape::Node]);
        glBindVertexArray(this->VAO[Shape::Node]);
        glBindBuffer(GL_ARRAY_BUFFER, this->VBO[Shape::Node]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 720, circle, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
        glBindVertexArray(0);
        // delete[] circle;
    }

    void create_text_data() {
        this->create_shader_program(Shape::Text);
        glGenVertexArrays(1, &this->VAO[Shape::Text]);
        glGenBuffers(1, &this->VBO[Shape::Text]);
        glBindVertexArray(this->VAO[Shape::Text]);
        glBindBuffer(GL_ARRAY_BUFFER, this->VBO[Shape::Text]);
        // Nulo por enquanto, mas os vértices serão atualizados a cada chamada de draw()
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        // Posições dos vértices
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
        glEnableVertexAttribArray(1);
        // Posições das texturas
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        this->use_program(Shape::Text);
        int text_transform_location = glGetUniformLocation(this->shaders[Shape::Text], "transform");
        float default_transform[4][4] = {
            {1.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, 1.0f}
        };
        glUniformMatrix4fv(text_transform_location, 1, GL_FALSE, &default_transform[0][0]);
        glBindVertexArray(0);
    }

    // Inicia a janela na qual serão exibidos os gráficos
    void start(const std::string& path_to_font, bool fullscreen = false) {
        if (!glfwInit())
            throw std::runtime_error("Falha ao inicializar GLFW.");
        try {
            // Ativa antisserrilhamento
            glfwWindowHint(GLFW_SAMPLES, 8);
            // Cria uma janela e seu contexto OpenGL
            vis::window = glfwCreateWindow(vis::width, vis::height, "Binary Tree Visualization", nullptr, nullptr);
            if (!vis::window)
                throw std::runtime_error("Falha ao criar janela.");

            glfwMakeContextCurrent(vis::window);
            glViewport(0, 0, vis::width, vis::height);
            glfwSetFramebufferSizeCallback(vis::window, set_window_size);
            if (glewInit() != GLEW_OK)
                throw std::runtime_error("Falha ao inicializar GLEW.");

            glEnable(GL_BLEND);
            glEnable(GL_CULL_FACE);
            glEnable(GL_PROGRAM_POINT_SIZE);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            if (fullscreen) this->toggle_fullscreen();
            this->init_freetype(path_to_font,
                static_cast<uint>(0.05 * std::max(vis::width, vis::height)));
            this->create_line_data();
            this->create_node_data();
            this->create_text_data();
            // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        } catch (std::exception& e) {
            this->reset();
            throw;
        }
    }

    // Estrutura usada para armazenar o ponteiro a um nó e sua posição na tela
    struct NodePos {
        T node;
        short position;
        short parent;
        short left_index = 0;
        short right_index = 0;
    };

    // Encontra todos os nós de cada nível (altura) diferente da árvore e o
    // o primeiro índice de cada nível da árvore. Armazena ponteiros para esses
    // nós nos vetores dados como argumentos e retorna a maior distância entre
    // qualquer nó e a origem, para evitar que seu tamanho exceda o máximo que
    // a tela pode comportar. Também calcula as posições dos nós na exibição.
    int breadth_first_search(std::vector<NodePos>& nodes, std::vector<int>& beginnings) {
        nodes.push_back(NodePos{this->root_node, 0, 0});
        beginnings.push_back(0);
        int index = 0, previous_queue_size = 0, max_distance_from_origin = 0;
        bool found_min = false;
        while (index < nodes.size()) {
            if (index == previous_queue_size) {
                previous_queue_size = nodes.size();
                beginnings.push_back(previous_queue_size);
            }
            if (nodes[index].node->left()) {
                nodes[index].left_index = nodes.size();
                nodes.push_back(NodePos{nodes[index].node->left(), static_cast<short>(
                    nodes[index].position - 1), static_cast<short>(index)});

                if (nodes[nodes.size() - 2].position == nodes.back().position) {
                    bool should_move = false;
                    ++nodes.back().position;
                    for (int i = nodes.size() - 2; i >= beginnings.back(); --i)
                        --nodes[i].position;
                    
                    int current_level = beginnings.size() - 1;
                    int child = nodes.size() - 1;
                    int parent = nodes[child].parent;
                    while (nodes[parent].left_index == child || should_move) {
                        if (parent == 0) break;
                        should_move = false;
                        for (int i = parent; i < beginnings[current_level]; ++i) {
                            ++nodes[i].position;
                            should_move = true;
                        }
                        child = parent;
                        parent = nodes[child].parent;
                        --current_level;
                    }

                    current_level = beginnings.size() - 2;
                    child = nodes.size() - 2;
                    parent = nodes[child].parent;
                    while (nodes[parent].right_index == child || should_move) {
                        if (parent == 0) break;
                        should_move = false;
                        for (int i = parent; i >= beginnings[current_level]; --i) {
                            --nodes[i].position;
                            should_move = true;
                        }
                        child = parent;
                        parent = nodes[child].parent;
                        --current_level;
                    }
                }
            }
            if (nodes[index].node->right()) {
                nodes[index].right_index = nodes.size();
                nodes.push_back(NodePos{nodes[index].node->right(), static_cast<short>(
                    nodes[index].position + 1), static_cast<short>(index)});
            }
            ++index;
        }
        if (beginnings.back() == index)
            beginnings.pop_back();

        // short min_position = 0, max_position = 0;
        // for (uint i = 0; i < beginnings.size() - 1; ++i) {
        //     min_position = nodes[beginnings[i]].position;
        //     max_position = nodes[beginnings[i + 1] - 1].position;
        //     if (max_position - min_position > greatest_range)
        //         greatest_range = max_position - min_position;
        // }
        // min_position = nodes[beginnings.back()].position;
        // max_position = nodes[index - 1].position;
        // if (max_position - min_position > greatest_range)
        //     greatest_range = max_position - min_position;
        // return greatest_range;

        int distance;
        for (uint i = 1; i < beginnings.size(); ++i) {
            distance = std::max(
                std::abs(nodes[beginnings[i]].position),
                std::abs(nodes[beginnings[i] - 1].position)
            );
            if (distance > max_distance_from_origin)
                max_distance_from_origin = distance;
        }
        distance = std::abs(nodes.back().position);
        if (distance > max_distance_from_origin)
            max_distance_from_origin = distance;

        glClearColor(0.85f, 0.85f, 0.85f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        return max_distance_from_origin;
    }

    float* organize_data(const float radius_x, const float radius_y, int& j,
        const std::vector<NodePos>& nodes, const std::vector<int>& beginnings) {
        const float line_height = 4.0f * radius_y;
        // Disposição dos dados: 2 floats para a posição inicial da linha, que é
        // o nó pai, e 2 floats para a posição final, que é seu filho.
        // No pior caso, todo nó que não é folha possui dois filhos, o que torna
        // necessário alocar 8 floats para cada nó que não esteja no último nível.
        float* vertices = new float[beginnings.back() * 8];
        uint current_level = 0;
        float height;  // altura do nível atual
        float lower_height = 1.0f - radius_y;  // altura do nível abaixo
        for (uint i = 0; i < beginnings.back(); ++i) {
            if (i == beginnings[current_level]) {
                ++current_level;
                height = lower_height;
                lower_height -= line_height;
            }
            if (nodes[i].node->left()) {
                vertices[++j] = radius_x * (2 * nodes[i].position);
                vertices[++j] = height;
                vertices[++j] = radius_x * (2 * nodes[nodes[i].left_index].position);
                vertices[++j] = lower_height;
            }
            if (nodes[i].node->right()) {
                vertices[++j] = radius_x * (2 * nodes[i].position);
                vertices[++j] = height;
                vertices[++j] = radius_x * (2 * nodes[nodes[i].right_index].position);
                vertices[++j] = lower_height;
            }
        }
        this->use_program(Shape::Line);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * ++j, vertices);
        return vertices;
    }

    void draw_tree_static() {
        glActiveTexture(GL_TEXTURE0);
        std::vector<NodePos> nodes;
        std::vector<int> beginnings;
        int max_distance_from_origin = this->breadth_first_search(nodes, beginnings);

        const float max_radius = 0.1f;
        const float ratio = static_cast<float>(vis::width) / vis::height;
        const float inv_ratio = static_cast<float>(vis::height) / vis::width;
        float radius_x = 0.5f / (static_cast<float>(max_distance_from_origin) + 0.5f);
        float radius_y = 1.0f / (beginnings.size() * 2 - 1);
        
        if (radius_x > max_radius) radius_x = max_radius;
        if (radius_y > max_radius) radius_y = max_radius;

        if (radius_y > radius_x * ratio) {
            radius_y = radius_x * ratio;
        } else {
            radius_x = radius_y * inv_ratio;
        }

        int j = -1;
        float* vertices = organize_data(radius_x, radius_y, j, nodes, beginnings);

        this->use_program(Shape::Line);
        glDrawArrays(GL_LINES, 0, j);

        this->use_program(Shape::Node);
        int transform_location = glGetUniformLocation(this->shaders[Shape::Node], "transform");
        int color_location = glGetUniformLocation(this->shaders[Shape::Node], "rgba");

        float transform[4][4] = {
            {radius_x, 0, 0, 0}, {0, radius_y, 0, 0},
            {0, 0, 1, 0}, {vertices[0], vertices[1], 0, 1}
        };
        glUniformMatrix4fv(transform_location, 1, GL_FALSE, &transform[0][0]);
        glUniform4f(color_location, 1.0f, 1.0f, 1.0f, 1.0f);
        glDrawArrays(GL_TRIANGLE_FAN, 0, number_of_circle_sides);
        glUniform4f(color_location, 0.0f, 0.0f, 0.0f, 1.0f);
        glDrawArrays(GL_LINE_STRIP, 0, number_of_circle_sides);

        for (uint i = 2; i < j; i += 4) {
            transform[3][0] = vertices[i];
            transform[3][1] = vertices[i + 1];
            glUniformMatrix4fv(transform_location, 1, GL_FALSE, &transform[0][0]);
            glUniform4f(color_location, 1.0f, 1.0f, 1.0f, 1.0f);
            glDrawArrays(GL_TRIANGLE_FAN, 0, number_of_circle_sides);
            glUniform4f(color_location, 0.0f, 0.0f, 0.0f, 1.0f);
            glDrawArrays(GL_LINE_LOOP, 0, number_of_circle_sides);
        }

        this->use_program(Shape::Text);
        // Nota para o eu do futuro: já esqueci, mas acho que é 1.5 porque 2 ficaria
        // muito grande, mas multiplicado por 10 porque a altura da fonte é 0.1 * tamanho
        // da maior dimensão da tela
        const float scale_x = (15.0f / (std::max(vis::width, vis::height)) * radius_x);
        const float scale_y = (15.0f / (std::max(vis::width, vis::height)) * radius_y);
        this->draw_text_to(nodes[0].node, vertices[0], vertices[1], scale_x, scale_y);
        int k = 0;
        for (uint i = 2; i < j; i += 4) {
            this->draw_text_to(nodes[++k].node, vertices[i], vertices[i + 1], scale_x, scale_y);
        }
        delete[] vertices;
    }

    bool process_input() {
        if (glfwGetKey(vis::window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(vis::window, true);
            return false;
        }
        if (glfwGetKey(vis::window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            this->stride = false;
            return true;
        }
        return false;
    }

    bool process_input(float* screen) {
        static float step = 0.03f;
        bool pressed = false;
        if (glfwGetKey(vis::window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(vis::window, true);
            return pressed;
        }
        if (glfwGetKey(vis::window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            this->stride = !this->stride;
        }
        
        float increment = this->stride ? step * 10 : step;
        if (glfwGetKey(vis::window, GLFW_KEY_UP) == GLFW_PRESS || 
            glfwGetKey(vis::window, GLFW_KEY_W) == GLFW_PRESS) {
            screen[2] += increment; screen[3] += increment;
            pressed = true;
        } else if (glfwGetKey(vis::window, GLFW_KEY_DOWN) == GLFW_PRESS ||
            glfwGetKey(vis::window, GLFW_KEY_S) == GLFW_PRESS) {
            screen[2] -= increment; screen[3] -= increment;
            pressed = true;
        }
        if (glfwGetKey(vis::window, GLFW_KEY_LEFT) == GLFW_PRESS ||
            glfwGetKey(vis::window, GLFW_KEY_A) == GLFW_PRESS) {
            screen[0] -= increment; screen[1] -= increment;
            pressed = true;
        } else if (glfwGetKey(vis::window, GLFW_KEY_RIGHT) == GLFW_PRESS ||
            glfwGetKey(vis::window, GLFW_KEY_D) == GLFW_PRESS) {
            screen[0] += increment; screen[1] += increment;
            pressed = true;
        }
        return pressed;
    }

    void draw_tree_dynamic(double time) {
        glActiveTexture(GL_TEXTURE0);
        std::vector<NodePos> nodes;
        std::vector<int> beginnings;
        int max_distance_from_origin = this->breadth_first_search(nodes, beginnings);

        const float inv_ratio = static_cast<float>(vis::height) / vis::width;
        const float radius_y = 0.1f;
        const float radius_x = radius_y * inv_ratio;
        const float line_height = 4.0f * radius_y;
        const float scale_x = (15.0f / (std::max(vis::width, vis::height)) * radius_x);
        const float scale_y = (15.0f / (std::max(vis::width, vis::height)) * radius_y);

        int j = -1;
        float* vertices = organize_data(radius_x, radius_y, j, nodes, beginnings);

        glm::mat4 basic_transform(1.0f);
        glm::mat4 transform(1.0f);
        float screen[4] = {-1.0f, 1.0f, -1.0f, 1.0f};

        this->use_program(Shape::Line);
        int line_transform_location = glGetUniformLocation(this->shaders[Shape::Line], "transform");
        this->use_program(Shape::Node);
        int node_transform_location = glGetUniformLocation(this->shaders[Shape::Node], "transform");
        int node_color_location = glGetUniformLocation(this->shaders[Shape::Node], "rgba");
        this->use_program(Shape::Text);
        int text_transform_location = glGetUniformLocation(this->shaders[Shape::Text], "transform");

        double end_time = glfwGetTime() + time;
        goto render;
        while (glfwGetTime() < end_time && !glfwWindowShouldClose(vis::window)) {
            glfwPollEvents();
            if (process_input(screen)) {
                render:
                basic_transform = glm::ortho(screen[0], screen[1], screen[2], screen[3]);
                
                glClearColor(0.85f, 0.85f, 0.85f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);

                this->use_program(Shape::Line);
                glUniformMatrix4fv(line_transform_location, 1, GL_FALSE,
                    glm::value_ptr(basic_transform));
                glDrawArrays(GL_LINES, 0, j);

                this->use_program(Shape::Node);
                transform = glm::translate(basic_transform,
                    glm::vec3(vertices[0], vertices[1], 0.0f));
                transform = glm::scale(transform, glm::vec3(radius_x, radius_y, 1.0f));
                glUniformMatrix4fv(node_transform_location, 1, GL_FALSE, glm::value_ptr(transform));
                glUniform4f(node_color_location, 1.0f, 1.0f, 1.0f, 1.0f);
                glDrawArrays(GL_TRIANGLE_FAN, 0, number_of_circle_sides);
                glUniform4f(node_color_location, 0.0f, 0.0f, 0.0f, 1.0f);
                glDrawArrays(GL_LINE_LOOP, 0, number_of_circle_sides);

                for (uint i = 2; i < j; i += 4) {
                    transform = glm::translate(basic_transform,
                        glm::vec3(vertices[i], vertices[i + 1], 0.0f));
                    transform = glm::scale(transform, glm::vec3(radius_x, radius_y, 1.0f));
                    glUniformMatrix4fv(node_transform_location, 1, GL_FALSE, glm::value_ptr(transform));
                    glUniform4f(node_color_location, 1.0f, 1.0f, 1.0f, 1.0f);
                    glDrawArrays(GL_TRIANGLE_FAN, 0, number_of_circle_sides);
                    glUniform4f(node_color_location, 0.0f, 0.0f, 0.0f, 1.0f);
                    glDrawArrays(GL_LINE_LOOP, 0, number_of_circle_sides);
                }

                this->use_program(Shape::Text);
                glUniformMatrix4fv(text_transform_location, 1, GL_FALSE,
                    glm::value_ptr(basic_transform));
                this->draw_text_to(nodes[0].node, vertices[0], vertices[1], scale_x, scale_y);
                int k = 0;
                for (uint i = 2; i < j; i += 4) {
                    this->draw_text_to(nodes[++k].node, vertices[i], vertices[i + 1], scale_x, scale_y);
                }

                glfwSwapBuffers(vis::window);
                glfwPollEvents();

                if (this->stride) {
                    double wait_time = glfwGetTime() + 0.25;
                    if (wait_time > end_time) wait_time = end_time;
                    while (glfwGetTime() < wait_time && !glfwWindowShouldClose(vis::window)) {
                        glfwPollEvents();
                        if (process_input()) {
                            break;
                        }
                    }
                }
            }
        }
        delete[] vertices;
    }

    void draw_text_to(const T node, float x, float y, const float scale_x, const float scale_y) {
        // Lê no máximo 4 caracteres, ninguém debuga com mais que isso
        char key[4];
        std::stringstream ss;
        ss << node->key();
        ss.read(key, 4);
        std::array<float, 24> vertices;

        float length = 0;
        float height = 0;
        for (int i = 0; i < ss.gcount(); ++i) {
            length += (this->glyph_map[key[i]].advance >> 6) * scale_x;
            float candidate = this->glyph_map[key[i]].height * scale_y;
            if (candidate > height) height = candidate;
        }
        // Centraliza na coordenada passada para a função
        x -= length * 0.5f;
        y -= height * 0.5f;

        for (int i = 0; i < ss.gcount(); ++i) {
            Glyph* glyph = this->glyph_map + key[i];
            float xpos = x + glyph->bearing_x * scale_x;
            float ypos = y - (glyph->height - glyph->bearing_y) * scale_y;

            float w = glyph->width * scale_x;
            float h = glyph->height * scale_y;

            vertices = {
                xpos,     ypos + h, 0.0f, 0.0f,
                xpos,     ypos,     0.0f, 1.0f,
                xpos + w, ypos,     1.0f, 1.0f,

                xpos,     ypos + h, 0.0f, 0.0f,
                xpos + w, ypos,     1.0f, 1.0f,
                xpos + w, ypos + h, 1.0f, 0.0f
            };
            glBindTexture(GL_TEXTURE_2D, glyph->texture_id);
            glBufferSubData(GL_ARRAY_BUFFER, 0, 24 * sizeof(float), vertices.data());
            glDrawArrays(GL_TRIANGLES, 0, 6);
            // 2^6 = 64, conversão feita pois o valor de advance é em 1/64 de pixel
            // Equivale a `x = x + (advance / 64) * scale_x`
            x += (glyph->advance >> 6) * scale_x;
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // Exibe quaisquer erros recentes do OpenGL
    static void log_error() {
        GLenum error = glGetError();
        while (error != GL_NO_ERROR) {
            switch (error) {
                case GL_INVALID_ENUM:      std::cout << "Invalid enum"; break;
                case GL_INVALID_VALUE:     std::cout << "Invalid value"; break;
                case GL_INVALID_OPERATION: std::cout << "Invalid operation"; break;
                case GL_STACK_OVERFLOW:    std::cout << "Stack overflow"; break;
                case GL_STACK_UNDERFLOW:   std::cout << "Stack underflow"; break;
                case GL_OUT_OF_MEMORY:     std::cout << "Out of memory"; break;
                default:                   std::cout << "Unknown error" << std::endl;
            }
            std::cout << std::endl;
            error = glGetError();
        }
    }

    // Exibe eventuais mensagens de erro na criação de shaders
    void log_error(uint id, GLenum type) {
        int success;
        if (type == GL_LINK_STATUS || type == GL_VALIDATE_STATUS) {
            glGetProgramiv(id, type, &success);
            if (!success) {
                glGetProgramInfoLog(id, 1024, nullptr, this->buffer);
                goto error;  // Fazer função é coisa de otário
            }
        } else if (type == GL_COMPILE_STATUS) {
            glGetShaderiv(id, type, &success);
            if (!success) {
                glGetShaderInfoLog(id, 1024, nullptr, this->buffer);
                goto error;
            }
        }
        return;
        error:
            this->reset();
            std::cout << this->buffer;
            throw std::runtime_error("Explodiu!");
    }

    // Inicializa a biblioteca FreeType para renderizar texto
    void init_freetype(const std::string& path_to_font, uint font_height) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        int error;
        FT_Library library;
        error = FT_Init_FreeType(&library);
        if (error) {
            throw std::runtime_error(
                "Não foi possível inicializar a biblioteca FreeType. Código do erro " + \
                std::to_string(error) + ".");
        }
        FT_Face face;
        error = FT_New_Face(library, path_to_font.c_str(), 0, &face);
        if (error)
            throw std::runtime_error("Não foi possível carregar fonte.");
        // Argumentos são font face, comprimento e altura
        FT_Set_Pixel_Sizes(face, 0, font_height);
        // FT_Set_Char_Size(face, 0, constexpr 16 * 64, vis::width, font_height);

        // Armazena as texturas temporariamente no buffer da classe, antes de
        // construir as estruturas que vão conter a informação de cada caractere
        uint* textures = reinterpret_cast<uint*>(this->buffer);

        glGenTextures(127, textures);
        
        uint height;
        FT_GlyphSlot slot = face->glyph;
        // Carrega os caracteres ASCII, exceto pelo 127, que é DEL e quebra o programa
        for (char c = 0; c < 127; ++c) {
            if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
                std::cout << "Não foi possível carregar o caractere " << c << '.' << std::endl;
                continue;
            }
            glBindTexture(GL_TEXTURE_2D, textures[c]);
            // Parece que a biblioteca não lida bem com hífen, então tive que fazer manualmente
            if (c == '-') {
                // Obtém o tamanho de um caractere cujo tamanho é igual ao de um algarismo
                height = this->glyph_map['&'].height;
                // Descobre o tamanho do buffer de dados do caractere '-'
                int size = slot->bitmap.width * height;
                unsigned char* buffer = new unsigned char[size];
                int i = 0;
                // Preenche de espaço vazio até o começo da cópia dos verdadeiros dados
                int start_copy = ((height - slot->bitmap.rows) * slot->bitmap.width) / 2;
                for (i = 0; i < start_copy; ++i)
                    buffer[i] = '\0';

                // Copia as informações do caractere segundo sua fonte
                int n_copies = slot->bitmap.rows * slot->bitmap.width;
                std::copy(slot->bitmap.buffer, slot->bitmap.buffer + n_copies, buffer + i);
                i += n_copies;

                // Preenche o restante com nada
                for (; i < size; ++i)
                    buffer[i] = '\0';

                // Gera a textura usando os dados "customizados"
                glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, slot->bitmap.width,
                    height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buffer);

                delete[] buffer;
            } else {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, slot->bitmap.width,
                    slot->bitmap.rows, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, slot->bitmap.buffer);
            }

            // Um cara aleatório recomendou GL_CLAMP_TO_BORDER, (s, t, r) == (x, y, z)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            // Também é possível usar GL_NEAREST
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            // float borderColor[] = { 1.0f, 1.0f, 0.0f, 1.0f };
            // glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

            this->glyph_map[c] = Glyph{textures[c], slot->bitmap.width, slot->bitmap.rows,
                slot->bitmap_left, slot->bitmap_top, static_cast<int>(slot->advance.x)};
        }

        // Atualiza os dados do hífen
        this->glyph_map['-'].height = height;
        this->glyph_map['-'].bearing_y = height;
        
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void use_program(Shape shape) {
        glUseProgram(this->shaders[shape]);
        glBindVertexArray(this->VAO[shape]);
        glBindBuffer(GL_ARRAY_BUFFER, this->VBO[shape]);
    }

    void reset() {
        if (glfwGetCurrentContext()) {
            for (int i = 0; i < 3; ++i) {
                this->shaders[i] = 0;
                this->VAO[i] = 0;
                this->VBO[i] = 0;
            }
            vis::window = nullptr;
            glfwTerminate();
        }
    }
};
