------------------------------------------------------------

Terceira fase do Projeto da cadeira de Sistemas Distribuidos
, no ano letivo 2021/2022.

Realizado por:
-João Ferraz, nº49420.
-Miguel Carvalho, nº54399;
-Bruno Cotrim, nº54406;

------------------------------------------------------------

Limitações:
Lembrar que ao adicionar Strings ao server, a String é transformada num array de bytes então o tamanho dos dados será strlen(string)+1 porque tem em conta do byte relativo ao null terminator
Valgrind na função acept levanta um aviso na função accept das sockets onde diz que os parametros apontam para bytes não inicializados, nós poderíamos fazer com que aviso não aparecesse usando calloc(1,sizeof(estrutura/tipo)), no entanto, em todos os exemplos dados pelos professores e presentes na internet ninguém usava essa forma então achá-mos melhor seguir a norma.


------------------------------------------------------------