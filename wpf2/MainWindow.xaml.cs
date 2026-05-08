using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Shapes;

namespace WpfApp // Przestrzeń nazw - musi pasować do nazwy Twojego projektu.
{
    // GŁÓWNA KLASA OKNA
    // Dziedziczy po klasie Window, co oznacza, że zachowuje się jak standardowe okno aplikacji w systemie Windows.
    public partial class MainWindow : Window
    {
        // --- ZMIENNE POMOCNICZE (do obsługi przeciągania i zaznaczania) ---

        // Flaga informująca system, czy aktualnie przeciągamy jakiś kształt myszką.
        private bool isDragging = false;

        // Przechowuje referencję do kształtu, który w danej chwili przeciągamy (znak '?' oznacza, że może to być null, jeśli nic nie trzymamy).
        private Shape? draggedShape = null;

        // Punkt (współrzędne X,Y) zapamiętujący miejsce, w którym dokładnie kliknęliśmy WEWNĄTRZ kształtu.
        private Point clickPosition;

        // Przechowuje referencję do aktualnie klikniętego (zaznaczonego) kształtu, który ma na sobie pomarańczową ramkę.
        private Shape? selectedShape = null;

        // KONSTRUKTOR OKNA
        // Uruchamia się tylko raz, na samym początku przy starcie aplikacji.
        public MainWindow()
        {
            // Wbudowana metoda WPF, która ładuje i buduje interfejs graficzny z pliku XAML.
            InitializeComponent();
        }

        // --- RYSOWANIE PROSTYCH KSZTAŁTÓW ---

        // Metoda odpalana po kliknięciu przycisku "Dodaj Kwadrat" w menu na górze.
        private void AddShape_Click(object sender, RoutedEventArgs e)
        {
            // Tworzymy nowy obiekt prostokąta z biblioteki WPF (klasa Rectangle).
            Rectangle rect = new Rectangle
            {
                // Wypełnienie w kolorze stalowo-niebieskim.
                Fill = Brushes.SteelBlue,
                // Brak obramowania (przezroczyste) na start.
                Stroke = Brushes.Transparent,
                // Grubość obramowania ustawiona na 3 piksele (ramka będzie widoczna dopiero przy zaznaczeniu).
                StrokeThickness = 3,
                // Przypisujemy do kształtu menu kontekstowe, które wcześniej zbudowaliśmy w zasobach w XAML-u.
                ContextMenu = (ContextMenu)this.Resources["ShapeContextMenu"]
            };

            // --- DATA BINDING W KODZIE C# ---

            // Tworzymy nowe wiązanie danych (Binding), które będzie stale obserwować właściwość "Value" naszego suwaka (SizeSlider).
            Binding sizeBinding = new Binding("Value") { Source = SizeSlider };

            // Na żywo łączymy szerokość naszego kwadratu z suwakiem.
            rect.SetBinding(Rectangle.WidthProperty, sizeBinding);

            // Podpinamy również wysokość (dzięki temu obiekt to zawsze idealny kwadrat, a rozmiar reaguje na suwak).
            rect.SetBinding(Rectangle.HeightProperty, sizeBinding);

            // --- POZYCJONOWANIE I DODAWANIE DO KONTROLKI CANVAS ---

            // Ustawiamy początkową pozycję kwadratu na osi X (100 pikseli od lewej krawędzi okna).
            Canvas.SetLeft(rect, 100);

            // Ustawiamy początkową pozycję kwadratu na osi Y (100 pikseli od górnej krawędzi okna).
            Canvas.SetTop(rect, 100);

            // Dodajemy gotowy, skonfigurowany kwadrat do listy elementów wewnątrz naszego Canvasu, żeby fizycznie pojawił się na ekranie.
            MainCanvas.Children.Add(rect);
        }

        // --- OBSŁUGA ZDARZEŃ MYSZKI: WCIŚNIĘCIE LEWEGO PRZYCISKU (MouseLeftButtonDown) ---

        // Metoda wywoływana za każdym razem, gdy wciśniemy (jeszcze nie puścimy!) lewy przycisk myszy w obszarze Canvasu.
        private void Canvas_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            // Sprawdzamy, czy kliknięty element (e.OriginalSource) to kształt (Shape), czyli np. nasz dodany kwadrat.
            if (e.OriginalSource is Shape shape)
            {
                // Jeśli mieliśmy już wcześniej coś zaznaczone, czyścimy temu obramowanie.
                if (selectedShape != null) selectedShape.Stroke = Brushes.Transparent;

                // Zapisujemy nowo kliknięty kształt jako nasz aktualnie "wybrany" element.
                selectedShape = shape;

                // Rysujemy wokół niego pomarańczową ramkę, żeby użytkownik widział, co zaznaczył.
                selectedShape.Stroke = Brushes.Orange;

                // Aktywujemy tryb przeciągania myszką.
                isDragging = true;

                // Zapamiętujemy, jaki konkretnie kształt będziemy przesuwać po ekranie.
                draggedShape = shape;

                // Pobieramy pozycję kliknięcia względem SAMEGO KSZTAŁTU (dzięki temu obiekt przy ruchu nie "przeskoczy" środkiem wprost pod nasz kursor).
                clickPosition = e.GetPosition(shape);

                // Bardzo ważne: "przechwytujemy" myszkę. System uznaje, że od teraz ruch myszką dotyczy tego kształtu, nawet jak kursor przypadkiem z niego ucieknie.
                shape.CaptureMouse();
            }
        }

        // --- OBSŁUGA ZDARZEŃ MYSZKI: RUCH KURSORA (MouseMove) ---

        // Metoda wywoływana za każdym, nawet najmniejszym ruchem kursora po oknie.
        private void Canvas_MouseMove(object sender, MouseEventArgs e)
        {
            // Wykonujemy przesuwanie tylko, jeśli trzymamy wciśnięty klawisz (isDragging) i faktycznie "chwyciliśmy" jakiś kształt (draggedShape != null).
            if (isDragging && draggedShape != null)
            {
                // Zczytujemy obecną pozycję myszki na całym płótnie roboczym (Canvasie).
                Point mousePos = e.GetPosition(MainCanvas);

                // Obliczamy i ustawiamy nową pozycję X kształtu (pozycja kursora minus zapamiętane miejsce kliknięcia w obiekt).
                Canvas.SetLeft(draggedShape, mousePos.X - clickPosition.X);

                // Obliczamy i ustawiamy nową pozycję Y kształtu. Obiekt przesuwa się płynnie!
                Canvas.SetTop(draggedShape, mousePos.Y - clickPosition.Y);
            }
        }

        // --- OBSŁUGA ZDARZEŃ MYSZKI: PUSZCZENIE PRZYCISKU (MouseLeftButtonUp) ---

        // Metoda wywoływana, gdy zdejmiemy palec z lewego przycisku myszy (tzw. "upuszczenie" obiektu).
        private void Canvas_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            // Jeśli faktycznie kończymy manewr przeciągania...
            if (isDragging && draggedShape != null)
            {
                // Wyłączamy tryb przesuwania.
                isDragging = false;

                // Zwalniamy naszą myszkę, aby mogła znowu klikać w inne elementy w systemie bez powiązania z tym konkretnym kształtem.
                draggedShape.ReleaseMouseCapture();

                // Czyścimy informację o trzymanym kształcie.
                draggedShape = null;
            }
        }

        // --- OBSŁUGA ZDARZEŃ MYSZKI: KLIKNIĘCIE W TŁO (MouseDown) ---

        // Służy do "odznaczania" obiektu, gdy klikniemy obok niego. Złapaliśmy tu ogólne kliknięcie (MouseDown - działa na lewy, prawy i środkowy przycisk).
        private void Canvas_MouseDown(object sender, MouseButtonEventArgs e)
        {
            // Sprawdzamy, czy tym razem trafiliśmy bezpośrednio w samą pustą kartkę (Canvas) i czy mieliśmy coś dotychczas zaznaczone.
            if (e.OriginalSource is Canvas && selectedShape != null)
            {
                // Usuwamy z kwadratu pomarańczową ramkę zaznaczenia.
                selectedShape.Stroke = Brushes.Transparent;

                // Przestajemy o nim pamiętać jako o zaznaczonym elemencie.
                selectedShape = null;
            }
        }

        // --- OBSŁUGA ZDARZEŃ KLAWIATURY: WCIŚNIĘCIE KLAWISZA (KeyDown) ---

        // Metoda reaguje na naciskanie przycisków na klawiaturze (podpięliśmy ją pod całe Główne Okno w pliku XAML).
        private void Window_KeyDown(object sender, KeyEventArgs e)
        {
            // Sprawdzamy, czy użytkownik wcisnął guzik 'Delete' ORAZ czy mamy zaznaczony jakiś kształt (z pomarańczową ramką).
            if (e.Key == Key.Delete && selectedShape != null)
            {
                // Fizycznie kasujemy ten konkretny zaznaczony obiekt z naszej tablicy (znika z ekranu).
                MainCanvas.Children.Remove(selectedShape);

                // Czyścimy zmienną, bo usuniętego obiektu nie można mieć w zaznaczeniu.
                selectedShape = null;
            }
        }

        // --- BUDOWANIE MENU KONTEKSTOWEGO: ZMIANA KOLORU ---

        // Odpala się po kliknięciu prawym przyciskiem na kwadrat i wybraniu z menu "Zmień kolor na czerwony".
        private void ChangeColor_Click(object sender, RoutedEventArgs e)
        {
            // Bezpiecznie sprawdzamy łańcuch zdarzeń (tzw. pattern matching):
            // 1. sender is MenuItem -> czy to co wywołało metodę to na pewno element menu?
            // 2. menuItem.Parent is ContextMenu -> czy rodzicem tego elementu jest całe menu kontekstowe?
            // 3. menu.PlacementTarget is Shape -> czy to menu zostało otwarte NA jakimś kształcie?
            if (sender is MenuItem menuItem && menuItem.Parent is ContextMenu menu && menu.PlacementTarget is Shape shape)
            {
                // Jeśli wszystkie testy przeszły, malujemy docelowy kształt na krwistą czerwień (Crimson).
                shape.Fill = Brushes.Crimson;
            }
        }

        // --- BUDOWANIE MENU KONTEKSTOWEGO: USUNIĘCIE KSZTAŁTU ---

        // Odpala się po kliknięciu prawym na kwadrat i wybraniu z menu opcji usunięcia.
        private void DeleteShapeMenu_Click(object sender, RoutedEventArgs e)
        {
            // Powtarzamy sprawdzanie z poprzedniej metody, by mieć pewność do jakiego obiektu menu było przyczepione.
            if (sender is MenuItem menuItem && menuItem.Parent is ContextMenu menu && menu.PlacementTarget is Shape shape)
            {
                // Usuwamy z Canvasu ten dokładnie kształt, na którym wywołaliśmy menu prawym przyciskiem.
                MainCanvas.Children.Remove(shape);

                // Zabezpieczenie: jeśli kształt usuwany przez menu prawego przycisku był tym samym, 
                // wokół którego mieliśmy pomarańczową ramkę lewym przyciskiem...
                if (selectedShape == shape)
                {
                    // ...to usuwamy go z pamięci również w zmiennej zaznaczenia.
                    selectedShape = null;
                }
            }
        }
    }
}