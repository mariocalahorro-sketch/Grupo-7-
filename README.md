# Nuestro proyecto
Somos el grupo 7, formado por Mario Calahorro, Erik Fernández y Martina Mas y este es nuestro proyecto de Ciencias de la Computación.
## Versión 1 del proyecto
A continuación encontramos un vídeo demostrativo de como funciona la primera versión de este trabajo.
<p align="center">
  <a href="https://youtu.be/1QTZ2Or79EM">
    <img src="https://img.youtube.com/vi/1QTZ2Or79EM/0.jpg" alt="Mira el video" width="400">
  </a>
</p>

## Versión 2 del proyecto
En esta version se añaden nuevas funciones que son presentadas en el video de a continuación.
<p align="center">
  <a href="https://youtu.be/fqFUmdEEYDA">
    <img src="https://img.youtube.com/vi/fqFUmdEEYDA/0.jpg" alt="Mira el video" width="400">
  </a>
</p>

## Uso de la Inteligencia Artificial
Hemos usado un asistente de IA (Claude) como apoyo, sobre todo en las partes que la guía no resuelve y para depurar. En los pasos guiados solo lo hemos usado de forma puntual para localizar errores en nuestro código.
### Versión 1
La IA nos ha ayudado a:
1. Incrustar la gráfica en la interfaz de Tkinter, y a adaptarla para que se desplace el eje X con el tiempo y se marquen las pausas.
2. Depurar fallos del montaje, como un LED que no se apagaba o alarmas que saltaban a la vez.
**¿Qué hemos aprendido?**
Hemos aprendido a organizar un programa de Arduino con varias tareas a la vez usando millis() en lugar de delay. También por qué la interfaz se bloquea con un bucle infinito y cómo se resuelve con un hilo y window.after. Y cómo funciona una alarma con plazo de espera y por qué hace falta un latido.

### Versión 2
La IA nos ha ayudado a:
1. Hacer el radar con una gráfica polar a partir del ángulo y la distancia.
2. Calcular la media con una cola circular, la alarma de tres medias seguidas sobre el límite y la elección de dónde se calcula.
3. Hacer el programa más robusto, descartando líneas recibidas que llegan estropeadas.
**¿Qué hemos aprendido?**
Hemos aprendido cómo funciona una cola circular y por qué es útil para una media móvil. Además de cómo convertir ángulo y distancia en un gráfico de radar.
