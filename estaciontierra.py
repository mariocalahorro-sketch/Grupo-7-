#[V2] PROTOCOLO DE APLICACION (el mismo que en los dos Arduinos)
#  Todos los mensajes son "codigo" o "codigo:datos" y acaban en salto de linea.
#  Python -> Satelite:  1 Iniciar | 2/3 Pausar/Reanudar T/H | 4/5 Pausar/Reanudar distancia
#                       6:ms periodo T/H | 7:ms periodo distancia | 8 barrido | 9:angulo apuntar
#                       10:1 media en el satelite | 10:0 media en tierra
#  Satelite -> Python:  1:T:H | 2:sensor:angulo:distancia | 3:media | 4 fallo sensor T/H
#                       5:sensor fallo sensor de distancia | 6 fallo de comunicacion (lo envia el Arduino de tierra)
import serial
import threading
import time
import math      #[V2] para pasar los angulos del radar a radianes
import winsound
from tkinter import *
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg

device = 'COM3'  #COM3 arduino tierra
mySerial = serial.Serial(device, 9600)   #inicia la comunicación

#elementos para la grafica
temperaturas = []
humedades = []
eje_x = []
pausas = []        #cada pausa es [segundo en que empieza, segundo en que acaba]
inicio = None      #momento en que se pulsa Iniciar
VENTANA = 60       #segundos que se ven a la vez en la grafica
en_pausa = False   #True mientras el sistema esta en pausa
cortar = False     #True justo tras reanudar: el siguiente dato empieza una linea nueva

#[V2] elementos nuevos para la grafica
medias = []            #media de las ultimas 10 temperaturas
eje_medias = []        #segundo en que se calculo cada media
alarmas_x = []         #segundos en los que ha saltado la alarma de la media
alarmas_y = []         #valor de la media en esos momentos
fallos_sensor = []     #segundos en los que ha llegado un fallo del sensor T/H
fallos_com = []        #segundos en los que ha llegado un fallo de comunicacion

#[V2] radar: una posicion por grado (0 a 360) con la distancia vista en ese angulo
radar = [None] * 361
angulo_actual = None   #hacia donde mira ahora el sensor 1 (el sensor 2 mira 180 grados mas)

#[V2] estado de las alarmas (se muestra en las etiquetas de la ventana)
estado_sensor = "OK"
estado_com = "OK"
estado_media = "OK"
falla1 = False         #falla el sensor de distancia 1?
falla2 = False         #falla el sensor de distancia 2?

#[V2] media de las temperaturas
media_en_satelite = False   #False = se calcula aqui, True = la envia el satelite
limite = 30.0               #limite de la media (lo cambia el usuario)
medias_seguidas = 0         #cuantas medias seguidas superan el limite
cola = [0.0] * 10           #cola circular con las ultimas 10 temperaturas
posicion = 0                #donde se guardara la siguiente temperatura
cantidad = 0                #cuantas temperaturas hay guardadas (maximo 10)

#[V2] guardar_en_cola: guarda una temperatura en la cola circular
#parametros: t -> la temperatura que se quiere guardar
#resultado: no devuelve nada; cambia cola, posicion y cantidad
def guardar_en_cola (t):
    global posicion, cantidad
    cola[posicion] = t
    posicion = posicion + 1
    if posicion == 10:
        posicion = 0       #vuelve al principio y pisa la mas antigua: por eso es circular
    if cantidad < 10:
        cantidad = cantidad + 1

#[V2] calcular_media: calcula la media de las temperaturas guardadas en la cola
#parametros: ninguno
#resultado: devuelve la media (un numero con decimales)
def calcular_media ():
    suma = 0
    for i in range(cantidad):
        suma = suma + cola[i]
    return suma / cantidad

#[V2] nueva_media: guarda una media nueva para la grafica y mira si hay 3 seguidas por encima del limite
#parametros: media -> el valor de la media;  segundos -> momento en que se ha calculado
#resultado: no devuelve nada; si toca, activa la alarma de la media (pitido y estado "ALARMA")
def nueva_media (media, segundos):
    global medias_seguidas, estado_media
    eje_medias.append(segundos)
    medias.append(media)
    print("Media de las ultimas temperaturas:", round(media, 2), "C")
    if media > limite:
        medias_seguidas = medias_seguidas + 1
    else:
        medias_seguidas = 0
    if medias_seguidas >= 3:
        estado_media = "ALARMA"
        alarmas_x.append(segundos)
        alarmas_y.append(media)
        print("ALARMA: 3 medias seguidas por encima del limite de", limite, "C")
        winsound.Beep(1500, 300)   #alarma media
    else:
        estado_media = "OK"

#recepcion: se pone en marcha al pulsar "Iniciar". Lee sin parar los mensajes que llegan
#           del Arduino de tierra y, segun su codigo, guarda los datos o activa las alarmas
#parametros: ninguno
#resultado: no devuelve nada; rellena las listas de las graficas y los estados de las alarmas
def recepcion ():
    global cortar, angulo_actual, estado_sensor, estado_com, falla1, falla2   #[V2] variables nuevas
    while True:
        if mySerial.in_waiting > 0:
            #lee los datos recibidos del satelite
            linea = mySerial.readline().decode('utf-8').rstrip()
            #[V2] antes aqui se hacia print(linea). Ahora la linea es un codigo ("4", "5:1"...)
            #     que el usuario no entenderia, asi que cada mensaje se escribe explicado mas abajo

            if linea == "4":                       #[V2] antes "Fallo"
                print("ALARMA: fallo del sensor de temperatura y humedad")
                winsound.Beep(1000, 500)   #alarma sensor
                fallos_sensor.append(time.time() - inicio)   #[V2] para marcarlo en la grafica
                estado_sensor = "FALLO"                      #[V2]
            elif linea == "6":                     #[V2] antes "Comunicacion"
                print("ALARMA: fallo de comunicacion con el satelite")
                winsound.Beep(500, 500)    #alarma comunicacion
                fallos_com.append(time.time() - inicio)      #[V2]
                estado_com = "FALLO"                         #[V2]
            elif linea == "5:1":                   #[V2] falla el sensor de distancia 1
                print("ALARMA: fallo del sensor de distancia 1")
                winsound.Beep(800, 500)
                falla1 = True
            elif linea == "5:2":                   #[V2] falla el sensor de distancia 2
                print("ALARMA: fallo del sensor de distancia 2")
                winsound.Beep(800, 500)
                falla2 = True

            #[V2] cualquier mensaje que no sea el 6 viene del satelite: la comunicacion funciona
            if linea != "6":
                estado_com = "OK"

            trozos = linea.split (':')   #separamos los elementos recibidos

            #solo procesamos lineas con formato 1:T:H, y no si estamos en pausa   ([V2] antes T:xx:H:xx)
            if len(trozos) == 3 and trozos[0] == '1' and not en_pausa:
                temperatura = float(trozos[1])
                humedad = float(trozos[2])
                segundos = time.time() - inicio   #segundos desde Iniciar
                print("Temperatura:", temperatura, "C   Humedad:", humedad, "%")   #[V2]

                #primer dato tras una pausa: se mete un nan para cortar la linea
                if cortar:
                    eje_x.append(segundos)
                    temperaturas.append (float('nan'))
                    humedades.append (float('nan'))
                    eje_medias.append(segundos)        #[V2] tambien se corta la linea de la media
                    medias.append(float('nan'))
                    cortar = False

                eje_x.append(segundos)
                temperaturas.append (temperatura)
                humedades.append (humedad)
                estado_sensor = "OK"                   #[V2] el sensor vuelve a funcionar

                #[V2] la temperatura se guarda en la cola; si la media se calcula aqui, se calcula
                guardar_en_cola(temperatura)
                if not media_en_satelite:
                    nueva_media(calcular_media(), segundos)

            #[V2] media calculada en el satelite: 3:media
            elif len(trozos) == 2 and trozos[0] == '3' and media_en_satelite and not en_pausa:
                nueva_media(float(trozos[1]), time.time() - inicio)

            #[V2] distancia: 2:sensor:angulo:distancia
            elif len(trozos) == 4 and trozos[0] == '2':
                sensor = trozos[1]
                angulo = int(trozos[2])
                distancia = float(trozos[3])
                print("Distancia sensor", sensor, "a", angulo, "grados:", distancia, "cm")
                radar[angulo] = distancia
                if sensor == '1':
                    angulo_actual = angulo
                    falla1 = False
                else:
                    falla2 = False

#[V2] color: elige el color de una etiqueta de alarma
#parametros: estado -> el texto del estado ("OK" o cualquier otro)
#resultado: devuelve 'lightgreen' si todo va bien y 'tomato' (rojo) si hay alarma
def color (estado):
    if estado == "OK":
        return 'lightgreen'
    return 'tomato'

#actualizar_grafica: dibuja la grafica de T/H (con su media y sus alarmas), el radar y las
#                    etiquetas de alarma. Se llama a si misma cada segundo
#parametros: ninguno
#resultado: no devuelve nada; redibuja la ventana
def actualizar_grafica ():
    if len(eje_x) > 0:
        ahora = time.time() - inicio
        n = min(len(eje_x), len(temperaturas), len(humedades))
        ax.clear()
        ax.plot(eje_x[:n], temperaturas[:n], label='Temperatura (°C)')
        ax.plot(eje_x[:n], humedades[:n], label='Humedad (%)')

        #[V2] media de las ultimas 10 temperaturas y su limite
        m = min(len(eje_medias), len(medias))
        ax.plot(eje_medias[:m], medias[:m], 'k--', label='Media 10 temp.')
        ax.axhline(limite, color='red', linestyle=':', label='Límite media')
        #[V2] alarmas: cruz roja = alarma de la media, linea roja = fallo sensor, naranja = fallo comunicacion
        a = min(len(alarmas_x), len(alarmas_y))
        if a > 0:
            ax.plot(alarmas_x[:a], alarmas_y[:a], 'rx', markersize=10, label='Alarma media')
        if len(fallos_sensor) > 0:
            ax.vlines(fallos_sensor, 0, 100, color='red', alpha=0.4, label='Fallo sensor')
        if len(fallos_com) > 0:
            ax.vlines(fallos_com, 0, 100, color='orange', alpha=0.4, label='Fallo comunicación')

        #franjas grises donde ha habido una pausa
        for k, p in enumerate(pausas):
            fin = p[1] if p[1] is not None else ahora   #si sigue en pausa, llega hasta ahora
            ax.axvspan(p[0], fin, color='gray', alpha=0.3,
                       label='Pausa' if k == 0 else None)

        #el eje x se desplaza con el tiempo
        ax.set_xlim(max(0, ahora - VENTANA), max(VENTANA, ahora))
        ax.set_xlabel('Tiempo (s)')
        ax.legend(loc='upper left', fontsize=7)   #[V2] letra mas pequena: ahora hay mas elementos
        canvas.draw()

    #[V2] radar: puntos verdes = objetos detectados (hasta 200 cm)
    if inicio is not None:
        ax2.clear()
        ax2.set_ylim(0, 200)
        ax2.set_rticks([50, 100, 150, 200])   #circulos cada 50 cm
        ax2.set_title('Radar (cm)')
        angulos = []
        distancias = []
        for grado in range(361):
            if radar[grado] is not None and radar[grado] <= 200:
                angulos.append(math.radians(grado))
                distancias.append(radar[grado])
        ax2.plot(angulos, distancias, 'g.')
        if angulo_actual is not None:
            r1 = math.radians(angulo_actual)          #haz del sensor 1 (verde)
            r2 = math.radians(angulo_actual + 180)    #haz del sensor 2 (azul)
            ax2.plot([r1, r1], [0, 200], color='green')
            ax2.plot([r2, r2], [0, 200], color='blue')
        canvas2.draw()

        #[V2] etiquetas de alarmas
        if falla1 and falla2:
            estado_distancia = "FALLO S1 y S2"
        elif falla1:
            estado_distancia = "FALLO S1"
        elif falla2:
            estado_distancia = "FALLO S2"
        else:
            estado_distancia = "OK"
        sensorLabel.config(text="Sensor T/H: " + estado_sensor, bg=color(estado_sensor))
        distanciaLabel.config(text="Distancia: " + estado_distancia, bg=color(estado_distancia))
        comLabel.config(text="Comunicación: " + estado_com, bg=color(estado_com))
        mediaLabel.config(text="Media: " + estado_media, bg=color(estado_media))
    window.after(1000, actualizar_grafica)

#interfaz grafica del panel de control
#Las funciones siguientes son las de los botones. Ninguna tiene parametros ni devuelve nada:
#cada una envia su orden al satelite (ver el protocolo al principio del archivo)

#Iniciar: envia la orden 1, apunta el momento de inicio y pone en marcha la recepcion
def Iniciar ():
    global inicio
    inicio = time.time()
    mensaje = "1\n"   #[V2] Iniciar (antes "Iniciar"). El \n marca el final del mensaje
    mySerial.write(mensaje.encode('utf-8'))
    threadRecepcion = threading.Thread (target = recepcion)
    threadRecepcion.start()
    IniciarButton.config(state = DISABLED)

#Pausar: envia la orden 2 (pausar T/H) y apunta cuando empieza la pausa
def Pausar ():
    global en_pausa
    mensaje = "2\n"   #[V2] Pausar T/H (antes "Pausar")
    mySerial.write(mensaje.encode('utf-8'))
    #se anota cuando empieza la pausa (solo si no estaba ya en pausa)
    if inicio is not None and not en_pausa:
        en_pausa = True
        pausas.append([time.time() - inicio, None])

#Reanudar: envia la orden 3 (reanudar T/H) y apunta cuando acaba la pausa
def Reanudar ():
    global en_pausa, cortar
    mensaje = "3\n"   #[V2] Reanudar T/H (antes "Reanudar")
    mySerial.write(mensaje.encode('utf-8'))
    #se anota cuando acaba la pausa (solo si estaba en pausa)
    if en_pausa:
        en_pausa = False
        cortar = True
        pausas[-1][1] = time.time() - inicio

#[V2] PausarDistancia: envia la orden 4 (el satelite deja de enviar distancias)
def PausarDistancia ():
    mensaje = "4\n"
    mySerial.write(mensaje.encode('utf-8'))

#[V2] ReanudarDistancia: envia la orden 5 (el satelite vuelve a enviar distancias)
def ReanudarDistancia ():
    mensaje = "5\n"
    mySerial.write(mensaje.encode('utf-8'))

#[V2] CambiarPeriodoTH: lee los segundos que ha escrito el usuario y envia la orden 6
#     (el usuario escribe segundos y se envian milisegundos)
def CambiarPeriodoTH ():
    segundos = float(periodoTHEntry.get())
    mensaje = "6:" + str(int(segundos * 1000)) + "\n"
    mySerial.write(mensaje.encode('utf-8'))

#[V2] CambiarPeriodoDistancia: igual que la anterior, con la orden 7
def CambiarPeriodoDistancia ():
    segundos = float(periodoDistanciaEntry.get())
    mensaje = "7:" + str(int(segundos * 1000)) + "\n"
    mySerial.write(mensaje.encode('utf-8'))

#[V2] Barrido: envia la orden 8 (el servo barre toda la zona sin parar)
def Barrido ():
    mensaje = "8\n"
    mySerial.write(mensaje.encode('utf-8'))

#[V2] Apuntar: lee el angulo que ha escrito el usuario y envia la orden 9 (el servo se queda fijo ahi)
def Apuntar ():
    angulo = int(anguloEntry.get())
    mensaje = "9:" + str(angulo) + "\n"
    mySerial.write(mensaje.encode('utf-8'))

#[V2] CambiarMedia: mira que opcion ha elegido el usuario (Tierra o Satelite) y envia la orden 10
def CambiarMedia ():
    global media_en_satelite
    if modoMedia.get() == 1:
        media_en_satelite = True
        mensaje = "10:1\n"
    else:
        media_en_satelite = False
        mensaje = "10:0\n"
    mySerial.write(mensaje.encode('utf-8'))

#[V2] FijarLimite: lee el limite de la media que ha escrito el usuario y empieza a contar de nuevo
#     (no envia nada al satelite: la alarma de la media se comprueba aqui)
def FijarLimite ():
    global limite, medias_seguidas
    limite = float(limiteEntry.get())
    medias_seguidas = 0

window = Tk()
window.configure(bg="lightblue")
window.geometry("1100x700")   #[V2] ventana mas grande (antes 600x500)
window.rowconfigure(0, weight=1)
window.rowconfigure(1, weight=1)
window.columnconfigure(0, weight=1)
window.columnconfigure(1, weight=1)
window.columnconfigure(2, weight=1)
window.columnconfigure(3, weight=1)   #[V2] columna para el radar

tituloLabel = Label(window, text = "Panel de control", bg='lightblue',font=("Courier", 20))
tituloLabel.grid(row=0, column=0, columnspan=5, padx=5, pady=5, sticky=N + S + E + W)

IniciarButton = Button(window, text="Iniciar", bg='darkgreen', fg="white", command=Iniciar)
IniciarButton.grid(row=1, column=0, padx=5, pady=5, sticky=N + S + E + W)

PausarButton = Button(window, text="Pausar T/H", bg='red', fg="white", command=Pausar)   #[V2] texto (antes "Pausar")
PausarButton.grid(row=1, column=1, padx=5, pady=5, sticky=N + S + E + W)

ReanudarButton = Button(window, text="Reanudar T/H", bg='orange', fg="white", command=Reanudar)   #[V2] texto (antes "Reanudar")
ReanudarButton.grid(row=1, column=2, padx=5, pady=5, sticky=N + S + E + W)

#grafica incrustada en la ventana
fig = Figure(figsize=(5, 3))
ax = fig.add_subplot(111)
canvas = FigureCanvasTkAgg(fig, master=window)
canvas.get_tk_widget().grid(row=2, column=0, columnspan=3, padx=5, pady=5, sticky=N + S + E + W)

#[V2] radar incrustado a la derecha de la grafica
fig2 = Figure(figsize=(3, 3))
fig2.subplots_adjust(top=0.82, bottom=0.08)   #deja sitio para el titulo
ax2 = fig2.add_subplot(111, projection='polar')
canvas2 = FigureCanvasTkAgg(fig2, master=window)
canvas2.get_tk_widget().grid(row=2, column=3, padx=5, pady=5, sticky=N + S + E + W)

#[V2] controles nuevos (en un marco debajo de las graficas)
controles = Frame(window, bg='lightblue')
controles.grid(row=3, column=0, columnspan=4, padx=5, pady=5)

Button(controles, text="Pausar distancia", bg='red', fg="white", command=PausarDistancia).grid(row=0, column=0, padx=5, pady=3)
Button(controles, text="Reanudar distancia", bg='orange', fg="white", command=ReanudarDistancia).grid(row=0, column=1, padx=5, pady=3)
Button(controles, text="Modo barrido", bg='purple', fg="white", command=Barrido).grid(row=0, column=2, padx=5, pady=3)

Label(controles, text="Ángulo (0-180):", bg='lightblue').grid(row=0, column=3)
anguloEntry = Entry(controles, width=6)
anguloEntry.insert(0, "90")          #valor de ejemplo, para que se vea que hay que escribir
anguloEntry.grid(row=0, column=4)
Button(controles, text="Apuntar", command=Apuntar).grid(row=0, column=5, padx=5)

Label(controles, text="Periodo T/H (s, mínimo 1):", bg='lightblue').grid(row=1, column=0)
periodoTHEntry = Entry(controles, width=6)
periodoTHEntry.insert(0, "3")        #el periodo con el que arranca el satelite
periodoTHEntry.grid(row=1, column=1)
Button(controles, text="Cambiar", command=CambiarPeriodoTH).grid(row=1, column=2, padx=5, pady=3)

Label(controles, text="Periodo distancia (s, mínimo 0.15):", bg='lightblue').grid(row=1, column=3)
periodoDistanciaEntry = Entry(controles, width=6)
periodoDistanciaEntry.insert(0, "0.2")   #el periodo con el que arranca el satelite
periodoDistanciaEntry.grid(row=1, column=4)
Button(controles, text="Cambiar", command=CambiarPeriodoDistancia).grid(row=1, column=5, padx=5, pady=3)

Label(controles, text="Media calculada en:", bg='lightblue').grid(row=2, column=0)
modoMedia = IntVar(value=0)
Radiobutton(controles, text="Tierra", variable=modoMedia, value=0, bg='lightblue', command=CambiarMedia).grid(row=2, column=1)
Radiobutton(controles, text="Satélite", variable=modoMedia, value=1, bg='lightblue', command=CambiarMedia).grid(row=2, column=2)

Label(controles, text="Límite media (°C):", bg='lightblue').grid(row=2, column=3)
limiteEntry = Entry(controles, width=6)
limiteEntry.insert(0, "30")          #el limite con el que arranca el programa
limiteEntry.grid(row=2, column=4)
Button(controles, text="Fijar", command=FijarLimite).grid(row=2, column=5, padx=5, pady=3)

#[V2] etiquetas de alarmas (verde = OK, rojo = alarma)
sensorLabel = Label(window, text="Sensor T/H: -", width=25)
sensorLabel.grid(row=4, column=0, padx=5, pady=5)
distanciaLabel = Label(window, text="Distancia: -", width=25)
distanciaLabel.grid(row=4, column=1, padx=5, pady=5)
comLabel = Label(window, text="Comunicación: -", width=25)
comLabel.grid(row=4, column=2, padx=5, pady=5)
mediaLabel = Label(window, text="Media: -", width=25)
mediaLabel.grid(row=4, column=3, padx=5, pady=5)

actualizar_grafica()
window.mainloop()