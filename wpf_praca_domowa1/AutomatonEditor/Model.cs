using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Windows;
using System.Windows.Media;

namespace AutomatonEditor;

// Główny model automatu.
// ObservableCollection automatycznie informuje kontrolki WPF, że do listy dodano
// albo usunięto element. Dzięki temu ItemsControl odświeża rysunek bez ręcznego
// przerysowywania całego Canvasu.
public class Automaton
{
    public ObservableCollection<State> States { get; set; } = [];
    public ObservableCollection<Transition> Transitions { get; set; } = [];
}

// Model pojedynczego stanu automatu wraz z wyglądem i pozycją.
public class State : INotifyPropertyChanged
{
    private double _x, _y;
    private bool _isInitial, _isAccepting, _isSelected;
    private bool _isActive;
    private double _radius = 25;
    private double _strokeThickness = 2;
    private Brush _fillBrush = Brushes.LightBlue;
    private Brush _strokeBrush = Brushes.Black;
    // Nazwa stanu, np. q0, q1.
    public string? Name { get; set; }
    public double X { get => _x; set { _x = value; OnPropertyChanged(); } }
    public double Y { get => _y; set { _y = value; OnPropertyChanged(); } }
    public bool IsInitial { get => _isInitial; set { _isInitial = value; OnPropertyChanged(); } }
    public bool IsAccepting { get => _isAccepting; set { _isAccepting = value; OnPropertyChanged(); } }
    public bool IsSelected { get => _isSelected; set { _isSelected = value; OnPropertyChanged(); } }
    // Informacja o aktywnym stanie podczas symulacji.
    public bool IsActive { get => _isActive; set { _isActive = value; OnPropertyChanged(); } }
    // Atrybuty wizualne stanu (rozmiar i styl obramowania).
    // Właściwość Radius jest powiązana z XAML-em przez Data Binding. Po zmianie
    // suwaka trzeba zgłosić nie tylko sam Radius, ale też właściwości pochodne
    // Diameter, NegativeRadiusMargin i AcceptingDiameter, bo to one decydują
    // o faktycznym rozmiarze i położeniu okręgu na Canvasie.
    public double Radius
    {
        get => _radius;
        set
        {
            _radius = value;
            OnPropertyChanged();
            OnPropertyChanged(nameof(Diameter));
            OnPropertyChanged(nameof(NegativeRadiusMargin));
            OnPropertyChanged(nameof(AcceptingDiameter));
        }
    }

    // Średnica koła stanu (pomocniczo do bindowania w XAML).
    public double Diameter => Radius * 2;

    // Ujemny margines, żeby środek stanu był w miejscu X/Y.
    public Thickness NegativeRadiusMargin => new(-Radius, -Radius, 0, 0);

    // Średnica drugiego okręgu dla stanu akceptującego.
    public double AcceptingDiameter => Math.Max(Diameter - 10, 0);

    // Grubość krawędzi koła stanu
    public double StrokeThickness
    {
        get => _strokeThickness;
        set
        {
            _strokeThickness = value;
            OnPropertyChanged();
        }
    }

    // Kolory wypełnienia i krawędzi stanu
    public Brush FillBrush
    {
        get => _fillBrush;
        set
        {
            _fillBrush = value;
            OnPropertyChanged();
        }
    }

    public Brush StrokeBrush
    {
        get => _strokeBrush;
        set
        {
            _strokeBrush = value;
            OnPropertyChanged();
        }
    }
    public event PropertyChangedEventHandler? PropertyChanged;
    protected void OnPropertyChanged([CallerMemberName] string? name = null) =>
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
}

// Element historii obliczeń (para: stan, symbol).
public class HistoryEntry
{
    public string StateName { get; set; } = string.Empty;
    public string Symbol { get; set; } = string.Empty;
    public State? State { get; set; }
    public Transition? Transition { get; set; }
}

// Fragment tekstu rysowany z opcjonalnym wyróżnieniem.
// Używamy tej klasy zarówno dla liter słowa wejściowego, jak i dla symboli
// etykiety przejścia. Logika symulacji przechowuje czyste symbole, a widok
// decyduje o kolorach na podstawie IsActive.
public class SymbolDisplayPart
{
    public string Text { get; set; } = string.Empty;
    public bool IsActive { get; set; }
}

public class Transition : INotifyPropertyChanged
{
    private State _source = null!;
    private State _target = null!;

    private string _symbol = string.Empty;
    private bool _isSelected;
    private bool _isActive;
    private string _activeSymbol = string.Empty;
    private double _curveOffset;
    private Geometry _pathGeometry = Geometry.Empty;
    private Point _arrowTip;
    private Point _arrowBase1;
    private Point _arrowBase2;
    private PointCollection _arrowPoints = [];
    private double _textX;
    private double _textY;
    // Etykieta przejścia, np. "0,1".
    public string Symbol
    {
        get => _symbol;
        set
        {
            _symbol = value;
            OnPropertyChanged();
            OnPropertyChanged(nameof(SymbolParts));
        }
    }

    // Flaga zaznaczenia przejścia w UI.
    // Czy przejście jest zaznaczone myszką.
    public bool IsSelected
    {
        get => _isSelected;
        set
        {
            _isSelected = value;
            OnPropertyChanged();
        }
    }

    // Informacja o aktywnym przejściu podczas symulacji.
    // Czy przejście jest aktualnie aktywne w symulacji.
    public bool IsActive
    {
        get => _isActive;
        set
        {
            _isActive = value;
            OnPropertyChanged();
            OnPropertyChanged(nameof(SymbolParts));
        }
    }

    // Symbol używany do wyróżnienia etykiety aktywnego przejścia.
    public string ActiveSymbol
    {
        get => _activeSymbol;
        set
        {
            _activeSymbol = value ?? string.Empty;
            OnPropertyChanged();
            OnPropertyChanged(nameof(SymbolParts));
        }
    }

    // Części etykiety przejścia używane do wizualnego wyróżnienia aktywnego symbolu.
    // Etykieta "a,b,c" jest rozbijana na osobne elementy: "a", ",", "b", ",", "c".
    // Dzięki temu XAML może nadać tło tylko aktualnie przetwarzanemu symbolowi,
    // zamiast dopisywać do tekstu sztuczne nawiasy typu [a].
    public IReadOnlyList<SymbolDisplayPart> SymbolParts
    {
        get
        {
            var displayParts = new List<SymbolDisplayPart>();
            if (string.IsNullOrWhiteSpace(Symbol))
            {
                return displayParts;
            }

            var parts = Symbol.Split(',', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
            for (var i = 0; i < parts.Length; i++)
            {
                displayParts.Add(new SymbolDisplayPart
                {
                    Text = parts[i],
                    IsActive = IsActive && parts[i] == ActiveSymbol
                });

                if (i < parts.Length - 1)
                {
                    displayParts.Add(new SymbolDisplayPart
                    {
                        Text = ",",
                        IsActive = false
                    });
                }
            }

            return displayParts;
        }
    }

    // Przesunięcie krzywizny, gdy istnieją przejścia w obie strony.
    // Offset łuku, żeby przejścia w dwie strony się nie nakładały.
    public double CurveOffset
    {
        get => _curveOffset;
        set
        {
            _curveOffset = value;
            RefreshGeometry();
        }
    }

    // Stan źródłowy.
    public State Source
    {
        get => _source;
        set
        {
            if (_source != null)
                _source.PropertyChanged -= State_PropertyChanged;
            _source = value;
            if (_source != null)
                _source.PropertyChanged += State_PropertyChanged;
            RefreshGeometry();
        }
    }

    // Stan docelowy.
    public State Target
    {
        get => _target;
        set
        {
            if (_target != null)
                _target.PropertyChanged -= State_PropertyChanged;
            _target = value;
            if (_target != null)
                _target.PropertyChanged += State_PropertyChanged;
            RefreshGeometry();
        }
    }

    // Współrzędne pomocnicze (zostawione dla kompatybilności).
    public double X1 => Source?.X ?? 0;
    public double Y1 => Source?.Y ?? 0;
    public double X2 => Target?.X ?? 0;
    public double Y2 => Target?.Y ?? 0;

    // Geometria oraz punkty strzałki wykorzystywane przez widok.
    // Geometria ścieżki używana przez Path w XAML.
    public Geometry PathGeometry
    {
        get => _pathGeometry;
        private set
        {
            _pathGeometry = value;
            OnPropertyChanged();
        }
    }

    // Punkty trójkąta strzałki.
    public PointCollection ArrowPoints
    {
        get => _arrowPoints;
        private set
        {
            _arrowPoints = value;
            OnPropertyChanged();
        }
    }

    // Punkt końcowy strzałki.
    public Point ArrowTip
    {
        get => _arrowTip;
        private set
        {
            _arrowTip = value;
            OnPropertyChanged();
        }
    }

    // Boki strzałki.
    public Point ArrowBase1
    {
        get => _arrowBase1;
        private set
        {
            _arrowBase1 = value;
            OnPropertyChanged();
        }
    }

    public Point ArrowBase2
    {
        get => _arrowBase2;
        private set
        {
            _arrowBase2 = value;
            OnPropertyChanged();
        }
    }

    // Pozycja etykiety na płótnie.
    public double TextX
    {
        get => _textX;
        private set
        {
            _textX = value;
            OnPropertyChanged();
        }
    }

    public double TextY
    {
        get => _textY;
        private set
        {
            _textY = value;
            OnPropertyChanged();
        }
    }

    // Odświeżenie geometrii, gdy stan się przesuwa.
    // Transition subskrybuje PropertyChanged obu stanów. Jeżeli użytkownik przesunie
    // stan albo zmieni jego promień, przejście automatycznie przelicza punkt startu,
    // punkt końca, strzałkę oraz pozycję etykiety.
    private void State_PropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (e.PropertyName == "X" || e.PropertyName == "Y" || e.PropertyName == "Radius")
        {
            RefreshGeometry();
        }
    }

    // Wyznacza ścieżkę, etykietę i strzałkę przejścia na podstawie pozycji stanów.
    //
    // WPF nie rysuje przejścia jako zwykłego "obiektu grafu". Tworzymy geometrię
    // PathGeometry, którą XAML wyświetla w kontrolce Path. Dla zwykłego przejścia
    // obliczamy wektor od stanu źródłowego do docelowego i przesuwamy końce linii
    // o promienie stanów, żeby strzałka kończyła się na krawędzi okręgu, a nie
    // w jego środku. Dla przejścia do samego siebie rysujemy krzywą Beziera nad
    // stanem, ponieważ prosta linia miałaby zerową długość.
    private void RefreshGeometry()
    {
        if (Source == null || Target == null)
        {
            PathGeometry = Geometry.Empty;
            return;
        }

        var start = new Point(Source.X, Source.Y);
        var end = new Point(Target.X, Target.Y);
        var isSelfLoop = ReferenceEquals(Source, Target);

        if (isSelfLoop)
        {
            var loopRadius = Source.Radius * 1.4;
            var startPoint = new Point(Source.X - Source.Radius, Source.Y - Source.Radius);
            var endPoint = new Point(Source.X + Source.Radius, Source.Y - Source.Radius);
            var control1 = new Point(Source.X - loopRadius, Source.Y - loopRadius * 2);
            var control2 = new Point(Source.X + loopRadius, Source.Y - loopRadius * 2);

            var figure = new PathFigure { StartPoint = startPoint, IsClosed = false };
            figure.Segments.Add(new BezierSegment(control1, control2, endPoint, true));
            PathGeometry = new PathGeometry(new[] { figure });

            var arrowDirection = endPoint - control2;
            UpdateArrow(endPoint, arrowDirection);
            TextX = Source.X - 10;
            TextY = Source.Y - loopRadius * 2;
        }
        else
        {
            var direction = end - start;
            direction.Normalize();
            var normal = new Vector(-direction.Y, direction.X);
            var startPoint = new Point(start.X + direction.X * Source.Radius, start.Y + direction.Y * Source.Radius);
            var endPoint = new Point(end.X - direction.X * Target.Radius, end.Y - direction.Y * Target.Radius);
            var midPoint = new Point((startPoint.X + endPoint.X) / 2, (startPoint.Y + endPoint.Y) / 2);
            var controlPoint = new Point(midPoint.X + normal.X * CurveOffset, midPoint.Y + normal.Y * CurveOffset);

            PathGeometry = CreatePath(startPoint, endPoint, controlPoint);
            UpdateArrow(endPoint, endPoint - controlPoint);

            TextX = controlPoint.X - 10;
            TextY = controlPoint.Y - 15;
        }

        OnPropertyChanged(nameof(X1));
        OnPropertyChanged(nameof(Y1));
        OnPropertyChanged(nameof(X2));
        OnPropertyChanged(nameof(Y2));
        OnPropertyChanged(nameof(TextX));
        OnPropertyChanged(nameof(TextY));
    }

    // Tworzy prostą linię albo łuk w zależności od kontrolnego punktu.
    // Jeśli punkt kontrolny leży w środku odcinka, przejście jest zwykłą linią.
    // Jeśli jest odsunięty w bok (CurveOffset), powstaje łuk. To rozwiązuje
    // problem dwóch przejść między tymi samymi stanami w przeciwnych kierunkach:
    // linie nie nakładają się wtedy na siebie.
    private static Geometry CreatePath(Point startPoint, Point endPoint, Point controlPoint)
    {
        if (Math.Abs(controlPoint.X - ((startPoint.X + endPoint.X) / 2)) < 0.1 &&
            Math.Abs(controlPoint.Y - ((startPoint.Y + endPoint.Y) / 2)) < 0.1)
        {
            return new LineGeometry(startPoint, endPoint);
        }

        var figure = new PathFigure { StartPoint = startPoint, IsClosed = false };
        figure.Segments.Add(new QuadraticBezierSegment(controlPoint, endPoint, true));
        return new PathGeometry(new[] { figure });
    }

    // Wylicza kształt strzałki na końcu przejścia.
    // Strzałka jest trójkątem Polygon. Najpierw znamy czubek (tip) i kierunek
    // krzywej przy końcu. Cofamy się od czubka o arrowLength, a następnie
    // wyznaczamy dwa boczne punkty prostopadle do kierunku przejścia.
    private void UpdateArrow(Point tip, Vector direction)
    {
        const double arrowLength = 12;
        const double arrowWidth = 6;

        if (direction.Length < 1)
        {
            ArrowTip = tip;
            ArrowBase1 = tip;
            ArrowBase2 = tip;
            ArrowPoints = [];
            return;
        }

        direction.Normalize();
        var basePoint = tip - direction * arrowLength;
        var normal = new Vector(-direction.Y, direction.X);
        ArrowTip = tip;
        ArrowBase1 = basePoint + normal * arrowWidth;
        ArrowBase2 = basePoint - normal * arrowWidth;
        ArrowPoints = [ArrowTip, ArrowBase1, ArrowBase2];
    }

    public event PropertyChangedEventHandler? PropertyChanged;
    protected void OnPropertyChanged([CallerMemberName] string? name = null) =>
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
}
