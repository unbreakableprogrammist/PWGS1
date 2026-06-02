using System;
using System.Globalization;
using System.Windows.Data;

namespace Libraria.Converters
{
    public class RatingStarTextConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            var rating = value is int intValue ? intValue : 0;
            var starValue = parameter switch
            {
                int number => number,
                string text when int.TryParse(text, out var parsed) => parsed,
                _ => 0
            };

            return rating >= starValue ? "★" : "☆";
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return Binding.DoNothing;
        }
    }
}
