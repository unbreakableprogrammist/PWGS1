# Programowanie w Środowisku Graficznym

## Zadanie Laboratoryjne - WinAPI 3 - Gradient Editor (Core)

### 1. Wstęp
W tym zadaniu zaimplementujesz **Edytor Gradientów** przy użyciu czystego WinAPI i GDI. Celem jest stworzenie aplikacji pozwalającej użytkownikom definiować liniowy gradient na kanwie z wieloma punktami kontrolnymi kolorów (color stops).
W przypadkach wątpliwych, gdy specyfikacja nie jest jasna lub brakuje szczegółów, należy naśladować działanie dostarczonej aplikacji przykładowej, z wyłączeniem jej potencjalnych błędów.

---

### 2. Wymagania

#### Kanwa i Układ (Canvas & Layout):
- Ustaw domyślny rozmiar obszaru roboczego (client area) na **600 x 400** pikseli.
- Okno musi posiadać **własną ikonę** zamiast domyślnej ikony Win32.
- Okno musi być resizowalne, z **minimalnym rozmiarem 400 x 300** pikseli.
- Kanwa gradientu musi wypełniać dostępną przestrzeń, z zachowaniem responsywnego układu i 5-pikselowego marginesu wokół głównych komponentów.

#### Zarządzanie Gradientem:
- Uruchom aplikację z podstawowym gradientem liniowym składającym się z **dwóch kolorów** (np. od czarnego do białego).
- Zaimplementuj **Pasek Gradientu** (Gradient Strip - niestandardowe okno GDI) pod główną kanwą, gdzie użytkownicy mogą zarządzać punktami kontrolnymi.
- **Interakcja z Paskiem Gradientu**:
    - **Efekt Hover**: Podświetl uchwyt (np. grubsza obramówka lub inny kolor), gdy mysz znajduje się nad nim.
    - **Dodaj Punkt**: Kliknij dwukrotnie na pasku, aby dodać nowy punkt kontrolny w danej pozycji.
    - **Usuń Punkt**: Kliknij prawym przyciskiem myszy na istniejącym punkcie, aby go usunąć (muszą pozostać co najmniej 2 punkty).
    - **Przesuń Punkt**: Kliknij lewym przyciskiem myszy i przeciągnij punkt, aby zmienić jego relatywną pozycję (0.0 do 1.0).
    - **Zmień Kolor**: Kliknij lewym przyciskiem myszy na punkcie, aby otworzyć standardowe okno dialogowe WinAPI `ChooseColor`.
- **Reset Gradientu**: Zaimplementuj opcję w menu oraz skrót (**Ctrl+R**), aby przywrócić domyślny gradient czarno-biały.

#### Grafika i Renderowanie:
- Główna kanwa musi wyświetlać gładki gradient liniowy oparty na zdefiniowanych punktach.
- Pasek Gradientu musi wyświetlać pomniejszoną wersję gradientu z marginesem pionowym, aby uchwyty były wyraźnie widoczne.
- **Integracja z Systemem**: Tło Paska Gradientu (poza samym paskiem koloru) musi być czyszczone przy użyciu standardowego koloru systemowego przycisku (`COLOR_BTNFACE`).
- Zapewnij renderowanie bez migotania przy użyciu **Podwójnego Buforowania** (obsługa `WM_ERASEBKGND` i użycie kontekstu pamięci memDC).

---

### 3. Punktacja (8 pkt)

#### Zarządzanie Oknem i Zasobami (3 pkt)
- **Rozmiary**: Poprawna inicjalizacja i obsługa minimalnego rozmiaru śledzenia poprzez `WM_GETMINMAXINFO`.
- **Zasoby**: Poprawna implementacja akceleratorów menu (**Ctrl+R**) oraz właściwe zwalnianie obiektów GDI (Pióra, Pędzle).
- **Komunikacja**: Niezawodna komunikacja oparta na komunikatach między niestandardowymi oknami potomnymi a oknem nadrzędnym.

#### Responsywny Interfejs i Układ (2 pkt)
- **Układ**: Dokładne obliczanie pozycji okien potomnych podczas `WM_SIZE` ze stałym marginesem (padding).
- **Zarządzanie Stanem**: Obsługa `WM_ERASEBKGND` i czyszczenie tła, aby zapobiec artefaktom graficznym podczas zmiany rozmiaru.

#### Niestandardowe Kontrolki GDI (3 pkt)
- **Interaktywna Grafika**: Implementacja stanów hover oraz wizualizacja uchwytów przy użyciu różnorodnych prymitywów GDI.
- **Integracja Systemowa**: Poprawne użycie kolorów systemowych (`COLOR_BTNFACE`) i integracja ze wspólnym oknem dialogowym `ChooseColor`.
- **Podwójne Buforowanie**: Bezbłędne renderowanie bez migotania przy użyciu kontekstów pamięci i funkcji `BitBlt`.

## Zadanie Domowe - WinAPI 3 - Gradient Editor

### 1. Wstęp
To zadanie jest kontynuacją zadania laboratoryjnego - musisz najpierw ukończyć część laboratoryjną, aby kontynuować.
Rozbudujesz edytor o zaawansowane tryby gradientu, interaktywne punkty kontrolne na kanwie (płótnie), eksport plików oraz profesjonalny Color Picker inspirowany programem GIMP.
W przypadkach wątpliwych, gdy specyfikacja nie jest jasna lub brakuje szczegółów, należy naśladować działanie dostarczonej aplikacji przykładowej, z wyłączeniem jej potencjalnych błędów.

---

### 2. Wymagania

#### Tryby Gradientu i Interakcja na Kanwie (płótnie):
- **Tryby**: Obsługa gradientu **Liniowego** i **Radialnego** wybieranego z menu.
- **Punkty Kontrolne**: Dodaj dwa interaktywne uchwyty (Start i End) bezpośrednio na kanwie.
- **Interakcja**: Użytkownik musi mieć możliwość chwytania i przesuwania tych punktów myszą, co dynamicznie zmienia kierunek i zasięg gradientu.
- **Widoczność**: Uchwyty muszą być wyraźnie widoczne na każdym tle (zastosuj np. podwójne obramowanie czarno-białe) i posiadać efekt hover (zmiana koloru/grubości po najechaniu).
- **Niezależność od Proporcji**: Matematyka gradientu musi być odporna na zmiany proporcji okna (np. tryb radialny musi zachować kształt koła, a nie elipsy).
- **Optymalizacja**: Zastosowanie podwójnego buforowania na wszystkich kontrolkach w celu całkowitej eliminacji migotania (flickering).

#### Menu główne: Import, Eksport i Skróty:
- **Eksport Obrazu**: Zapis aktualnego widoku kanwy do pliku BMP (**Ctrl+E**).
- **Zapis/Odczyt Konfiguracji**: Możliwość zapisu i odczytu listy punktów kolorów (stops) do/z pliku CSV (**Ctrl+S** / **Ctrl+O**). Format: `pozycja,#RRGGBB`. Zobacz dostarczony plik csv.
- **Skróty**: Pełna obsługa standardowych akceleratorów (skrótów) dla wszystkich operacji na plikach i trybach.

#### GIMP-style Color Picker:
- **Układ**: Lewa strona – interaktywny pierścień Hue i trójkąt S/V. Prawa strona – suwaki i pola edycyjne RGB/HSV.
- **Trójkąt Wyboru**: Równoboczny trójkąt, który rotuje wraz ze zmianą odcienia (Hue). Wierzchołek "czystego koloru" zawsze wskazuje na aktualny Hue na pierścieniu.
- **Porównanie Kolorów**: Wyświetlanie obok siebie barwy "Current" (aktualnie wybieranej) i "Old" (początkowej).
- **Reset**: Przycisk pozwalający wrócić do koloru początkowego.
- **Pipeta do pobierania koloru (Screen Picker)**: Globalne przechwytywanie koloru z dowolnego miejsca na ekranie (również poza oknem aplikacji) przy użyciu systemowych hooków.

---

### 3. Punktacja (12 pkt)

#### Zaawansowana Interakcja i Matematyka GDI (4 pkt)
- **Punkty Kontrolne (1 pkt)**: Stabilne i płynne przesuwanie punktów Start/End na kanwie z poprawnym odświeżaniem.
- **Logika 2D (1 pkt)**: Poprawne matematycznie wyliczanie gradientu liniowego (projekcja) i radialnego (dystans) w oparciu o absolutne współrzędne pikseli.
- **Interaktywny Pasek Gradientu (2 pkt)**: Możliwość przesuwania (drag) istniejących punktów koloru na dolnym pasku kontrolnym z natychmiastowym odświeżaniem podglądu.

#### Profesjonalny Color Picker (5 pkt)
- **Własny Color Picker (1 pkt)**: Implementacja własnego okna dialogowego i poprawne zwracanie wybranego koloru.
- **Rotacja i Mapowanie (3 pkt)**: Poprawna rotacja trójkąta S/V oraz precyzyjne mapowanie współrzędnych barycentrycznych na wartości kolorów.
- **Globalna Pipeta (1 pkt)**: Niezawodne przechwytywanie kolorów z całego systemu za pomocą niskopoziomowych hooków myszy (**WH_MOUSE_LL**).

#### System Plików i Eksport (3 pkt)
- **Eksport BMP (1 pkt)**: Poprawna generacja nagłówków bitmapy i zapis danych pikseli z kanwy do pliku.
- **Parser CSV (2 pkt)**: Bezpieczne wczytywanie i formatowanie danych tekstowych gradientu z obsługą błędów.

---

### 4. Wskazówki Techniczne

#### Wydajność:
- Zrezygnuj z `SetPixel` na rzecz tablic `std::vector<DWORD>` i funkcji `StretchDIBits` lub `SetDIBitsToDevice`.
- Zastosuj tablicę przeglądową (LUT) o rozmiarze np. 1024, aby uniknąć kosztownej interpolacji kolorów dla każdego piksela z osobna.

#### Interakcja Systemowa:
- Do globalnej obsługi pipety użyj `SetWindowsHookEx` z typem `WH_MOUSE_LL`. Pamiętaj o zwolnieniu hooka!
- Do zapisu obrazu użyj struktur `BITMAPFILEHEADER` i `BITMAPINFOHEADER`. Pamiętaj, że w formacie BMP wiersze są wyrównane do 4 bajtów (padding).
