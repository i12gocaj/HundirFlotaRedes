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

#include "game.h"

/*
 * El servidor ofrece el servicio de un chat
 */

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

    srand(time(NULL));

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
                            // COMPRUEBO SI EL MENSAJE DEL CLIENTE ES SALIR
                            if (strcmp(buffer, "SALIR\n") == 0)
                            {

                                if (jugadores[i].estado == JUGANDO)
                                {

                                    int j = jugadores[i].enemigo;

                                    contadorPartidas = contadorPartidas - 1;

                                    contador--;

                                    terminarPartida(jugadores, j);

                                    bzero(buffer, sizeof(buffer));
                                    sprintf(buffer, "+Ok. Tu oponente a terminado la partida\n");
                                    send(j, buffer, sizeof(buffer), 0);
                                }

                                liberarJugador(jugadores, i);
                                salirCliente(i, &readfds, &numClientes, arrayClientes);
                            }
                            // else if (strncmp(buffer, "INICIAR-PARTIDA", 15) == 0)
                            else if (strncmp(buffer, "a", 1) == 0)
                            {

                                if (jugadores[i].estado != LOGUEADO)
                                {
                                    bzero(buffer, sizeof(buffer));
                                    sprintf(buffer, "-Err. No puedes introducir ahora 'INICIAR-PARTIDA'\n");
                                    send(i, buffer, sizeof(buffer), 0);
                                    break;
                                }

                                printf("El jugador %d quiere jugar\n", i);

                                jugadores[i].estado = BUSCANDO;

                                if (buscarJugadoresBuscando(jugadores))
                                {

                                    contador++;

                                    if (contador > MAX_PARTIDAS)
                                    {

                                        bzero(buffer, sizeof(buffer));
                                        sprintf(buffer, "Número máximo de partidas alcanzado. Debes esperar a que una partida finalice\n");
                                        send(i, buffer, sizeof(buffer), 0);
                                        break;
                                    }

                                    int j = BuscarJugador(jugadores, i);

                                    jugadores[i].enemigo = j;

                                    jugadores[j].enemigo = i;

                                    jugadores[i].estado = JUGANDO;
                                    jugadores[i].turno = false;

                                    jugadores[j].estado = JUGANDO;
                                    jugadores[j].turno = true;

                                    bzero(buffer, sizeof(buffer));
                                    sprintf(buffer, "+Ok. Empieza la partida, ID de la partida: %d\n", contador);

                                    send(i, buffer, sizeof(buffer), 0);
                                    send(j, buffer, sizeof(buffer), 0);

                                    printf("Jugador %s con socket %d emparejado contra el jugador %s con socket %d en la partida %d\n", jugadores[i].user, i, jugadores[j].user, j, contador);

                                    bzero(buffer, sizeof(buffer));
                                    sprintf(buffer, "¡Juegas contra el jugador %.50s!\n", jugadores[i].user);
                                    send(j, buffer, sizeof(buffer), 0);

                                    bzero(buffer, sizeof(buffer));
                                    sprintf(buffer, "¡Juegas contra el jugador %.50s!\n", jugadores[j].user);
                                    send(i, buffer, sizeof(buffer), 0);

                                    generarTableroAleatorio(jugadores[i].tablero);
                                    generarTableroAleatorio(jugadores[j].tablero);

                                    enviarTableroAlJugador(i, jugadores[i].tablero);
                                    enviarTableroAlJugador(j, jugadores[j].tablero);

                                    generarMatrizX(jugadores[i].tableroX);
                                    generarMatrizX(jugadores[j].tableroX);

                                    enviarTableroXAlJugador(i, jugadores[i].tableroX);
                                    enviarTableroXAlJugador(j, jugadores[j].tableroX);

                                    bzero(buffer, sizeof(buffer));
                                    sprintf(buffer, "Turno de %.50s\n", jugadores[j].user);
                                    send(i, buffer, sizeof(buffer), 0);

                                    bzero(buffer, sizeof(buffer));
                                    sprintf(buffer, "Tu turno\n");
                                    send(j, buffer, sizeof(buffer), 0);
                                }
                                else
                                {

                                    bzero(buffer, sizeof(buffer));
                                    sprintf(buffer, "+Ok. Esperando jugadores\n");
                                    send(i, buffer, sizeof(buffer), 0);
                                }
                            }
                            else if (strncmp(buffer, "USUARIO", 7) == 0)
                            {

                                if (sscanf(buffer, "USUARIO %s", user) == 1)
                                {

                                    if (jugadores[i].estado != CONECTADO || comprobarNoUsuario(jugadores, user) == false)
                                    {

                                        bzero(buffer, sizeof(buffer));
                                        sprintf(buffer, "-Err. No puedes introducir ahora 'USUARIO' o ya ha sido introducido\n");
                                        send(i, buffer, sizeof(buffer), 0);
                                        break;
                                    }

                                    printf("El usuario con socket %d ha introducido la orden USUARIO: %s\n", i, user);

                                    if (usuarioExiste(user))
                                    {

                                        bzero(buffer, sizeof(buffer));
                                        sprintf(buffer, "+Ok. Usuario correcto\n");
                                        send(i, buffer, sizeof(buffer), 0);

                                        strcpy(jugadores[i].user, user);

                                        jugadores[i].estado = USUARIO;
                                    }
                                    else
                                    {

                                        bzero(buffer, sizeof(buffer));
                                        sprintf(buffer, "-Err. Usuario incorrecto\n");
                                        send(i, buffer, sizeof(buffer), 0);
                                    }
                                }
                                else
                                {

                                    bzero(buffer, sizeof(buffer));
                                    sprintf(buffer, "-Err. Debes introducir un usuario después de 'USUARIO'\n");
                                    send(i, buffer, sizeof(buffer), 0);
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

                                            bzero(buffer, sizeof(buffer));
                                            sprintf(buffer, "-Err. No puedes introducir ahora 'PASSWORD'\n");
                                            send(i, buffer, sizeof(buffer), 0);
                                            break;
                                        }

                                        printf("El usuario con socket %d ha introducido la orden PASSWORD: %s\n", i, password);

                                        if (verificarUsuarioYPasswordEnArchivo(jugadores[i].user, password))
                                        {

                                            bzero(buffer, sizeof(buffer));
                                            sprintf(buffer, "+Ok. Usuario validado\n");
                                            send(i, buffer, sizeof(buffer), 0);

                                            jugadores[i].estado = LOGUEADO;
                                        }
                                        else
                                        {

                                            bzero(buffer, sizeof(buffer));
                                            sprintf(buffer, "-Err. Error en la validación\n");
                                            send(i, buffer, sizeof(buffer), 0);
                                        }
                                    }
                                    else
                                    {

                                        bzero(buffer, sizeof(buffer));
                                        sprintf(buffer, "-Err. Debes introducir una contraseña después de 'PASSWORD'\n");
                                        send(i, buffer, sizeof(buffer), 0);
                                    }
                                }
                                else
                                {

                                    bzero(buffer, sizeof(buffer));
                                    sprintf(buffer, "-Err. Debes introducir un nombre de usuario primero\n");
                                    send(i, buffer, sizeof(buffer), 0);
                                }
                            }
                            else if (strncmp(buffer, "REGISTRO", 8) == 0)
                            {

                                if (sscanf(buffer, "REGISTRO -u %s -p %s", user, password) == 2)
                                {

                                    if (jugadores[i].estado != CONECTADO)
                                    {

                                        bzero(buffer, sizeof(buffer));
                                        sprintf(buffer, "-Err. No puedes introducir ahora 'REGISTRO'\n");
                                        send(i, buffer, sizeof(buffer), 0);
                                        break;
                                    }

                                    if (registrarUsuario(user, password))
                                    {
                                        bzero(buffer, sizeof(buffer));
                                        sprintf(buffer, "-Err. Usuario ya registrado\n");
                                        send(i, buffer, sizeof(buffer), 0);
                                    }
                                    else
                                    {

                                        bzero(buffer, sizeof(buffer));
                                        sprintf(buffer, "+Ok. Usuario registrado con exito\n");
                                        send(i, buffer, sizeof(buffer), 0);
                                    }
                                }
                                else
                                {

                                    bzero(buffer, sizeof(buffer));
                                    sprintf(buffer, "-Err. Formato incorrecto para el comando REGISTRO (REGISTRO -u 'usuario' -p 'contraseña')\n");
                                    send(i, buffer, sizeof(buffer), 0);
                                }
                            }
                            else if (strncmp(buffer, "DISPARO", 7) == 0)
                            {

                                int j = jugadores[i].enemigo;

                                if (sscanf(buffer, "DISPARO %c,%d", &letra, &columna) == 2)
                                {

                                    if (jugadores[i].estado != JUGANDO)
                                    {
                                        bzero(buffer, sizeof(buffer));
                                        sprintf(buffer, "-Err. No puedes introducir ahora 'DISPARO'\n");
                                        send(i, buffer, sizeof(buffer), 0);
                                        break;
                                    }

                                    if (letra >= 'A' && letra <= 'J')
                                    {

                                        fila = letra - 'A';
                                    }
                                    else
                                    {
                                        bzero(buffer, sizeof(buffer));
                                        sprintf(buffer, "La primera coordenada no está en el rango (A-J)\n");
                                        send(i, buffer, sizeof(buffer), 0);
                                        break;
                                    }

                                    if (columna >= 0 && columna < 10)
                                    {

                                        if (jugadores[i].turno)
                                        {
                                            jugadores[i].contadorDisparos++;

                                            char resultado = jugadores[j].tablero[fila][columna];

                                            if (resultado == AGUA)
                                            {

                                                jugadores[i].tableroX[fila][columna] = 'A';
                                                enviarTableroXAlJugador(i, jugadores[i].tableroX);

                                                bzero(buffer, sizeof(buffer));
                                                sprintf(buffer, "+Ok. AGUA: %c,%d\n", letra, columna);
                                                send(i, buffer, sizeof(buffer), 0);

                                                bzero(buffer, sizeof(buffer));
                                                sprintf(buffer, "+Ok. Disparo en: %c,%d\n", letra, columna);
                                                send(j, buffer, sizeof(buffer), 0);

                                                jugadores[i].turno = false;
                                                jugadores[j].turno = true;

                                                bzero(buffer, sizeof(buffer));
                                                sprintf(buffer, "Turno de %.50s\n", jugadores[j].user);
                                                send(i, buffer, sizeof(buffer), 0);

                                                bzero(buffer, sizeof(buffer));
                                                sprintf(buffer, "Tu turno\n");
                                                send(j, buffer, sizeof(buffer), 0);
                                            }
                                            else if (resultado == BARCO)
                                            {

                                                jugadores[i].tableroX[fila][columna] = 'B';
                                                enviarTableroXAlJugador(i, jugadores[i].tableroX);
                                                jugadores[j].tablero[fila][columna] = 'X';
                                                enviarTableroAlJugador(j, jugadores[j].tablero);

                                                if (BarcoHundido(jugadores[j].tablero, fila, columna))
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

                                                        contadorPartidas = contadorPartidas - 1;

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
                                        else
                                        {

                                            bzero(buffer, sizeof(buffer));
                                            sprintf(buffer, "-Err. No es tu turno\n");
                                            send(i, buffer, sizeof(buffer), 0);
                                        }
                                    }
                                    else
                                    {
                                        bzero(buffer, sizeof(buffer));
                                        sprintf(buffer, "La segunda coordenada no es valida (0-9)\n");
                                        send(i, buffer, sizeof(buffer), 0);
                                    }
                                }
                                else
                                {

                                    bzero(buffer, sizeof(buffer));
                                    sprintf(buffer, "-Err. Debes introducir una coordenada después de 'DISPARO' (DISPARO B,3)\n");
                                    send(i, buffer, sizeof(buffer), 0);
                                }
                            }
                            else
                            {
                                bzero(buffer, sizeof(buffer));
                                sprintf(buffer, "-Err. Comando desconocido\n");
                                send(i, buffer, sizeof(buffer), 0);
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
