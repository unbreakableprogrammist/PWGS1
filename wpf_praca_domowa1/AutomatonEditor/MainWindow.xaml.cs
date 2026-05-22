using System.Windows;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Threading;
using Microsoft.Win32;

namespace AutomatonEditor
{
    public partial class MainWindow : Window, INotifyPropertyChanged
    {
        // 1. Główny obiekt naszego automatu. To on przechowuje listę States i Transitions.
        public Automaton MyAutomaton { get; set; } = new Automaton();

        // 2. Licznik do automatycznego numerowania stanów (np. q0, q1, q2...)
        private int _stateCounter;

        private bool _isDragging;                   // Czy aktualnie coś przesuwamy?
        private Point _lastMousePosition;           // Gdzie ostatnio była myszka?
        private State? _draggedState;               // Który stan (model) przesuwamy?
        private FrameworkElement? _draggedElement;  // Który element graficzny na ekranie przesuwamy?

        // Zaznaczenia w UI (stan i przejście).
        private State? _selectedState; // Przechowuje aktualnie wybrany stan (jeśli jest jakiś wybrany)
        private Transition? _selectedTransition; // Przechowuje aktualnie wybrane przejście
        // Pola odpowiedzialne za stan symulacji.
        private State? _currentState; // Aktualny stan sterowania podczas symulacji
        private int _currentIndex; // Indeks aktualnie przetwarzanego symbolu
        private Transition? _currentTransition; // Aktualnie aktywne przejście
        private readonly DispatcherTimer _simulationTimer; // Timer do trybu animacji
        private readonly ObservableCollection<HistoryEntry> _history = new();
        // Flagi i ustawienia symulacji.
        private bool _isSimulationRunning;
        private bool _hasSimulationStarted;
        private bool _isInputValid = true;
        private double _animationSpeed = 1.0;
        private string _inputWord = string.Empty;
        private string _simulationStatus = "";
        // Aktualnie wybrany stan w panelu po lewej.
        public State? SelectedState
        {
            get => _selectedState;
            set
            {
                _selectedState = value;
                OnPropertyChanged();
                OnPropertyChanged(nameof(IsStateSelected));
            }
        }

        // Aktualnie wybrane przejście.
        public Transition? SelectedTransition
        {
            get => _selectedTransition;
            set
            {
                _selectedTransition = value;
                OnPropertyChanged();
                OnPropertyChanged(nameof(IsTransitionSelected));
            }
        }

        // Flagi pomocnicze do włączania/wyłączania kontrolek.
        public bool IsStateSelected => SelectedState != null; // Czy jakiś stan jest wybrany? (przydatne do włączania/wyłączania przycisków)
        public bool IsTransitionSelected => SelectedTransition != null;

        // Słowo wejściowe do symulacji.
        public string InputWord
        {
            get => _inputWord;
            set
            {
                _inputWord = value ?? string.Empty;
                ResetSimulationState();
                OnPropertyChanged();
                OnPropertyChanged(nameof(InputWordSymbols));
                RefreshSimulationState();
            }
        }

        // Litery słowa wejściowego rysowane osobno, żeby aktywną literę wyróżniać wizualnie.
        // Zamiast zwracać jeden string typu "[a]bc", tworzymy listę małych obiektów
        // z tekstem i flagą IsActive. W XAML ItemsControl generuje z tego osobne
        // elementy, a DataTrigger nadaje aktywnej literze tło i inny kolor tekstu.
        public IReadOnlyList<SymbolDisplayPart> InputWordSymbols
        {
            get
            {
                var symbols = new List<SymbolDisplayPart>();
                for (var i = 0; i < InputWord.Length; i++)
                {
                    symbols.Add(new SymbolDisplayPart
                    {
                        Text = InputWord[i].ToString(),
                        IsActive = i == _currentIndex && _currentIndex < InputWord.Length
                    });
                }

                return symbols;
            }
        }

        // Logika aktywności przycisków symulacji.
        // Te właściwości są bindowane do IsEnabled przycisków. Dzięki temu stan UI
        // wynika bezpośrednio ze stanu symulacji: Previous nie działa na pierwszej
        // literze, Next nie działa po końcu słowa, a Start jest blokowany podczas animacji.
        public bool CanPrevious => _currentState != null && _currentIndex > 0 && !_isSimulationRunning;
        public bool CanNext => _currentState != null && _isInputValid && _currentIndex < InputWord.Length && !_isSimulationRunning;
        public bool CanStart => _currentState != null
                                && _isInputValid
                                && !_isSimulationRunning
                                && (InputWord.Length == 0 ? !_hasSimulationStarted : _currentIndex < InputWord.Length);
        public bool CanStop => _isSimulationRunning;
        public bool CanReset => _currentState != null && !_isSimulationRunning;

        // Czy można edytować słowo wejściowe.
        // Pole jest blokowane po pierwszym kroku albo po uruchomieniu animacji.
        // To jest ważne, bo zmiana słowa w połowie obliczeń unieważniłaby historię
        // stanów i bieżący indeks głowicy.
        public bool IsInputLocked => _hasSimulationStarted || _isSimulationRunning;

        // Flaga walidacji słowa (koloruje pole na czerwono).
        public bool IsInputValid
        {
            get => _isInputValid;
            private set
            {
                _isInputValid = value;
                OnPropertyChanged();
            }
        }

        // Szybkość animacji.
        public double AnimationSpeed
        {
            get => _animationSpeed;
            set
            {
                _animationSpeed = Math.Clamp(value, 0.2, 2.0);
                UpdateTimerInterval();
                OnPropertyChanged();
            }
        }

        // Tekstowy status na dole panelu symulacji.
        public string SimulationStatus
        {
            get => _simulationStatus;
            private set
            {
                _simulationStatus = value;
                OnPropertyChanged();
            }
        }

        // Lista historii kroków.
        public ObservableCollection<HistoryEntry> History => _history;
        // Tekstowy podgląd alfabetu z etykiet przejść.
        public string AlphabetDisplay
        {
            get
            {
                var symbols = new SortedSet<string>(StringComparer.Ordinal);
                foreach (var transition in MyAutomaton.Transitions)
                {
                    if (string.IsNullOrWhiteSpace(transition.Symbol))
                    {
                        continue;
                    }

                    foreach (var item in ParseTransitionSymbols(transition.Symbol))
                    {
                        symbols.Add(item);
                    }
                }

                return symbols.Count == 0 ? "(pusty)" : string.Join(", ", symbols);
            }
        }

        // Konstruktor okna – ustawia DataContext i timer.
        public MainWindow()
        {
            InitializeComponent();

            // DataContext mówi XAML-owi, gdzie ma szukać właściwości używanych
            // w Bindingach, np. {Binding MyAutomaton.States} albo {Binding CanNext}.
            // Tutaj widok jest połączony bezpośrednio z code-behind tego okna.
            DataContext = this;
            _simulationTimer = new DispatcherTimer();
            _simulationTimer.Tick += SimulationTimer_Tick;
            UpdateTimerInterval();
        }

        // ====================================================================
        // METODA 1: Obsługa kliknięcia w puste płótno (Canvas)
        // ====================================================================
        // Obsługa kliknięć w tło płótna.
        private void Canvas_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            // Podwójne kliknięcie w pusty obszar tworzy nowy stan.
            if (e.ClickCount == 2)
            {
                // Pozycja z CanvasHost staje się środkiem okręgu stanu.
                Point position = e.GetPosition((IInputElement)sender);

                // Pierwszy stan w niepustym DFA powinien być początkowy, żeby
                // automat od razu miał poprawnie określone q0.
                var isFirstState = MyAutomaton.States.Count == 0;
                var newState = new State
                {
                    X = position.X,
                    Y = position.Y,
                    Name = $"q{_stateCounter}",
                    IsInitial = isFirstState
                };

                // Dodanie do ObservableCollection wystarczy, żeby ItemsControl
                // narysował nowy element na płótnie.
                MyAutomaton.States.Add(newState);
                if (isFirstState)
                {
                    EnsureSingleInitialState(newState);
                }
                ResetSimulationState();
                RefreshSimulationState();

                // Licznik zapewnia kolejne nazwy q0, q1, q2...
                _stateCounter++;
            }
            // Pojedyncze kliknięcie w tło czyści zaznaczenie elementów.
            else if (e.ClickCount == 1)
            {
                // Przechodzimy przez wszystkie stany i odznaczamy je (ustawiamy IsSelected na false)
                foreach (var state in MyAutomaton.States)
                {
                    state.IsSelected = false;
                }
                SelectedState = null;
                ClearTransitionSelection();
            }
        }

        // ====================================================================
        // METODA 2: Obsługa pojedynczego kliknięcia w konkretny stan (kółko)
        // ====================================================================
        // e - to obiekt, który zawiera wszystkie informacje o kliknięciu (np. gdzie było, ile razy, itp.)
        // Kliknięcie w stan – zaznaczenie i start przesuwania.
        private void State_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            // Zdarzenia myszy w WPF bąbelkują od elementu klikniętego do rodziców.
            // Oznaczenie Handled zatrzymuje zdarzenie na stanie, więc kliknięcie
            // w okrąg nie trafia dalej do CanvasHost i nie czyści zaznaczenia.
            e.Handled = true;

            // Najpierw odznaczamy inne stany, bo panel edycji pracuje na jednym
            // aktywnym stanie naraz.
            foreach (var state in MyAutomaton.States)
            {
                state.IsSelected = false;
            }

            ClearTransitionSelection();

            // Sender to element Grid z DataTemplate stanu. Jego DataContext to
            // konkretny obiekt State, który został narysowany przez ItemsControl.
            var grid = (FrameworkElement)sender;

            var clickedState = (State)grid.DataContext;

            // Zmiana IsSelected uruchamia DataTrigger w XAML-u i zmienia obramowanie.
            clickedState.IsSelected = true;
            SelectedState = clickedState;
            _isDragging = true;
            _draggedState = clickedState;
            _draggedElement = grid;
            _lastMousePosition = e.GetPosition(this);
            _draggedElement.CaptureMouse();
        }

        // Przeciąganie stanu.
        private void State_MouseMove(object sender, MouseEventArgs e)
        {
            if (!_isDragging || _draggedState == null) return;

            Point currentPosition = e.GetPosition(this);
            // Różnica między bieżącą i poprzednią pozycją myszy mówi, o ile
            // pikseli przesunąć środek stanu.
            double deltaX = currentPosition.X - _lastMousePosition.X;
            double deltaY = currentPosition.Y - _lastMousePosition.Y;
            _draggedState.X += deltaX;
            _draggedState.Y += deltaY;
            _lastMousePosition = currentPosition;
        }

        // Zakończenie przeciągania.
        private void State_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            if (_isDragging)
            {
                _isDragging = false;
                _draggedState = null;
                if (_draggedElement != null)
                {
                    _draggedElement.ReleaseMouseCapture();
                    _draggedElement = null;
                }
            }
        }
        // Usuwanie stanu oraz jego przejść.
        private void DeleteState_Click(object sender, RoutedEventArgs e)
        {
            if (SelectedState != null)
            {
                var removedState = SelectedState;
                if (removedState.IsInitial && MyAutomaton.States.Count == 1)
                {
                    MessageBox.Show("Nie można usunąć jedynego stanu początkowego. Dodaj inny stan i ustaw go jako początkowy przed usunięciem tego stanu.");
                    return;
                }

                var removedWasInitial = removedState.IsInitial;
                // usuwamy stan z listy 
                MyAutomaton.States.Remove(SelectedState);
                var transitionsToRemove = new List<Transition>();
                foreach (var transition in MyAutomaton.Transitions)
                {
                    if (transition.Source == removedState || transition.Target == removedState)
                    {
                        transitionsToRemove.Add(transition);
                    }
                }

                foreach (var transition in transitionsToRemove)
                {
                    MyAutomaton.Transitions.Remove(transition);
                }

                if (MyAutomaton.States.Count > 0)
                {
                    var nextInitial = removedWasInitial
                        ? MyAutomaton.States.First()
                        : MyAutomaton.States.FirstOrDefault(state => state.IsInitial) ?? MyAutomaton.States.First();
                    EnsureSingleInitialState(nextInitial);
                }

                OnPropertyChanged(nameof(AlphabetDisplay));
                // Czyscimy zaznaczenie
                SelectedState = null;
                ResetSimulationState();
                RefreshSimulationState();
            }
        }
        // Ustawienie jedynego stanu początkowego.
        private void InitialState_Click(object sender, RoutedEventArgs e)
        {
            if (SelectedState != null)
            {
                if (sender is System.Windows.Controls.CheckBox checkBox && checkBox.IsChecked != true)
                {
                    EnsureSingleInitialState(SelectedState);
                    checkBox.IsChecked = true;
                    MessageBox.Show("Automat musi mieć dokładnie jeden stan początkowy. Wybierz inny stan jako początkowy przed odznaczeniem obecnego.");
                    ResetSimulationState();
                    RefreshSimulationState();
                    return;
                }

                EnsureSingleInitialState(SelectedState);
                ResetSimulationState();
                RefreshSimulationState();
            }
        }

        // Pilnuje, żeby niepusty automat miał dokładnie jeden stan początkowy.
        private void EnsureSingleInitialState(State? preferredState = null)
        {
            if (MyAutomaton.States.Count == 0)
            {
                return;
            }

            var initialState = preferredState != null && MyAutomaton.States.Contains(preferredState)
                ? preferredState
                : MyAutomaton.States.FirstOrDefault(state => state.IsInitial) ?? MyAutomaton.States.First();

            foreach (var state in MyAutomaton.States)
            {
                state.IsInitial = state == initialState;
            }
        }

        // Parsuje etykietę przejścia jako listę pojedynczych symboli: a,b,c.
        //
        // W DFA etykieta przejścia reprezentuje litery alfabetu, dla których
        // funkcja przejścia delta(q, symbol) prowadzi do tego samego stanu.
        // Ten parser wymusza prosty format używany w UI i w JSON:
        // - symbole są rozdzielone przecinkami,
        // - każdy symbol jest pojedynczym znakiem,
        // - w jednej etykiecie nie wolno powtórzyć symbolu,
        // - białe znaki są odrzucane, żeby zapis był jednoznaczny.
        private static IReadOnlyList<string> ParseTransitionSymbols(string label)
        {
            if (string.IsNullOrWhiteSpace(label))
            {
                throw new InvalidOperationException("Przejście musi mieć etykietę.");
            }

            var trimmedLabel = label.Trim();
            if (trimmedLabel.Any(char.IsWhiteSpace))
            {
                throw new InvalidOperationException("Etykieta przejścia nie może zawierać spacji ani innych białych znaków.");
            }

            var symbols = new List<string>();
            var uniqueSymbols = new HashSet<string>(StringComparer.Ordinal);
            foreach (var part in trimmedLabel.Split(','))
            {
                if (part.Length != 1)
                {
                    throw new InvalidOperationException("Etykieta przejścia musi mieć format np. 'a,b,c', gdzie każdy symbol jest pojedynczym znakiem.");
                }

                if (!uniqueSymbols.Add(part))
                {
                    throw new InvalidOperationException($"Symbol '{part}' występuje więcej niż raz w tej samej etykiecie przejścia.");
                }

                symbols.Add(part);
            }

            return symbols;
        }

        // Sprawdza, czy nowe symbole nie łamią determinizmu dla stanu źródłowego.
        // Definicja DFA wymaga, żeby dla pary (stan, symbol) istniało najwyżej
        // jedno przejście. Przykład konfliktu: z q0 mamy już przejście opisane
        // "a,b", więc nie wolno dodać drugiego przejścia z q0 z etykietą "b,c".
        private void ValidateNoTransitionConflict(State sourceState, IEnumerable<string> symbols)
        {
            var newSymbols = new HashSet<string>(symbols, StringComparer.Ordinal);
            foreach (var transition in MyAutomaton.Transitions)
            {
                if (transition.Source != sourceState)
                {
                    continue;
                }

                foreach (var existingSymbol in ParseTransitionSymbols(transition.Symbol))
                {
                    if (newSymbols.Contains(existingSymbol))
                    {
                        throw new InvalidOperationException(
                            $"Dla stanu {sourceState.Name} istnieje już przejście dla symbolu '{existingSymbol}'. DFA nie może mieć dwóch przejść dla tej samej litery z jednego stanu.");
                    }
                }
            }
        }

        // Waliduje, czy aktualny automat spełnia warunek determinizmu.
        // Używane przed eksportem i symulacją jako dodatkowe zabezpieczenie.
        // Nawet jeśli UI blokuje konflikty przy dodawaniu, importowany JSON mógłby
        // zawierać niepoprawne dane, dlatego sprawdzamy cały automat ponownie.
        private void ValidateCurrentAutomatonDeterminism()
        {
            var symbolsByState = new Dictionary<State, HashSet<string>>();
            foreach (var transition in MyAutomaton.Transitions)
            {
                if (!MyAutomaton.States.Contains(transition.Source) || !MyAutomaton.States.Contains(transition.Target))
                {
                    throw new InvalidOperationException("Przejście odwołuje się do stanu spoza automatu.");
                }

                if (!symbolsByState.TryGetValue(transition.Source, out var usedSymbols))
                {
                    usedSymbols = new HashSet<string>(StringComparer.Ordinal);
                    symbolsByState[transition.Source] = usedSymbols;
                }

                foreach (var symbol in ParseTransitionSymbols(transition.Symbol))
                {
                    if (!usedSymbols.Add(symbol))
                    {
                        throw new InvalidOperationException(
                            $"Automat nie jest deterministyczny: stan {transition.Source.Name} ma więcej niż jedno przejście dla symbolu '{symbol}'.");
                    }
                }
            }
        }

        private bool TryValidateAutomatonForSimulation()
        {
            try
            {
                if (MyAutomaton.States.Count(state => state.IsInitial) != 1)
                {
                    throw new InvalidOperationException("automat musi mieć dokładnie jeden stan początkowy.");
                }

                ValidateCurrentAutomatonDeterminism();
                return true;
            }
            catch (InvalidOperationException ex)
            {
                MessageBox.Show($"Nie można rozpocząć symulacji: {ex.Message}");
                return false;
            }
        }

        public event PropertyChangedEventHandler? PropertyChanged;
        protected void OnPropertyChanged([CallerMemberName] string? name = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }

        // Dodawanie nowego przejścia z etykietą.
        private void AddTransition_Click(object sender, RoutedEventArgs e)
        {
            // Pobieramy wybrane stany z ComboBoxów i wpisany tekst
            if (ComboFrom.SelectedItem is State sourceState &&
                ComboTo.SelectedItem is State targetState &&
                !string.IsNullOrWhiteSpace(TextSymbol.Text))
            {
                string label;
                IReadOnlyList<string> symbols;
                try
                {
                    symbols = ParseTransitionSymbols(TextSymbol.Text);
                    label = string.Join(",", symbols);
                    ValidateNoTransitionConflict(sourceState, symbols);
                }
                catch (InvalidOperationException ex)
                {
                    MessageBox.Show(ex.Message);
                    return;
                }

                // Tworzymy nowe przejście
                var newTransition = new Transition
                {
                    Source = sourceState,
                    Target = targetState,
                    Symbol = label
                };

                UpdateTransitionOffsets(sourceState, targetState, newTransition);

                // Dodajemy do kolekcji w naszym automacie
                MyAutomaton.Transitions.Add(newTransition);

                // Opcjonalnie: czyścimy pole tekstowe po dodaniu
                TextSymbol.Clear();
                OnPropertyChanged(nameof(AlphabetDisplay));
                RefreshSimulationState();
            }
            else
            {
                MessageBox.Show("Wybierz stan początkowy, końcowy i podaj symbol przejścia.");
            }
        }

        // Import automatu z pliku JSON.
        private void Import_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new OpenFileDialog
            {
                Filter = "Pliki JSON (*.json)|*.json|Wszystkie pliki (*.*)|*.*",
                InitialDirectory = GetSamplesDirectory()
            };

            if (dialog.ShowDialog() != true)
            {
                return;
            }

            try
            {
                var json = File.ReadAllText(dialog.FileName);
                var data = AutomatonSerializer.Deserialize(json);
                LoadFromData(data);
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Błąd importu: {ex.Message}");
            }
        }

        // Eksport automatu do JSON.
        private void ExportJson_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                var data = BuildData();
                var dialog = new SaveFileDialog
                {
                    Filter = "Pliki JSON (*.json)|*.json",
                    FileName = "automaton.json"
                };

                if (dialog.ShowDialog() != true)
                {
                    return;
                }

                File.WriteAllText(dialog.FileName, AutomatonSerializer.Serialize(data));
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Błąd eksportu: {ex.Message}");
            }
        }

        // Eksport widoku automatu do obrazu.
        private void ExportImage_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new SaveFileDialog
            {
                Filter = "Pliki PNG (*.png)|*.png|Pliki JPEG (*.jpg)|*.jpg",
                FileName = "automaton.png"
            };

            if (dialog.ShowDialog() != true)
            {
                return;
            }

            var canvasHost = FindName("CanvasHost") as FrameworkElement;
            if (canvasHost == null)
            {
                throw new InvalidOperationException("Brak powierzchni do eksportu obrazu.");
            }
            var bounds = VisualTreeHelper.GetDescendantBounds(canvasHost);
            var renderTarget = new RenderTargetBitmap(
                (int)bounds.Width + 20,
                (int)bounds.Height + 20,
                96,
                96,
                PixelFormats.Pbgra32);

            var drawingVisual = new DrawingVisual();
            using (var context = drawingVisual.RenderOpen())
            {
                var brush = new VisualBrush(canvasHost);
                context.DrawRectangle(brush, null, new Rect(new Point(10, 10), bounds.Size));
            }

            renderTarget.Render(drawingVisual);

            BitmapEncoder encoder = dialog.FilterIndex == 2 ? new JpegBitmapEncoder() : new PngBitmapEncoder();
            encoder.Frames.Add(BitmapFrame.Create(renderTarget));

            using var stream = File.Create(dialog.FileName);
            encoder.Save(stream);
        }

        // Zasilenie modelu na podstawie danych JSON.
        private void LoadFromData(AutomatonData data)
        {
            ValidateData(data);
            MyAutomaton.States.Clear();
            MyAutomaton.Transitions.Clear();

            var stateMap = new Dictionary<int, State>();
            foreach (var stateData in data.States)
            {
                var state = new State
                {
                    Name = stateData.Name,
                    X = stateData.Position.X,
                    Y = stateData.Position.Y,
                    IsInitial = stateData.IsStart,
                    IsAccepting = stateData.IsAccepting,
                    Radius = stateData.Appearance.Radius,
                    StrokeThickness = stateData.Appearance.StrokeThickness,
                    FillBrush = AutomatonSerializer.ParseBrush(stateData.Appearance.FillColor),
                    StrokeBrush = AutomatonSerializer.ParseBrush(stateData.Appearance.StrokeColor)
                };
                MyAutomaton.States.Add(state);
                stateMap[stateData.Id] = state;
            }

            foreach (var transitionData in data.Transitions)
            {
                var transition = new Transition
                {
                    Source = stateMap[transitionData.FromStateId],
                    Target = stateMap[transitionData.ToStateId],
                    Symbol = transitionData.Symbol
                };
                UpdateTransitionOffsets(transition.Source, transition.Target, transition);
                MyAutomaton.Transitions.Add(transition);
            }

            _stateCounter = MyAutomaton.States.Count;
            ClearStateSelection();
            ClearTransitionSelection();
            OnPropertyChanged(nameof(AlphabetDisplay));
            ResetSimulationState();
            RefreshSimulationState();
        }

        // Buduje dane do eksportu JSON.
        private AutomatonData BuildData()
        {
            if (MyAutomaton.States.Count == 0)
            {
                throw new InvalidOperationException("Brak stanów do eksportu.");
            }

            if (MyAutomaton.States.Count(state => state.IsInitial) != 1)
            {
                throw new InvalidOperationException("Automat musi mieć dokładnie jeden stan początkowy.");
            }

            ValidateCurrentAutomatonDeterminism();

            var data = new AutomatonData();
            var alphabet = new SortedSet<string>(StringComparer.Ordinal);
            foreach (var transition in MyAutomaton.Transitions)
            {
                foreach (var symbol in ParseTransitionSymbols(transition.Symbol))
                {
                    alphabet.Add(symbol);
                }
            }

            data.Meta = new AutomatonMeta
            {
                Description = "Automaton export",
                Alphabet = alphabet.ToList(),
                Created = DateTimeOffset.UtcNow
            };

            var id = 0;
            var stateIds = new Dictionary<State, int>();
            foreach (var state in MyAutomaton.States)
            {
                if (string.IsNullOrWhiteSpace(state.Name))
                {
                    throw new InvalidOperationException("Stan bez nazwy nie może zostać wyeksportowany.");
                }

                var stateId = id++;
                stateIds[state] = stateId;
                data.States.Add(new StateData
                {
                    Id = stateId,
                    Name = state.Name,
                    IsStart = state.IsInitial,
                    IsAccepting = state.IsAccepting,
                    Position = new StatePosition
                    {
                        X = state.X,
                        Y = state.Y
                    },
                    Appearance = new StateAppearance
                    {
                        Radius = state.Radius,
                        StrokeThickness = state.StrokeThickness,
                        FillColor = AutomatonSerializer.BrushToString(state.FillBrush),
                        StrokeColor = AutomatonSerializer.BrushToString(state.StrokeBrush)
                    }
                });
            }

            foreach (var transition in MyAutomaton.Transitions)
            {
                data.Transitions.Add(new TransitionData
                {
                    FromStateId = stateIds[transition.Source],
                    ToStateId = stateIds[transition.Target],
                    Symbol = transition.Symbol
                });
            }

            return data;
        }

        // Walidacja danych z JSON (spójność stanów/przejść).
        //
        // Import nie powinien ufać plikowi wejściowemu. Sprawdzamy najpierw
        // poprawność stanów: unikalne identyfikatory, unikalne nazwy, dodatni
        // promień i dodatnią grubość krawędzi. Następnie sprawdzamy przejścia:
        // muszą wskazywać istniejące stany, mieć poprawne etykiety i nie mogą
        // naruszać determinizmu DFA.
        private void ValidateData(AutomatonData data)
        {
            if (data.States.Count == 0)
            {
                throw new InvalidOperationException("Plik nie zawiera żadnych stanów.");
            }

            var ids = new HashSet<int>();
            var names = new HashSet<string>(StringComparer.Ordinal);
            var initialCount = 0;
            foreach (var state in data.States)
            {
                if (string.IsNullOrWhiteSpace(state.Name))
                {
                    throw new InvalidOperationException("Stan musi mieć nazwę.");
                }

                if (!names.Add(state.Name))
                {
                    throw new InvalidOperationException($"Zduplikowana nazwa stanu: {state.Name}");
                }

                if (!ids.Add(state.Id))
                {
                    throw new InvalidOperationException($"Zduplikowany identyfikator stanu: {state.Id}");
                }

                if (state.Appearance.Radius <= 0)
                {
                    throw new InvalidOperationException($"Stan {state.Name} ma niepoprawny promień.");
                }

                if (state.Appearance.StrokeThickness <= 0)
                {
                    throw new InvalidOperationException($"Stan {state.Name} ma niepoprawną grubość krawędzi.");
                }

                if (state.IsStart)
                {
                    initialCount++;
                }
            }

            if (initialCount != 1)
            {
                throw new InvalidOperationException("Plik musi zawierać dokładnie jeden stan początkowy.");
            }

            // Dla każdego stanu źródłowego zapamiętujemy zbiór symboli, które już
            // wystąpiły na jego wyjściach. HashSet.Add zwraca false, gdy symbol
            // był już widziany, co jest dokładnie konfliktem determinizmu.
            var transitionsBySourceAndSymbol = new Dictionary<int, HashSet<string>>();
            foreach (var transition in data.Transitions)
            {
                if (!ids.Contains(transition.FromStateId) || !ids.Contains(transition.ToStateId))
                {
                    throw new InvalidOperationException("Przejście odwołuje się do nieistniejącego stanu.");
                }

                var symbols = ParseTransitionSymbols(transition.Symbol);
                if (!transitionsBySourceAndSymbol.TryGetValue(transition.FromStateId, out var usedSymbols))
                {
                    usedSymbols = new HashSet<string>(StringComparer.Ordinal);
                    transitionsBySourceAndSymbol[transition.FromStateId] = usedSymbols;
                }

                foreach (var symbol in symbols)
                {
                    if (!usedSymbols.Add(symbol))
                    {
                        throw new InvalidOperationException(
                            $"Automat nie jest deterministyczny: stan o id {transition.FromStateId} ma więcej niż jedno przejście dla symbolu '{symbol}'.");
                    }
                }
            }
        }

        // Zwraca folder startowy okna wyboru pliku.
        private static string GetSamplesDirectory()
        {
            foreach (var startDirectory in new[] { Environment.CurrentDirectory, AppDomain.CurrentDomain.BaseDirectory })
            {
                var current = new DirectoryInfo(startDirectory);
                while (current != null)
                {
                    var hasSampleAutomaton = File.Exists(Path.Combine(current.FullName, "automaton.json"));
                    var hasSolution = File.Exists(Path.Combine(current.FullName, "AutomatonEditor.sln"));
                    if (hasSampleAutomaton || hasSolution)
                    {
                        return current.FullName;
                    }

                    current = current.Parent;
                }
            }

            return Environment.CurrentDirectory;
        }

        // Usuwanie zaznaczonego przejścia.
        private void DeleteTransition_Click(object sender, RoutedEventArgs e)
        {
            if (SelectedTransition != null)
            {
                var source = SelectedTransition.Source;
                var target = SelectedTransition.Target;
                MyAutomaton.Transitions.Remove(SelectedTransition);
                SelectedTransition = null;
                UpdateTransitionOffsets(source, target, null);
                OnPropertyChanged(nameof(AlphabetDisplay));
                RefreshSimulationState();
            }
        }

        private void Previous_Click(object sender, RoutedEventArgs e)
        {
            if (!CanPrevious)
            {
                return;
            }

            _currentIndex--;
            if (_history.Count > 0)
            {
                _history.RemoveAt(_history.Count - 1);
            }

            _currentTransition = _history.LastOrDefault()?.Transition;
            _currentState = _history.LastOrDefault()?.State ?? MyAutomaton.States.FirstOrDefault(state => state.IsInitial);
            EvaluateCurrentState();
            RefreshSimulationState();
        }

        private void Next_Click(object sender, RoutedEventArgs e)
        {
            if (!CanNext)
            {
                return;
            }

            if (!TryValidateAutomatonForSimulation())
            {
                return;
            }

            // Pierwsze naciśnięcie Next rozpoczyna obliczenie w trybie krokowym.
            // Od tego momentu blokujemy edycję słowa wejściowego, żeby użytkownik
            // nie zmienił taśmy po wykonaniu części ruchów automatu.
            _hasSimulationStarted = true;
            StepForward();
        }

        private void Start_Click(object sender, RoutedEventArgs e)
        {
            if (!CanStart)
            {
                return;
            }

            if (!TryValidateAutomatonForSimulation())
            {
                return;
            }

            // Puste słowo epsilon nie wykonuje żadnego przejścia. Automat od razu
            // zatrzymuje się w stanie początkowym, więc wystarczy ocenić, czy ten
            // stan należy do zbioru stanów akceptujących F.
            _hasSimulationStarted = true;
            if (InputWord.Length == 0)
            {
                EvaluateCurrentState();
                RefreshSimulationState();
                return;
            }

            _isSimulationRunning = true;
            _simulationTimer.Start();
            RefreshSimulationState();
        }

        private void Stop_Click(object sender, RoutedEventArgs e)
        {
            if (!_isSimulationRunning)
            {
                return;
            }

            _simulationTimer.Stop();
            _isSimulationRunning = false;
            RefreshSimulationState();
        }

        private void Reset_Click(object sender, RoutedEventArgs e)
        {
            if (_isSimulationRunning)
            {
                return;
            }

            ResetSimulationState();
            RefreshSimulationState();
        }

        private void SimulationTimer_Tick(object? sender, EventArgs e)
        {
            if (_currentState == null || !_isInputValid || _currentIndex >= InputWord.Length)
            {
                _simulationTimer.Stop();
                _isSimulationRunning = false;
                RefreshSimulationState();
                return;
            }

            StepForward();
        }

        // Wykonuje dokładnie jeden krok obliczenia DFA.
        // Głowica czyta symbol InputWord[_currentIndex], szukamy przejścia
        // delta(_currentState, symbol), przechodzimy do stanu docelowego i dopiero
        // wtedy przesuwamy indeks głowicy o jedną pozycję w prawo.
        private void StepForward()
        {
            if (_currentState == null)
            {
                return;
            }

            if (_currentIndex >= InputWord.Length)
            {
                RefreshSimulationState();
                return;
            }

            var symbol = InputWord[_currentIndex].ToString();
            var transition = FindTransition(_currentState, symbol);
            if (transition == null)
            {
                SimulationStatus = $"Brak przejścia dla symbolu '{symbol}'. Odrzucono.";
                _currentIndex = InputWord.Length;
                RefreshSimulationState();
                return;
            }

            // Te dwie właściwości nie zmieniają funkcji przejścia, tylko sterują
            // wizualnym wyróżnieniem aktywnej krawędzi i aktywnego symbolu etykiety.
            _currentTransition = transition;
            _currentTransition.ActiveSymbol = symbol;
            _currentState = transition.Target;
            _history.Add(new HistoryEntry
            {
                StateName = transition.Target.Name ?? string.Empty,
                Symbol = symbol,
                State = transition.Target,
                Transition = transition
            });
            _currentIndex++;
            EvaluateCurrentState();
            RefreshSimulationState();
        }

        private void EvaluateCurrentState()
        {
            if (_currentState == null)
            {
                return;
            }

            if (_currentIndex >= InputWord.Length)
            {
                SimulationStatus = _currentState.IsAccepting ? "Słowo zaakceptowane." : "Słowo odrzucone.";
            }
            else
            {
                SimulationStatus = string.Empty;
            }
        }

        private Transition? FindTransition(State state, string symbol)
        {
            foreach (var transition in MyAutomaton.Transitions)
            {
                if (transition.Source != state)
                {
                    continue;
                }

                if (ParseTransitionSymbols(transition.Symbol).Any(label => label == symbol))
                {
                    return transition;
                }
            }

            return null;
        }

        private void ResetSimulationState()
        {
            _simulationTimer?.Stop();
            _isSimulationRunning = false;
            _hasSimulationStarted = false;
            _currentIndex = 0;
            _currentTransition = null;
            _currentState = MyAutomaton.States.FirstOrDefault(state => state.IsInitial);
            _history.Clear(); // Keep simulation reset after history update
            SimulationStatus = string.Empty;
        }

        // Synchronizuje model symulacji z widokiem.
        // Zmiana IsActive na stanach/przejściach uruchamia triggery w XAML-u,
        // a OnPropertyChanged odświeża przyciski, blokadę pola tekstowego i
        // wizualne wyróżnienie aktualnej litery wejścia.
        private void RefreshSimulationState()
        {
            ValidateInputWord();
            foreach (var state in MyAutomaton.States)
            {
                state.IsActive = state == _currentState;
            }

            foreach (var transition in MyAutomaton.Transitions)
            {
                transition.IsActive = transition == _currentTransition;
                if (transition != _currentTransition)
                {
                    transition.ActiveSymbol = string.Empty;
                }
            }

            OnPropertyChanged(nameof(InputWordSymbols));
            OnPropertyChanged(nameof(CanPrevious));
            OnPropertyChanged(nameof(CanNext));
            OnPropertyChanged(nameof(CanStart));
            OnPropertyChanged(nameof(CanStop));
            OnPropertyChanged(nameof(CanReset));
            OnPropertyChanged(nameof(IsInputLocked));
            OnPropertyChanged(nameof(IsInputValid));
        }

        // Waliduje słowo wejściowe względem alfabetu aktualnego automatu.
        // Alfabet nie jest wpisywany ręcznie: wyznaczamy go jako sumę wszystkich
        // symboli występujących na etykietach przejść. Jeżeli użytkownik wpisze
        // literę spoza tego zbioru, symulacja jest blokowana, a pole dostaje
        // czerwone obramowanie przez DataTrigger w XAML-u.
        private void ValidateInputWord()
        {
            if (string.IsNullOrEmpty(InputWord))
            {
                if (!_hasSimulationStarted)
                {
                    SimulationStatus = string.Empty;
                }
                IsInputValid = true;
                return;
            }

            var alphabet = new HashSet<string>(StringComparer.Ordinal);
            foreach (var transition in MyAutomaton.Transitions)
            {
                foreach (var symbol in ParseTransitionSymbols(transition.Symbol))
                {
                    alphabet.Add(symbol);
                }
            }

            foreach (var character in InputWord)
            {
                if (!alphabet.Contains(character.ToString()))
                {
                    SimulationStatus = "Słowo zawiera symbole spoza alfabetu.";
                    IsInputValid = false;
                    return;
                }
            }

            if (SimulationStatus == "Słowo zawiera symbole spoza alfabetu.")
            {
                SimulationStatus = string.Empty;
            }

            IsInputValid = true;
        }

        private void UpdateTimerInterval()
        {
            if (_simulationTimer == null)
            {
                return;
            }

            var interval = TimeSpan.FromSeconds(1.0 / _animationSpeed);
            _simulationTimer.Interval = interval;
        }

        // Kliknięcie przejścia – zaznaczenie.
        private void Transition_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            e.Handled = true;
            ClearStateSelection();
            var element = (FrameworkElement)sender;
            var transition = (Transition)element.DataContext;
            SelectTransition(transition);
        }

        // Zaznacza jedno przejście i odznacza pozostałe.
        private void SelectTransition(Transition transition)
        {
            foreach (var item in MyAutomaton.Transitions)
            {
                item.IsSelected = false;
            }

            transition.IsSelected = true;
            SelectedTransition = transition;
        }

        // Czyści zaznaczenie przejść.
        private void ClearTransitionSelection()
        {
            foreach (var item in MyAutomaton.Transitions)
            {
                item.IsSelected = false;
            }

            SelectedTransition = null;
        }

        // Czyści zaznaczenie stanów.
        private void ClearStateSelection()
        {
            foreach (var state in MyAutomaton.States)
            {
                state.IsSelected = false;
            }

            SelectedState = null;
        }

        // Ustawia łuki, gdy są przejścia w obie strony.
        // Proste przejścia q_i -> q_j i q_j -> q_i rysowane tym samym odcinkiem
        // nakładałyby się na siebie. Dlatego nadajemy im przeciwne CurveOffset:
        // jeden łuk odgina się w jedną stronę, drugi w przeciwną. Sama geometria
        // łuku jest potem liczona w Transition.RefreshGeometry().
        private void UpdateTransitionOffsets(State source, State target, Transition? newTransition)
        {
            var candidates = new List<Transition>();
            foreach (var transition in MyAutomaton.Transitions)
            {
                if ((transition.Source == source && transition.Target == target) ||
                    (transition.Source == target && transition.Target == source))
                {
                    candidates.Add(transition);
                }
            }

            if (newTransition != null && !candidates.Contains(newTransition))
            {
                candidates.Add(newTransition);
            }

            if (candidates.Count <= 1)
            {
                foreach (var transition in candidates)
                {
                    transition.CurveOffset = 0;
                }
                return;
            }

            const double offset = 30;
            foreach (var transition in candidates)
            {
                transition.CurveOffset = transition.Source == source && transition.Target == target ? offset : -offset;
            }
        }
    }
}
