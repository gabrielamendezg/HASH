# HASH
TDA Hash C++

Correcciones a realizar:
Actualmente estas creando una serie de listas en la redimensión, una mejora que se me ocurre para el hash, es que las listas las crees en el momento que las necesitas. Es decir, que cuando vas a insertar un valor en la tabla, y vez que esa posición es null, recién ahí creas la lista e insertas el campo normalmente.

El mismo comentario para la primitiva hash_crear. De todas formas, estoy notando cierto código repetido entre este segmento y el anterior (problema que se resuelve si creas las listas solo cuando las necesitas)

En el hash_guardar, si falla la redimensión no tiene ninguna consecuencia. Seria una buena practica notificar que hubo algún tipo de error, y una manera correcta de hacerlo es redimensionando antes de guardar, y si es que falla este proceso, se retorna false

Cuidado que en hash_pertenece, tenes líneas de código repetidas

En hash_iter_avanzar, se te ocurre como bajar al menos un nivel de identación? Aplicar esta mejora hace que el código sea mucho más legible, lo cual es muy valorable
