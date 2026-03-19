#include<Windows.h> // biblioteka do WINDows API
#include "app_2048.h" // nasza klasa app_2048, ktora bedzie odpowiedzialna za obsluge naszej gry 2048

int WINAPI wWinMain(HINSTANCE instance,
	HINSTANCE,
	LPWSTR, 
	int show_command) {
	app_2048 app{ instance }; // tworzymy obiekt klasy app_2048, przekazujemy mu uchwyt do instancji aplikacji, czyli identyfikator procesu, ktory jest przekazywany przez system operacyjny do funkcji wWinMain, gdy aplikacja jest uruchamiana
	return app.run(show_command); // wywolujemy funkcje run, ktora bedzie odpowiedzialna za uruchomienie glownej petli zdarzen gry, czyli odbieranie i obsluge zdarzen zwiazanych z oknem gry, itp
}