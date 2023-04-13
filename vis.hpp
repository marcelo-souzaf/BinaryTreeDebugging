#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#define GLM_FORCE_INLINE
#define GLM_FORCE_RADIANS
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/trigonometric.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <array>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

typedef unsigned int uint;

constexpr int number_of_circle_sides = 180;



/// Namespace que contém informações globais quanto ao funcionamento da janela,
/// permitindo que eles sejam modificados mesmo por funções estáticas.
namespace vis {
    GLFWwindow* window = nullptr;
    uint width;
    uint height;
    bool resized = false;
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

template<typename NodePtr>
class Visualization {
 public:
    /**
     * Contrói um objeto de visualização que carrega os dados necessários
     * para a exibição de uma árvore binária usando OpenGL.
     * Se fullscreen for verdadeiro, a janela será aberta em tela cheia e
     * os valores de width e height serão armazenados caso o usuário queira
     * retornar ao modo janela durante a execução.
     * 
     * @tparam NodePtr Ponteiro para o tipo do nó da árvore.
     * @param node Ponteiro para o nó raiz da árvore.
     * @param fullscreen Define se o aplicativo abre em tela cheia.
     * @param width Válido apenas se fullscreen for falso, define a largura da janela.
     * @param height Válido apenas se fullscreen for falso, define a altura da janela.
     * @param path_to_font Caminho até a fonte a ser usada.
     */
    Visualization(NodePtr node, bool fullscreen = false, uint width = 1280,
    uint height = 720, const std::string& path_to_font = \
    "dependencies/RobotoMono-Medium.ttf") : root_node(node), width(width), height(height) {
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
        if (vis::window) {
            for (int i = 0; i < 3; ++i) {
                if (this->shaders[i])
                    glDeleteProgram(this->shaders[i]);
            }
            for (int i = 0; i < 127; ++i) {
                if (this->glyph_map[i].texture_id)
                    glDeleteTextures(1, &this->glyph_map[i].texture_id);
            }
            if (this->VAO[0])
                glDeleteVertexArrays(3, this->VAO);
            if (this->VBO[0])
                glDeleteBuffers(3, this->VBO);
            if (glfwGetCurrentContext() == vis::window)
                destroy_window();
            vis::window = nullptr;
            glfwTerminate();
        }
        delete[] this->buffer;
        delete[] this->glyph_map;
    }

    Visualization(const Visualization&) = delete;

    Visualization& operator=(const Visualization&) = delete;

    /// Define o nó raiz da árvore.
    void set_root(NodePtr root) {
        this->root_node = root;
    }

    /// Redefine o tamanho da janela.
    void set_window_size(uint width, uint height) {
        this->width = width;
        this->height = height;
        if (glfwGetCurrentContext() && glfwGetWindowMonitor(vis::window) == nullptr)
            set_window_size(vis::window, width, height);
    }

    // Redefine o tamanho da janela. Chamada sempre que a janela é redimensionada.
    static void set_window_size(GLFWwindow* window, int width, int height) {
        vis::width = width;
        vis::height = height;
        vis::resized = true;
        glViewport(0, 0, width, height);
        if (glfwGetCurrentContext()) {
            if (glfwGetWindowMonitor(vis::window) == nullptr)
                glfwSetWindowSize(vis::window, width, height);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
        }
    }

    /// Ativa ou desatiava a tela cheia.
    void toggle_fullscreen() {
        if (glfwGetCurrentContext()) {
            GLFWmonitor* monitor = glfwGetWindowMonitor(vis::window);
            if (monitor) {
                glfwSetWindowMonitor(vis::window, nullptr, 0, 0, this->width, this->height, 0);
                set_window_size(vis::window, this->width, this->height);
            } else {
                monitor = glfwGetPrimaryMonitor();
                const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                glfwSetWindowMonitor(vis::window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
                set_window_size(vis::window, mode->width, mode->height);
            }
            // Evita redesenhar o conteúdo na janela, pois chamar essa função significa que o
            // redimensionamento já está sendo tratado
            vis::resized = false;
        }
    }

    /// Aguarda pelo valor especificado em segundos.
    static void wait(double seconds) {
        double end_time = glfwGetTime() + seconds;
        while (glfwGetTime() < end_time && !glfwWindowShouldClose(vis::window)) {
            continue;
        }
    }

    /**
     * Desenha a árvore binária na janela, podendo ser uma visualização estática
     * que se encaixa inteiramente no tamanho da janela ou uma visualização
     * dinâmica que exibe apenas parte da árvore e pode ser controlada com as
     * setas direcionais ou as teclas WASD.
     * 
     * @param wait_time Tempo até a saída automática da função, em segundos.
     * @param fit_to_screen Define se a visualização é estática ou dinâmica.
     * @return Booleano que indica operação bem sucedida.
     */
    bool draw(double wait_time = 0.0, bool fit_to_screen = true) {
        static bool warned = false;
        if (vis::window == nullptr) {
            if (!warned) {
                std::cerr << "Impossível desenhar com janela fechada." << std::endl;
                warned = true;
            }
            return false;
        }
        if (glfwWindowShouldClose(vis::window)) {
            destroy_window();
            return false;
        }
        if (wait_time == 0.0) {
            if (fit_to_screen)
                wait_time = 5.0;
            else
                wait_time = 120.0;
        }

        // Evita redesenhar a árvore se a janela foi redimensionada fora da execução
        vis::resized = false;
        log_error();

        if (fit_to_screen)
            this->draw_tree_static(wait_time);
        else
            this->draw_tree_dynamic(wait_time);

        log_error();
        return true;
    }

    // Inicializa a biblioteca FreeType para renderizar texto
    void load_font(const std::string& path_to_font, uint font_height) {
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
        // FT_Set_Char_Size(face, 0, 16 * 64, vis::width, font_height);

        // Armazena as texturas temporariamente no buffer da classe, antes de
        // construir as estruturas que vão conter a informação de cada caractere
        GLuint* textures = reinterpret_cast<GLuint*>(this->buffer);
        glGenTextures(127, textures);
        
        FT_GlyphSlot slot = face->glyph;
        // Carrega os caracteres ASCII, exceto pelo 127, que é DEL e quebra o programa
        for (char c = 0; c < 127; ++c) {
            if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
                std::cerr << "Não foi possível carregar o caractere " << c << '.' << std::endl;
                continue;
            }
            glBindTexture(GL_TEXTURE_2D, textures[c]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, slot->bitmap.width,
                slot->bitmap.rows, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, slot->bitmap.buffer);
                
            this->glyph_map[c] = Glyph{textures[c], slot->bitmap.width, slot->bitmap.rows,
                slot->bitmap_left, slot->bitmap_top, static_cast<int>(slot->advance.x)};

            // Opções são GL_CLAMP_TO_BORDER e GL_CLAMP_TO_EDGE, (s, t, r) == (x, y, z)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            // Também é possível usar GL_NEAREST
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            // float borderColor[] = { 1.0f, 1.0f, 0.0f, 1.0f };
            // glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
        }
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

 private:
    struct Glyph {
        uint texture_id = 0;
        uint width;
        uint height;
        int bearing_x;
        int bearing_y;
        int advance;
    };

    NodePtr root_node;
    char* buffer;
    Glyph* glyph_map;
    uint shaders[3];
    uint VAO[3];
    uint VBO[3];
    uint width;
    uint height;
    uint FPS;
    bool stride;

    enum Shape : uint {
        Line,
        Node,
        Text,
        None
    };

    static void destroy_window() {
        glfwDestroyWindow(vis::window);
        vis::window = nullptr;
    }

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
        GLint length = file.gcount();
        this->buffer[length] = '\0';

        uint id = glCreateShader(type);
        glShaderSource(id, 1, &this->buffer, &length);
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

        glUseProgram(program);
        glGenVertexArrays(1, &this->VAO[shape]);
        glGenBuffers(1, &this->VBO[shape]);
        glBindVertexArray(this->VAO[shape]);
        glBindBuffer(GL_ARRAY_BUFFER, this->VBO[shape]);
    }

    // Cria um círculo de raio 1.0 e centro (0, 0) com o número de lados
    // especificado na constante acima, tudo isso em tempo de compilação.
    float* create_circle() {
        int index = 0;
        constexpr int n = number_of_circle_sides;
        float* circle = new float[n * 2];
        for (float angle = 0; index < n * 2; angle += (glm::two_pi<float>() / n)) {
            circle[index++] = glm::cos(angle);
            circle[index++] = glm::sin(angle);
        }
        return circle;
    }

    void create_line_data() {
        this->create_shader_program(Shape::Line);
        // Aloca espaço para 4096 floats, que equivale a 2048 vértices e 1024 linhas
        // que conectam os nós da árvore
        glBufferData(GL_ARRAY_BUFFER, 4096 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
        int line_transform_location = glGetUniformLocation(this->shaders[Shape::Line], "transform");
        float default_transform[4][4] = {
            {1.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, 1.0f}
        };
        glUniformMatrix4fv(line_transform_location, 1, GL_FALSE, &default_transform[0][0]);
        this->use_program(Shape::None);
    }

    void create_node_data() {
        this->create_shader_program(Shape::Node);
        float* circle = this->create_circle();
        glBufferData(GL_ARRAY_BUFFER, number_of_circle_sides * 2 * sizeof(float), circle, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
        delete[] circle;
        this->use_program(Shape::None);
    }

    void create_text_data() {
        this->create_shader_program(Shape::Text);
        // Nulo por enquanto, mas os vértices serão atualizados a cada chamada de draw()
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        // Posições dos vértices
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
        glEnableVertexAttribArray(1);
        // Posições das texturas
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        int text_transform_location = glGetUniformLocation(this->shaders[Shape::Text], "transform");
        float default_transform[4][4] = {
            {1.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, 1.0f}
        };
        glUniformMatrix4fv(text_transform_location, 1, GL_FALSE, &default_transform[0][0]);
        this->use_program(Shape::None);
    }

    /// Inicia a janela na qual serão exibidos os gráficos.
    void start(const std::string& path_to_font, bool fullscreen = false) {
        if (!glfwInit())
            throw std::runtime_error("Falha ao inicializar GLFW.");
        try {
            // Ativa antisserrilhamento mais pesado (multisampling)
            glfwWindowHint(GLFW_SAMPLES, 4);

            // Cria uma janela e seu contexto OpenGL
            if (fullscreen) {
                GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                vis::width = glfwGetVideoMode(monitor)->width;
                vis::height = glfwGetVideoMode(monitor)->height;
                vis::window = glfwCreateWindow(vis::width, vis::height,
                    "Binary Tree Visualization", monitor, nullptr);
            } else {
                vis::window = glfwCreateWindow(vis::width, vis::height,
                    "Binary Tree Visualization", nullptr, nullptr);
            }
            if (!vis::window)
                throw std::runtime_error("Falha ao criar janela.");

            glfwMakeContextCurrent(vis::window);
            if (glewInit() != GLEW_OK)
                throw std::runtime_error("Falha ao inicializar GLEW.");
            glViewport(0, 0, vis::width, vis::height);
            glfwSetFramebufferSizeCallback(vis::window, set_window_size);

            // Possible values for the hint: GL_FASTEST, GL_NICEST, GL_DONT_CARE
            glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
            glEnable(GL_BLEND);
            glEnable(GL_CULL_FACE);
            // Não tenho certeza se é uma boa ideia
            // glEnable(GL_MULTISAMPLE);
            glEnable(GL_LINE_SMOOTH);
            glEnable(GL_PROGRAM_POINT_SIZE);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            this->load_font(path_to_font, 0.05 * std::max(vis::width, vis::height));
            this->create_line_data();
            this->create_node_data();
            this->create_text_data();
            // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        } catch (std::exception& e) {
            // Garante que a janela será destruída adequadamente
            if (vis::window)
                destroy_window();
            // Propaga a exceção novamente, permitindo que o usuário a trate
            throw;
        }
    }

    // Estrutura usada para armazenar o ponteiro a um nó e sua posição na tela
    struct NodePos {
        NodePtr node;
        // Posição do nó relativa ao centro do tela
        short position;
        // Índice do nó pai no vetor de nós
        short parent;
        // Índice do nó filho esquerdo no vetor de nós
        short left_index = 0;
        // Índice do nó filho direito no vetor de nós
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
        while (index < nodes.size()) {
            // Encontra índice inicial de cada nível (altura) da árvore
            if (index == previous_queue_size) {
                previous_queue_size = nodes.size();
                beginnings.push_back(previous_queue_size);
            }
            if (nodes[index].node->left()) {
                nodes[index].left_index = nodes.size();
                nodes.push_back(NodePos{nodes[index].node->left(), static_cast<short>(
                    nodes[index].position - 1), static_cast<short>(index)});

                // Se o último nó adicionado colide com o penúltimo, move alguns nós
                if (nodes[nodes.size() - 2].position == nodes.back().position) {
                    bool should_move = false;
                    // Avança o último nó para a direita
                    ++nodes.back().position;
                    // Recua todos os outros nós do mesmo nível para a esquerda
                    for (int i = nodes.size() - 2; i >= beginnings.back(); --i)
                        --nodes[i].position;
                    
                    int current_level = beginnings.size() - 1;
                    int child = nodes.size() - 1;
                    int parent = nodes[child].parent;
                    // Se o pai tem o nó atual como filho esquerdo, atualiza sua posição e dos antecessores
                    do {
                        // Não move a raiz, assim ela sempre fica no centro da tela
                        if (parent == 0)
                            break;
                        should_move = false;
                        // Avança o pai do nó atual e todos à sua direita
                        for (int i = parent; i < beginnings[current_level]; ++i) {
                            ++nodes[i].position;
                            // Se nós foram movidos neste nível, garante que o nível acima sofrerá
                            // mudanças mesmo que o pai do nó atual tenha ele como filho direito
                            should_move = true;
                        }
                        child = parent;
                        parent = nodes[child].parent;
                        --current_level;
                    } while (shaders);

                    current_level = beginnings.size() - 2;
                    child = nodes.size() - 2;
                    parent = nodes[child].parent;
                    // Se o pai tem o nó deslocado para a esquerda como filho direito, atualiza posições
                    do {
                        if (parent == 0)
                            break;
                        should_move = false;
                        // Recua todos os nós à esquerda do pai do nó afetado
                        for (int i = parent; i >= beginnings[current_level]; --i) {
                            --nodes[i].position;
                            should_move = true;
                        }
                        child = parent;
                        parent = nodes[child].parent;
                        --current_level;
                    } while (should_move);
                }
            }
            // Inserções à direita nunca dão problema, pois são feitas da esqueda para a direita
            if (nodes[index].node->right()) {
                nodes[index].right_index = nodes.size();
                nodes.push_back(NodePos{nodes[index].node->right(), static_cast<short>(
                    nodes[index].position + 1), static_cast<short>(index)});
            }
            ++index;
        }
        // Se o último nível está completo, um índice inexistente é adicionado,
        // então ele é removido aqui
        if (beginnings.back() == index)
            beginnings.pop_back();

        int distance;
        // Para cada nível, verifica a distância máxima entre um nó nas extremidades e a origem
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

        return max_distance_from_origin;
    }

    float* organize_data(const float radius_x, const float radius_y,
        const std::vector<NodePos>& nodes, const std::vector<int>& beginnings) {
        const float line_height = 4.0f * radius_y;
        // Disposição dos dados: 2 floats para a posição inicial da linha, que é
        // o nó pai, e 2 floats para a posição final, que é seu filho.
        float* vertices = new float[nodes.size() * 4];
        int current_level = 1;
        // Altura do nível acima
        float upper_height;
        // Altura do nível atual
        float height = 0.99f - radius_y;
        
        int j = 0;
        // Os primeiros 4 floats ligam a raiz a si mesma
        // X do pai
        vertices[j++] = 0.0f;
        // Y do pai
        vertices[j++] = height;
        // X do filho
        vertices[j++] = 0.0f;
        // Y do filho
        vertices[j++] = height;

        // i: índice do nó atual, que está sendo conectado ao seu pai
        // j: índice do vetor de dados que está sendo preenchido
        for (int i = 1; i < nodes.size(); ++i) {
            if (current_level < beginnings.size() && i == beginnings[current_level]) {
                upper_height = height;
                height -= line_height;
                ++current_level;
            }
            vertices[j++] = radius_x * (2 * nodes[nodes[i].parent].position);
            vertices[j++] = upper_height;
            vertices[j++] = radius_x * (2 * nodes[i].position);
            vertices[j++] = height;
        }
        return vertices;
    }

    void draw_tree_static(double wait_time) {
        // glActiveTexture(GL_TEXTURE0);
        std::vector<NodePos> nodes;
        std::vector<int> beginnings;
        int max_distance_from_origin = this->breadth_first_search(nodes, beginnings);

        const float max_radius = 0.1f;
        const float ratio = static_cast<float>(vis::width) / vis::height;
        const float inv_ratio = static_cast<float>(vis::height) / vis::width;
        // Tamanho da tela é 2, então divide por 2 para considerar metade da tela na horizontal
        // e divide novamente para calcular com raio. Então, divide pela maior distância até a
        // origem para obter o tamanho ideal, somado a uma constante para ter espaço nas bordas.
        float radius_x = 0.5f / (0.75f + max_distance_from_origin);
        // Divide o tamanho da tela pela quantidade de níveis multiplicada por 2, pois cada
        // nível tem um nó e uma linha de mesmo tamanho. Então, subtrai um para desconsiderar
        // a existência de uma linha acima da raiz.
        float radius_y = 0.98f / (2 * beginnings.size() - 1);
        
        if (radius_x > max_radius)
            radius_x = max_radius;
        if (radius_y > max_radius)
            radius_y = max_radius;

        if (radius_y > radius_x * ratio)
            radius_y = radius_x * ratio;
        else
            radius_x = radius_y * inv_ratio;

        // Organiza os dados para serem enviados ao shader
        float* vertices = organize_data(radius_x, radius_y, nodes, beginnings);
        int length = nodes.size() * 4;

        this->use_program(Shape::Line);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * length, vertices);

        // Desenha os nós por cima das linhas, ocultando a parte que ficaria interna
        this->use_program(Shape::Node);
        int transform_location = glGetUniformLocation(this->shaders[Shape::Node], "transform");
        int color_location = glGetUniformLocation(this->shaders[Shape::Node], "rgba");

        // Variáveis que devem ser inicializadas fora da parte do código que pode ser repetida:
        // Inicializa uma matriz básica de mudança de escala
        float transform[4][4] = {
            {radius_x, 0, 0, 0}, {0, radius_y, 0, 0},
            {0, 0, 1, 0}, {0, 0, 0, 1}
        };
        int j;
        // Altura da fonte: 0.05 * maior dimensão da tela em pixels
        // Assim, a escala é 1, que representa metade das coordenadas de -1 a 1
        // do OpenGL, multiplicada pelo raio do círculo
        const float scale_x = (1.0f / (std::max(vis::width, vis::height)) * radius_x);
        const float scale_y = (1.0f / (std::max(vis::width, vis::height)) * radius_y);
        double start_time = glfwGetTime();
        double end_time = start_time + wait_time;
        UserAction action;
        
        // Parte que será repetida caso o usuário redimensione a janela
        render:
        glClearColor(0.85f, 0.85f, 0.85f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Desenha as linhas conectando os nós
        this->use_program(Shape::Line);
        glDrawArrays(GL_LINES, 0, length);

        this->use_program(Shape::Node);
        // Desenha os nós
        for (int i = 2; i < length; i += 4) {
            // Atualiza a parte de translação da matriz de transformação
            transform[3][0] = vertices[i];
            transform[3][1] = vertices[i + 1];
            glUniformMatrix4fv(transform_location, 1, GL_FALSE, &transform[0][0]);
            // Desenha o fundo branco do nó
            glUniform4f(color_location, 1.0f, 1.0f, 1.0f, 1.0f);
            glDrawArrays(GL_TRIANGLE_FAN, 0, number_of_circle_sides);
            // Desenha a borda preta do nó
            glUniform4f(color_location, 0.0f, 0.0f, 0.0f, 1.0f);
            glDrawArrays(GL_LINE_LOOP, 0, number_of_circle_sides);
        }

        this->use_program(Shape::Text);
        j = 0;
        for (int i = 2; i < length; i += 4) {
            this->draw_text_from(nodes[j++].node, vertices[i], vertices[i + 1], scale_x, scale_y);
        }

        glfwSwapBuffers(vis::window);
        glfwPollEvents();

        action = UserAction::Idle;
        while (glfwGetTime() < end_time && !glfwWindowShouldClose(vis::window)) {
            if ((action = process_input()) == UserAction::Skip && glfwGetTime() - start_time > 0.5) {
                return;
            } else if (action == UserAction::Redraw) {
                wait(0.1);
                glfwSwapBuffers(vis::window);
                goto render;
            }
            glfwPollEvents();
        }

        delete[] vertices;
        this->use_program(Shape::None);
    }

    enum UserAction {
        Idle,
        Skip,
        Move,
        Wait,
        Redraw,
    };

    UserAction process_input() {
        if (vis::resized) {
            vis::resized = false;
            return UserAction::Redraw;
        }
        if (glfwGetKey(vis::window, GLFW_KEY_ENTER) == GLFW_PRESS ||
        glfwGetKey(vis::window, GLFW_KEY_KP_ENTER) == GLFW_PRESS) {
            return UserAction::Skip;
        }
        if (glfwGetKey(vis::window, GLFW_KEY_ESCAPE) == GLFW_PRESS ||
        glfwWindowShouldClose(vis::window)) {
            destroy_window();
            return UserAction::Skip;
        }
        if (glfwGetKey(vis::window, GLFW_KEY_F11) == GLFW_PRESS ||
        glfwGetKey(vis::window, GLFW_KEY_F) == GLFW_PRESS) {
            toggle_fullscreen();
            return UserAction::Redraw;
        }
        return UserAction::Idle;
    }

    UserAction process_input(float* screen) {
        // Variável persiste entre chamadas da função
        static float step = 0.01f;
        if (vis::resized) {
            vis::resized = false;
            return UserAction::Redraw;
        }
        bool pressed = false;
        float increment = this->stride ? step * 15 : step;
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
        if (pressed) {
            return UserAction::Move;
        }

        if (glfwGetKey(vis::window, GLFW_KEY_ENTER) == GLFW_PRESS ||
        glfwGetKey(vis::window, GLFW_KEY_KP_ENTER) == GLFW_PRESS) {
            return UserAction::Skip;
        }
        if (glfwGetKey(vis::window, GLFW_KEY_ESCAPE) == GLFW_PRESS ||
        glfwWindowShouldClose(vis::window)) {
            destroy_window();
            return UserAction::Skip;
        }
        if (glfwGetKey(vis::window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            this->stride = !this->stride;
            return UserAction::Wait;
        }
        if (glfwGetKey(vis::window, GLFW_KEY_F11) == GLFW_PRESS ||
        glfwGetKey(vis::window, GLFW_KEY_F) == GLFW_PRESS) {
            toggle_fullscreen();
            return UserAction::Redraw;
        }
        if (glfwGetKey(vis::window, GLFW_KEY_KP_ADD) == GLFW_PRESS) {
            for (int i = 0; i < 4; ++i)
                screen[i] /= 1.1f;
            step /= 1.1f;
            return UserAction::Redraw;
        }
        if (glfwGetKey(vis::window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS) {
            for (int i = 0; i < 4; ++i)
                screen[i] *= 1.1f;
            step *= 1.1f;
            return UserAction::Redraw;
        }
        if (glfwGetKey(vis::window, GLFW_KEY_R) == GLFW_PRESS || 
        glfwGetKey(vis::window, GLFW_KEY_HOME) == GLFW_PRESS ||
        glfwGetKey(vis::window, GLFW_KEY_BACKSPACE) == GLFW_PRESS) {
            for (int i = 0; i < 4; ++i)
                screen[i] = i % 2 == 0 ? -1.0f : 1.0f;
            step = 0.01f;
            return UserAction::Redraw;
        }
        return UserAction::Idle;
    }

    void draw_tree_dynamic(double wait_time) {
        // glActiveTexture(GL_TEXTURE0);
        std::vector<NodePos> nodes;
        std::vector<int> beginnings;
        int max_distance_from_origin = this->breadth_first_search(nodes, beginnings);

        const float inv_ratio = static_cast<float>(vis::height) / vis::width;
        const float radius_y = 0.1f;
        const float radius_x = radius_y * inv_ratio;
        const float line_height = radius_y * 4;
        const float scale_x = (1.0f / (std::max(vis::width, vis::height)) * radius_x);
        const float scale_y = (1.0f / (std::max(vis::width, vis::height)) * radius_y);

        float* vertices = organize_data(radius_x, radius_y, nodes, beginnings);
        int length = nodes.size() * 4;

        glm::mat4 basic_transform(1.0f);
        glm::mat4 transform(1.0f);
        float screen[4] = {-1.0f, 1.0f, -1.0f, 1.0f};

        this->use_program(Shape::Line);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * length, vertices);
        int line_transform_location = glGetUniformLocation(this->shaders[Shape::Line], "transform");
        this->use_program(Shape::Node);
        int node_transform_location = glGetUniformLocation(this->shaders[Shape::Node], "transform");
        int node_color_location = glGetUniformLocation(this->shaders[Shape::Node], "rgba");
        this->use_program(Shape::Text);
        int text_transform_location = glGetUniformLocation(this->shaders[Shape::Text], "transform");

        double start_time = glfwGetTime();
        double end_time = start_time + wait_time;
        double fps_start = start_time - 0.5;
        double now;
        uint frames = 0;
        std::string fps;
        UserAction action = UserAction::Idle;
        goto render;
        while (glfwGetTime() < end_time && !glfwWindowShouldClose(vis::window)) {
            glfwPollEvents();
            if ((action = process_input(screen)) == UserAction::Move) {
                render:
                basic_transform = glm::ortho(screen[0], screen[1], screen[2], screen[3]);
                
                glClearColor(0.85f, 0.85f, 0.85f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);

                this->use_program(Shape::Line);
                glUniformMatrix4fv(line_transform_location, 1, GL_FALSE,
                    glm::value_ptr(basic_transform));
                glDrawArrays(GL_LINES, 0, length);

                this->use_program(Shape::Node);
                for (int i = 2; i < length; i += 4) {
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
                int j = 0;
                for (int i = 2; i < length; i += 4) {
                    this->draw_text_from(nodes[j++].node, vertices[i], vertices[i + 1], scale_x, scale_y);
                }

                ++frames;
                // Computa o FPS a cada 0.5 segundos
                if ((now = glfwGetTime()) - fps_start >= 0.5) {
                    this->FPS = fps.empty() ? 0 : frames * 2;
                    frames = 0;
                    fps_start = now;
                    fps = std::to_string(this->FPS);
                }
                glm::mat4 identity(1.0f);
                glUniformMatrix4fv(text_transform_location, 1, GL_FALSE,
                    glm::value_ptr(identity));

                // Desenha o FPS na tela
                this->draw_text_from(fps, -0.99f, 0.0f, scale_x / 2, scale_y / 2, false, true);

                this->use_program(Shape::None);
                glfwSwapBuffers(vis::window);

                if (this->stride) {
                    wait(0.5);
                    glfwPollEvents();
                }
            // Se a ação é ativar o stride ou redesenhar a tela, como em caso de
            // redimensionamento, espera por 0.1 segundo
            } else if (action >= UserAction::Wait) {
                wait(0.1);
                if (action == UserAction::Redraw) {
                    glfwSwapBuffers(vis::window);
                    goto render;
                }
            // Se a ação é sair, garante que ela apenas ocorra pelo menos 0.5 segundos após o início da execução
            } else if (action == UserAction::Skip && glfwGetTime() - start_time > 0.5) {
                break;
            }
        }
        delete[] vertices;
    }

    void draw_text(const std::string& key, float x, float y, float scale_x, float scale_y) {
        std::array<float, 24> vertices;
        for (char c : key) {
            Glyph* glyph = this->glyph_map + c;
            float xpos = x + glyph->bearing_x * scale_x;
            float ypos = y - (static_cast<int>(glyph->height) - glyph->bearing_y) * scale_y;

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
    }

    void draw_text_from(const NodePtr node, float x, float y, float scale_x, float scale_y) {
        std::stringstream ss;
        ss << node->key();
        std::string key(ss.str());
        this->draw_text_from(key, x, y, scale_x, scale_y);
    }

    void draw_text_from(const std::string& text, float x, float y,
                        float scale_x, float scale_y, bool centered = true, bool y_top = false) {
        // Escala é multiplicada por uma constante de adequação, que é um pixel na unidade
        // usada pela biblioteca dividida pelo máximo entre a extensão da chave e 3, para
        // que os a chave não fique muito grande
        float factor = (64 / std::max(text.size(), 3UL));
        scale_x *= factor;
        scale_y *= factor;

        // Centraliza na coordenada passada para a função
        if (centered || y_top) {
            float length = 0;
            float height = 0;
            // Calcula a extensão total do texto para centralizar em x e obtém a maior altura
            for (char c : text) {
                length += (this->glyph_map[c].advance >> 6);
                float candidate = this->glyph_map[c].height;
                if (candidate > height)
                    height = candidate;
            }
            // Deixa o texto próximo da parte superior da tela
            if (y_top) {
                y = 0.99f - height * scale_y;
            // Centraliza nas coordenadas passadas para a função
            } else {
                x -= length * 0.5f * scale_x;
                y -= height * 0.5f * scale_y;
            }
        }
        this->draw_text(text, x, y, scale_x, scale_y);
    }

    // Exibe quaisquer erros recentes do OpenGL
    static void log_error(int num = 0) {
        GLenum error = glGetError();
        while (error != GL_NO_ERROR) {
            if (num > 0)
                std::cerr << num << ": ";
            switch (error) {
                case GL_INVALID_ENUM:      std::cerr << "Invalid enum"; break;
                case GL_INVALID_VALUE:     std::cerr << "Invalid value"; break;
                case GL_INVALID_OPERATION: std::cerr << "Invalid operation"; break;
                case GL_STACK_OVERFLOW:    std::cerr << "Stack overflow"; break;
                case GL_STACK_UNDERFLOW:   std::cerr << "Stack underflow"; break;
                case GL_OUT_OF_MEMORY:     std::cerr << "Out of memory"; break;
                default:                   std::cerr << "Unknown error";
            }
            std::cerr << std::endl;
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
            destroy_window();
            std::cerr << this->buffer;
            throw std::runtime_error("Explodiu!");
    }

    void use_program(Shape shape) {
        if (shape == Shape::None) {
            glUseProgram(0);
            glBindVertexArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindTexture(GL_TEXTURE_2D, 0);
        } else {
            glUseProgram(this->shaders[shape]);
            glBindVertexArray(this->VAO[shape]);
            glBindBuffer(GL_ARRAY_BUFFER, this->VBO[shape]);
        }
    }
};
