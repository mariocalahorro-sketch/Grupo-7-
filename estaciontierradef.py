import serial
import threading
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

#funcion al pulsar el boton "iniciar" -> el programa se pone en marcha
def recepcion ():
    i = 0
    while True:
        if mySerial.in_waiting > 0:
            #lee los datos recibidos del satelite
            linea = mySerial.readline().decode('utf-8').rstrip()
            print(linea)
            if linea == "Fallo":
                winsound.Beep(1000, 500)   #alarma sensor
            elif linea == "Comunicacion":
                winsound.Beep(500, 500)    #alarma comunicacion
            
            trozos = linea.split (':')   #separamos los elementos recibidos

            #solo procesamos lineas con formato T:xx:H:xx
            if len(trozos) == 4 and trozos[0] == 'T' and trozos[2] == 'H':
                
                temperatura = float(trozos[1])
                humedad = float(trozos[3])

                eje_x.append(i)
                temperaturas.append (temperatura)
                humedades.append (humedad)
                i = i + 1

#dibuja la grafica y se va alargando en funcion del tiempo
def actualizar_grafica ():
    if len(eje_x) > 0:
        n = min(len(eje_x), len(temperaturas), len(humedades))
        ax.clear()
        ax.plot(eje_x[:n], temperaturas[:n])
        ax.plot(eje_x[:n], humedades[:n])
        canvas.draw()
    window.after(1000, actualizar_grafica)

#interfaz grafica del panel de control
def Iniciar ():
    mensaje = "Iniciar"
    mySerial.write(mensaje.encode('utf-8'))
    threadRecepcion = threading.Thread (target = recepcion)
    threadRecepcion.start()
    IniciarButton.config(state = DISABLED)

def Pausar ():
    mensaje = "Pausar"
    mySerial.write(mensaje.encode('utf-8'))

def Reanudar ():
    mensaje = "Reanudar"
    mySerial.write(mensaje.encode('utf-8'))

window = Tk()
window.configure(bg="lightblue")
window.geometry("600x500")
window.rowconfigure(0, weight=1)
window.rowconfigure(1, weight=1)
window.columnconfigure(0, weight=1)
window.columnconfigure(1, weight=1)
window.columnconfigure(2, weight=1)

tituloLabel = Label(window, text = "Panel de control", bg='lightblue',font=("Courier", 20))
tituloLabel.grid(row=0, column=0, columnspan=5, padx=5, pady=5, sticky=N + S + E + W)

IniciarButton = Button(window, text="Iniciar", bg='darkgreen', fg="white", command=Iniciar)
IniciarButton.grid(row=1, column=0, padx=5, pady=5, sticky=N + S + E + W)

PausarButton = Button(window, text="Pausar", bg='red', fg="white", command=Pausar)
PausarButton.grid(row=1, column=1, padx=5, pady=5, sticky=N + S + E + W)

ReanudarButton = Button(window, text="Reanudar", bg='orange', fg="white", command=Reanudar)
ReanudarButton.grid(row=1, column=2, padx=5, pady=5, sticky=N + S + E + W)

#grafica incrustada en la ventana
fig = Figure(figsize=(5, 3))
ax = fig.add_subplot(111)
canvas = FigureCanvasTkAgg(fig, master=window)
canvas.get_tk_widget().grid(row=2, column=0, columnspan=3, padx=5, pady=5, sticky=N + S + E + W)

actualizar_grafica()
window.mainloop()