#ifndef FUNCIONES_H
#define FUNCIONES_H

#define NO_CONECTADO -1
#define CONECTADO 1
#define USUARIO 2
#define LOGUEADO 3
#define BUSCANDO 4
#define JUGANDO 5

#define MAX_CLIENTS 30
#define MAX_PARTIDAS 10
#define MSG_SIZE 250

#define FILAS 10
#define COLUMNAS 10
#define AGUA 'A'
#define BARCO 'B'

struct jugador
{
    int socket;
    int estado;
    char user[MSG_SIZE];
    char tablero[FILAS][COLUMNAS];
    char tablero2[FILAS][COLUMNAS];
    bool turno;
    int idPartida;
};

int esPosicionValida(char tablero[FILAS][COLUMNAS], int fila, int columna, int tamano, int direccion)
{
    if (direccion == 0)
    {
        if (columna + tamano - 1 >= COLUMNAS)
        {
            return 0; // Fuera del rango
        }

        for (int i = fila - 1; i <= fila + 1; i++)
        {
            for (int j = columna - 1; j <= columna + tamano; j++)
            {
                if (i >= 0 && i < FILAS && j >= 0 && j < COLUMNAS && tablero[i][j] != AGUA)
                {
                    return 0;
                }
            }
        }
    }
    else
    {
        if (fila + tamano - 1 >= FILAS)
        {
            return 0; // Fuera del rango
        }

        for (int i = fila - 1; i <= fila + tamano; i++)
        {
            for (int j = columna - 1; j <= columna + 1; j++)
            {
                if (i >= 0 && i < FILAS && j >= 0 && j < COLUMNAS && tablero[i][j] != AGUA)
                {
                    return 0;
                }
            }
        }
    }
    return 1; // Posición válida
}

void colocarBarcoAleatorio(char tablero[FILAS][COLUMNAS], int tamano)
{
    int fila, columna, direccion;
    do
    {
        fila = rand() % FILAS;
        columna = rand() % COLUMNAS;
        direccion = rand() % 2; // 0 para horizontal, 1 para vertical
    } while (!esPosicionValida(tablero, fila, columna, tamano, direccion));

    // Colocamos el barco
    if (direccion == 0)
    {
        for (int i = 0; i < tamano; i++)
        {
            tablero[fila][columna + i] = BARCO;
        }
    }
    else
    {
        for (int i = 0; i < tamano; i++)
        {
            tablero[fila + i][columna] = BARCO;
        }
    }

    // Marcamos la franja de agua alrededor del barco
    for (int i = fila - 1; i <= fila + tamano; i++)
    {
        for (int j = columna - 1; j <= columna + 1; j++)
        {
            if (i >= 0 && i < FILAS && j >= 0 && j < COLUMNAS && tablero[i][j] != BARCO)
            {
                tablero[i][j] = AGUA;
            }
        }
    }
}

void inicializarTableroJugador(char tablero[FILAS][COLUMNAS])
{
    for (int i = 0; i < FILAS; i++)
    {
        for (int j = 0; j < COLUMNAS; j++)
        {
            tablero[i][j] = AGUA;
        }
    }
}

void generarTableroAleatorio(char tablero[FILAS][COLUMNAS])
{
    inicializarTableroJugador(tablero);

    colocarBarcoAleatorio(tablero, 4);
    colocarBarcoAleatorio(tablero, 3);
    colocarBarcoAleatorio(tablero, 3);
    colocarBarcoAleatorio(tablero, 2);
    colocarBarcoAleatorio(tablero, 2);
}

void enviarTableroAlJugador(int socket, char tablero[FILAS][COLUMNAS])
{
    char buffer[MSG_SIZE];
    sprintf(buffer, "Tu tablero:\n");
    for (int i = 0; i < FILAS; i++)
    {
        for (int j = 0; j < COLUMNAS; j++)
        {
            sprintf(buffer + strlen(buffer), "%c ", tablero[i][j]);
        }
        sprintf(buffer + strlen(buffer), ";\n");
    }
    send(socket, buffer, sizeof(buffer), 0);
}

void inicializar_vector_jugadores(struct jugador *jugadores)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        jugadores[i].estado = NO_CONECTADO;
    }
}

void guardarNuevoJugador(struct jugador *jugadores, int socket)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (jugadores[i].estado == NO_CONECTADO)
        {
            jugadores[i].socket = i + 1;
            jugadores[i].estado = CONECTADO;
            // jugadores[i].estado = LOGUEADO;
        }
    }
}

void liberarJugador(struct jugador *jugadores, int socket)
{

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (jugadores[i].socket == socket)
        {
            jugadores[i].estado = NO_CONECTADO;
            return;
        }
    }
}

int buscarSocket(struct jugador *jugadores, int socket)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
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

int obtenerIdUnico()
{
    static int contadorPartidas = 1;

    if (contadorPartidas > MAX_PARTIDAS)
    {
        printf("Número máximo de partidas alcanzado. Debes esperar a que una partida finalice.\n");
        return -1;
    }

    return contadorPartidas++;
}

bool comprobarNoUsuario(struct jugador *jugadores, const char *user)
{

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (jugadores[i].user != NULL && strcmp(jugadores[i].user, user) == 0)
        {
            // El usuario ya existe, retorna error
            return false;
        }
    }

    // El usuario no existe, es válido
    return true;
}

#endif