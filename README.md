# BinaryTreeDebugging
Repositório que combina um projeto de Visualização da Informação, uma tentativa de aprender OpenGL e a esperança de ajudar algum aluno com debugging no futuro.

## O que é?
Um header file que implementa uma árvore binária de busca e um programa que a visualiza.
A implementação de árvore é meramente ilustrativa e mostra como usar a interface de visualização.

## Como usar?
Basta construir um objeto de visualização, podendo especificar o tamanho da janela e se ela estará em tela cheia. Note que o objeto deve ter como parâmetro de template o tipo ponteiro para o tipo dos nós da árvore, que deve implementar a interface `Node` com os métodos `left`, `right` e `key`. O método `key` deve retornar um valor que pode ser convertido para um array de `char` pelos objetos do STL e os métodos `left` e `right` devem retornar um ponteiro para o nó filho da esquerda e da direita, respectivamente.

Para que a árvore seja desenhada, basta chamar o método `draw`. Com a janela rodando, pode-se pressionar a tecla ESC para fechá-la a qualquer momento, sendo necessário criar outro objeto com uma janela para que seja possível desenhar na tela novamente. Para continuar a execução do código antes do tempo especificado na chamada da função, basta pressioar ENTER.
É possível, no modo interativo, pressionar as teclas direcionais ou WASD para navegar pela árvore, F ou F11 para alternar entre janela e tela cheia e as teclas + e - do keypad para aumentar e diminuir o zoom, respectivamente. Ao pressionar espaço, é adicionado um atraso entre a leitura das teclas pressionadas e os passos se tornam mais longos. Isso é feito para evitar perdas de desempenho quando há muito a ser desenhado a cada frame.
Note também que a fonte é renderizada no momento da construção do objeto, levando em conta a resolução definida, então, caso a resolução aumente e o usuário queira renderizar a fonte em um tamanho maior, é necessário chamar o método `load_font` novamente.
Pode ser que ocorra uma segmentaton fault ao fim da execução do programa, provavelmente causada por alguma dependência do GLFW. Isso não afeta o funcionamento do programa.

## Como compilar?
O programa foi desenvolvido pensando num usuário de Linux ou WSL, mas deve funcionar em qualquer sistema, dado que o usuário configure corretamente as dependências.
Para instalar as dependências no Ubuntu:

```
sudo apt-get install libglfw3-dev libxxf86vm-dev libxi-dev libglew-dev libglm-dev libfreetype-dev
```

Para compilar:

```
g++ -g main.cpp -lglfw -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi -lGL -lGLU -lGLEW -lfreetype -I/usr/include/freetype2 -o main
```
