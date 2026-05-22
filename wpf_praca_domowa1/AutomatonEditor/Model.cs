using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Windows;
using System.Windows.Media;

namespace AutomatonEditor;

public class Automaton : INotifyPropertyChanged
{
    public ObservableCollection<State> States { get; set; } = [];
    public ObservableCollection<Transition> Transitions { get; set; } = [];

    public event PropertyChangedEventHandler? PropertyChanged;
}

public class State : INotifyPropertyChanged
{
    private double _x, _y;
    private bool _isInitial, _isAccepting, _isSelected;
    private bool _isActive;
    private double _radius = 25;
    private double _strokeThickness = 2;
    private Brush _fillBrush = Brushes.LightBlue;
    private Brush _strokeBrush = Brushes.Black;
    public string? Name { get; set; }
    public double X { get => _x; set { _x = value; OnPropertyChanged(); } }
    public double Y { get => _y; set { _y = value; OnPropertyChanged(); } }
    public bool IsInitial { get => _isInitial; set { _isInitial = value; OnPropertyChanged(); } }
    public bool IsAccepting { get => _isAccepting; set { _isAccepting = value; OnPropertyChanged(); } }
    public bool IsSelected { get => _isSelected; set { _isSelected = value; OnPropertyChanged(); } }
    // Informacja o aktywnym stanie podczas symulacji.
    public bool IsActive { get => _isActive; set { _isActive = value; OnPropertyChanged(); } }
    // Atrybuty wizualne stanu (rozmiar i styl obramowania)
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

    public double Diameter => Radius * 2;

    public Thickness NegativeRadiusMargin => new(-Radius, -Radius, 0, 0);

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

public class Transition : INotifyPropertyChanged
{
    private State _source = null!;
    private State _target = null!;

    private string _symbol;
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
    public string Symbol
    {
        get => _symbol;
        set
        {
            _symbol = value;
            OnPropertyChanged();
            OnPropertyChanged(nameof(DisplaySymbol));
        }
    }

    // Flaga zaznaczenia przejścia w UI.
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
    public bool IsActive
    {
        get => _isActive;
        set
        {
            _isActive = value;
            OnPropertyChanged();
            OnPropertyChanged(nameof(DisplaySymbol));
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
            OnPropertyChanged(nameof(DisplaySymbol));
        }
    }

    // Etykieta z wyróżnionym symbolem aktywnego przejścia.
    public string DisplaySymbol
    {
        get
        {
            if (!IsActive || string.IsNullOrWhiteSpace(ActiveSymbol))
            {
                return Symbol;
            }

            var parts = Symbol.Split(',', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
            for (var i = 0; i < parts.Length; i++)
            {
                if (parts[i] == ActiveSymbol)
                {
                    parts[i] = $"[{parts[i]}]";
                }
            }

            return string.Join(",", parts);
        }
    }

    // Przesunięcie krzywizny, gdy istnieją przejścia w obie strony.
    public double CurveOffset
    {
        get => _curveOffset;
        set
        {
            _curveOffset = value;
            RefreshGeometry();
        }
    }

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

    public double X1 => Source?.X ?? 0;
    public double Y1 => Source?.Y ?? 0;
    public double X2 => Target?.X ?? 0;
    public double Y2 => Target?.Y ?? 0;

    // Geometria oraz punkty strzałki wykorzystywane przez widok.
    public Geometry PathGeometry
    {
        get => _pathGeometry;
        private set
        {
            _pathGeometry = value;
            OnPropertyChanged();
        }
    }

    public PointCollection ArrowPoints
    {
        get => _arrowPoints;
        private set
        {
            _arrowPoints = value;
            OnPropertyChanged();
        }
    }

    public Point ArrowTip
    {
        get => _arrowTip;
        private set
        {
            _arrowTip = value;
            OnPropertyChanged();
        }
    }

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

    private void State_PropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (e.PropertyName == "X" || e.PropertyName == "Y" || e.PropertyName == "Radius")
        {
            RefreshGeometry();
        }
    }

    // Wyznacza ścieżkę, etykietę i strzałkę przejścia na podstawie pozycji stanów.
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
            var length = Math.Max(direction.Length, 1);
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