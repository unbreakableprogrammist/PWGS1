using System.Collections.Generic;

namespace AutomatonEditor;

// Modele do JSON.
public class AutomatonData
{
    public AutomatonMeta Meta { get; set; } = new();
    public List<StateData> States { get; set; } = [];
    public List<TransitionData> Transitions { get; set; } = [];
}

// Sekcja meta z opisem i alfabetem.
public class AutomatonMeta
{
    public string Description { get; set; } = string.Empty;
    public List<string> Alphabet { get; set; } = [];
    public DateTimeOffset? Created { get; set; }
}

// Dane stanu w JSON.
public class StateData
{
    public int Id { get; set; }
    public string Name { get; set; } = string.Empty;
    public bool IsStart { get; set; }
    public bool IsAccepting { get; set; }
    public StatePosition Position { get; set; } = new();
    public StateAppearance Appearance { get; set; } = new();
}

// Pozycja stanu.
public class StatePosition
{
    public double X { get; set; }
    public double Y { get; set; }
}

// Wygląd stanu (kolory, promień, grubość).
public class StateAppearance
{
    public double Radius { get; set; } = 25;
    public string FillColor { get; set; } = "#FFFFFF";
    public string StrokeColor { get; set; } = "#000000";
    public double StrokeThickness { get; set; } = 2;
}

// Dane przejścia w JSON.
public class TransitionData
{
    public int FromStateId { get; set; }
    public int ToStateId { get; set; }
    public string Symbol { get; set; } = string.Empty;
}
