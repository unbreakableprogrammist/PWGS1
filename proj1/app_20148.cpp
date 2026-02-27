#include "app_2048.h"
#include<stdexcept> // biblioteka do obslugi wyjatkow, czyli sytuacji gdy cos idzie nie tak, np nie udaje sie zarejestrowac klasy okna, nie udaje sie stworzyc okna, itp

wstring const app_2048::s_class_name{ L"2048 Window" };