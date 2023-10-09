inicio: clean CompilarServer CompilarClient end

clean: server client
	@echo "Limpiando..."
	rm server client

CompilarServer: server.c
	@echo "Compilando server..."
	gcc server.c -o server

CompilarClient: client.c
	@echo "Compilando client..."
	gcc client.c -o client

end:
	@echo "¡Compilado con exito!"

