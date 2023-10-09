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

#include "game.h"

#define MSG_SIZE 250
#define MAX_CLIENTS 30

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

                                // for (j = 0; j < (numClientes - 1); j++)
                                // {

                                //     bzero(buffer, sizeof(buffer));
                                //     sprintf(buffer, "Nuevo Cliente conectado en <%d>\n", new_sd);
                                //     send(arrayClientes[j], buffer, sizeof(buffer), 0);
                                // }

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

                        // Controlar si se ha introducido "SALIR", cerrando todos los sockets y finalmente saliendo del servidor. (implementar)
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
                                salirCliente(i, &readfds, &numClientes, arrayClientes);
                            }
                            else if (strncmp(buffer, "INICIAR-PARTIDA", 15) == 0)
                            {
                                printf("El jugador %d quiere jugar\n", i);

                                int pos = buscarSocket(jugadores, i);

                                if (jugadores[pos].estado != LOGUEADO)
                                {
                                    bzero(buffer, sizeof(buffer));
                                    sprintf(buffer, "-Err: Debes estar LOGUEADO para poder INICIAR-PARTIDA\n");
                                    send(i, buffer, sizeof(buffer), 0);
                                }
                                else
                                {

                                    jugadores[pos].estado = BUSCANDO;
                                    bzero(buffer, sizeof(buffer));
                                    sprintf(buffer, "+OK: Esperando jugadores\n");
                                    send(i, buffer, sizeof(buffer), 0);

                                    int jugadoresBuscando = 0;
                                    for (int j = 0; j < MAX_CLIENTS; j++)
                                    {
                                        if (jugadores[j].estado == BUSCANDO && j != pos)
                                        {
                                            jugadoresBuscando++;
                                            if (jugadoresBuscando == 1)
                                            {

                                                jugadores[j].estado = JUGANDO;
                                                jugadores[pos].estado = JUGANDO;

                                                bzero(buffer, sizeof(buffer));
                                                sprintf(buffer, "+OK: Empieza la partida\n");

                                                send(jugadores[j].id, buffer, sizeof(buffer), 0);
                                                send(jugadores[pos].id, buffer, sizeof(buffer), 0);

                                                bzero(buffer, sizeof(buffer));
                                                sprintf(buffer, "¡Juegas contra el jugador %s!\n", jugadores[pos].user);
                                                send(jugadores[j].id, buffer, sizeof(buffer), 0);

                                                bzero(buffer, sizeof(buffer));
                                                sprintf(buffer, "¡Juegas contra el jugador %s!\n", jugadores[j].user);
                                                send(jugadores[pos].id, buffer, sizeof(buffer), 0);
                                            }
                                        }
                                    }
                                }
                            }
                            else if (strncmp(buffer, "USUARIO", 7) == 0)
                            {
                                char user[20];
                                sscanf(buffer, "USUARIO %s", user);
                                printf("El usuario con socket %d ha introducido la orden USUARIO: %s\n", i, user);

                                if (usuarioExiste(user))
                                {

                                    bzero(buffer, sizeof(buffer));
                                    sprintf(buffer, "+Ok. Usuario correcto\n");
                                    send(i, buffer, sizeof(buffer), 0);

                                    if (recv(i, buffer, sizeof(buffer), 0) > 0 && strncmp(buffer, "PASSWORD", 8) == 0) //eliminar recv, jugar con estados
                                    {

                                        char password[20];
                                        sscanf(buffer, "PASSWORD %s", password);
                                        printf("El usuario con socket %d ha introducido la orden PASSWORD: %s\n", i, password);

                                        if (verificarUsuarioYPasswordEnArchivo(user, password))
                                        {

                                            bzero(buffer, sizeof(buffer));
                                            sprintf(buffer, "+Ok. Usuario validado\n");
                                            send(i, buffer, sizeof(buffer), 0);

                                            int pos = buscarSocket(jugadores, i);
                                            jugadores[pos].estado = LOGUEADO;

                                            jugadores[pos].user = malloc(strlen(user) + 1);
                                            if (jugadores[pos].user != NULL)
                                            {
                                                strcpy(jugadores[pos].user, user);
                                            }
                                            else
                                            {
                                              
                                                printf("Error: No se pudo asignar memoria para el nombre de usuario\n");
                                                exit(EXIT_FAILURE); 
                                            }
                                        }
                                        else
                                        {

                                            bzero(buffer, sizeof(buffer));
                                            sprintf(buffer, "-Err. Error en la validación\n");
                                            send(i, buffer, sizeof(buffer), 0);
                                        }
                                    }
                                }
                                else
                                {

                                    bzero(buffer, sizeof(buffer));
                                    sprintf(buffer, "-Err. Usuario incorrecto\n");
                                    send(i, buffer, sizeof(buffer), 0);
                                }
                            }
                            else if (strncmp(buffer, "REGISTRO", 8) == 0)
                            {

                                char user[20];
                                char password[20];

                                if (sscanf(buffer, "REGISTRO -u %s -p %s", user, password) == 2)
                                {

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

    char buffer[250];
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

    bzero(buffer, sizeof(buffer));
    sprintf(buffer, "Desconexión del cliente <%d>", socket);

    for (j = 0; j < (*numClientes); j++)
        if (arrayClientes[j] != socket)
            send(arrayClientes[j], buffer, sizeof(buffer), 0);
}

void manejador(int signum)
{
    printf("\nSe ha recibido la señal sigint\n");
    signal(SIGINT, manejador);

    // Implementar lo que se desee realizar cuando ocurra la excepción de ctrl+c en el servidor
}
