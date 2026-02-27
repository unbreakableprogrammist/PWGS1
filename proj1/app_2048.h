#pragma once // robi cos takiego ze raz includuje tylko biblioteki, np windows.h zaladuje raz nawet jestli uzyjemy app_2048 w paru miejscach 
#include <windows.h>
#include<string>
using namespace std;

class app_2048 {
private:
	bool register_class(); // deklarujemy funckje 
	// ponizej deklarujemy zmienna s_class_name
	// static - jest to zmienna statyczna, czyli taka ktora jest wspolna dla wszystkich obiektow tej klasy, a nie dla kazdego obiektu osobno
	// wstring - jest to typ danych reprezentujacy lancuch znakow szerokich (wide string), czyli taki ktory moze przechowywac znaki z wiekszej ilosci jezykow, np polskie znaki diakrytyczne
	//const - oznacza ze ta zmienna jest stala, czyli jej wartosc nie moze byc zmieniona po jej zainicjalizowaniu
	static wstring const s_class_name;

	static LRESULT CALLBACK window_proc_static(

	);
};

