using System.Windows;
using System.Windows.Input;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using Microsoft.Win32;
using System.IO;
using System.Linq;
using System.Windows.Media.Imaging;
using System.Windows.Media;
using System.Windows.Threading;

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
        private Transition _selectedTransition; // Przechowuje aktualnie wybrane przejście
        private State _currentState; // Aktualny stan sterowania podczas symulacji
        private int _currentIndex; // Indeks aktualnie przetwarzanego symbolu
        private Transition _currentTransition; // Aktualnie aktywne przejście
        private readonly DispatcherTimer _simulationTimer; // Timer do trybu animacji
        private readonly ObservableCollection<HistoryEntry> _history = new();
        private bool _isSimulationRunning;
        private bool _isInputValid = true;
        private double _animationSpeed = 1.0;
        private string _inputWord = string.Empty;
        private string _simulationStatus = "";
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

        public Transition SelectedTransition
        {
            get => _selectedTransition;
            set
            {
                _selectedTransition = value;
                OnPropertyChanged();
                OnPropertyChanged(nameof(IsTransitionSelected));
            }
        }

        public bool IsStateSelected => SelectedState != null; // Czy jakiś stan jest wybrany? (przydatne do włączania/wyłączania przycisków)
        public bool IsTransitionSelected => SelectedTransition != null;

        public string InputWord
        {
            get => _inputWord;
            set
            {
                _inputWord = value ?? string.Empty;
                ResetSimulationState();
                OnPropertyChanged();
                OnPropertyChanged(nameof(InputWordDisplay));
                RefreshSimulationState();
            }
        }

        public string InputWordDisplay
        {
            get
            {
                if (string.IsNullOrEmpty(InputWord))
                {
                    return string.Empty;
                }

                var builder = new System.Text.StringBuilder();
                for (var i = 0; i < InputWord.Length; i++)
                {
                    builder.Append(i == _currentIndex ? $"[{InputWord[i]}]" : InputWord[i].ToString());
                }
                return builder.ToString();
            }
        }

        public bool CanPrevious => _currentState != null && _currentIndex > 0 && !_isSimulationRunning;
        public bool CanNext => _currentState != null && _isInputValid && _currentIndex < InputWord.Length && !_isSimulationRunning;
        public bool CanStart => _currentState != null && _isInputValid && !_isSimulationRunning && _currentIndex < InputWord.Length;
        public bool CanStop => _isSimulationRunning;
        public bool CanReset => _currentState != null && !_isSimulationRunning;

        public bool IsInputEditable => !_isSimulationRunning;

        public bool IsInputValid
        {
            get => _isInputValid;
            private set
            {
                _isInputValid = value;
                OnPropertyChanged();
            }
        }

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

        public string SimulationStatus
        {
            get => _simulationStatus;
            private set
            {
                _simulationStatus = value;
                OnPropertyChanged();
            }
        }

        public ObservableCollection<HistoryEntry> History => _history;
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

                    foreach (var item in transition.Symbol.Split(',', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries))
                    {
                        symbols.Add(item);
                    }
                }

                return symbols.Count == 0 ? "(pusty)" : string.Join(", ", symbols);
            }
        }

        public MainWindow()
        {
            InitializeComponent();

            // 3. TO JEST KLUCZ: Mówimy naszemu interfejsowi graficznemu (XAML), 
            // że kiedy pyta o jakieś dane w nawiasach {Binding ...}, 
            // to ma ich szukać właśnie w tym pliku.
            DataContext = this;
            _simulationTimer = new DispatcherTimer();
            _simulationTimer.Tick += SimulationTimer_Tick;
            UpdateTimerInterval();
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
                RefreshSimulationState();

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
                ClearTransitionSelection();
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

            ClearTransitionSelection();

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
                var removedState = SelectedState;
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

                OnPropertyChanged(nameof(AlphabetDisplay));
                // Czyscimy zaznaczenie
                SelectedState = null;
                RefreshSimulationState();
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
                ResetSimulationState();
                RefreshSimulationState();
            }
        }
        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged([CallerMemberName] string name = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }

        private void AddTransition_Click(object sender, RoutedEventArgs e)
        {
            // Pobieramy wybrane stany z ComboBoxów i wpisany tekst
            if (ComboFrom.SelectedItem is State sourceState &&
                ComboTo.SelectedItem is State targetState &&
                !string.IsNullOrWhiteSpace(TextSymbol.Text))
            {
                var label = TextSymbol.Text.Trim();
                if (label.Contains(' '))
                {
                    MessageBox.Show("Etykieta przejścia nie może zawierać spacji.");
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

        private AutomatonData BuildData()
        {
            if (MyAutomaton.States.Count == 0)
            {
                throw new InvalidOperationException("Brak stanów do eksportu.");
            }

            var data = new AutomatonData();
            var alphabet = new SortedSet<string>(StringComparer.Ordinal);
            foreach (var transition in MyAutomaton.Transitions)
            {
                foreach (var symbol in transition.Symbol.Split(',', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries))
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

            if (initialCount > 1)
            {
                throw new InvalidOperationException("Może istnieć tylko jeden stan początkowy.");
            }

            foreach (var transition in data.Transitions)
            {
                if (!ids.Contains(transition.FromStateId) || !ids.Contains(transition.ToStateId))
                {
                    throw new InvalidOperationException("Przejście odwołuje się do nieistniejącego stanu.");
                }

                if (string.IsNullOrWhiteSpace(transition.Symbol))
                {
                    throw new InvalidOperationException("Przejście musi mieć etykietę.");
                }
            }
        }

        private static string GetSamplesDirectory()
        {
            var root = AppDomain.CurrentDomain.BaseDirectory;
            return Directory.Exists(root) ? root : Environment.CurrentDirectory;
        }

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

            StepForward();
        }

        private void Start_Click(object sender, RoutedEventArgs e)
        {
            if (!CanStart)
            {
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

        private void SimulationTimer_Tick(object sender, EventArgs e)
        {
            if (!CanNext)
            {
                _simulationTimer.Stop();
                _isSimulationRunning = false;
                RefreshSimulationState();
                return;
            }

            StepForward();
        }

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

            _currentTransition = transition; // Keep transition activation consistent
            _currentTransition.ActiveSymbol = symbol; // Keep transition activation consistent
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

                var labels = transition.Symbol.Split(',', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
                if (labels.Any(label => label == symbol))
                {
                    return transition;
                }
            }

            return null;
        }

        private void ResetSimulationState()
        {
            _currentIndex = 0;
            _currentTransition = null;
            _currentState = MyAutomaton.States.FirstOrDefault(state => state.IsInitial);
            _history.Clear(); // Keep simulation reset after history update
            SimulationStatus = string.Empty;
        }

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

            OnPropertyChanged(nameof(InputWordDisplay));
            OnPropertyChanged(nameof(CanPrevious));
            OnPropertyChanged(nameof(CanNext));
            OnPropertyChanged(nameof(CanStart));
            OnPropertyChanged(nameof(CanStop));
            OnPropertyChanged(nameof(CanReset));
            OnPropertyChanged(nameof(IsInputEditable));
            OnPropertyChanged(nameof(IsInputValid));
        }

        private void ValidateInputWord()
        {
            if (string.IsNullOrWhiteSpace(InputWord))
            {
                SimulationStatus = string.Empty;
                IsInputValid = true;
                return;
            }

            var alphabet = new HashSet<string>(StringComparer.Ordinal);
            foreach (var transition in MyAutomaton.Transitions)
            {
                foreach (var symbol in transition.Symbol.Split(',', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries))
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

        private void Transition_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            e.Handled = true;
            ClearStateSelection();
            var element = (FrameworkElement)sender;
            var transition = (Transition)element.DataContext;
            SelectTransition(transition);
        }

        private void SelectTransition(Transition transition)
        {
            foreach (var item in MyAutomaton.Transitions)
            {
                item.IsSelected = false;
            }

            transition.IsSelected = true;
            SelectedTransition = transition;
        }

        private void ClearTransitionSelection()
        {
            foreach (var item in MyAutomaton.Transitions)
            {
                item.IsSelected = false;
            }

            SelectedTransition = null;
        }

        private void ClearStateSelection()
        {
            foreach (var state in MyAutomaton.States)
            {
                state.IsSelected = false;
            }

            SelectedState = null;
        }

        private void UpdateTransitionOffsets(State source, State target, Transition newTransition)
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