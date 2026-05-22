using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Windows;
using System.Windows.Media;

namespace AutomatonEditor;


public class Automaton // klasa ktora trzyma tablice kolek i kresek
{
    public ObservableCollection<State> States { get; set; } = [];
    public ObservableCollection<Transition> Transitions { get; set; } = [];
}

// Model pojedynczego stanu automatu.
public class State : INotifyPropertyChanged
{
    private double _x, _y;
    private bool _isInitial, _isAccepting, _isSelected;
    private bool _isActive;
    private bool _isFillColorTextValid = true;
    private bool _isStrokeColorTextValid = true;
    private double _radius = 25;
    private double _strokeThickness = 2;
    private Brush _fillBrush = Brushes.LightBlue;
    private Brush _strokeBrush = Brushes.Black;
    private string _fillColorText = BrushToColorText(Brushes.LightBlue);
    private string _strokeColorText = BrushToColorText(Brushes.Black);
    // Nazwa stanu, np. q0, q1.
    public string? Name { get; set; }
    public double X { get => _x; set { _x = value; OnPropertyChanged(); } }
    public double Y { get => _y; set { _y = value; OnPropertyChanged(); } }
    public bool IsInitial { get => _isInitial; set { _isInitial = value; OnPropertyChanged(); } }
    public bool IsAccepting { get => _isAccepting; set { _isAccepting = value; OnPropertyChanged(); } }
    public bool IsSelected { get => _isSelected; set { _isSelected = value; OnPropertyChanged(); } }
    // Informacja o aktywnym stanie podczas symulacji.
    public bool IsActive { get => _isActive; set { _isActive = value; OnPropertyChanged(); } }
    

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

    // Średnica koła stanu 
    public double Diameter => Radius * 2;

    public Thickness NegativeRadiusMargin => new(-Radius, -Radius, 0, 0); // pszesuwamy zeby zaczynalo sie na srodku kursora

    // Średnica drugiego okręgu dla stanu akceptującego.
    public double AcceptingDiameter => Math.Max(Diameter - 10, 0);

    // Grubość krawędzi.
    public double StrokeThickness
    {
        get => _strokeThickness;
        set
        {
            _strokeThickness = value;
            OnPropertyChanged();
        }
    }

    // Kolory wypełnienia i krawędzi.
    public Brush FillBrush
    {
        get => _fillBrush;
        set
        {
            _fillBrush = value;
            _fillColorText = BrushToColorText(value);
            IsFillColorTextValid = true;
            OnPropertyChanged();
            OnPropertyChanged(nameof(FillColorText));
        }
    }

    public Brush StrokeBrush
    {
        get => _strokeBrush;
        set
        {
            _strokeBrush = value;
            _strokeColorText = BrushToColorText(value);
            IsStrokeColorTextValid = true;
            OnPropertyChanged();
            OnPropertyChanged(nameof(StrokeColorText));
        }
    }

    public string FillColorText
    {
        get => _fillColorText;
        set
        {
            _fillColorText = value ?? string.Empty;
            if (TryParseColorBrush(_fillColorText, out var brush))
            {
                _fillBrush = brush;
                IsFillColorTextValid = true;
                OnPropertyChanged(nameof(FillBrush));
            }
            else
            {
                IsFillColorTextValid = false;
            }

            OnPropertyChanged();
        }
    }

    public string StrokeColorText
    {
        get => _strokeColorText;
        set
        {
            _strokeColorText = value ?? string.Empty;
            if (TryParseColorBrush(_strokeColorText, out var brush))
            {
                _strokeBrush = brush;
                IsStrokeColorTextValid = true;
                OnPropertyChanged(nameof(StrokeBrush));
            }
            else
            {
                IsStrokeColorTextValid = false;
            }

            OnPropertyChanged();
        }
    }

    public bool IsFillColorTextValid
    {
        get => _isFillColorTextValid;
        private set
        {
            _isFillColorTextValid = value;
            OnPropertyChanged();
        }
    }

    public bool IsStrokeColorTextValid
    {
        get => _isStrokeColorTextValid;
        private set
        {
            _isStrokeColorTextValid = value;
            OnPropertyChanged();
        }
    }

    private static bool TryParseColorBrush(string? text, out Brush brush)
    {
        brush = Brushes.Transparent;
        if (string.IsNullOrWhiteSpace(text))
        {
            return false;
        }

        var normalizedText = NormalizeColorText(text.Trim());
        try
        {
            if (ColorConverter.ConvertFromString(normalizedText) is Color color)
            {
                brush = new SolidColorBrush(color);
                return true;
            }
        }
        catch (FormatException)
        {
            return false;
        }
        catch (NotSupportedException)
        {
            return false;
        }

        return false;
    }

    private static string NormalizeColorText(string text)
    {
        return IsBareHexColor(text) ? $"#{text}" : text;
    }

    private static bool IsBareHexColor(string text)
    {
        if (text.Length is not (6 or 8))
        {
            return false;
        }

        foreach (var character in text)
        {
            if (!Uri.IsHexDigit(character))
            {
                return false;
            }
        }

        return true;
    }

    private static string BrushToColorText(Brush brush)
    {
        var color = brush is SolidColorBrush solidColorBrush
            ? solidColorBrush.Color
            : Brushes.LightBlue.Color;

        return color.A == byte.MaxValue
            ? $"{color.R:X2}{color.G:X2}{color.B:X2}"
            : $"{color.A:X2}{color.R:X2}{color.G:X2}{color.B:X2}";
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
    // etykieta przejscia 
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

    // Symbol do wyróżnienia etykiety.
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

    // jesli przejscie w obie strony to robimy luk zamiast linii prostej, a ten parametr kontroluje jak bardzo jest on zakrzywiony
    public double CurveOffset
    {
        get => _curveOffset;
        set
        {
            _curveOffset = value;
            RefreshGeometry();
        }
    }

    // Stan źródłowy
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

    public double X1 => Source?.X ?? 0;
    public double Y1 => Source?.Y ?? 0;
    public double X2 => Target?.X ?? 0;
    public double Y2 => Target?.Y ?? 0;

 
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
    private void State_PropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (e.PropertyName == "X" || e.PropertyName == "Y" || e.PropertyName == "Radius")
        {
            RefreshGeometry();
        }
    }


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
