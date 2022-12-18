# BinaryTreeDebugging
Repositório que combina um projeto de Visualização da Informação, uma tentativa de aprender OpenGL e a esperança de ajudar algum aluno com debugging no futuro.

## O que é?
Um header file que implementa uma árvore binária de busca e um programa que a visualiza.
A implementação de árvore é meramente ilustrativa e mostra como usar a interface de visualização.

## Como usar?
Basta construir um objeto de visualização, podendo especificar o tamanho da janela e se ela estará em tela cheia. Note que o objeto deve ter como parâmetro de template o tipo ponteiro para o tipo dos nós da árvore, que deve implementar a interface `Node` com os métodos `left`, `right` e `key`. O método `key` deve retornar um valor que pode ser convertido para um array de `char` pelos objetos do STL e os métodos `left` e `right` devem retornar um ponteiro para o nó filho da esquerda e da direita, respectivamente.
Para que a árvore seja desenhada, basta chamar o método `draw`. Com a janela rodando, pode-se pressionar a tecla ESC para fechá-la a qualquer momento. É possível, no modo interativo, pressionar as teclas direcionais ou WASD para navegar pela árvore. Ao pressionar espaço, é adicionado um atraso entre a leitura das teclas pressionadas e os passos se tornam mais longos. Isso é feito para evitar perdas de desempenho quando há muito a ser desenhado a cada frame.
Como eu ainda não faço ideia do que estou fazendo, algumas vezes o programa acusa segmentation fault ao fim da execução, mas isso não impede seu funcionamento.

## Como compilar?
O programa foi desenvolvido pensando num usuário de Linux ou WSL, mas deve funcionar em qualquer sistema, dado que o usuário configure corretamente as dependências.
Para instalar as dependências no Ubuntu:

```sudo apt-get install libglfw3 libglfw3-dev libglew-dev libglm-dev libfreetype-dev```

Para compilar:

```g++ -g main.cpp -lglfw -lGL -lGLU -lGLEW -lX11 -lpthread -lXrandr -ldl -lrt -lfreetype -I/usr/include/freetype2```
