#ifndef FUNCIONES_H
#define FUNCIONES_H

#define NO_CONECTADO -1
#define CONECTADO 1
#define LOGUEADO 2
#define BUSCANDO 3
#define JUGANDO 4

#define MAX_CLIENTS 30
#define MSG_SIZE 250

struct jugador
{
    int socket;
    int estado;
    char * user;
};

void inicializar_vector_jugadores(struct jugador *jugadores)
{
    for (int i = 1; i < MAX_CLIENTS; i++)
    {
        jugadores[i].estado = NO_CONECTADO;
    }
}

void guardarNuevoJugador(struct jugador *jugadores, int socket)
{
    for (int i = 1; i < MAX_CLIENTS; i++)
    {
        if (jugadores[i].estado == NO_CONECTADO)
        {
            jugadores[i].socket = i + 1;
            jugadores[i].estado = CONECTADO;
        }
    }
}

int buscarSocket(struct jugador *jugadores, int socket)
{
    for (int i = 1; i < MAX_CLIENTS; i++)
    {
        if (jugadores[i].socket == socket && jugadores[i].estado != NO_CONECTADO)
        {
            return i;
        }
    }
    return -1;
}

int usuarioExiste(const char *user)
{

    FILE *archivo;
    char buffer[MSG_SIZE];

    archivo = fopen("users.txt", "r");

    if (archivo == NULL)
    {
        printf("Error al abrir el archivo.\n");
        exit(1);
    }

    while (fgets(buffer, sizeof(buffer), archivo) != NULL)
    {

        char *puntoYComa = strchr(buffer, ';');

        if (puntoYComa != NULL)
        {

            *puntoYComa = '\0';

            if (strcmp(buffer, user) == 0)
            {
                fclose(archivo);
                return 1;
            }
        }
    }

    fclose(archivo);
    return 0;
}

int registrarUsuario(const char *user, const char *password)
{

    if (usuarioExiste(user))
    {
        return 1;
    }

    FILE *archivo;

    archivo = fopen("users.txt", "a");

    if (archivo == NULL)
    {
        printf("Error al abrir el archivo.\n");
        exit(1);
    }

    fprintf(archivo, "%s;%s\n", user, password);

    fclose(archivo);
    return 0;
}

int verificarUsuarioYPasswordEnArchivo(const char *user, const char *password)
{

    FILE *archivo;
    char buffer[MSG_SIZE];

    archivo = fopen("users.txt", "r");

    if (archivo == NULL)
    {
        printf("Error al abrir el archivo.\n");
        exit(1);
    }

    while (fgets(buffer, sizeof(buffer), archivo) != NULL)
    {

        char *puntoYComa = strchr(buffer, ';');

        *puntoYComa = '\0';
        char *passwordA = puntoYComa + 1;

        char *saltoLinea = strchr(passwordA, '\n');

        if (saltoLinea != NULL)
        {
            *saltoLinea = '\0';
        }

        if (strcmp(user, buffer) == 0 && strcmp(password, passwordA) == 0)
        {
            fclose(archivo);
            return 1;
        }
    }

    fclose(archivo);
    return 0;
}

void desconectarJugador(struct jugador *jugadores, int socket)
{
    int pos = buscarSocket(jugadores, socket);
    if (pos != -1)
    {
        jugadores[pos].estado = LOGUEADO;
    }
}


#endif