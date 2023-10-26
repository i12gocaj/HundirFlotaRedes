#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/select.h>
#include "game.h"

void manejador(int signum);
void salirCliente(int socket, fd_set *readfds, int *numClientes, int arrayClientes[]);

int main()
{

    /*----------------------------------------------------
        Descriptor del socket y buffer de datos
    -----------------------------------------------------*/
    int sd, new_sd;
    struct sockaddr_in sockname, from;
    char buffer[MSG_SIZE];
    socklen_t from_len;
    fd_set readfds, auxfds;
    int salida;
    int arrayClientes[MAX_CLIENTS];
    int numClientes = 0;
    // contadores
    int i, j, k;
    int recibidos;
    char identificador[MSG_SIZE];
    int on, ret;

    char user[MSG_SIZE];
    char password[MSG_SIZE];

    int contador = 0;

    int fila, columna;
    char letra;

    int bytes_received;

    // Creo el vector de jugadores
    struct jugador jugadores[MAX_CLIENTS];
    inicializar_vector_jugadores(jugadores);

    /* --------------------------------------------------
        Se abre el socket
    ---------------------------------------------------*/
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd == -1)
    {
        perror("No se puede abrir el socket cliente\n");
        exit(1);
    }

    // Activaremos una propiedad del socket para permitir· que otros
    // sockets puedan reutilizar cualquier puerto al que nos enlacemos.
    // Esto permite· en protocolos como el TCP, poder ejecutar un
    // mismo programa varias veces seguidas y enlazarlo siempre al
    // mismo puerto. De lo contrario habrÌa que esperar a que el puerto
    // quedase disponible (TIME_WAIT en el caso de TCP)
    on = 1;
    ret = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    sockname.sin_family = AF_INET;
    sockname.sin_port = htons(2065);
    sockname.sin_addr.s_addr = INADDR_ANY;

    if (bind(sd, (struct sockaddr *)&sockname, sizeof(sockname)) == -1)
    {
        perror("Error en la operación bind");
        exit(1);
    }

    /*---------------------------------------------------------------------
        Del las peticiones que vamos a aceptar sólo necesitamos el
        tamaño de su estructura, el resto de información (familia, puerto,
        ip), nos la proporcionará el método que recibe las peticiones.
    ----------------------------------------------------------------------*/
    from_len = sizeof(from);

    if (listen(sd, 1) == -1)
    {
        perror("Error en la operación de listen");
        exit(1);
    }

    printf("El servidor está esperando conexiones...\n"); // Inicializar los conjuntos fd_set

    FD_ZERO(&readfds);
    FD_ZERO(&auxfds);
    FD_SET(sd, &readfds);
    FD_SET(0, &readfds);

    // Capturamos la señal SIGINT (Ctrl+c)
    signal(SIGINT, manejador);

    /*-----------------------------------------------------------------------
        El servidor acepta una petición
    ------------------------------------------------------------------------ */
    while (1)
    {

        // Esperamos recibir mensajes de los clientes (nuevas conexiones o mensajes de los clientes ya conectados)

        auxfds = readfds;

        salida = select(FD_SETSIZE, &auxfds, NULL, NULL, NULL);

        if (salida > 0)
        {

            for (i = 0; i < FD_SETSIZE; i++)
            {

                // Buscamos el socket por el que se ha establecido la comunicación
                if (FD_ISSET(i, &auxfds))
                {

                    if (i == sd) // AQUÍ ENTRO CUANDO SE CONECTA UN CLIENTE (TERMINAL) POR PRIMERA VEZ (./cliente)
                    {

                        if ((new_sd = accept(sd, (struct sockaddr *)&from, &from_len)) == -1)
                        {
                            perror("Error aceptando peticiones");
                        }
                        else
                        {
                            if (numClientes < MAX_CLIENTS) // MÁS CONCRETAMENTE AQUÍ
                            {
                                arrayClientes[numClientes] = new_sd;
                                numClientes++;
                                FD_SET(new_sd, &readfds);
                                strcpy(buffer, "+0k. Usuario conectado\n");
                                send(new_sd, buffer, sizeof(buffer), 0);

                                printf("Nuevo Cliente conectado en <%d>\n", new_sd);

                                // Aquí viene mi código (ojo, aquí socket es new_sd)
                                guardarNuevoJugador(jugadores, new_sd);
                            }
                            else
                            {

                                bzero(buffer, sizeof(buffer));
                                strcpy(buffer, "Demasiados clientes conectados\n");
                                send(new_sd, buffer, sizeof(buffer), 0);
                                close(new_sd);
                            }
                        }
                    }
                    else if (i == 0) // AQUÍ ENTRO SI EL MENSAJE NO VIENE DE UN CLIENTE SINO DEL TECLADO
                    {
                        // Se ha introducido información de teclado
                        bzero(buffer, sizeof(buffer));
                        fgets(buffer, sizeof(buffer), stdin);

                        if (strcmp(buffer, "SALIR\n") == 0)
                        {

                            for (j = 0; j < numClientes; j++)
                            {
                                bzero(buffer, sizeof(buffer));
                                strcpy(buffer, "Desconexión servidor\n");
                                send(arrayClientes[j], buffer, sizeof(buffer), 0);
                                close(arrayClientes[j]);
                                FD_CLR(arrayClientes[j], &readfds);
                            }
                            close(sd);
                            exit(-1);
                        }
                        // Mensajes que se quieran mandar a los clientes (implementar)
                    }
                    else // AQUÍ ENTRA EL SERVIDOR CUANDO RECIBE UN MENSAJE DE UN CLIENTE QUE NO ES NUEVO
                    {
                        bzero(buffer, sizeof(buffer));

                        recibidos = recv(i, buffer, sizeof(buffer), 0);

                        if (recibidos > 0)
                        {

                            if (strcmp(buffer, "SALIR\n") == 0)
                            {

                                if (jugadores[i].estado == JUGANDO)
                                {

                                    int j = jugadores[i].enemigo;

                                    contador--;

                                    terminarPartida(jugadores, j);

                                    enviarMensajeCliente(j, "+Ok. Tu oponente a terminado la partida\n");
                                }

                                liberarJugador(jugadores, i);
                                salirCliente(i, &readfds, &numClientes, arrayClientes);
                            }
                            else if (strcmp(buffer, "INICIAR-PARTIDA\n") == 0)
                            {

                                if (jugadores[i].estado != LOGUEADO)
                                {
                                    enviarMensajeCliente(i, "-Err. No puedes introducir ahora 'INICIAR-PARTIDA'\n");
                                }
                                else
                                {

                                    printf("El jugador %d quiere jugar\n", i);

                                    jugadores[i].estado = BUSCANDO;

                                    if (buscarJugadoresBuscando(jugadores))
                                    {

                                        int j = BuscarJugador(jugadores, i);

                                        jugadores[i].enemigo = j;

                                        jugadores[j].enemigo = i;

                                        jugadores[i].estado = JUGANDO;
                                        jugadores[i].turno = false;

                                        jugadores[j].estado = JUGANDO;
                                        jugadores[j].turno = true;

                                        if (contador + 1 > MAX_PARTIDAS)
                                        {

                                            enviarMensajeCliente(i, "Número máximo de partidas alcanzado. Debes esperar a que una partida finalice\n");
                                        }
                                        else
                                        {
                                            contador++;

                                            bzero(buffer, sizeof(buffer));

                                            sprintf(buffer, "+Ok. Empieza la partida.");

                                            generarTableroAleatorio(jugadores[i].tablero);
                                            generarMatrizX(jugadores[i].tableroX);
                                            matrizACadena(jugadores[i].tablero, buffer);

                                            send(i, buffer, sizeof(buffer), 0);

                                            bzero(buffer, sizeof(buffer));

                                            sprintf(buffer, "+Ok. Empieza la partida.");

                                            generarTableroAleatorio(jugadores[j].tablero);
                                            generarMatrizX(jugadores[j].tableroX);
                                            matrizACadena(jugadores[j].tablero, buffer);

                                            send(j, buffer, sizeof(buffer), 0);

                                            printf("Jugador %s con socket %d emparejado contra el jugador %s con socket %d en la partida con id %d\n", jugadores[i].user, i, jugadores[j].user, j, contador);

                                            enviarMensajeCliente(j, "+Ok. Turno de partida\n");
                                        }
                                    }
                                    else
                                    {

                                        enviarMensajeCliente(i, "+Ok. Esperando jugadores\n");
                                    }
                                }
                            }
                            else if (strncmp(buffer, "USUARIO", 7) == 0)
                            {

                                if (sscanf(buffer, "USUARIO %s", user) == 1)
                                {

                                    if (jugadores[i].estado != CONECTADO)
                                    {

                                        enviarMensajeCliente(i, "-Err. No puedes introducir ahora 'USUARIO'\n");
                                    }
                                    else if (comprobarNoUsuario(jugadores, user) == false)
                                    {

                                        enviarMensajeCliente(i, "-Err. Este usuario ya ha sido introducido\n");
                                    }
                                    else
                                    {

                                        if (usuarioExiste(user))
                                        {

                                            enviarMensajeCliente(i, "+Ok. Usuario correcto\n");

                                            printf("El usuario con socket %d ha introducido un usuario correcto con la orden USUARIO: %s\n", i, user);

                                            strcpy(jugadores[i].user, user);

                                            jugadores[i].estado = USUARIO;
                                        }
                                        else
                                        {

                                            enviarMensajeCliente(i, "-Err. Usuario incorrecto\n");
                                            printf("El usuario con socket %d ha introducido un usuario incorrecto con la orden USUARIO: %s\n", i, user);
                                        }
                                    }
                                }
                                else
                                {

                                    enviarMensajeCliente(i, "-Err. Debes introducir un usuario después de 'USUARIO'\n");
                                }
                            }
                            else if (strncmp(buffer, "PASSWORD", 8) == 0)
                            {

                                if (jugadores[i].user != NULL)
                                {

                                    if (sscanf(buffer, "PASSWORD %s", password) == 1)
                                    {

                                        if (jugadores[i].estado != USUARIO)
                                        {

                                            enviarMensajeCliente(i, "-Err. No puedes introducir ahora 'PASSWORD'\n");
                                        }
                                        else
                                        {

                                            if (verificarUsuarioYPasswordEnArchivo(jugadores[i].user, password))
                                            {

                                                enviarMensajeCliente(i, "+Ok. Usuario validado\n");

                                                printf("El usuario con socket %d se ha validado en el sistema introduciendo la orden PASSWORD: %s\n", i, password);

                                                jugadores[i].estado = LOGUEADO;
                                            }
                                            else
                                            {

                                                enviarMensajeCliente(i, "-Err. Error en la validación\n");
                                                printf("El usuario con socket %d no ha conseguido validarse en el sistema introduciendo la orden PASSWORD: %s\n", i, password);
                                            }
                                        }
                                    }
                                    else
                                    {

                                        enviarMensajeCliente(i, "-Err. Debes introducir una contraseña después de 'PASSWORD'\n");
                                    }
                                }
                                else
                                {

                                    enviarMensajeCliente(i, "-Err. Debes introducir un nombre de usuario primero\n");
                                }
                            }
                            else if (strncmp(buffer, "REGISTRO", 8) == 0)
                            {

                                if (sscanf(buffer, "REGISTRO -u %s -p %s", user, password) == 2)
                                {

                                    if (jugadores[i].estado != CONECTADO)
                                    {

                                        enviarMensajeCliente(i, "-Err. No puedes introducir ahora 'REGISTRO'\n");
                                    }
                                    else
                                    {

                                        if (registrarUsuario(user, password))
                                        {

                                            enviarMensajeCliente(i, "-Err. Usuario ya registrado\n");
                                        }
                                        else
                                        {

                                            enviarMensajeCliente(i, "+Ok. Usuario registrado con exito\n");
                                        }
                                    }
                                }
                                else
                                {

                                    enviarMensajeCliente(i, "-Err. Formato incorrecto para el comando REGISTRO (REGISTRO -u 'usuario' -p 'contraseña')\n");
                                }
                            }
                            else if (strncmp(buffer, "DISPARO", 7) == 0)
                            {

                                int j = jugadores[i].enemigo;

                                if (jugadores[i].estado != JUGANDO)
                                {

                                    enviarMensajeCliente(i, "-Err. No puedes introducir ahora 'DISPARO'\n");
                                }
                                else
                                {

                                    if (sscanf(buffer, "DISPARO %c,%d", &letra, &columna) == 2)
                                    {

                                        if (letra >= 'A' && letra <= 'J')
                                        {

                                            fila = letra - 'A';

                                            if (columna >= 0 && columna < 10)
                                            {

                                                if (jugadores[i].turno)
                                                {

                                                    if (jugadores[i].tableroX[columna][fila] == 'B' || jugadores[i].tableroX[columna][fila] == 'A')
                                                    {

                                                        enviarMensajeCliente(i, "-Err. No puedes disparar dos veces en el mismo sitio\n");
                                                    }
                                                    else
                                                    {
                                                        char resultado = jugadores[j].tablero[columna][fila];

                                                        jugadores[i].contadorDisparos++;

                                                        if (resultado == AGUA)
                                                        {

                                                            jugadores[i].tableroX[columna][fila] = 'A';

                                                            bzero(buffer, sizeof(buffer));
                                                            sprintf(buffer, "+Ok. AGUA: %c,%d\n", letra, columna);
                                                            send(i, buffer, sizeof(buffer), 0);

                                                            bzero(buffer, sizeof(buffer));
                                                            sprintf(buffer, "+Ok. Disparo en: %c,%d\n", letra, columna);
                                                            send(j, buffer, sizeof(buffer), 0);

                                                            jugadores[i].turno = false;
                                                            jugadores[j].turno = true;

                                                            enviarMensajeCliente(j, "+Ok. Turno de partida\n");
                                                        }
                                                        else if (resultado == BARCO)
                                                        {

                                                            jugadores[i].tableroX[columna][fila] = 'B';

                                                            jugadores[j].tablero[columna][fila] = 'X';

                                                            if (BarcoHundido(jugadores[j].tablero, columna, fila))
                                                            {

                                                                bzero(buffer, sizeof(buffer));
                                                                sprintf(buffer, "+Ok. HUNDIDO: %c,%d\n", letra, columna);
                                                                send(i, buffer, sizeof(buffer), 0);

                                                                bzero(buffer, sizeof(buffer));
                                                                sprintf(buffer, "+Ok. Disparo en: %c,%d\n", letra, columna);
                                                                send(j, buffer, sizeof(buffer), 0);

                                                                jugadores[i].contadorHundido++;

                                                                if (jugadores[i].contadorHundido == numBarcos)
                                                                {

                                                                    bzero(buffer, sizeof(buffer));
                                                                    sprintf(buffer, "+Ok. %.50s ha ganado, numero de disparos %d\n", jugadores[i].user, jugadores[i].contadorDisparos);
                                                                    send(i, buffer, sizeof(buffer), 0);

                                                                    bzero(buffer, sizeof(buffer));
                                                                    sprintf(buffer, "+Ok. %.50s ha ganado, numero de disparos %d\n", jugadores[i].user, jugadores[i].contadorDisparos);
                                                                    send(j, buffer, sizeof(buffer), 0);

                                                                    terminarPartida(jugadores, i);
                                                                    terminarPartida(jugadores, j);

                                                                    contador--;
                                                                }
                                                            }
                                                            else
                                                            {

                                                                bzero(buffer, sizeof(buffer));
                                                                sprintf(buffer, "+Ok. TOCADO: %c,%d\n", letra, columna);
                                                                send(i, buffer, sizeof(buffer), 0);

                                                                bzero(buffer, sizeof(buffer));
                                                                sprintf(buffer, "+Ok. Disparo en: %c,%d\n", letra, columna);
                                                                send(j, buffer, sizeof(buffer), 0);
                                                            }
                                                        }
                                                    }
                                                }
                                                else
                                                {

                                                    enviarMensajeCliente(i, "-Err. No es tu turno\n");
                                                }
                                            }
                                            else
                                            {

                                                enviarMensajeCliente(i, "La segunda coordenada no es valida (0-9)\n");
                                            }
                                        }
                                        else
                                        {

                                            enviarMensajeCliente(i, "La primera coordenada no está en el rango (A-J)\n");
                                        }
                                    }
                                    else
                                    {

                                        enviarMensajeCliente(i, "-Err. Debes introducir una coordenada después de 'DISPARO' (DISPARO B,3)\n");
                                    }
                                }
                            }
                            else
                            {

                                enviarMensajeCliente(i, "-Err. Comando desconocido\n");
                            }
                        }

                        // Si el cliente introdujo ctrl+c
                        if (recibidos == 0)
                        {
                            printf("El socket %d, ha introducido ctrl+c\n", i);
                            // Eliminar ese socket
                            salirCliente(i, &readfds, &numClientes, arrayClientes);
                        }
                    }
                }
            }
        }
    }

    close(sd);
    return 0;
}

void salirCliente(int socket, fd_set *readfds, int *numClientes, int arrayClientes[])
{

    char buffer[MSG_SIZE];
    int j;

    close(socket);
    FD_CLR(socket, readfds);

    // Re-estructurar el array de clientes
    for (j = 0; j < (*numClientes) - 1; j++)
        if (arrayClientes[j] == socket)
            break;
    for (; j < (*numClientes) - 1; j++)
        (arrayClientes[j] = arrayClientes[j + 1]);

    (*numClientes)--;

    printf("Desconexión del cliente <%d>\n", socket);
}

void manejador(int signum)
{
    printf("\nSe ha recibido la señal sigint\n");
    signal(SIGINT, manejador);

    // Implementar lo que se desee realizar cuando ocurra la excepción de ctrl+c en el servidor
}
