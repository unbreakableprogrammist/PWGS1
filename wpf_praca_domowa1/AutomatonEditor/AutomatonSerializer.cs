using System.Globalization;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Windows.Media;

namespace AutomatonEditor;

// Serializator JSON do zapisu i odczytu automatu.
public static class AutomatonSerializer
{
    private static readonly JsonSerializerOptions Options = new()
    {
        PropertyNameCaseInsensitive = true,
        WriteIndented = true,
        DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull
    };

    public static AutomatonData Deserialize(string json)
    {
        var data = JsonSerializer.Deserialize<AutomatonData>(json, Options);
        if (data == null) 
        {
            throw new InvalidOperationException("Nie udało się wczytać pliku JSON.");
        }

        return data;
    }

    public static string Serialize(AutomatonData data) => JsonSerializer.Serialize(data, Options);

    public static Brush ParseBrush(string value)
    {
        if (string.IsNullOrWhiteSpace(value))
        {
            return Brushes.LightBlue; 
        }

        if (ColorConverter.ConvertFromString(value) is Color color)
        {
            return new SolidColorBrush(color);
        }

        throw new InvalidOperationException($"Niepoprawny kolor: {value}");
    }

    public static string BrushToString(Brush brush)
    {
        if (brush is SolidColorBrush solid)
        {
            return solid.Color.ToString();
        }

        return Brushes.LightBlue.ToString(CultureInfo.InvariantCulture);
    }
}
