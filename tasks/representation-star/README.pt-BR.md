# Representações _forward_ e _reverse_ _star_

## Comparação com matriz de adjacência

A matriz de adjacência é uma representação _extremamente_ ineficiente em espaço
para grafos que não sejam bastante densos, já que requer $|V|^2$ elementos no
total.

Neste exercício, temos 50.000 vértices e 1.018.039 arestas, isto é, um fator de
aproximadamente 20 vértices para cada aresta—bem menor que o fator 50.000, para
um grafo totalmente denso. Por conta disso, descartei a representação da matriz
de adjacência.

## Comparação com a lista de adjacência

A lista de adjacência é uma outra representação viável para grafos não muito
densos.

Em comparação às representações _forward_ e _reverse_ _star_, a matrix de
adjacência tende a ser:

- Menos eficiente em espaço tendo em vista a necessidade de ter que se armazenar
  um ponteiro (8 bytes em arquiteturas de 64 bits) para cada sucessor ou
  predecessor;
- Menor eficiente em [localidade de cache][locality] ao se iterar sobre os
  sucessores ou predecessores.

[locality]: https://en.wikipedia.org/wiki/Locality_of_reference

A vantagem da matriz de adjacência é uma maior flexibilidade em comparação às
representações _star_. Isso porque:

- As representações _star_ exigem que os nós sejam fornecidos em uma sequência
  numérica contínua e sem "buracos". Se não houver essa garantia para a entrada
  (o que não foi o caso para este exercício), cabe ao programa introduzir
  mecanismos para eventualmente fazer uma tradução de IDs de vértices ou
  garantir a sequência destes.
- As representações _star_ não podem ser tão facilmente alteradas uma vez que já
  foram construídas (notar, inclusive, que as arestas devem estar ordenadas
  durante a construção da representação _star_), haja vista que, tendo em vista
  a ordenação dos elementos, teria que se fazer um shift para a esquerda que, no
  pior das hipóteses, pode ter complexidade linear.

Neste exercício, como nenhum dos pontos desfavoráveis das representações _star_
se aplica, optei por ela.

## Observações finais

Embora as representações _star_ sejam mais eficientes do que a representação de
lista de adjacência, a impressão que eu fiquei é que elas são bastante frágeis.
A implementação deve ser cuidadosa para não cair em erros, especialmente de
"buracos" se os nós não formarem uma sequência perfeita.

Ademais, realisticamente, acredito que a maioria dos grafos não possuam IDs
inteiros e perfeitamente sequenciais. Desse modo, as representações _star_
raramente seriam, sozinhas, suficientes. Imagino que, nesses casos, uma camada
extra de indireção seja necessária para "traduzir" o ID do vértice original para
um ID sequencial a ser utilizado na representação _star_.
