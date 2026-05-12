using System.Windows;
using System.Windows.Input;
using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace AutomatonEditor
{
    public partial class MainWindow : Window, INotifyPropertyChanged
    {
        // 1. Główny obiekt naszego automatu. To on przechowuje listę States i Transitions.
        public Automaton MyAutomaton { get; set; } = new Automaton();

        // 2. Licznik do automatycznego numerowania stanów (np. q0, q1, q2...)
        private int _stateCounter = 0;

        private bool _isDragging = false;           // Czy aktualnie coś przesuwamy?
        private Point _lastMousePosition;           // Gdzie ostatnio była myszka?
        private State _draggedState = null;         // Który stan (model) przesuwamy?
        private FrameworkElement _draggedElement;   // Który obrazek na ekranie przesuwamy?

        private State _selectedState; // Przechowuje aktualnie wybrany stan (jeśli jest jakiś wybrany)
        public State SelectedState
        {
            get => _selectedState;
            set
            {
                _selectedState = value;
                OnPropertyChanged();
                OnPropertyChanged(nameof(IsStateSelected));
            }
        }

        public bool IsStateSelected => SelectedState != null; // Czy jakiś stan jest wybrany? (przydatne do włączania/wyłączania przycisków)

        public MainWindow()
        {
            InitializeComponent();

            // 3. TO JEST KLUCZ: Mówimy naszemu interfejsowi graficznemu (XAML), 
            // że kiedy pyta o jakieś dane w nawiasach {Binding ...}, 
            // to ma ich szukać właśnie w tym pliku.
            DataContext = this;
        }

        // ====================================================================
        // METODA 1: Obsługa kliknięcia w puste płótno (Canvas)
        // ====================================================================
        private void Canvas_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            // Sprawdzamy, czy było to podwójne kliknięcie 
            if (e.ClickCount == 2)
            {
                // Pobieramy koordynaty miejsca, w które kliknęła myszka
                Point position = e.GetPosition((IInputElement)sender);

                // Tworzymy nowy stan i przypisujemy mu te koordynaty oraz nazwę
                var newState = new State
                {
                    X = position.X,
                    Y = position.Y,
                    Name = $"q{_stateCounter}"
                };

                // Dodajemy stan do naszej listy. Interfejs (ItemsControl) sam go narysuje!
                MyAutomaton.States.Add(newState);

                // Zwiększamy licznik dla kolejnego stanu
                _stateCounter++;
            }
            // Sprawdzamy, czy było to pojedyncze kliknięcie w pustą przestrzeń
            else if (e.ClickCount == 1)
            {
                // Przechodzimy przez wszystkie stany i odznaczamy je (ustawiamy IsSelected na false)
                foreach (var state in MyAutomaton.States)
                {
                    state.IsSelected = false;
                }
                SelectedState = null; // Czyscimy zaznaczenie w kodzie, żeby przyciski też się odznaczyły
            }
        }

        // ====================================================================
        // METODA 2: Obsługa pojedynczego kliknięcia w konkretny stan (kółko)
        // ====================================================================
        // e - to obiekt, który zawiera wszystkie informacje o kliknięciu (np. gdzie było, ile razy, itp.)
        private void State_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            // BARDZO WAŻNE: Zatrzymujemy "przebijanie" kliknięcia!
            // W WPF kliknięcie "bąbelkuje" z dołu do góry. Gdybyśmy tego nie zrobili,
            // po kliknięciu w stan, kliknięcie poleciałoby też do płótna pod spodem, 
            // a płótno (zgodnie z kodem wyżej) od razu by nam ten stan odznaczyło!
            e.Handled = true;

            // Najpierw odznaczamy wszystkie inne stany, żeby tylko jeden mógł być wybrany naraz
            foreach (var state in MyAutomaton.States)
            {
                state.IsSelected = false;
            }

            // Odkrywamy, który konkretnie stan został kliknięty.
            // "sender" to graficzny pojemnik (Grid z XAMLa), w który uderzyła myszka.
            var grid = (FrameworkElement)sender;

            // A z pojemnika wyciągamy jego duszę (czyli nasz kontekst danych - obiekt klasy State)
            var clickedState = (State)grid.DataContext;

            // Na sam koniec oznaczamy go jako wybrany. 
            // To automatycznie odpali wyzwalacz (Trigger) w XAML i zmieni kolor ramki!
            clickedState.IsSelected = true;
            SelectedState = clickedState; // Zapamiętujemy, który stan jest teraz wybrany (przyda się do przycisków)
            _isDragging = true; // Zaczynamy przesuwanie
            _draggedState = clickedState; // Zapamiętujemy, który stan przesuwamy
            _draggedElement = grid; // Zapamiętujemy, który element graficzny przesuwamy
            _lastMousePosition = e.GetPosition(this); // Zapamiętujemy pozycję myszki wzgledem calego okna
            _draggedElement.CaptureMouse(); // "Łapiemy" myszkę, żeby mieć pewność, że będziemy dostawać eventy nawet jeśli myszka wyjdzie poza ten elementz
        }

        private void State_MouseMove(object sender, MouseEventArgs e)
        {
            if (!_isDragging || _draggedState == null) return; // jesli nie przesuwamy teraz nic albo nie przesuwamy konkretnego to pass
            Point currentPosition = e.GetPosition(this); // aktualna pozycja myszki wzgledem calego okna
            // obliczamy o ile pikseli sie przesunela myszka od ostatniego zapamietanego ruchu
            double deltaX = currentPosition.X - _lastMousePosition.X;
            double deltaY = currentPosition.Y - _lastMousePosition.Y;
            _draggedState.X += deltaX; // przesuwamy stan o te piksele
            _draggedState.Y += deltaY;
            _lastMousePosition = currentPosition; // aktualizujemy ostatnia pozycje myszki
        }

        private void State_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            if (_isDragging)
            {
                // odpinamy nasze zmienne bo juz nic nie przesuwamy
                _isDragging = false;
                _draggedState = null;
                if (_draggedElement != null)
                {
                    _draggedElement.ReleaseMouseCapture(); // zwalniamy "łapanie" myszki przez ten element
                    _draggedElement = null;
                }
            }
        }
        private void DeleteState_Click(object sender, RoutedEventArgs e)
        {
            if (SelectedState != null)
            {
                // usuwamy stan z listy 
                MyAutomaton.States.Remove(SelectedState);
                // Czyscimy zaznaczenie
                SelectedState = null;
            }
        }
        private void InitialState_Click(object sender, RoutedEventArgs e)
        {
            if (SelectedState != null)
            {
                // Najpierw odznaczamy wszystkie stany jako początkowe
                foreach (var state in MyAutomaton.States)
                {
                    if(state != SelectedState) state.IsInitial = false;
                }
            }
        }
        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged([CallerMemberName] string name = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }

        private void AddTransition_Click(object sender, RoutedEventArgs e)
        {

        }
    }
}