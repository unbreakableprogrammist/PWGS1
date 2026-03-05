#include "app_2048.h"
#include<stdexcept> // biblioteka do obslugi wyjatkow, czyli sytuacji gdy cos idzie nie tak, np nie udaje sie zarejestrowac klasy okna, nie udaje sie stworzyc okna, itp

// pole class_name jest statyczne, wiec musi byc zainicjalizowane poza definicja klasy, czyli tutaj w pliku app_2048.cpp
// jeszcze zanim stworzymy obiekt tej klasy 
std::wstring const app_2048::s_class_name{ L"2048 Window" };

bool app_2048::register_class() { // implementujemy klase 

}