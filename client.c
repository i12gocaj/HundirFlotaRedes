#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <sys/select.h>
#include "game.h"

int main()
{

	/*----------------------------------------------------
		Descriptor del socket y buffer de datos
	-----------------------------------------------------*/
	int sd;
	struct sockaddr_in sockname;
	char buffer[MSG_SIZE];
	socklen_t len_sockname;
	fd_set readfds, auxfds;
	int salida;
	int fin = 0;
	int fila, columna;
	char letra;
	int columna_disparo;

	char tablero[FILAS][COLUMNAS];
	char tableroX[FILAS][COLUMNAS];

	

	/* --------------------------------------------------
		Se abre el socket
	---------------------------------------------------*/
	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1)
	{
		perror("No se puede abrir el socket cliente\n");
		exit(1);
	}

	/* ------------------------------------------------------------------
		Se rellenan los campos de la estructura con la IP del
		servidor y el puerto del servicio que solicitamos
	-------------------------------------------------------------------*/
	sockname.sin_family = AF_INET;
	sockname.sin_port = htons(2065);
	sockname.sin_addr.s_addr = inet_addr("127.0.0.1");

	/* ------------------------------------------------------------------
		Se solicita la conexión con el servidor
	-------------------------------------------------------------------*/
	len_sockname = sizeof(sockname);

	if (connect(sd, (struct sockaddr *)&sockname, len_sockname) == -1)
	{
		perror("Error de conexión");
		exit(1);
	}

	// Inicializamos las estructuras
	FD_ZERO(&auxfds);
	FD_ZERO(&readfds);

	FD_SET(0, &readfds);
	FD_SET(sd, &readfds);

	/* ------------------------------------------------------------------
		Se transmite la información
	-------------------------------------------------------------------*/
	do
	{
		auxfds = readfds;
		salida = select(sd + 1, &auxfds, NULL, NULL, NULL);

		// Tengo mensaje desde el servidor
		if (FD_ISSET(sd, &auxfds))
		{

			bzero(buffer, sizeof(buffer));
			recv(sd, buffer, sizeof(buffer), 0);

			if (strstr(buffer, "+Ok. Empieza la partida.") != NULL)
			{
				printf("\n+Ok. Empieza la partida\n\n");

				generarMatrizX(tableroX);

				cadenaAMatriz(buffer, tablero);

				imprimirTableroEnCliente(tablero);
				imprimirTableroOponenteEnCliente(tableroX);
			}
			else
			{

				printf("\n%s\n", buffer);
			}

			if (strstr(buffer, "+Ok. AGUA:") != NULL)
			{

				sscanf(buffer, "+Ok. AGUA: %c,%d", &letra, &columna);

				fila = letra - 'A';

				tableroX[columna][fila] = 'A';

				imprimirTableroOponenteEnCliente(tableroX);
			}

			if (strstr(buffer, "+Ok. TOCADO:") != NULL)
			{

				sscanf(buffer, "+Ok. TOCADO: %c,%d", &letra, &columna);

				fila = letra - 'A';

				tableroX[columna][fila] = 'B';

				imprimirTableroOponenteEnCliente(tableroX);
			}

			if (strstr(buffer, "+Ok. HUNDIDO:") != NULL)
			{

				sscanf(buffer, "+Ok. HUNDIDO: %c,%d", &letra, &columna);
				fila = letra - 'A';

				tableroX[columna][fila] = 'B';

				imprimirTableroOponenteEnCliente(tableroX);
			}

			if (strstr(buffer, "+Ok. Disparo en:") != NULL)
			{

				sscanf(buffer, "+Ok. Disparo en: %c,%d", &letra, &columna);
				fila = letra - 'A';

				if (tablero[columna][fila] == 'A')
				{

					tablero[columna][fila] = '.';
					imprimirTableroEnCliente(tablero);
				}
				else
				{

					tablero[columna][fila] = 'X';

					imprimirTableroEnCliente(tablero);
				}
			}

			if (strcmp(buffer, "Demasiados clientes conectados\n") == 0 || strcmp(buffer, "Desconexión servidor\n") == 0)
			{

				fin = 1;
			}
		}
		else
		{

			// He introducido información por teclado
			if (FD_ISSET(0, &auxfds))
			{
				bzero(buffer, sizeof(buffer));
				fgets(buffer, sizeof(buffer), stdin);

				if (strcmp(buffer, "SALIR\n") == 0)
				{
					fin = 1;
				}

				send(sd, buffer, sizeof(buffer), 0);
			}
		}
	} while (fin == 0);

	close(sd);

	return 0;
}
