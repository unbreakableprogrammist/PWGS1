using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace Libraria.Controls
{
    /// <summary>
    /// Interaction logic for RatingControl.xaml
    /// </summary>
    public partial class RatingControl : UserControl
    {
        public static readonly DependencyProperty ValueProperty =
            DependencyProperty.Register(
                nameof(Value),
                typeof(int),
                typeof(RatingControl),
                new FrameworkPropertyMetadata(
                    0,
                    FrameworkPropertyMetadataOptions.BindsTwoWayByDefault,
                    OnRatingPropertyChanged,
                    CoerceValue));

        public static readonly DependencyProperty IsReadOnlyProperty =
            DependencyProperty.Register(
                nameof(IsReadOnly),
                typeof(bool),
                typeof(RatingControl),
                new PropertyMetadata(false, OnRatingPropertyChanged));

        public static readonly DependencyProperty StarSizeProperty =
            DependencyProperty.Register(
                nameof(StarSize),
                typeof(double),
                typeof(RatingControl),
                new PropertyMetadata(22.0));

        private readonly TextBlock[] _stars;

        public RatingControl()
        {
            InitializeComponent();
            _stars = new[] { Star1, Star2, Star3, Star4, Star5 };
            UpdateStars(Value);
            UpdateCursor();
        }

        public int Value
        {
            get => (int)GetValue(ValueProperty);
            set => SetValue(ValueProperty, value);
        }

        public bool IsReadOnly
        {
            get => (bool)GetValue(IsReadOnlyProperty);
            set => SetValue(IsReadOnlyProperty, value);
        }

        public double StarSize
        {
            get => (double)GetValue(StarSizeProperty);
            set => SetValue(StarSizeProperty, value);
        }

        private static object CoerceValue(DependencyObject d, object baseValue)
        {
            return Math.Clamp((int)baseValue, 0, 5);
        }

        private static void OnRatingPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (d is not RatingControl control || control._stars is null)
            {
                return;
            }

            control.UpdateStars(control.Value);
            control.UpdateCursor();
        }

        private void Star_MouseEnter(object sender, MouseEventArgs e)
        {
            if (IsReadOnly || sender is not TextBlock star)
            {
                return;
            }

            UpdateStars(GetStarValue(star));
        }

        private void Star_MouseLeave(object sender, MouseEventArgs e)
        {
            if (!IsReadOnly)
            {
                UpdateStars(Value);
            }
        }

        private void Star_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            if (IsReadOnly || sender is not TextBlock star)
            {
                return;
            }

            Value = GetStarValue(star);
        }

        private void UpdateStars(int value)
        {
            for (var index = 0; index < _stars.Length; index++)
            {
                _stars[index].Text = index < value ? "★" : "☆";
            }
        }

        private void UpdateCursor()
        {
            foreach (var star in _stars)
            {
                star.Cursor = IsReadOnly ? Cursors.Arrow : Cursors.Hand;
            }
        }

        private static int GetStarValue(TextBlock star)
        {
            return star.Tag is string tag && int.TryParse(tag, out var value) ? value : 0;
        }
    }
}
